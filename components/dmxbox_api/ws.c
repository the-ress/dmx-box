#include "ws.h"
#include "dmxbox_httpd.h"
#include "wifi_scan.h"
#include "ws_ap_found.h"
#include <cJSON.h>
#include <esp_check.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <stdbool.h>
#include <strings.h>

static const char TAG[] = "dmxbox_ws";
static httpd_handle_t dmxbox_ws_server;

typedef struct dmxbox_ws_session {
  bool scan_aps;
} dmxbox_ws_session_t;

static void dmxbox_ws_session_free(dmxbox_ws_session_t *session) {
  ESP_LOGI(TAG, "cleaning up a session");
  free(session);
}

static dmxbox_ws_session_t *dmxbox_ws_session_alloc() {
  ESP_LOGD(TAG, "allocating a session");
  return calloc(1, sizeof(dmxbox_ws_session_t));
}

static dmxbox_ws_session_t *dmxbox_ws_session_ensure(httpd_req_t *req) {
  dmxbox_ws_session_t *session = req->sess_ctx;
  if (!session) {
    session = dmxbox_ws_session_alloc();
    if (!session) {
      ESP_LOGE(TAG, "failed to allocate dmxbox_ws_session");
      return NULL;
    }
    req->free_ctx = (httpd_free_ctx_fn_t)dmxbox_ws_session_free;
    req->sess_ctx = session;
  }
  return session;
}

// frees json
static httpd_ws_frame_t dmxbox_ws_frame_from_json(const char *json) {
  httpd_ws_frame_t frame = {
      .payload = (uint8_t*)json,
      .len = strlen(json),
      .type = HTTPD_WS_TYPE_TEXT,
  };
  return frame;
}

// runs on the httpd task
static void dmxbox_ws_send_ap_records(void *arg) {
  dmxbox_wifi_scan_result_t *result = arg;
  esp_err_t ret = ESP_OK;

  int client_fds[CONFIG_DMXBOX_WEBSEVER_MAX_SOCKETS];
  size_t clients = CONFIG_DMXBOX_WEBSEVER_MAX_SOCKETS;
  ESP_GOTO_ON_ERROR(
      httpd_get_client_list(dmxbox_ws_server, &clients, client_fds),
      exit,
      TAG,
      "failed to get client list"
  );

  for (int r = 0; r < result->count; r++) {
    char *json = dmxbox_ws_ap_found_create(&result->records[r]);
    if (!json) {
      ESP_LOGE(TAG, "failed to create ws_ap_found");
      continue;
    }
    httpd_ws_frame_t ws_frame = dmxbox_ws_frame_from_json(json);
    for (int c = 0; c < clients; c++) {
      httpd_ws_client_info_t client_info =
          httpd_ws_get_fd_info(dmxbox_ws_server, client_fds[c]);
      if (client_info != HTTPD_WS_CLIENT_WEBSOCKET) {
        continue;
      }
      dmxbox_ws_session_t *session =
          httpd_sess_get_ctx(dmxbox_ws_server, client_fds[c]);
      if (!session || !session->scan_aps) {
        continue;
      }
      ESP_LOGI(TAG, "sending record %d to websocket %d", r, c);
      httpd_ws_send_frame_async(dmxbox_ws_server, client_fds[c], &ws_frame);
    }
    free(json);
  }

exit:
  free(result);
  (void)ret;
}

// runs on the wifi event task
static void dmxbox_ws_wifi_scan_callback(dmxbox_wifi_scan_result_t *result) {
  ESP_LOGI(
      TAG,
      "ws_wifi_scan callback, got %d records, queueing http work",
      result->count
  );
  esp_err_t ret = ESP_OK;
  ESP_GOTO_ON_ERROR( // TODO replace with ESP_RETURN_VOID_ON_ERROR
      httpd_queue_work(dmxbox_ws_server, dmxbox_ws_send_ap_records, result),
      error,
      TAG,
      "failed to queue work"
  );
error:
  (void)ret;
}

static cJSON *dmxbox_ws_frame_to_json(
    dmxbox_ws_session_t *session,
    const httpd_ws_frame_t *ws_frame
) {
  if (ws_frame->type != HTTPD_WS_TYPE_TEXT) {
    ESP_LOGI(TAG, "ignoring non-text frame");
    return NULL;
  }

  cJSON *json =
      cJSON_ParseWithLength((const char *)ws_frame->payload, ws_frame->len);
  if (!json) {
    ESP_LOGE(TAG, "could not parse json");
    return NULL;
  }
  return json;
}

static esp_err_t dmxbox_ws_handle_request(
    dmxbox_ws_session_t *session,
    const char *type,
    const cJSON *json
) {
  if (!strcmp(type, "settings/startApScan")) {
    ESP_LOGI(TAG, "starting AP scan");
    session->scan_aps = true;
    ESP_RETURN_ON_ERROR(dmxbox_wifi_scan_start(), TAG, "failed to start scan");
  } else if (!strcmp(type, "settings/stopApScan")) {
    ESP_LOGI(TAG, "stopping AP scan");
    session->scan_aps = false;
  } else {
    ESP_LOGE(TAG, "unknown message type %s", type);
  }
  return ESP_OK;
}

static esp_err_t dmxbox_ws_handler(httpd_req_t *req) {
  dmxbox_ws_session_t *session = dmxbox_ws_session_ensure(req);

  if (req->method == HTTP_GET) {
    ESP_LOGI(TAG, "connected");
    return ESP_OK;
  }

  static uint8_t buffer[512];
  httpd_ws_frame_t ws_frame = {
      .payload = buffer,
  };

  ESP_RETURN_ON_ERROR(
      httpd_ws_recv_frame(req, &ws_frame, sizeof(buffer)),
      TAG,
      "failed to receive frame"
  );

  ESP_LOGI(TAG, "received %d-byte frame", ws_frame.len);
  cJSON *json = dmxbox_ws_frame_to_json(session, &ws_frame);
  if (!json) {
    return ESP_OK;
  }
  const cJSON *type_json = cJSON_GetObjectItemCaseSensitive(json, "type");
  const char *type = cJSON_GetStringValue(type_json);
  dmxbox_ws_handle_request(session, type, json);
  cJSON_Delete(json);
  return ESP_OK;
}

esp_err_t dmxbox_ws_register(httpd_handle_t server) {
  ESP_LOGI(TAG, "registering ws handler");
  static const httpd_uri_t uri = {
      .uri = "/api/ws",
      .method = HTTP_GET,
      .handler = dmxbox_ws_handler,
      .is_websocket = true,
  };
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &uri),
      TAG,
      "failed to register ws handler"
  );
  ESP_RETURN_ON_ERROR(
      dmxbox_wifi_scan_register_callback(dmxbox_ws_wifi_scan_callback),
      TAG,
      "failed to register wifi_scan callback"
  );

  dmxbox_ws_server = server;
  return ESP_OK;
}
