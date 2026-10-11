/* core/miscobj.c — misc objects: hammers, jumping coins, floatey
 * numbers. */
#include "miscobj.h"
#include "collide.h"
#include "move.h"
#include "offscreen.h"

/* smb.asm: HammerXSpdData */
static const uint8_t kHammerXSpdData[] = {0x10u, 0xf0u};

/* smb.asm: JumpingCoinTiles */
static const uint8_t kJumpingCoinTiles[] = {0x60u, 0x61u, 0x62u, 0x63u};

/* smb.asm: the DrawHammer pose tables */
static const uint8_t kFirstSprXPos[] = {0x04u, 0x00u, 0x04u, 0x00u};
static const uint8_t kFirstSprYPos[] = {0x00u, 0x04u, 0x00u, 0x04u};
static const uint8_t kSecondSprXPos[] = {0x00u, 0x08u, 0x00u, 0x08u};
static const uint8_t kSecondSprYPos[] = {0x08u, 0x00u, 0x08u, 0x00u};
static const uint8_t kFirstSprTilenum[] = {0x80u, 0x82u, 0x81u, 0x83u};
static const uint8_t kSecondSprTilenum[] = {0x81u, 0x83u, 0x80u, 0x82u};
static const uint8_t kHammerSprAttrib[] = {0x03u, 0x03u, 0xc3u, 0xc3u};

/* smb.asm: DumpTwoSpr — the same byte into a sprite and the one after */
static void dump_two_spr(CoreState *st, uint8_t y, uint8_t a) {
  st->ram[SMBC_SPRITE_DATA + 4u + y] = a;
  st->ram[SMBC_SPRITE_DATA + y] = a;
}

/* smb.asm: JCoinGfxHandler — state under 2 is the spinning coin,
 * 2 and up the rising "200" floatey number
 * (DrawFloateyNumber_Coin) */
static void jcoin_gfx_handler(CoreState *st, uint8_t x) {
  uint8_t y = st->ram[SMBC_MISC_SPR_DATA_OFFSET + x];
  if (st->ram[SMBC_MISC_STATE + x] >= 0x02u) {
    /* the number rises a pixel every other frame */
    if ((st->ram[SMBC_FRAME_COUNTER] & 0x01u) == 0x00u) {
      st->ram[SMBC_MISC_Y_POSITION + x]--;
    }
    dump_two_spr(st, y, st->ram[SMBC_MISC_Y_POSITION + x]);
    uint8_t xpos = st->ram[SMBC_MISC_REL_XPOS];
    st->ram[SMBC_SPRITE_X_POSITION + y] = xpos;
    st->ram[SMBC_SPRITE_X_POSITION + 4u + y] = (uint8_t)(xpos + 0x08u);
    st->ram[SMBC_SPRITE_ATTRIBUTES + y] = 0x02u;
    st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] = 0x02u;
    st->ram[SMBC_SPRITE_TILENUMBER + y] = 0xf7u;
    st->ram[SMBC_SPRITE_TILENUMBER + 4u + y] = 0xfbu;
    return;
  }
  uint8_t ypos = st->ram[SMBC_MISC_Y_POSITION + x];
  st->ram[SMBC_SPRITE_Y_POSITION + y] = ypos;
  st->ram[SMBC_SPRITE_Y_POSITION + 4u + y] = (uint8_t)(ypos + 0x08u);
  uint8_t xpos = st->ram[SMBC_MISC_REL_XPOS];
  st->ram[SMBC_SPRITE_X_POSITION + y] = xpos;
  st->ram[SMBC_SPRITE_X_POSITION + 4u + y] = xpos;
  uint8_t t = (uint8_t)((st->ram[SMBC_FRAME_COUNTER] >> 1u) & 0x03u);
  dump_two_spr(st, (uint8_t)(y + 1u), kJumpingCoinTiles[t]);
  st->ram[SMBC_SPRITE_ATTRIBUTES + y] = 0x02u;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] = 0x82u; /* vertical flip */
}

/* smb.asm: ProcJumpCoin — state 1 runs the coin under gravity; the
 * higher states count up as a floatey number, drifting with the
 * scroll, until $30 nullifies the object */
static void proc_jump_coin(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_MISC_STATE + x] == 0x01u) {
    impose_gravity(st, (uint8_t)(x + 0x0du), 0x50u, 0x03u, 0x06u, 0x00u);
    if (st->ram[SMBC_MISC_Y_SPEED + x] == 0x05u) {
      st->ram[SMBC_MISC_STATE + x]++;
    }
  } else {
    st->ram[SMBC_MISC_STATE + x]++;
    uint16_t xpos = (uint16_t)st->ram[SMBC_MISC_X_POSITION + x] +
                    st->ram[SMBC_SCROLL_AMOUNT];
    st->ram[SMBC_MISC_X_POSITION + x] = (uint8_t)xpos;
    st->ram[SMBC_MISC_PAGE_LOC + x] += (uint8_t)(xpos >> 8u);
    if (st->ram[SMBC_MISC_STATE + x] == 0x30u) {
      st->ram[SMBC_MISC_STATE + x] = 0x00u;
      return;
    }
  }
  /* smb.asm: RunJCSubs */
  relative_misc_position(st, x);
  get_misc_offscreen_bits(st, x);
  get_misc_bound_box(st, x);
  jcoin_gfx_handler(st, x);
}

/* smb.asm: PlayerHammerCollision — every other frame only, and never
 * under the master timer control or with offscreen bits; a fresh hit
 * bounces the hammer back and, unless star, injures the player */
