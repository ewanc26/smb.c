/* tests/core/move.c — exercises src/core/move.c: the horizontal
 * mover and the ImposeGravity family. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/move.h"

static void horizontal(void) {
  /* smb.asm: MoveObjectHorizontally — the low nybble feeds the
   * fractional counter, the high nybble is the whole-pixel adder */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x18u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x50u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] = 0x01u;
  uint8_t a = move_enemy_horizontally(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_MOVE_FORCE + 0] == 0x80u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x51u);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] == 0x01u);
  assert(a == 0x01u);

  /* the fractional carry adds an extra pixel */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x18u;
  smbc.state.ram[SMBC_ENEMY_X_MOVE_FORCE + 0] = 0x90u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x50u;
  a = move_enemy_horizontally(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_MOVE_FORCE + 0] == 0x10u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x52u);
  assert(a == 0x02u);

  /* a negative speed sign-extends the high nybble and borrows from
   * the page only when the pixel wraps */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0xf8u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x50u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] = 0x01u;
  a = move_enemy_horizontally(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x4fu);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] == 0x01u);
  assert(a == 0xffu);

  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0xf8u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] = 0x01u;
  (void)move_enemy_horizontally(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0xffu);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] == 0x00u);
}

static void gravity_basic(void) {
  /* smb.asm: MoveD_EnemyVertically — the force counter grows by $3d
   * each frame and carries into the speed */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x50u;
  smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] = 0x01u;
  move_d_enemy_vertically(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x3du);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x50u);

  /* the speed clamps at the maximum of three */
  for (int i = 0; i < 60; i++) move_d_enemy_vertically(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0x03u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] > 0x50u);

  /* the spiny's egg state uses the falling platform's amount */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_STATE + 0] = 0x05u;
  move_d_enemy_vertically(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x20u);
}

static void gravity_upwards(void) {
  /* smb.asm: ImposeGravity — a negative speed moves the object up and
   * sign-extends into the high byte */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] = 0xfeu; /* -2 */
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x50u;
  smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] = 0x01u;
  move_d_enemy_vertically(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x4eu);
  assert(smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] == 0x01u);

  /* crossing the unit boundary decrements the high byte */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] = 0xfeu;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] = 0x01u;
  move_d_enemy_vertically(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0xffu);
  assert(smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] == 0x00u);
}

static void red_p_troopa_gravity(void) {
  /* smb.asm: MoveRedPTroopa with the up flag — the upward amount
   * (twice the downward) undoes the growth, and the speed clamps at
   * the negative maximum */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x80u;
  smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] = 0x01u;
  move_red_p_troopa(&smbc.state, 0x00u, 0x01u);
  /* force 3 - 6 borrows: speed 0 -> $ff, moving up by one */
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0xffu);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0xfdu);
  /* the position still moved by the pre-update speed of zero */
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x80u);
  move_red_p_troopa(&smbc.state, 0x00u, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x7fu);

  /* the negative clamp is strict (the asm's bpl skips at equality)
   * and gated on the force, so from rest the speed settles into an
   * oscillation around the negative maximum, crossing the unit
   * boundary after enough frames */
  for (int i = 0; i < 200; i++) move_red_p_troopa(&smbc.state, 0x00u, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] >= 0xfdu);
  assert(smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] == 0x00u);

  /* a speed already below the negative maximum clamps back to it */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] = 0xfdu; /* -3 */
  smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] = 0x10u;
  smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] = 0x01u;
  move_red_p_troopa(&smbc.state, 0x00u, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0xfeu);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0xffu);

  /* moving down clamps at the positive maximum instead (from rest the
   * $03 force growth is far too slow to reach it, so start fast) */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] = 0x05u;
  smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] = 0x90u;
  smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] = 0x01u;
  move_red_p_troopa(&smbc.state, 0x00u, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x00u);
}

static void platform_amounts(void) {
  /* smb.asm: the remaining entries and their movement amounts */
  Smbc smbc;
  smbc_init(&smbc);
  move_j_enemy_vertically(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x1cu);

  smbc_init(&smbc);
  move_drop_platform(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x7fu);

  smbc_init(&smbc);
  move_enemy_slow_vert(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x0fu);

  smbc_init(&smbc);
  move_falling_platform(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x20u);

  smbc_init(&smbc);
  move_platform_vert(&smbc.state, 0x00u, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x05u);
}

int main(void) {
  horizontal();
  gravity_basic();
  gravity_upwards();
  red_p_troopa_gravity();
  platform_amounts();
  puts("tests/core/move: ok");
  return 0;
}
