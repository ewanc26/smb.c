/* core/pause.c — see core/pause.h for the contract. */
#include "pause.h"

#include "frame.h"
#include "state.h"

void pause_routine(Smbc *smbc) {
  /* smb.asm: PauseRoutine */
  CoreState *state = &smbc->state;
  uint8_t mode = state->ram[SMBC_OPER_MODE];
  if (mode != SMBC_MODE_VICTORY) {
    if (mode != SMBC_MODE_GAME) {
      return; /* smb.asm: ExitPause */
    }
    if (state->ram[SMBC_OPER_MODE_TASK] != 0x03) {
      return; /* not running the game engine */
    }
  }
  if (state->ram[SMBC_GAME_PAUSE_TIMER] != 0) {
    state->ram[SMBC_GAME_PAUSE_TIMER]--;
    return;
  }
  if ((state->ram[SMBC_SAVED_JOYPAD1_BITS] & SMBC_PAD_START) != 0) {
    if ((state->ram[SMBC_GAME_PAUSE_STATUS] & 0x80u) != 0) {
      return; /* smb.asm: ExitPause — residual timer-flag check */
    }
    state->ram[SMBC_GAME_PAUSE_TIMER] = 0x2bu;
    uint8_t status = state->ram[SMBC_GAME_PAUSE_STATUS];
    state->ram[SMBC_PAUSE_SOUND_QUEUE] = (uint8_t)(status + 1); /* tay; iny; sty */
    state->ram[SMBC_GAME_PAUSE_STATUS] = (uint8_t)((status ^ 0x01u) | 0x80u);
  } else {
    state->ram[SMBC_GAME_PAUSE_STATUS] &= 0x7fu; /* smb.asm: ClrPauseTimer */
  }
}
