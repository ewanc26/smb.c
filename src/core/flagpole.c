/* core/flagpole.c — the flagpole flag and its score floatey number. */
#include "flagpole.h"
#include "offscreen.h"
#include "screen.h"

/* smb.asm: FlagpoleScoreMods */
static const uint8_t kFlagpoleScoreMods[] = {0x05u, 0x02u, 0x08u, 0x04u, 0x01u};

/* smb.asm: FlagpoleScoreDigits */
static const uint8_t kFlagpoleScoreDigits[] = {0x03u, 0x03u, 0x04u, 0x04u, 0x04u};

/* smb.asm: FlagpoleScoreNumTiles */
static const uint8_t kFlagpoleScoreNumTiles[] = {
  0xf9u, 0x50u, 0xf7u, 0x50u, 0xfau, 0xfbu, 0xf8u, 0xfbu, 0xf6u, 0xfbu,
};

/* smb.asm: DumpTwoSpr — the same byte into a sprite and the one after */
static void dump_two_spr(CoreState *st, uint8_t y, uint8_t a) {
  st->ram[SMBC_SPRITE_DATA + 4u + y] = a;
  st->ram[SMBC_SPRITE_DATA + y] = a;
}

/* smb.asm: DrawSpriteObject as entered by DrawOneSpriteRow — one row of
 * two side-by-side sprites; the trailing y and x increments that walk
 * multi-row callers to the next row are dead here */
static void draw_one_sprite_row(CoreState *st, uint8_t y, uint8_t tile1,
                                uint8_t tile2, uint8_t flip, uint8_t attr,
                                uint8_t ypos, uint8_t xpos) {
  uint8_t a;
  if (flip & 0x02u) { /* d1 after the two lsr: horizontal flip */
    st->ram[SMBC_SPRITE_TILENUMBER + 4u + y] = tile1;
    st->ram[SMBC_SPRITE_TILENUMBER + y] = tile2;
    a = 0x40u;
  } else {
    st->ram[SMBC_SPRITE_TILENUMBER + y] = tile1;
    st->ram[SMBC_SPRITE_TILENUMBER + 4u + y] = tile2;
    a = 0x00u;
  }
  a |= attr;
  st->ram[SMBC_SPRITE_ATTRIBUTES + y] = a;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] = a;
  st->ram[SMBC_SPRITE_Y_POSITION + y] = ypos;
  st->ram[SMBC_SPRITE_Y_POSITION + 4u + y] = ypos;
  st->ram[SMBC_SPRITE_X_POSITION + y] = xpos;
  st->ram[SMBC_SPRITE_X_POSITION + 4u + y] = (uint8_t)(xpos + 0x08u);
}

/* smb.asm: FlagpoleGfxHandler — three sprites for the flag (triangle,
 * skull, triangle), then the score floatey number if the collision
 * coordinate is set */
static void flagpole_gfx_handler(CoreState *st, uint8_t x) {
  uint8_t y = st->ram[SMBC_ENEMY_SPR_DATA_OFFSET + x];
  uint8_t xpos = st->ram[SMBC_ENEMY_REL_XPOS];
  st->ram[SMBC_SPRITE_X_POSITION + y] = xpos;
  uint8_t t = (uint8_t)(xpos + 0x08u);
  st->ram[SMBC_SPRITE_X_POSITION + 4u + y] = t;
  st->ram[SMBC_SPRITE_X_POSITION + 8u + y] = t;
  uint8_t fnum_x = (uint8_t)(t + 0x0cu); /* $05 for the floatey number */
  uint8_t ypos = st->ram[SMBC_ENEMY_Y_POSITION + x];
  dump_two_spr(st, y, ypos);
  st->ram[SMBC_SPRITE_Y_POSITION + 8u + y] = (uint8_t)(ypos + 0x08u);
  st->ram[SMBC_SPRITE_ATTRIBUTES + y] = 0x01u;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] = 0x01u;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 8u + y] = 0x01u;
  st->ram[SMBC_SPRITE_TILENUMBER + y] = 0x7eu;
  st->ram[SMBC_SPRITE_TILENUMBER + 8u + y] = 0x7eu;
  st->ram[SMBC_SPRITE_TILENUMBER + 4u + y] = 0x7fu;
  if (st->ram[SMBC_FLAGPOLE_COLLISION_Y_POS] != 0x00u) {
    y = (uint8_t)(y + 0x0cu);
    uint8_t off = (uint8_t)(st->ram[SMBC_FLAGPOLE_SCORE] << 1);
    draw_one_sprite_row(st, y, kFlagpoleScoreNumTiles[off],
                        kFlagpoleScoreNumTiles[off + 1u], 0x01u, 0x01u,
                        st->ram[SMBC_FLAGPOLE_FNUM_Y_POS], fnum_x);
  }
  /* smb.asm: ChkFlagOffscreen — d3-d1 set moves all six sprites
   * offscreen (MoveSixSpritesOffscreen through the dump chain) */
  x = st->ram[SMBC_OBJECT_OFFSET];
  y = st->ram[SMBC_ENEMY_SPR_DATA_OFFSET + x];
  if (st->ram[SMBC_ENEMY_OFFSCREEN_BITS] & 0x0eu) {
    for (uint8_t off = 0x00u; off <= 0x14u; off = (uint8_t)(off + 4u)) {
      st->ram[SMBC_SPRITE_DATA + off + y] = 0xf8u;
    }
  }
}

void flagpole_routine(CoreState *st) {
  uint8_t x = 0x05u; /* smb.asm: special use slot */
  st->ram[SMBC_OBJECT_OFFSET] = x;
  if (st->ram[SMBC_ENEMY_ID + x] != 0x30u) return; /* FlagpoleFlagObject */
  if (st->ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x04u &&
      st->ram[SMBC_PLAYER_STATE] == 0x03u) {
    uint8_t ey = st->ram[SMBC_ENEMY_Y_POSITION + x];
    if (ey < 0xaau && st->ram[SMBC_PLAYER_Y_POSITION] < 0xa2u) {
      /* slide: the flag down, the floatey number up — the cmp against
       * the player's $a2 leaves carry clear for the first adc */
      uint16_t dummy = (uint16_t)st->ram[SMBC_ENEMY_YMF_DUMMY + x] + 0xffu;
      st->ram[SMBC_ENEMY_YMF_DUMMY + x] = (uint8_t)dummy;
      st->ram[SMBC_ENEMY_Y_POSITION + x] =
          (uint8_t)(ey + 1u + (dummy >> 8u));
      uint8_t fdummy = st->ram[SMBC_FLAGPOLE_FNUM_YMFDUMMY];
      st->ram[SMBC_FLAGPOLE_FNUM_YMFDUMMY] = (uint8_t)(fdummy - 0xffu);
      st->ram[SMBC_FLAGPOLE_FNUM_Y_POS] =
          (uint8_t)(st->ram[SMBC_FLAGPOLE_FNUM_Y_POS] - 2u +
                    (fdummy >= 0xffu ? 1u : 0u));
    } else {
      /* smb.asm: GiveFPScr — the height score, then end of level on
       * the next frame */
      uint8_t yoff = st->ram[SMBC_FLAGPOLE_SCORE];
      st->ram[SMBC_DIGIT_MODIFIER + kFlagpoleScoreDigits[yoff]] =
          kFlagpoleScoreMods[yoff];
      add_to_score(st);
      st->ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x05u;
    }
  }
  /* smb.asm: FPGfx */
  get_enemy_offscreen_bits(st, x);
  relative_enemy_position(st, x);
  flagpole_gfx_handler(st, x);
}
