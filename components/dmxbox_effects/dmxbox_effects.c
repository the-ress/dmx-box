#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <math.h>
#include <sdkconfig.h>
#include <stdint.h>
#include <string.h>

#include "dmxbox_artnet.h"
#include "dmxbox_const.h"
#include "dmxbox_effects.h"
#include "dmxbox_espnow.h"
#include "dmxbox_storage.h"

static const char *TAG = "effects";

#define EFFECTS_PERIOD 30
// #define EFFECTS_PERIOD 300
#define LOG_DMX_DATA false

#define SYNC_QUEUE_SIZE 50
#define SYNC_QUEUE_MAX_DELAY 15

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

static const uint32_t us_per_ms = 1000;

static const uint8_t default_effect_rate_raw = 127;

static uint16_t effect_control_universe_address = 1;

static const uint32_t distributed_sync_period_us = 10000 * us_per_ms;

typedef struct effect_state_sync_event {
  uint16_t effect_id;
  uint8_t level;
  uint8_t rate_raw;
  double progress;
  bool first_pass;
  int64_t receive_time_us;
} effect_state_sync_event_t;

typedef struct step_channel {
  struct step_channel *next;

  uint16_t channel;
  uint8_t level;
} step_channel_t;

typedef struct step {
  struct step *next;

  uint32_t time;
  uint32_t in;
  uint32_t dwell;
  uint32_t out;

  struct step_channel *channels_head;

  // precalculated values
  uint32_t offset_us;
} step_t;

typedef struct effect_distributed_state {
  bool is_leader;

  uint8_t last_level;
  uint8_t last_rate_raw;
  uint64_t last_sync_us;
} effect_distributed_state_t;

typedef struct effect {
  struct effect *next;

  uint16_t id;
  uint16_t level_channel;
  uint16_t rate_channel;
  bool distributed;

  step_t *steps_head;

  // precalculated values
  uint32_t effect_length_us;

  // internal state
  bool active;
  double progress;
  bool first_pass;

  effect_distributed_state_t distributed_state;
} effect_t;

static step_channel_t *step_channel_alloc() {
  return calloc(1, sizeof(step_channel_t));
}

// static void step_channel_free(step_channel_t *data) { free(data); }

static step_t *step_alloc() { return calloc(1, sizeof(step_t)); }

// static void step_free(step_t *data) { free(data); }

static effect_t *effect_alloc() { return calloc(1, sizeof(effect_t)); }

// static void effect_free(effect_t *data) { free(data); }

static effect_t *effects_head = NULL;

// Synchronizes module output
portMUX_TYPE dmxbox_effects_spinlock = portMUX_INITIALIZER_UNLOCKED;

uint8_t dmxbox_effects_data[DMX_CHANNEL_COUNT] = {0};

double rate_from_fader_level[UINT8_MAX + 1];

static QueueHandle_t effect_state_sync_queue;

static uint32_t ms_to_us(uint32_t value) { return value * us_per_ms; }

static uint32_t get_step_in_end_us(step_t *step) { return ms_to_us(step->in); }

static uint32_t get_step_dwell_end_us(step_t *step) {
  return ms_to_us(step->in + step->dwell);
}

static uint32_t get_step_out_end_us(step_t *step) {
  return ms_to_us(step->in + step->dwell + step->out);
}

static double get_rate_from_fader_level(uint8_t level) {
  if (level == 0) {
    return 0;
  }

  // 2^((x - 127) / 30.035) * 104.5 - 4.5 works out to:
  // around 1 at x=1
  // 100 at x=127
  // around 2000 at x=255
  return (pow(2, ((double)level - 127) / 30.035) * 1.045 - .045);
}

static void distributed_follower_callback(
    uint16_t effect_id,
    uint8_t level,
    uint8_t rate_raw,
    double progress,
    bool first_pass
) {
  effect_state_sync_event_t event = {
      .effect_id = effect_id,
      .level = level,
      .rate_raw = rate_raw,
      .progress = progress,
      .first_pass = first_pass,
      .receive_time_us = esp_timer_get_time(),
  };

  if (xQueueSend(
          effect_state_sync_queue,
          &event,
          SYNC_QUEUE_MAX_DELAY * portTICK_PERIOD_MS
      ) != pdTRUE) {
    ESP_LOGW(TAG, "Failed to enqueue sync event for effect %d", effect_id);
  }
}

