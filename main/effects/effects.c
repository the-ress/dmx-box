#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <math.h>
#include <sdkconfig.h>
#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "artnet/artnet.h"
#include "const.h"
#include "effects.h"
#include "hashmap.h"
#include "dmxbox_storage.h"

static const char *TAG = "effects";

#define EFFECTS_PERIOD 30
// #define EFFECTS_PERIOD 300
#define LOG_DMX_DATA false

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

// TODO use uint32_t instead of int64_t

// actually microseconds, TODO rename or something
static const uint32_t ticks_per_millisecond = 1000;

struct step_channel {
  struct step_channel *next;

  uint16_t channel;
  uint8_t level;
};

struct step {
  struct step *next;

  uint32_t time;
  uint32_t in;
  uint32_t dwell;
  uint32_t out;

  struct step_channel *channels_head;

  // precalculated values
  uint32_t time_ticks;
  uint32_t in_ticks;
  uint32_t dwell_ticks;
  uint32_t out_ticks;

  uint32_t in_end_ticks;
  uint32_t dwell_end_ticks;
  uint32_t out_end_ticks;

  uint32_t offset;
  uint32_t out_end_offset;

  // internal state
  bool active;
  int64_t total_offset; // only relevant when active
};

struct effect {
  struct effect *next;

  // private int rateSubmasterNumber;
  struct step *steps_head;

  // precalculated values
  uint32_t effect_length;

  // internal state
  bool active;
  int rate;
  int64_t initial_ticks;
  int64_t saved_total_offset_internal_value;
};

static struct step_channel step3_channel2 =
    {.next = NULL, .channel = 6, .level = 127};

static struct step_channel step3_channel1 =
    {.next = &step3_channel2, .channel = 5, .level = 255};

static struct step step3 = {
    .next = NULL,
    .time = 1000,
    .in = 250,
    .dwell = 250,
    .out = 250,
    .channels_head = &step3_channel1,
};

static struct step_channel step2_channel2 =
    {.next = NULL, .channel = 4, .level = 127};

static struct step_channel step2_channel1 =
    {.next = &step2_channel2, .channel = 3, .level = 255};

static struct step step2 = {
    .next = &step3,
    .time = 1000,
    .in = 250,
    .dwell = 250,
    .out = 250,
    .channels_head = &step2_channel1,
};

static struct step_channel step1_channel2 =
    {.next = NULL, .channel = 2, .level = 127};

static struct step_channel step1_channel1 =
    {.next = &step1_channel2, .channel = 1, .level = 255};

static struct step step1 = {
    .next = &step2,
    .time = 1000,
    .in = 250,
    .dwell = 250,
    .out = 250,
    .channels_head = &step1_channel1,
};

static struct effect effect1 = {
    .next = NULL,
    .steps_head = &step1,
};

static struct effect *effects_head = &effect1;

// last_data = calloc(DMX_CHANNEL_COUNT, sizeof(*last_data));
// free(entry->last_data);

portMUX_TYPE dmxbox_effects_spinlock = portMUX_INITIALIZER_UNLOCKED;
uint8_t dmxbox_effects_data[DMX_CHANNEL_COUNT] = {0};

