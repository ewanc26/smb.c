/* core/runtime.c — see core/runtime.h for the contract. */
#include "runtime.h"

#include "../hw/joypad.h"

#define SMBC_SPEED_NORMAL_INDEX 2u /* 100% */

static const uint32_t SMBC_SPEED_PERCENT[SMBC_SPEED_LEVELS] = {25, 50, 100,
                                                               200, 400};

void runtime_init(SmbcRuntime *rt, SmbcHistory *history) {
  rt->paused = false;
  rt->speed_index = SMBC_SPEED_NORMAL_INDEX;
  rt->accumulator = 0;
  rt->overlay = false;
  rt->step_requested = false;
  rt->reset_requested = false;
  rt->rewind_requested = false;
  rt->frames_run = 0;
  rt->history = history;
}

void runtime_command(SmbcRuntime *rt, SmbcCommand cmd) {
  switch (cmd) {
  case SMBC_CMD_PAUSE_TOGGLE:
    rt->paused = !rt->paused;
    rt->accumulator = 0;
    break;
  case SMBC_CMD_STEP:
    if (rt->paused) {
      rt->step_requested = true;
    }
    break;
  case SMBC_CMD_RESET:
    rt->reset_requested = true;
    break;
  case SMBC_CMD_SPEED_UP:
    if (rt->speed_index + 1u < SMBC_SPEED_LEVELS) {
      rt->speed_index++;
    }
    break;
  case SMBC_CMD_SPEED_DOWN:
    if (rt->speed_index > 0) {
      rt->speed_index--;
    }
    break;
  case SMBC_CMD_SPEED_NORMAL:
    rt->speed_index = SMBC_SPEED_NORMAL_INDEX;
    break;
  case SMBC_CMD_REWIND:
    rt->rewind_requested = true;
    break;
  case SMBC_CMD_OVERLAY_TOGGLE:
    rt->overlay = !rt->overlay;
    break;
  }
}

uint32_t runtime_speed_percent(const SmbcRuntime *rt) {
  return SMBC_SPEED_PERCENT[rt->speed_index];
}

uint32_t runtime_tick(SmbcRuntime *rt, Smbc *smbc, uint8_t p1, uint8_t p2) {
  if (rt->reset_requested) {
    rt->reset_requested = false;
    smbc_init(smbc);
    rt->frames_run = 0;
    rt->accumulator = 0;
    if (rt->history != NULL) {
      rt->history->count = 0;
    }
  }

  if (rt->rewind_requested) {
    rt->rewind_requested = false;
    if (rt->history != NULL && history_pop(rt->history, smbc) &&
        rt->frames_run > 0) {
      rt->frames_run--;
    }
  }

  uint32_t frames;
  if (rt->paused) {
    frames = rt->step_requested ? 1u : 0u;
    rt->accumulator = 0;
  } else {
    rt->accumulator += SMBC_SPEED_PERCENT[rt->speed_index];
    frames = rt->accumulator / 100u;
    rt->accumulator %= 100u;
  }
  rt->step_requested = false;

  for (uint32_t i = 0; i < frames; i++) {
    /* the snapshot is taken before the frame, so one rewind undoes one
     * frame and the history always matches the frames actually run */
    if (rt->history != NULL) {
      history_push(rt->history, smbc);
    }
    joypad_set_buttons(&smbc->joypad1, p1);
    joypad_set_buttons(&smbc->joypad2, p2);
    smbc_frame(smbc);
    rt->frames_run++;
  }
  return frames;
}