void dmxbox_effects_init() {
  // Store a sample effect

  dmxbox_effect_step_t *step1 = dmxbox_effect_step_alloc(2);
  dmxbox_effect_step_t *step2 = dmxbox_effect_step_alloc(2);
  dmxbox_effect_step_t *step3 = dmxbox_effect_step_alloc(5);

  step1->time = 500;
  step1->in = 250;
  step1->dwell = 250;
  step1->out = 250;

  step1->channels[0].channel.index = 1;
  step1->channels[0].level = 255;

  step1->channels[1].channel.index = 2;
  step1->channels[1].level = 255;

  step2->time = 500;
  step2->in = 250;
  step2->dwell = 250;
  step2->out = 250;

  step2->channels[0].channel.index = 8;
  step2->channels[0].level = 255;
  step2->channels[1].channel.index = 10;
  step2->channels[1].level = 255;

  step3->time = 500;
  step3->in = 250;
  step3->dwell = 250;
  step3->out = 250;

  step3->channels[0].channel.index = 15;
  step3->channels[0].level = 255;
  step3->channels[1].channel.index = 16;
  step3->channels[1].level = 255;
  step3->channels[2].channel.index = 17;
  step3->channels[2].level = 255;
  step3->channels[3].channel.index = 18;
  step3->channels[3].level = 255;
  step3->channels[4].channel.index = 19;
  step3->channels[4].level = 255;

  // dmxbox_effect_step_t *step1 = dmxbox_effect_step_alloc(6);
  // dmxbox_effect_step_t *step2 = dmxbox_effect_step_alloc(1);
  // dmxbox_effect_step_t *step3 = dmxbox_effect_step_alloc(1);

  // step1->time = 150;

  // step1->channels[0].channel.index = 1;
  // step1->channels[0].level = 255;
  // step1->channels[1].channel.index = 3;
  // step1->channels[1].level = 255;
  // step1->channels[2].channel.index = 8;
  // step1->channels[2].level = 255;
  // step1->channels[3].channel.index = 10;
  // step1->channels[3].level = 255;
  // step1->channels[4].channel.index = 15;
  // step1->channels[4].level = 255;
  // step1->channels[5].channel.index = 17;
  // step1->channels[5].level = 255;

  // step2->time = 500;

  // step2->channels[0].channel.index = 150;
  // step2->channels[0].level = 255;

  // step3->time = 500;

  // step3->channels[0].channel.index = 150;
  // step3->channels[0].level = 255;

  uint16_t effect_id = 1;

  dmxbox_effect_step_set(effect_id, 1, step1);
  dmxbox_effect_step_set(effect_id, 2, step2);
  dmxbox_effect_step_set(effect_id, 3, step3);

  // dmxbox_effect_step_set(effect_id, 1, 6, step1);
  // dmxbox_effect_step_set(effect_id, 2, 1, step2);
  // dmxbox_effect_step_set(effect_id, 3, 1, step3);

  free(step1);
  free(step2);
  free(step3);

  // Load effect from storage

  effect_t *effect1 = effect_alloc();
  effect1->id = effect_id;
  effect1->level_channel = 1;
  effect1->rate_channel = 2;
  effect1->distributed = true;

  size_t step_count = 3;

  step_t *steps_tail = NULL;
  for (uint16_t step_id = 1; step_id <= step_count; step_id++) {
    ESP_LOGI(TAG, "step %d", step_id);

    dmxbox_effect_step_t *step_data;
    ESP_ERROR_CHECK(dmxbox_effect_step_get(effect_id, step_id, &step_data));

    step_t *step = step_alloc();
    step->time = step_data->time;
    step->in = step_data->in;
    step->dwell = step_data->dwell;
    step->out = step_data->out;

    ESP_LOGI(TAG, "step %d time = %d", step_id, (int)step->time);
    ESP_LOGI(TAG, "step %d in = %d", step_id, (int)step->in);
    ESP_LOGI(TAG, "step %d dwell = %d", step_id, (int)step->dwell);
    ESP_LOGI(TAG, "step %d out = %d", step_id, (int)step->out);

    step_channel_t *channels_tail = NULL;
    for (size_t i = 0; i < step_data->channel_count; i++) {
      step_channel_t *channel = step_channel_alloc();
      channel->channel = step_data->channels[i].channel.index;
      channel->level = step_data->channels[i].level;

      ESP_LOGI(
          TAG,
          "step %d channel %d: %d @ %d",
          step_id,
          i,
          channel->channel,
          channel->level
      );

      if (channels_tail) {
        channels_tail->next = channel;
        channels_tail = channel;
      } else {
        step->channels_head = channels_tail = channel;
      }
    }

    if (steps_tail) {
      steps_tail->next = step;
      steps_tail = step;
    } else {
      effect1->steps_head = steps_tail = step;
    }

    free(step_data);
  }

  effects_head = effect1;

  // Prepare internal state

  for (effect_t *effect = effects_head; effect; effect = effect->next) {
    effect->active = false;
    effect->distributed_state.is_leader = false;
    effect->distributed_state.last_sync_us = 0;

    uint32_t current_offset_us = 0;

    int step_number = 1;
    for (step_t *step = effect->steps_head; step;
         step = step->next, step_number++) {
      if ((step->in == 0) && (step->dwell == 0) && (step->out == 0)) {
        step->dwell = step->time;
      }

      step->offset_us = current_offset_us;
      current_offset_us += ms_to_us(step->time);
    }

    effect->effect_length_us = current_offset_us;
  }

  for (int i = 0; i <= UINT8_MAX; i++) {
    rate_from_fader_level[i] = get_rate_from_fader_level(i);
  }

  effect_state_sync_queue =
      xQueueCreate(SYNC_QUEUE_SIZE, sizeof(effect_state_sync_event_t));
  dmxbox_espnow_register_effect_state_callback(distributed_follower_callback);
}