void dmxbox_effects_initialize() {
  for (struct effect *effect = effects_head; effect; effect = effect->next) {
    effect->active = false;
    effect->initial_ticks = 0;
    effect->rate = 100;
    effect->saved_total_offset_internal_value = 0;

    uint32_t current_offset = 0;

    int step_number = 1;
    for (struct step *step = effect->steps_head; step;
         step = step->next, step_number++) {
      step->active = false;

      step->time_ticks = step->time * ticks_per_millisecond;
      step->in_ticks = step->in * ticks_per_millisecond;
      step->dwell_ticks = step->dwell * ticks_per_millisecond;
      step->out_ticks = step->out * ticks_per_millisecond;

      if ((step->in_ticks == 0) && (step->dwell_ticks == 0) &&
          (step->out_ticks == 0)) {
        step->dwell_ticks = step->time_ticks;
      }

      step->in_end_ticks = step->in_ticks;
      step->dwell_end_ticks = step->in_end_ticks + step->dwell_ticks;
      step->out_end_ticks = step->dwell_end_ticks + step->out_ticks;

      step->offset = current_offset;
      step->out_end_offset = step->offset + step->out_end_ticks;

      current_offset += step->time_ticks;
    }

    effect->effect_length = current_offset;
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

  return (int)(pow(2, ((double)level - 127) / 30.035) * 104.5 - 4.5);
}

static int64_t
update_chase_effect_rate(struct effect *effect, int64_t ticks, int new_rate) {
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

static void
activate_new_steps(struct effect *effect, int64_t current_total_offset) {
  if (effect->effect_length == 0)
    return;

  uint32_t current_offset = current_total_offset % effect->effect_length;
  int64_t current_offset_base = current_total_offset - current_offset;

  // TODO: check if firing multiple steps at once (even at the end of the cycle)
  // works

  if (effect->effect_length <= current_total_offset) {
    // start any steps we might have missed at the end of previous cycle
    int step_number = 1;
    for (struct step *step = effect->steps_head; step;
         step = step->next, step_number++) {

      if ((step->out_end_offset - effect->effect_length) <= (current_offset)) {
        continue; // step already finished
      }

      step->active = true;
      step->total_offset =
          (current_offset_base - effect->effect_length + step->offset);
    }
  }

  int step_number = 1;
  for (struct step *step = effect->steps_head; step;
       step = step->next, step_number++) {

    if (current_offset < step->offset) {
      break; // it's not yet time to start this step (or any following steps)
    }

    if (step->out_end_offset <= current_offset) {
      continue; // step already finished
    }

    step->active = true;
    // if step is already active from the previous cycle, this will restart it
    step->total_offset = (current_offset_base + step->offset);
  }
}

static void process_chase_effect(
    uint8_t tick_data[DMX_CHANNEL_COUNT],
    struct effect *effect,
    uint8_t effect_level,
    uint8_t rate_raw,
    int64_t ticks
) {
  int new_rate = rate_from_fader_level(rate_raw);
  int64_t current_total_offset =
      update_chase_effect_rate(effect, ticks, new_rate);

  activate_new_steps(effect, current_total_offset);

  int step_number = 1;
  for (struct step *step = effect->steps_head; step;
       step = step->next, step_number++) {

    if (!step->active) {
      continue;
    }

    uint32_t current_step_offset = current_total_offset - step->total_offset;

    uint8_t step_fade_level = 0;

    if (current_step_offset < step->in_end_ticks) {
      step_fade_level = (uint8_t)(255 * current_step_offset / step->in_ticks);
    } else if (current_step_offset < step->dwell_end_ticks) {
      step_fade_level = 255;
    } else if (current_step_offset < step->out_end_ticks) {
      step_fade_level =
          (uint8_t)(255 - (255 * (current_step_offset - step->dwell_end_ticks) /
                           step->out_ticks));
    } else {
      step->active = false;
      continue;
    }

    uint8_t step_fade_level_adjusted =
        multiply_levels(effect_level, step_fade_level);

    for (struct step_channel *channel = step->channels_head; channel;
         channel = channel->next) {
      uint16_t channel_index = channel->channel - 1;

      uint8_t level = multiply_levels(step_fade_level_adjusted, channel->level);
      tick_data[channel_index] = MAX(tick_data[channel_index], level);
    }
  }
}

static void process_effect(
    uint8_t tick_data[DMX_CHANNEL_COUNT],
    struct effect *effect,
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

  taskENTER_CRITICAL(&dmxbox_artnet_spinlock);
  uint8_t effect_level = dmxbox_artnet_in_data[11 - 1];
  uint8_t rate_raw = dmxbox_artnet_in_data[12 - 1];
  taskEXIT_CRITICAL(&dmxbox_artnet_spinlock);

  for (struct effect *effect = effects_head; effect; effect = effect->next) {
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
  while (1) {
    dmxbox_effects_tick();
    // TOOD change to vTaskDelayUntil
    vTaskDelay(EFFECTS_PERIOD / portTICK_PERIOD_MS);
  }

  // vTaskDelete(NULL);
}
