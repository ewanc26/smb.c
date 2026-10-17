/* tests/core/runtime.c — the command layer: default behaviour, pause and
 * step, speed, rewind, reset and the overlay flag (#26, #30). */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../src/core/replay.h"
#include "../../src/core/runtime.h"

static uint8_t script_input(unsigned frame) {
  if (frame == 30) return 0x10u;
  if (frame == 31) return 0x00u;
  if (frame >= 60) return (frame % 90 < 45) ? 0x81u : 0x01u;
  return 0x00u;
}

int main(void) {
  /* default: one frame per tick, identical to a plain smbc_frame loop */
  {
    Smbc a, b;
    smbc_init(&a);
    smbc_init(&b);
    SmbcRuntime rt;
    runtime_init(&rt, NULL);
    assert(runtime_speed_percent(&rt) == 100);
    for (unsigned i = 0; i < 120; i++) {
      assert(runtime_tick(&rt, &a, script_input(i), 0x00u) == 1);
      joypad_set_buttons(&b.joypad1, script_input(i));
      smbc_frame(&b);
    }
    assert(rt.frames_run == 120);
    assert(smbc_state_hash(&a) == smbc_state_hash(&b));
  }

  /* pause stops the clock; a step runs exactly one frame; unpausing resumes */
  {
    Smbc a, b;
    smbc_init(&a);
    smbc_init(&b);
    SmbcRuntime rt;
    runtime_init(&rt, NULL);
    for (unsigned i = 0; i < 40; i++) {
      runtime_tick(&rt, &a, script_input(i), 0);
      joypad_set_buttons(&b.joypad1, script_input(i));
      smbc_frame(&b);
    }
    runtime_command(&rt, SMBC_CMD_PAUSE_TOGGLE);
    for (unsigned i = 0; i < 25; i++) {
      assert(runtime_tick(&rt, &a, 0x00u, 0) == 0);
    }
    assert(smbc_state_hash(&a) == smbc_state_hash(&b));
    runtime_command(&rt, SMBC_CMD_STEP);
    assert(runtime_tick(&rt, &a, 0x00u, 0) == 1);
    assert(runtime_tick(&rt, &a, 0x00u, 0) == 0); /* the step is one-shot */
    joypad_set_buttons(&b.joypad1, 0x00u);
    smbc_frame(&b);
    assert(smbc_state_hash(&a) == smbc_state_hash(&b));
    runtime_command(&rt, SMBC_CMD_PAUSE_TOGGLE);
    assert(runtime_tick(&rt, &a, 0x00u, 0) == 1);
  }

  /* speed: 50% runs a frame every second tick, 400% four per tick, and the
   * levels clamp at both ends */
  {
    Smbc smbc;
    smbc_init(&smbc);
    SmbcRuntime rt;
    runtime_init(&rt, NULL);
    runtime_command(&rt, SMBC_CMD_SPEED_DOWN);
    assert(runtime_speed_percent(&rt) == 50);
    uint32_t total = 0;
    for (int i = 0; i < 10; i++) {
      total += runtime_tick(&rt, &smbc, 0x00u, 0);
    }
    assert(total == 5);

    runtime_command(&rt, SMBC_CMD_SPEED_UP);
    runtime_command(&rt, SMBC_CMD_SPEED_UP);
    runtime_command(&rt, SMBC_CMD_SPEED_UP);
    runtime_command(&rt, SMBC_CMD_SPEED_UP); /* clamps at 400% */
    assert(runtime_speed_percent(&rt) == 400);
    assert(runtime_tick(&rt, &smbc, 0x00u, 0) == 4);

    runtime_command(&rt, SMBC_CMD_SPEED_DOWN);
    runtime_command(&rt, SMBC_CMD_SPEED_DOWN);
    runtime_command(&rt, SMBC_CMD_SPEED_DOWN);
    runtime_command(&rt, SMBC_CMD_SPEED_DOWN);
    runtime_command(&rt, SMBC_CMD_SPEED_DOWN);
    runtime_command(&rt, SMBC_CMD_SPEED_DOWN);
    assert(runtime_speed_percent(&rt) == 25); /* clamps at 25% */

    runtime_command(&rt, SMBC_CMD_SPEED_NORMAL);
    assert(runtime_speed_percent(&rt) == 100);
  }

  /* rewind: each rewind undoes one frame, landing on the hash recorded
   * after that many frames */
  {
    enum { CAP = 16 };
    SmbcHistory h;
    uint8_t *store = malloc(CAP * smbc_snapshot_size());
    assert(store != NULL);
    history_init(&h, store, CAP);

    Smbc smbc;
    smbc_init(&smbc);
    SmbcRuntime rt;
    runtime_init(&rt, &h);
    uint64_t hashes[6];
    hashes[0] = smbc_state_hash(&smbc);
    for (unsigned i = 1; i <= 5; i++) {
      runtime_tick(&rt, &smbc, script_input(i), 0);
      hashes[i] = smbc_state_hash(&smbc);
    }
    assert(rt.frames_run == 5);

    /* paused, so a rewind tick moves the clock back and runs nothing */
    runtime_command(&rt, SMBC_CMD_PAUSE_TOGGLE);
    runtime_command(&rt, SMBC_CMD_REWIND);
    assert(runtime_tick(&rt, &smbc, 0x00u, 0) == 0);
    assert(smbc_state_hash(&smbc) == hashes[4]);
    assert(rt.frames_run == 4);

    runtime_command(&rt, SMBC_CMD_REWIND);
    assert(runtime_tick(&rt, &smbc, 0x00u, 0) == 0);
    assert(smbc_state_hash(&smbc) == hashes[3]);
    assert(rt.frames_run == 3);

    /* rewinding past the start stops at the oldest snapshot */
    for (int i = 0; i < 10; i++) {
      runtime_command(&rt, SMBC_CMD_REWIND);
      runtime_tick(&rt, &smbc, 0x00u, 0);
    }
    assert(h.count == 0);
    assert(rt.frames_run == 0);
    assert(smbc_state_hash(&smbc) == hashes[0]);
    free(store);
  }

  /* rewind without a history is a no-op rather than a crash */
  {
    Smbc smbc;
    smbc_init(&smbc);
    SmbcRuntime rt;
    runtime_init(&rt, NULL);
    runtime_tick(&rt, &smbc, 0x00u, 0);
    uint64_t before = smbc_state_hash(&smbc);
    runtime_command(&rt, SMBC_CMD_REWIND);
    runtime_tick(&rt, &smbc, 0x00u, 0);
    assert(smbc_state_hash(&smbc) != before); /* the tick still ran a frame */
  }

  /* reset returns to the fresh reset state, and while paused runs nothing */
  {
    Smbc fresh, smbc;
    smbc_init(&fresh);
    smbc_init(&smbc);
    SmbcRuntime rt;
    runtime_init(&rt, NULL);
    for (unsigned i = 0; i < 90; i++) {
      runtime_tick(&rt, &smbc, script_input(i), 0);
    }
    runtime_command(&rt, SMBC_CMD_PAUSE_TOGGLE);
    runtime_command(&rt, SMBC_CMD_RESET);
    assert(runtime_tick(&rt, &smbc, 0x00u, 0) == 0);
    assert(rt.frames_run == 0);
    assert(smbc_state_hash(&smbc) == smbc_state_hash(&fresh));
  }

  /* the overlay flag toggles and nothing else changes */
  {
    SmbcRuntime rt;
    runtime_init(&rt, NULL);
    assert(!rt.overlay);
    runtime_command(&rt, SMBC_CMD_OVERLAY_TOGGLE);
    assert(rt.overlay && !rt.paused && runtime_speed_percent(&rt) == 100);
    runtime_command(&rt, SMBC_CMD_OVERLAY_TOGGLE);
    assert(!rt.overlay);
  }

  printf("runtime: ok\n");
  return 0;
}
