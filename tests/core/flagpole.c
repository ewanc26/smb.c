/* tests/core/flagpole.c — the flagpole routine and flag draw. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/flagpole.h"
#include "../../src/core/frame.h"
#include "../../src/core/initialize.h"
#include "../../src/core/state.h"

/* screen spanning $0020-$0110 for the offscreen/relative calls */
static void set_screen(Smbc *smbc) {
  smbc->state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x00u;
  smbc->state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x20u;
  smbc->state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x01u;
  smbc->state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0x10u;
}

static void not_a_flag_leaves_alone(void) {
  /* smb.asm: no FlagpoleFlagObject in slot $05: leave — nothing but
   * the ObjectOffset store happens */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  flagpole_routine(&smbc.state);
  assert(smbc.state.ram[SMBC_OBJECT_OFFSET] == 0x05u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER] == 0x00u);
}

static void flag_slides(void) {
  /* smb.asm: sliding — the flag down (y plus one plus the dummy
   * carry), the floatey number up (minus two without the borrow) */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 5] = 0x30u; /* FlagpoleFlagObject */
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x04u;
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x03u; /* climbing */
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 5] = 0x60u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 5] = 0x80u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 5] = 0x00u;
  smbc.state.ram[SMBC_ENEMY_YMF_DUMMY + 5] = 0x80u; /* +ff wraps */
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x50u;
  smbc.state.ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  smbc.state.ram[SMBC_FLAGPOLE_FNUM_YMFDUMMY] = 0x10u; /* no borrow */
  smbc.state.ram[SMBC_FLAGPOLE_FNUM_Y_POS] = 0x40u;
  smbc.state.ram[SMBC_ENEMY_SPR_DATA_OFFSET + 5] = 0x24u;
  flagpole_routine(&smbc.state);
  assert(smbc.state.ram[SMBC_ENEMY_YMF_DUMMY + 5] == 0x7fu);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 5] == 0x62u);
  assert(smbc.state.ram[SMBC_FLAGPOLE_FNUM_YMFDUMMY] == 0x11u);
  assert(smbc.state.ram[SMBC_FLAGPOLE_FNUM_Y_POS] == 0x3eu);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x04u);
  /* smb.asm: FlagpoleGfxHandler — triangle, skull, triangle at rel x */
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x24u] == 0x7eu);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x28u] == 0x7fu);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x2cu] == 0x7eu);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x24u] == 0x60u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x28u] == 0x68u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x2cu] == 0x68u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x24u] == 0x62u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x28u] == 0x62u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x2cu] == 0x6au);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x24u] == 0x01u);
}

static void flag_down_gives_score(void) {
  /* smb.asm: GiveFPScr — the flag at $aa: the score modifier into the
   * digit for the score offset, and the end-of-level subroutine next */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 5] = 0x30u;
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x04u;
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x03u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 5] = 0xaau;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 5] = 0x80u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x50u;
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  smbc.state.ram[SMBC_FLAGPOLE_SCORE] = 0x02u; /* 2000 points row */
  smbc.state.ram[SMBC_ENEMY_SPR_DATA_OFFSET + 5] = 0x24u;
  flagpole_routine(&smbc.state);
  /* FlagpoleScoreMods[2] = $08 into DigitModifier[FlagpoleScoreDigits[2]]
   * — AddToScore walks the digits down from $0b, so index 4
   * lands at $0a */
  assert(smbc.state.ram[SMBC_DISPLAY_DIGITS + 0x0au] == 0x08u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x05u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 5] == 0xaau);
}

static void player_low_gives_score(void) {
  /* smb.asm: the player at $a2 ends it even with the flag high */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 5] = 0x30u;
  smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x04u;
  smbc.state.ram[SMBC_PLAYER_STATE] = 0x03u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 5] = 0x60u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 5] = 0x80u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0xa2u;
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  smbc.state.ram[SMBC_FLAGPOLE_SCORE] = 0x04u; /* lowest: 100 */
  smbc.state.ram[SMBC_ENEMY_SPR_DATA_OFFSET + 5] = 0x24u;
  flagpole_routine(&smbc.state);
  assert(smbc.state.ram[SMBC_DISPLAY_DIGITS + 0x0au] == 0x01u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x05u);
}

static void not_sliding_skips_score(void) {
  /* smb.asm: the draw runs whenever the flag exists, but the slide
   * and score need subroutine $04 and the climbing state */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 5] = 0x30u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 5] = 0x60u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 5] = 0x80u;
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x50u;
  smbc.state.ram[SMBC_ENEMY_SPR_DATA_OFFSET + 5] = 0x24u;
  flagpole_routine(&smbc.state);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 5] == 0x60u);
  assert(smbc.state.ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x00u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x24u] == 0x7eu);
}

static void collision_pos_draws_floatey(void) {
  /* smb.asm: a nonzero FlagpoleCollisionYPos renders the score row
   * twelve bytes on, at the floatey number's coordinate */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 5] = 0x30u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 5] = 0x60u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 5] = 0x80u;
  smbc.state.ram[SMBC_ENEMY_SPR_DATA_OFFSET + 5] = 0x24u;
  smbc.state.ram[SMBC_FLAGPOLE_COLLISION_Y_POS] = 0x80u;
  smbc.state.ram[SMBC_FLAGPOLE_SCORE] = 0x03u; /* tiles $f8,$fb: 1000 */
  smbc.state.ram[SMBC_FLAGPOLE_FNUM_Y_POS] = 0x38u;
  flagpole_routine(&smbc.state);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x30u] == 0xf8u);
  assert(smbc.state.ram[SMBC_SPRITE_TILENUMBER + 0x34u] == 0xfbu);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x30u] == 0x38u);
  assert(smbc.state.ram[SMBC_SPRITE_Y_POSITION + 0x34u] == 0x38u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x30u] == 0x74u);
  assert(smbc.state.ram[SMBC_SPRITE_X_POSITION + 0x34u] == 0x7cu);
  assert(smbc.state.ram[SMBC_SPRITE_ATTRIBUTES + 0x30u] == 0x01u);
}

static void offscreen_moves_sprites(void) {
  /* smb.asm: ChkFlagOffscreen — d3-d1 of the offscreen bits send all
   * six flag sprites offscreen */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_ENEMY_ID + 5] = 0x30u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 5] = 0x60u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 5] = 0xf0u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 5] = 0x01u; /* past the edge */
  smbc.state.ram[SMBC_ENEMY_SPR_DATA_OFFSET + 5] = 0x24u;
  flagpole_routine(&smbc.state);
  for (uint8_t off = 0x24u; off <= 0x38u; off = (uint8_t)(off + 4u)) {
    assert(smbc.state.ram[SMBC_SPRITE_DATA + off] == 0xf8u);
  }
}

int main(void) {
  not_a_flag_leaves_alone();
  flag_slides();
  flag_down_gives_score();
  player_low_gives_score();
  not_sliding_skips_score();
  collision_pos_draws_floatey();
  offscreen_moves_sprites();
  printf("tests/core/flagpole: ok\n");
  return 0;
}
