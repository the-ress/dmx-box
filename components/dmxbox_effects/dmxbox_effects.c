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
#include "dmxbox_storage.h"

static const char *TAG = "effects";

#define EFFECTS_PERIOD 30
// #define EFFECTS_PERIOD 300
#define LOG_DMX_DATA false

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

// TODO use uint32_t instead of int64_t?

// actually microseconds, TODO rename or something
static const uint32_t ticks_per_millisecond = 1000;

static const uint8_t default_effect_rate_raw = 127;

static uint16_t effect_control_universe_address = 1;

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
  uint32_t offset_ticks;

  // internal state
  bool active;
  int64_t total_offset; // only relevant when active
} step_t;

typedef struct effect {
  struct effect *next;

  uint16_t level_channel;
  uint16_t rate_channel;

  step_t *steps_head;

  // precalculated values
  uint32_t effect_length_ticks;

  // internal state
  bool active;
  int rate;
  int64_t initial_ticks;
  int64_t saved_total_offset_internal_value;
} effect_t;

static step_channel_t *step_channel_alloc() {
  return calloc(1, sizeof(step_channel_t));
}

static void step_channel_free(step_channel_t *data) { free(data); }

static step_t *step_alloc() { return calloc(1, sizeof(step_t)); }

static void step_free(step_t *data) { free(data); }

static effect_t *effect_alloc() { return calloc(1, sizeof(effect_t)); }

static void effect_free(effect_t *data) { free(data); }

static effect_t *effects_head = NULL;

portMUX_TYPE dmxbox_effects_spinlock = portMUX_INITIALIZER_UNLOCKED;
uint8_t dmxbox_effects_data[DMX_CHANNEL_COUNT] = {0};

static uint32_t ms_to_ticks(uint32_t value) {
  return value * ticks_per_millisecond;
}

static uint32_t get_step_in_end_ticks(step_t *step) {
  return ms_to_ticks(step->in);
}

static uint32_t get_step_dwell_end_ticks(step_t *step) {
  return ms_to_ticks(step->in + step->dwell);
}

static uint32_t get_step_out_end_ticks(step_t *step) {
  return ms_to_ticks(step->in + step->dwell + step->out);
}

static uint32_t get_step_out_end_offset(step_t *step) {
  return step->offset_ticks + get_step_out_end_ticks(step);
}

