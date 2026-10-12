/* tests/core/nmi.c — exercises src/core/nmi.c: the timer cascade, the
 * frame counter, and the 7-byte pseudorandom rotate. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/nmi.h"
#include "../../src/core/state.h"

int main(void) {
  Smbc smbc;
  smbc_init(&smbc);

  /* park the mode tree — this test exercises the timer cascade, not the
   * title screen tasks */
  smbc.state.ram[SMBC_OPER_MODE] = 0xffu;

  /* frame timers decrement when TimerControl is clear; with the interval
   * control at 1, only frame timers ($0780-$0794) are touched */
  smbc.state.ram[SMBC_TIMERS] = 5u;
  smbc.state.ram[SMBC_TIMERS + 0x20] = 9u; /* an interval timer */
  smbc.state.ram[SMBC_INTERVAL_TIMER_CONTROL] = 1u;
  nmi_run(&smbc);
  assert(smbc.state.ram[SMBC_TIMERS] == 4u);
  assert(smbc.state.ram[SMBC_TIMERS + 0x20] == 9u);
  assert(smbc.state.ram[SMBC_INTERVAL_TIMER_CONTROL] == 0u);
  assert(smbc.state.ram[SMBC_FRAME_COUNTER] == 1u);

  /* when the interval control expires it re-arms at $14 and interval
   * timers decrement too */
  smbc.state.ram[SMBC_INTERVAL_TIMER_CONTROL] = 0u;
  nmi_run(&smbc);
  assert(smbc.state.ram[SMBC_TIMERS] == 3u);
  assert(smbc.state.ram[SMBC_TIMERS + 0x20] == 8u);
  assert(smbc.state.ram[SMBC_INTERVAL_TIMER_CONTROL] == 0x14u);

  /* TimerControl postpones the whole cascade until it runs down */
  smbc.state.ram[SMBC_TIMER_CONTROL] = 2u;
  uint8_t t = smbc.state.ram[SMBC_TIMERS];
  nmi_run(&smbc);
  assert(smbc.state.ram[SMBC_TIMER_CONTROL] == 1u);
  assert(smbc.state.ram[SMBC_TIMERS] == t);
  nmi_run(&smbc);
  assert(smbc.state.ram[SMBC_TIMER_CONTROL] == 0u);
  assert(smbc.state.ram[SMBC_TIMERS] == (uint8_t)(t - 1u));

  /* pause d0 skips timers and the frame counter */
  uint8_t fc = smbc.state.ram[SMBC_FRAME_COUNTER];
  smbc.state.ram[SMBC_GAME_PAUSE_STATUS] |= 0x01u;
  nmi_run(&smbc);
  assert(smbc.state.ram[SMBC_FRAME_COUNTER] == fc);
  smbc.state.ram[SMBC_GAME_PAUSE_STATUS] &= 0xfeu;

  /* LFSR: seed $A5 (d1=0), byte 1 = $00 (d1=0) -> carry in 0.
   * $A5 ror -> $52 carry out 1 -> byte 1 becomes $80. */
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG] = 0xa5u;
  for (int i = 1; i < 7; i++) {
    smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + i] = 0x00u;
  }
  nmi_run(&smbc);
  assert(smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG] == 0x52u);
  assert(smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1] == 0x80u);

  printf("tests/core/nmi: ok\n");
  return 0;
}
