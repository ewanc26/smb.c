/* tests/core/pause.c — exercises src/core/pause.c: start toggles pause in
 * game mode task 3, the timer gates re-entry, and the sound queue write. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/pause.h"
#include "../../src/core/state.h"

int main(void) {
  Smbc smbc;
  smbc_init(&smbc);

  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x03u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = SMBC_PAD_START;

  pause_routine(&smbc);
  assert(smbc.state.ram[SMBC_GAME_PAUSE_TIMER] == 0x2bu);
  assert(smbc.state.ram[SMBC_GAME_PAUSE_STATUS] == 0x81u);
  assert(smbc.state.ram[SMBC_PAUSE_SOUND_QUEUE] == 0x01u);

  /* while the timer runs down, presses are ignored */
  pause_routine(&smbc);
  assert(smbc.state.ram[SMBC_GAME_PAUSE_TIMER] == 0x2au);
  assert(smbc.state.ram[SMBC_GAME_PAUSE_STATUS] == 0x81u);

  /* the d7 residual flag blocks a held start: releasing it first clears the
   * flag (and leaves pause itself, d0, untouched) */
  smbc.state.ram[SMBC_GAME_PAUSE_TIMER] = 0x00u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = 0x00u;
  pause_routine(&smbc);
  assert(smbc.state.ram[SMBC_GAME_PAUSE_STATUS] == 0x01u);

  /* a fresh press then toggles pause back off */
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = SMBC_PAD_START;
  pause_routine(&smbc);
  assert(smbc.state.ram[SMBC_GAME_PAUSE_STATUS] == 0x80u);
  assert(smbc.state.ram[SMBC_PAUSE_SOUND_QUEUE] == 0x02u);
  smbc.state.ram[SMBC_GAME_PAUSE_TIMER] = 0x00u;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = 0x00u;
  pause_routine(&smbc);
  assert(smbc.state.ram[SMBC_GAME_PAUSE_STATUS] == 0x00u);

  /* outside game mode (and not victory), start does nothing */
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_TITLE;
  smbc.state.ram[SMBC_SAVED_JOYPAD1_BITS] = SMBC_PAD_START;
  pause_routine(&smbc);
  assert(smbc.state.ram[SMBC_GAME_PAUSE_STATUS] == 0x00u);

  printf("tests/core/pause: ok\n");
  return 0;
}