static uint8_t multiply_levels(uint8_t level1, uint8_t level2) {
  if (level1 == 255)
    return level2;
  if (level2 == 255)
    return level1;
  if (level1 == 0)
    return 0;
  if (level2 == 0)
    return 0;

  return (uint8_t)(level1 * level2 / 255);
}

static void advance_chase_effect_progress(
    effect_t *effect,
    uint8_t rate_raw,
    int64_t time_increment_us
) {
  effect->progress += (time_increment_us * rate_from_fader_level[rate_raw]);

  if (effect->effect_length_us <= effect->progress) {
    effect->first_pass = false;
    effect->progress -= (int)(effect->progress / effect->effect_length_us) *
                        effect->effect_length_us;
  }
}

static void process_chase_effect(
    uint8_t tick_data[DMX_CHANNEL_COUNT],
    effect_t *effect,
    uint8_t effect_level,
    uint8_t rate_raw,
    int64_t time_increment_us
) {
  advance_chase_effect_progress(effect, rate_raw, time_increment_us);

  int step_number = 1;
  for (step_t *step = effect->steps_head; step;
       step = step->next, step_number++) {

    double progress_in_step = effect->progress - step->offset_us;

    if (progress_in_step < 0 && !effect->first_pass) {
      // There might be overlapping steps from the previous pass
      progress_in_step += effect->effect_length_us;
    }

    if (progress_in_step <= 0) {
      continue; // too early
    }

    if (get_step_out_end_us(step) <= progress_in_step) {
      continue; // too late
    }

    uint8_t step_fade_level = 0;
    if (progress_in_step < get_step_in_end_us(step)) {
      step_fade_level = (uint8_t)(255 * progress_in_step / ms_to_us(step->in));
    } else if (progress_in_step < get_step_dwell_end_us(step)) {
      step_fade_level = 255;
    } else if (progress_in_step < get_step_out_end_us(step)) {
      step_fade_level =
          (uint8_t)(255 -
                    (255 * (progress_in_step - get_step_dwell_end_us(step)) /
                     ms_to_us(step->out)));
    }

    uint8_t step_fade_level_adjusted =
        multiply_levels(effect_level, step_fade_level);

    for (step_channel_t *channel = step->channels_head; channel;
         channel = channel->next) {
      uint16_t channel_index = channel->channel - 1;

      uint8_t level = multiply_levels(step_fade_level_adjusted, channel->level);
      tick_data[channel_index] = MAX(tick_data[channel_index], level);
    }
  }
}

static bool should_send_sync(
    effect_distributed_state_t *state,
    uint8_t level,
    uint8_t rate_raw,
    int64_t current_time_us
) {
  // Sync on parameter change
  if (state->last_level != level || state->last_rate_raw != rate_raw) {
    return true;
  }

  // Sync periodicallyj
  if ((state->last_sync_us + distributed_sync_period_us) < current_time_us) {
    return true;
  }

  return false;
}

static void handle_distributed_leader(
    effect_t *effect,
    uint8_t level,
    uint8_t rate_raw,
    int64_t current_time_us
) {
  effect_distributed_state_t *state = &effect->distributed_state;

  if (should_send_sync(state, level, rate_raw, current_time_us)) {
    dmxbox_espnow_send_effect_state(
        effect->id,
        level,
        rate_raw,
        effect->progress,
        effect->first_pass
    );
    state->last_sync_us = current_time_us;
  }

  state->last_level = level;
  state->last_rate_raw = rate_raw;
}

static void handle_distributed_follower(
    effect_t *effect,
    uint8_t *level,
    uint8_t *rate_raw
) {
  effect_distributed_state_t *state = &effect->distributed_state;
  *level = state->last_level;
  *rate_raw = state->last_rate_raw;
}