void dmxbox_effects_initialize() {
  dmxbox_storage_effect_step_t *step1 =  calloc(
      1,
      sizeof(dmxbox_storage_effect_step_t) +
          (2 - 1) * sizeof(dmxbox_storage_channel_level_t)
  );
  dmxbox_storage_effect_step_t *step2 = calloc(
      1,
      sizeof(dmxbox_storage_effect_step_t) +
          (2 - 1) * sizeof(dmxbox_storage_channel_level_t)
  );
  dmxbox_storage_effect_step_t *step3 = calloc(
      1,
      sizeof(dmxbox_storage_effect_step_t) +
          (5 - 1) * sizeof(dmxbox_storage_channel_level_t)
  );

  step1->time = 1000;
  step1->in = 250;
  step1->dwell = 250;
  step1->out = 250;

  step1->channels[0].channel.index = 1;
  step1->channels[0].level = 255;

  step1->channels[1].channel.index = 2;
  step1->channels[1].level = 255;

  step2->time = 1000;
  step2->in = 250;
  step2->dwell = 250;
  step2->out = 250;

  step2->channels[0].channel.index = 8;
  step2->channels[0].level = 255;
  step2->channels[1].channel.index = 10;
  step2->channels[1].level = 255;

  step3->time = 1000;
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

  uint16_t effect_id = 1;

  dmxbox_storage_effect_step_set(effect_id, 1, 2, step1);
  dmxbox_storage_effect_step_set(effect_id, 2, 2, step2);
  dmxbox_storage_effect_step_set(effect_id, 3, 5, step3);

  free(step1);
  free(step2);
  free(step3);

  effect_t *effect1 = effect_alloc();
  effect1->level_channel = 1;
  effect1->rate_channel = 2;

  size_t step_count = 3;

  step_t *steps_tail = NULL;
  for (uint16_t step_id = 1; step_id <= step_count; step_id++) {
    ESP_LOGI(TAG, "step %d", step_id);

    size_t channel_count;
    dmxbox_storage_effect_step_t *step_data;
    ESP_ERROR_CHECK(dmxbox_storage_effect_step_get(
        effect_id,
        step_id,
        &channel_count,
        &step_data
    ));

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
    for (size_t i = 0; i < channel_count; i++) {
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

  for (effect_t *effect = effects_head; effect; effect = effect->next) {
    effect->active = false;
    effect->initial_ticks = 0;
    effect->rate = 100;
    effect->saved_total_offset_internal_value = 0;

    uint32_t current_offset = 0;

    int step_number = 1;
    for (step_t *step = effect->steps_head; step;
         step = step->next, step_number++) {
      step->active = false;

      if ((step->in == 0) && (step->dwell == 0) && (step->out == 0)) {
        step->dwell = step->time;
      }

      step->offset_ticks = current_offset;
      current_offset += ms_to_ticks(step->time);
    }

    effect->effect_length_ticks = current_offset;
  }
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

static int rate_from_fader_level(uint8_t level) {
  if (level == 0) {
    return 0;
  }

  // 2^((x - 127) / 30.035) * 104.5 - 4.5 works out to:
  // around 1 at x=1
  // 100 at x=127
  // around 2000 at x=255
  return (int)(pow(2, ((double)level - 127) / 30.035) * 104.5 - 4.5);
}

static int64_t
update_chase_effect_rate(effect_t *effect, int64_t ticks, int new_rate) {
  if (effect->rate != 0) // Effect isn't paused
  {
    effect->saved_total_offset_internal_value =
        effect->rate * (ticks - effect->initial_ticks);
  }

  if (effect->rate != new_rate) {
    if (new_rate != 0) // Effect isn't becoming paused
    {
      effect->initial_ticks =
          ticks - effect->saved_total_offset_internal_value / new_rate;
    }
    effect->rate = new_rate;
  }

  return effect->saved_total_offset_internal_value / 100;
}

static void activate_new_steps(effect_t *effect, int64_t current_total_offset) {
  if (effect->effect_length_ticks == 0)
    return;

  uint32_t current_offset = current_total_offset % effect->effect_length_ticks;
  int64_t current_offset_base = current_total_offset - current_offset;

  // TODO: check if firing multiple steps at once (even at the end of the cycle)
  // works

  if (effect->effect_length_ticks <= current_total_offset) {
    // start any steps we might have missed at the end of previous cycle
    int step_number = 1;
    for (step_t *step = effect->steps_head; step;
         step = step->next, step_number++) {

      if ((get_step_out_end_offset(step) - effect->effect_length_ticks) <=
          (current_offset)) {
        continue; // step already finished
      }

      step->active = true;
      step->total_offset =
          (current_offset_base - effect->effect_length_ticks +
           step->offset_ticks);
    }
  }

  int step_number = 1;
  for (step_t *step = effect->steps_head; step;
       step = step->next, step_number++) {

    if (current_offset < step->offset_ticks) {
      break; // it's not yet time to start this step (or any following steps)
    }

    if (get_step_out_end_offset(step) <= current_offset) {
      continue; // step already finished
    }

    step->active = true;
    // if step is already active from the previous cycle, this will restart it
    step->total_offset = (current_offset_base + step->offset_ticks);
  }
}

static void process_chase_effect(
    uint8_t tick_data[DMX_CHANNEL_COUNT],
    effect_t *effect,
    uint8_t effect_level,
    uint8_t rate_raw,
    int64_t ticks
) {
  int new_rate = rate_from_fader_level(rate_raw);
  int64_t current_total_offset =
      update_chase_effect_rate(effect, ticks, new_rate);

  activate_new_steps(effect, current_total_offset);

  int step_number = 1;
  for (step_t *step = effect->steps_head; step;
       step = step->next, step_number++) {

    if (!step->active) {
      continue;
    }

    uint32_t current_step_offset = current_total_offset - step->total_offset;

    uint8_t step_fade_level = 0;

    if (current_step_offset < get_step_in_end_ticks(step)) {
      step_fade_level =
          (uint8_t)(255 * current_step_offset / ms_to_ticks(step->in));
    } else if (current_step_offset < get_step_dwell_end_ticks(step)) {
      step_fade_level = 255;
    } else if (current_step_offset < get_step_out_end_ticks(step)) {
      step_fade_level =
          (uint8_t)(255 -
                    (255 *
                     (current_step_offset - get_step_dwell_end_ticks(step)) /
                     ms_to_ticks(step->out)));
    } else {
      step->active = false;
      continue;
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

static void process_effect(
    uint8_t tick_data[DMX_CHANNEL_COUNT],
    effect_t *effect,
    uint8_t effect_level,
    uint8_t rate_raw,
    int64_t ticks
) {
  if (effect_level == 0) {
    effect->active = false;
    return;
  }

  if (!effect->active) {
    effect->initial_ticks = ticks;
    effect->active = true;
  }

  process_chase_effect(tick_data, effect, effect_level, rate_raw, ticks);
}

static void dmxbox_effects_tick() {
  uint8_t tick_data[DMX_CHANNEL_COUNT] = {0};
  int64_t ticks = esp_timer_get_time();

  uint8_t control_data[DMX_CHANNEL_COUNT] = {0};

  taskENTER_CRITICAL(&dmxbox_artnet_spinlock);
  const uint8_t *control_data_unsafe =
      dmxbox_artnet_get_universe_data(effect_control_universe_address);
  if (control_data_unsafe) {
    memcpy(control_data, control_data_unsafe, DMX_CHANNEL_COUNT);
  }
  taskEXIT_CRITICAL(&dmxbox_artnet_spinlock);

  for (effect_t *effect = effects_head; effect; effect = effect->next) {
    uint8_t effect_level = 0;
    uint8_t rate_raw = default_effect_rate_raw;

    if (effect->level_channel) {
      effect_level = control_data[effect->level_channel - 1];
    }
    if (effect->rate_channel) {
      rate_raw = control_data[effect->rate_channel - 1];
    }

    process_effect(tick_data, effect, effect_level, rate_raw, ticks);
  }

  if (LOG_DMX_DATA) {
    ESP_LOG_BUFFER_HEX(TAG, tick_data, 16);
  }

  taskENTER_CRITICAL(&dmxbox_effects_spinlock);
  memcpy(dmxbox_effects_data, tick_data, DMX_CHANNEL_COUNT);
  taskEXIT_CRITICAL(&dmxbox_effects_spinlock);
}

void dmxbox_effects_task(void *parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (1) {
    dmxbox_effects_tick();
    vTaskDelayUntil(&xLastWakeTime, EFFECTS_PERIOD / portTICK_PERIOD_MS);
  }

  // vTaskDelete(NULL);
}
