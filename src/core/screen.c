/* core/screen.c — see core/screen.h for the contract. */
#include "screen.h"

#include "../hw/ppu.h"
#include "frame.h"

void write_ppu_reg1(Smbc *smbc, uint8_t value) {
  /* smb.asm: WritePPUReg1 */
  ppu_write(&smbc->ppu, 0, value);
  smbc->state.ram[SMBC_MIRROR_PPU_CTRL1] = value;
}

void init_scroll(Smbc *smbc, uint8_t value) {
  /* smb.asm: InitScroll */
  ppu_write(&smbc->ppu, 5, value);
  ppu_write(&smbc->ppu, 5, value);
}

void update_screen(Smbc *smbc) {
  /* TODO(port): smb.asm: UpdateScreen / WriteBufferToScreen */
  (void)smbc;
}

/* smb.asm: TopScoreCheck — six-digit subtraction, lowest digit first; the
 * score is copied only if no borrow survives the final digit. */
static void top_score_check(CoreState *state, int x) {
  int y = 5;
  int no_borrow = 1; /* sec on entry */
  for (int i = 0; i < 6; i++) {
    int diff = state->ram[SMBC_PLAYER_SCORE_DISPLAY + x] -
               state->ram[SMBC_TOP_SCORE_DISPLAY + y] - (1 - no_borrow);
    no_borrow = diff >= 0;
    x--;
    y--;
  }
  if (!no_borrow) {
    return; /* smb.asm: NoTopSc */
  }
  x++;
  y++;
  for (int i = 0; i < 6; i++) {
    state->ram[SMBC_TOP_SCORE_DISPLAY + y] =
        state->ram[SMBC_PLAYER_SCORE_DISPLAY + x];
    x++;
    y++;
  }
}

void update_top_score(CoreState *state) {
  /* smb.asm: UpdateTopScore — mario's score, then luigi's */
  top_score_check(state, 0x05);
  top_score_check(state, 0x0b);
}