static void player_hammer_collision(CoreState *st, uint8_t x) {
  if ((st->ram[SMBC_FRAME_COUNTER] & 0x01u) == 0x00u) return;
  if ((st->ram[SMBC_TIMER_CONTROL] |
       st->ram[SMBC_MISC_OFFSCREEN_BITS]) != 0x00u) return;
  uint8_t y = (uint8_t)(x * 4u + 0x24u);
  if (player_collision_core(st, y) == 0x00u) {
    st->ram[SMBC_MISC_COLLISION_FLAG + x] = 0x00u;
    return;
  }
  if (st->ram[SMBC_MISC_COLLISION_FLAG + x] != 0x00u) return;
  st->ram[SMBC_MISC_COLLISION_FLAG + x] = 0x01u;
  st->ram[SMBC_MISC_X_SPEED + x] =
      (uint8_t)(~st->ram[SMBC_MISC_X_SPEED + x] + 1u);
  if (st->ram[SMBC_STAR_INVINCIBLE_TIMER] != 0x00u) return;
  injure_player(st);
}

/* smb.asm: DrawHammer — the pose cycles every four frames while the
 * hammer flies (state 1); otherwise pose 0. Any offscreen bits
 * outside d1-d0 nullify the object and park its sprites */
static void draw_hammer(CoreState *st, uint8_t x) {
  uint8_t y = st->ram[SMBC_MISC_SPR_DATA_OFFSET + x];
  uint8_t pose = 0x00u;
  if (st->ram[SMBC_TIMER_CONTROL] == 0x00u &&
      (st->ram[SMBC_MISC_STATE + x] & 0x7fu) == 0x01u) {
    pose = (uint8_t)((st->ram[SMBC_FRAME_COUNTER] >> 2u) & 0x03u);
  }
  uint8_t ypos = (uint8_t)(st->ram[SMBC_MISC_REL_YPOS] +
                           kFirstSprYPos[pose]);
  st->ram[SMBC_SPRITE_Y_POSITION + y] = ypos;
  st->ram[SMBC_SPRITE_Y_POSITION + 4u + y] =
      (uint8_t)(ypos + kSecondSprYPos[pose]);
  uint8_t xpos = (uint8_t)(st->ram[SMBC_MISC_REL_XPOS] +
                           kFirstSprXPos[pose]);
  st->ram[SMBC_SPRITE_X_POSITION + y] = xpos;
  st->ram[SMBC_SPRITE_X_POSITION + 4u + y] =
      (uint8_t)(xpos + kSecondSprXPos[pose]);
  st->ram[SMBC_SPRITE_TILENUMBER + y] = kFirstSprTilenum[pose];
  st->ram[SMBC_SPRITE_TILENUMBER + 4u + y] = kSecondSprTilenum[pose];
  st->ram[SMBC_SPRITE_ATTRIBUTES + y] = kHammerSprAttrib[pose];
  st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] = kHammerSprAttrib[pose];
  if ((st->ram[SMBC_MISC_OFFSCREEN_BITS] & 0xfcu) != 0x00u) {
    st->ram[SMBC_MISC_STATE + x] = 0x00u;
    dump_two_spr(st, y, 0xf8u);
  }
}

/* smb.asm: ProcHammerObj — under the master timer control the hammer
 * only draws. Below state 2 it flies under gravity; at 2 it leaves
 * the bro with the direction's speed, and from then on tracks two
 * pixels right and ten pixels above the bro that spawned it */
static void proc_hammer_obj(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_TIMER_CONTROL] == 0x00u) {
    uint8_t state = (uint8_t)(st->ram[SMBC_MISC_STATE + x] & 0x7fu);
    uint8_t y = st->ram[SMBC_HAMMER_ENEMY_OFFSET + x];
    if (state < 0x02u) {
      impose_gravity(st, (uint8_t)(x + 0x0du), 0x10u, 0x0fu, 0x04u,
                     0x00u);
      move_object_horizontally(st, (uint8_t)(x + 0x0du));
      player_hammer_collision(st, x);
    } else {
      if (state == 0x02u) {
        st->ram[SMBC_MISC_Y_SPEED + x] = 0xfeu;
        st->ram[SMBC_ENEMY_STATE + y] &= 0xf7u;
        st->ram[SMBC_MISC_X_SPEED + x] =
            kHammerXSpdData[st->ram[SMBC_ENEMY_MOVING_DIR + y] - 1u];
      }
      st->ram[SMBC_MISC_STATE + x]--;
      uint16_t xpos = (uint16_t)st->ram[SMBC_ENEMY_X_POSITION + y] + 0x02u;
      st->ram[SMBC_MISC_X_POSITION + x] = (uint8_t)xpos;
      st->ram[SMBC_MISC_PAGE_LOC + x] =
          (uint8_t)(st->ram[SMBC_ENEMY_PAGE_LOC + y] + (xpos >> 8u));
      st->ram[SMBC_MISC_Y_POSITION + x] =
          (uint8_t)(st->ram[SMBC_ENEMY_Y_POSITION + y] - 0x0au);
      st->ram[SMBC_MISC_Y_HIGH_POS + x] = 0x01u;
    }
  }
  /* smb.asm: RunHSubs */
  get_misc_offscreen_bits(st, x);
  relative_misc_position(st, x);
  get_misc_bound_box(st, x);
  draw_hammer(st, x);
}

void misc_objects_core(CoreState *st) {
  for (uint8_t x = 0x08u;; x--) {
    st->ram[SMBC_OBJECT_OFFSET] = x;
    uint8_t state = st->ram[SMBC_MISC_STATE + x];
    if (state != 0x00u) {
      if (state & 0x80u) {
        proc_hammer_obj(st, x);
      } else {
        proc_jump_coin(st, x);
      }
    }
    if (x == 0x00u) break;
  }
}