static void handle_distributed_effect(
    effect_t *effect,
    uint8_t *level,
    uint8_t *rate_raw,
    int64_t current_time_us
) {
  effect_distributed_state_t *state = &effect->distributed_state;

  if (*level != 0) {
    if (!state->is_leader) {
      ESP_LOGI(TAG, "Switching to leader role for effect %d", effect->id);
    }
    state->is_leader = true;
  }
  if (state->is_leader) {
    handle_distributed_leader(effect, *level, *rate_raw, current_time_us);
  } else {
    handle_distributed_follower(effect, level, rate_raw);
  }
}

static void process_effect(
    uint8_t tick_data[DMX_CHANNEL_COUNT],
    effect_t *effect,
    uint8_t effect_level,
    uint8_t rate_raw,
    int64_t current_time_us,
    int64_t time_increment_us
) {
  if (effect->distributed) {
    handle_distributed_effect(
        effect,
        &effect_level,
        &rate_raw,
        current_time_us
    );
  }

  if (effect_level == 0) {
    effect->active = false;
    return;
  }

  if (!effect->active) {
    effect->active = true;
    effect->progress = 0;
    effect->first_pass = true;
    time_increment_us = 0; // Start from beginning
  }

  process_chase_effect(
      tick_data,
      effect,
      effect_level,
      rate_raw,
      time_increment_us
  );
}

static void get_control_data(uint8_t control_data[512]) {
  taskENTER_CRITICAL(&dmxbox_artnet_spinlock);
  const uint8_t *control_data_unsafe =
      dmxbox_artnet_get_universe_data(effect_control_universe_address);
  if (control_data_unsafe) {
    memcpy(control_data, control_data_unsafe, DMX_CHANNEL_COUNT);
  }
  taskEXIT_CRITICAL(&dmxbox_artnet_spinlock);
}

static effect_t *find_effect(uint16_t id) {
  for (effect_t *effect = effects_head; effect; effect = effect->next) {
    if (effect->id == id) {
      return effect;
    }
  }

  return NULL;
}

void handle_sync_queue(int64_t current_time_us, int64_t time_increment_us) {
  effect_state_sync_event_t event;
  while (xQueueReceive(effect_state_sync_queue, &event, 0) == pdTRUE) {
    effect_t *effect = find_effect(event.effect_id);

    if (!effect->distributed) {
      ESP_LOGW(
          TAG,
          "Got sync info for non-distributed effect %d",
          event.effect_id
      );
      continue;
    }

    if (effect->distributed_state.is_leader) {
      ESP_LOGW(TAG, "Got sync info for a leader (effect %d)", event.effect_id);
      continue;
    }

    effect->distributed_state.last_level = event.level;
    effect->distributed_state.last_rate_raw = event.rate_raw;
    effect->progress = event.progress;
    effect->first_pass = event.first_pass;

    // Account for time elapsed since we got the event
    if (event.receive_time_us < current_time_us) {
      advance_chase_effect_progress(
          effect,
          event.rate_raw,
          current_time_us - event.receive_time_us
      );
    }
  }
}

static void
dmxbox_effects_tick(int64_t current_time_us, int64_t time_increment_us) {
  uint8_t control_data[DMX_CHANNEL_COUNT] = {0};
  get_control_data(control_data);

  uint8_t tick_data[DMX_CHANNEL_COUNT] = {0};
  for (effect_t *effect = effects_head; effect; effect = effect->next) {
    uint8_t effect_level = 0;
    uint8_t rate_raw = default_effect_rate_raw;

    if (effect->level_channel) {
      effect_level = control_data[effect->level_channel - 1];
    }
    if (effect->rate_channel) {
      rate_raw = control_data[effect->rate_channel - 1];
    }

    process_effect(
        tick_data,
        effect,
        effect_level,
        rate_raw,
        current_time_us,
        time_increment_us
    );
  }

  if (LOG_DMX_DATA) {
    ESP_LOG_BUFFER_HEX(TAG, tick_data, 16);
  }

  taskENTER_CRITICAL(&dmxbox_effects_spinlock);
  memcpy(dmxbox_effects_data, tick_data, DMX_CHANNEL_COUNT);
  taskEXIT_CRITICAL(&dmxbox_effects_spinlock);

  handle_sync_queue(current_time_us, time_increment_us);
}

void dmxbox_effects_task(void *parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  int64_t last_time_us = esp_timer_get_time();
  int64_t current_time_us = last_time_us;

  while (1) {
    dmxbox_effects_tick(current_time_us, current_time_us - last_time_us);
    vTaskDelayUntil(&xLastWakeTime, EFFECTS_PERIOD / portTICK_PERIOD_MS);

    last_time_us = current_time_us;
    current_time_us = esp_timer_get_time();
  }

  // vTaskDelete(NULL);
}
