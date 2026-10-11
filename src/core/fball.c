/* core/fball.c — the fireball and air bubble draw routines. */
#include "fball.h"

/* smb.asm: ExplosionTiles — newest to oldest */
static const uint8_t SMBC_EXPLOSION_TILES[3] = {0x68u, 0x67u, 0x66u};

/* smb.asm: DumpFourSpr — one tile number into four consecutive
 * sprites; Y points at the first sprite's tile slot */
static void smbc_dump_four_spr(CoreState *st, uint8_t y, uint8_t tile) {
  st->ram[SMBC_SPRITE_Y_POSITION + y] = tile;
  st->ram[SMBC_SPRITE_Y_POSITION + 4u + y] = tile;
  st->ram[SMBC_SPRITE_Y_POSITION + 8u + y] = tile;
  st->ram[SMBC_SPRITE_Y_POSITION + 12u + y] = tile;
}

void draw_fireball(CoreState *st, uint8_t x) {
  uint8_t y = st->ram[SMBC_FBALL_SPR_DATA_OFFSET + x];
  st->ram[SMBC_SPRITE_Y_POSITION + y] = st->ram[SMBC_FIREBALL_REL_YPOS];
  st->ram[SMBC_SPRITE_X_POSITION + y] = st->ram[SMBC_FIREBALL_REL_XPOS];
  /* smb.asm: DrawFirebar — the shared tile body; the tile alternates
   * every four frames, both flips every eight */
  uint8_t frame = st->ram[SMBC_FRAME_COUNTER];
  st->ram[SMBC_SPRITE_TILENUMBER + y] =
      (uint8_t)(((frame >> 2u) & 0x01u) ^ 0x64u);
  uint8_t attr = 0x02u;
  if (((frame >> 3u) & 0x01u) != 0x00u) attr |= 0xc0u;
  st->ram[SMBC_SPRITE_ATTRIBUTES + y] = attr;
}

void draw_explosion_fireball(CoreState *st, uint8_t x) {
  uint8_t y = st->ram[SMBC_ALT_SPR_DATA_OFFSET + x];
  uint8_t state = st->ram[SMBC_FIREBALL_STATE + x];
  st->ram[SMBC_FIREBALL_STATE + x] = (uint8_t)(state + 1u);
  uint8_t frame = (uint8_t)((state >> 1u) & 0x07u);
  if (frame >= 0x03u) {
    /* smb.asm: KillFireBall */
    st->ram[SMBC_FIREBALL_STATE + x] = 0x00u;
    return;
  }
  /* smb.asm: DrawExplosion_Fireworks — four sprites around the
   * relative position, flipping outward */
  smbc_dump_four_spr(st, (uint8_t)(y + 1u), SMBC_EXPLOSION_TILES[frame]);
  uint8_t top = (uint8_t)(st->ram[SMBC_FIREBALL_REL_YPOS] - 0x04u);
  st->ram[SMBC_SPRITE_Y_POSITION + y] = top;
  st->ram[SMBC_SPRITE_Y_POSITION + 8u + y] = top;
  top = (uint8_t)(top + 0x08u);
  st->ram[SMBC_SPRITE_Y_POSITION + 4u + y] = top;
  st->ram[SMBC_SPRITE_Y_POSITION + 12u + y] = top;
  uint8_t left = (uint8_t)(st->ram[SMBC_FIREBALL_REL_XPOS] - 0x04u);
  st->ram[SMBC_SPRITE_X_POSITION + y] = left;
  st->ram[SMBC_SPRITE_X_POSITION + 4u + y] = left;
  left = (uint8_t)(left + 0x08u);
  st->ram[SMBC_SPRITE_X_POSITION + 8u + y] = left;
  st->ram[SMBC_SPRITE_X_POSITION + 12u + y] = left;
  st->ram[SMBC_SPRITE_ATTRIBUTES + y] = 0x02u;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] = 0x82u;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 8u + y] = 0x42u;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 12u + y] = 0xc2u;
}

void draw_bubble(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_PLAYER_Y_HIGH_POS] != 0x01u) return;
  if ((st->ram[SMBC_BUBBLE_OFFSCREEN_BITS] & 0x08u) != 0x00u) return;
  uint8_t y = st->ram[SMBC_BUBBLE_SPR_DATA_OFFSET + x];
  st->ram[SMBC_SPRITE_X_POSITION + y] = st->ram[SMBC_BUBBLE_REL_XPOS];
  st->ram[SMBC_SPRITE_Y_POSITION + y] = st->ram[SMBC_BUBBLE_REL_YPOS];
  st->ram[SMBC_SPRITE_TILENUMBER + y] = 0x74u;
  st->ram[SMBC_SPRITE_ATTRIBUTES + y] = 0x02u;
}
