/* core/timer.c — see core/timer.h for the contract. */
#include "timer.h"

#include "../data/music.h"
#include "collide.h"
#include "screen.h"

/* smb.asm: RunGameTimer */
void run_game_timer(CoreState *st) {
  if (st->ram[SMBC_OPER_MODE] == 0x00u) return;                /* ExGTimer */
  uint8_t routine = st->ram[SMBC_GAME_ENGINE_SUBROUTINE];
  if (routine < 0x08u) return;                                 /* ExGTimer */
  if (routine == 0x0bu) return;                                /* ExGTimer */
  if (st->ram[SMBC_PLAYER_Y_HIGH_POS] >= 0x02u) return;        /* ExGTimer */
  if (st->ram[SMBC_GAME_TIMER_CTRL_TIMER] != 0x00u) return;    /* ExGTimer */

  uint8_t first = st->ram[SMBC_GAME_TIMER_DISPLAY];
  uint8_t second = st->ram[SMBC_GAME_TIMER_DISPLAY + 1u];
  uint8_t third = st->ram[SMBC_GAME_TIMER_DISPLAY + 2u];
  if ((first | second | third) == 0x00u) {
    /* TimeUpOn — A always holds zero here */
    st->ram[SMBC_PLAYER_STATUS] = 0x00u;
    force_injury(st);
    st->ram[SMBC_GAME_TIMER_EXPIRED_FLAG]++;
    return;
  }
  if (first == 0x01u && (second | third) == 0x00u) {
    st->ram[SMBC_EVENT_MUSIC_QUEUE] = SMBC_EVENT_MUSIC_TIME_RUNNING_OUT;
  }
  /* ResGTCtrl — reset control, decrement the last digit, redraw */
  st->ram[SMBC_GAME_TIMER_CTRL_TIMER] = 0x18u;
  st->ram[SMBC_DIGIT_MODIFIER + 5u] = 0xffu;
  digits_math_routine(st, 0x23u);
  print_status_bar_numbers(st, 0xa4u);
}
