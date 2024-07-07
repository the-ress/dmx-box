#include "static_files.h"
#include <string.h>

static const char *dmxbox_httpd_default_type = "application/octet-stream";

const char *dmxbox_httpd_type_from_path(const char *path) {
  const char *ext = strrchr(path, '.');
  if (!ext) {
    return dmxbox_httpd_default_type;
  }
  if (!strcmp(ext, ".js") || !strcmp(ext, ".mjs")) {
    return "application/javascript";
  }
  if (!strcmp(ext, ".html")) {
    return "text/html";
  }
  if (!strcmp(ext, ".css")) {
    return "text/css";
  }
  if (!strcmp(ext, ".jpeg")) {
    return "image/jpeg";
  }
  if (!strcmp(ext, ".png")) {
    return "image/png";
  }
  if (!strcmp(ext, ".ico")) {
    return "image/x-icon";
  }
  if (!strcmp(ext, ".woff2")) {
    return "font/woff2";
  }
  return dmxbox_httpd_default_type;
}

// strips leading /, ensures path contains no further / and no ..
const char *dmxbox_httpd_validate_uri(const char *uri) {
  if (*uri == '/') {
    uri++;
  }
  if (*uri == '\0') {
    return "index.html";
  }
  if (strchr(uri, '/')) {
    return NULL;
  }
  if (strstr(uri, "..")) {
    return NULL;
  }
  return uri;
}
