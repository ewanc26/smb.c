/* core/runtime.h — the emulator-style command layer (#26, #30).
 *
 * Owns: pause, single-frame step, emulation speed, reset, rewind and the
 * diagnostics-overlay flag, as one set of commands the frontend sends and
 * one tick function that applies them. Every decision here is frame-indexed
 * and deterministic: speed is a fixed percentage accumulated per tick, never
 * a wall-clock reading.
 *
 * Not owned: key bindings and presentation (host/), file I/O for save states
 * and screenshots (host/), and any timing of the host loop itself.
 *
 * Default experience: a fresh runtime runs at 100%, unpaused, with the
 * overlay off, so an unmodified session behaves exactly like smbc_frame.
 */
#ifndef SMBC_CORE_RUNTIME_H
#define SMBC_CORE_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

#include "frame.h"
#include "snapshot.h"

typedef enum {
  SMBC_CMD_PAUSE_TOGGLE,
  SMBC_CMD_STEP,        /* one frame while paused */
  SMBC_CMD_RESET,       /* smb.asm: Start, as if the console were reset */
  SMBC_CMD_SPEED_UP,    /* next speed level */
  SMBC_CMD_SPEED_DOWN,  /* previous speed level */
  SMBC_CMD_SPEED_NORMAL,
  SMBC_CMD_REWIND,      /* restore the frame before the last one run */
  SMBC_CMD_OVERLAY_TOGGLE
} SmbcCommand;

#define SMBC_SPEED_LEVELS 5u

typedef struct {
  bool paused;
  uint8_t speed_index;       /* into SMBC_SPEED_PERCENT */
  uint32_t accumulator;      /* percent-of-a-frame carried between ticks */
  bool overlay;              /* diagnostics overlay visible */
  bool step_requested;
  bool reset_requested;
  bool rewind_requested;
  uint32_t frames_run;       /* frames executed on this timeline */
  SmbcHistory *history;      /* host-provided ring; NULL disables rewind */
} SmbcRuntime;

void runtime_init(SmbcRuntime *rt, SmbcHistory *history);

/* Queue a command. Takes effect on the next runtime_tick. */
void runtime_command(SmbcRuntime *rt, SmbcCommand cmd);

/* Advance one host tick with the held buttons for both pads. Applies any
 * queued reset or rewind, then runs the frames this tick is due (none while
 * paused unless a step is queued). Returns the frames run. */
uint32_t runtime_tick(SmbcRuntime *rt, Smbc *smbc, uint8_t p1, uint8_t p2);

/* Emulation speed as a percentage of 60 Hz: 25, 50, 100, 200, 400. */
uint32_t runtime_speed_percent(const SmbcRuntime *rt);

#endif /* SMBC_CORE_RUNTIME_H */
