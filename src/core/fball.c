/* core/fball.c — the fireball and air bubble objects. */
#include "fball.h"

#include "collide.h"
#include "move.h"
#include "offscreen.h"

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

/* smb.asm: FireballXSpdData — left, right */
static const uint8_t SMBC_FIREBALL_X_SPD_DATA[2] = {0x40u, 0xc0u};

/* smb.asm: Bubble_MForceData / BubbleTimerData — by the LSFR bit */
static const uint8_t SMBC_BUBBLE_MFORCE_DATA[2] = {0xffu, 0x50u};
static const uint8_t SMBC_BUBBLE_TIMER_DATA[2] = {0x40u, 0x20u};

void fireball_obj_core(CoreState *st, uint8_t x) {
  st->ram[SMBC_OBJECT_OFFSET] = x;
  uint8_t state = st->ram[SMBC_FIREBALL_STATE + x];
  if ((state & 0x80u) != 0x00u) {
    /* smb.asm: FireballExplosion */
    relative_fireball_position(st, x);
    draw_explosion_fireball(st, x);
    return;
  }
  if (state == 0x00u) return; /* smb.asm: NoFBall */
  if (state != 0x01u) {
    /* smb.asm: a fresh fireball takes the player's position, the
     * facing direction's speed, the small bounding box and the
     * running state */
    uint16_t pos = (uint16_t)st->ram[SMBC_PLAYER_X_POSITION] + 0x04u;
    st->ram[SMBC_FIREBALL_X_POSITION + x] = (uint8_t)pos;
    st->ram[SMBC_FIREBALL_PAGE_LOC + x] =
        (uint8_t)(st->ram[SMBC_PLAYER_PAGE_LOC] + (uint8_t)(pos >> 8));
    st->ram[SMBC_FIREBALL_Y_POSITION + x] = st->ram[SMBC_PLAYER_Y_POSITION];
    st->ram[SMBC_FIREBALL_Y_HIGH_POS + x] = 0x01u;
    uint8_t dir = (uint8_t)(st->ram[SMBC_PLAYER_FACING_DIR] - 1u);
    st->ram[SMBC_FIREBALL_X_SPEED + x] = SMBC_FIREBALL_X_SPD_DATA[dir];
    st->ram[SMBC_FIREBALL_Y_SPEED + x] = 0x04u;
    st->ram[SMBC_FIREBALL_BOUND_BOX_CTRL + x] = 0x07u;
    st->ram[SMBC_FIREBALL_STATE + x] = 0x01u;
  }
  /* smb.asm: RunFB — gravity and horizontal movement over the
   * fireball's SprObject slot (seven in) */
  uint8_t s = (uint8_t)(x + 0x07u);
  impose_gravity(st, s, 0x50u, 0x00u, 0x03u, 0x00u);
  move_object_horizontally(st, s);
  relative_fireball_position(st, x);
  get_fireball_offscreen_bits(st, x);
  get_fireball_bound_box(st, x);
  fireball_bg_collision(st, x);
  if ((st->ram[SMBC_FBALL_OFFSCREEN_BITS] & 0xccu) != 0x00u) {
    /* smb.asm: EraseFB */
    st->ram[SMBC_FIREBALL_STATE + x] = 0x00u;
    return;
  }
  fireball_enemy_collision(st, x);
  draw_fireball(st, x);
}

void bubble_check(CoreState *st, uint8_t x) {
  uint8_t bit =
      (uint8_t)(st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1u + x] & 0x01u);
  if (st->ram[SMBC_BUBBLE_Y_POSITION + x] == 0xf8u) {
    if (st->ram[SMBC_AIR_BUBBLE_TIMER] != 0x00u) return;
    /* smb.asm: SetupBubble — ahead of the facing direction, eight
     * pixels down, the timer from the LSFR bit */
    uint8_t adder =
        (st->ram[SMBC_PLAYER_FACING_DIR] & 0x01u) != 0x00u ? 0x08u : 0x00u;
    uint16_t pos = (uint16_t)adder + st->ram[SMBC_PLAYER_X_POSITION];
    st->ram[SMBC_BUBBLE_X_POSITION + x] = (uint8_t)pos;
    st->ram[SMBC_BUBBLE_PAGE_LOC + x] =
        (uint8_t)(st->ram[SMBC_PLAYER_PAGE_LOC] + (uint8_t)(pos >> 8));
    st->ram[SMBC_BUBBLE_Y_POSITION + x] =
        (uint8_t)(st->ram[SMBC_PLAYER_Y_POSITION] + 0x08u);
    st->ram[SMBC_BUBBLE_Y_HIGH_POS + x] = 0x01u;
    st->ram[SMBC_AIR_BUBBLE_TIMER] = SMBC_BUBBLE_TIMER_DATA[bit];
  }
  /* smb.asm: MoveBubl — the dummy absorbs the movement force, the
   * borrow moves the bubble up a pixel, and above the status bar it
   * is sent offscreen */
  uint8_t dummy = st->ram[SMBC_BUBBLE_YMF_DUMMY + x];
  uint8_t mforce = SMBC_BUBBLE_MFORCE_DATA[bit];
  uint8_t borrow = dummy < mforce ? 1u : 0u;
  st->ram[SMBC_BUBBLE_YMF_DUMMY + x] = (uint8_t)(dummy - mforce);
  uint8_t y = (uint8_t)(st->ram[SMBC_BUBBLE_Y_POSITION + x] - borrow);
  if (y < 0x20u) y = 0xf8u;
  st->ram[SMBC_BUBBLE_Y_POSITION + x] = y;
}

void proc_fireball_bubble(CoreState *st) {
  if (st->ram[SMBC_PLAYER_STATUS] >= 0x02u) {
    /* smb.asm: fiery, B pressed on this frame only */
    uint8_t b = (uint8_t)(st->ram[SMBC_A_B_BUTTONS] & SMBC_PAD_B);
    if (b != 0x00u &&
        (b & st->ram[SMBC_PREV_A_B_BUTTONS]) == 0x00u) {
      uint8_t x = (uint8_t)(st->ram[SMBC_FIREBALL_COUNTER] & 0x01u);
      if (st->ram[SMBC_FIREBALL_STATE + x] == 0x00u &&
          st->ram[SMBC_PLAYER_Y_HIGH_POS] == 0x01u &&
          st->ram[SMBC_CROUCHING_FLAG] == 0x00u &&
          st->ram[SMBC_PLAYER_STATE] != 0x03u) {
        st->ram[SMBC_SQUARE1_SOUND_QUEUE] = SMBC_SFX_FIREBALL;
        st->ram[SMBC_FIREBALL_STATE + x] = 0x02u;
        uint8_t anim = st->ram[SMBC_PLAYER_ANIM_TIMER_SET];
        st->ram[SMBC_FIREBALL_THROWING_TIMER] = anim;
        st->ram[SMBC_PLAYER_ANIM_TIMER] = (uint8_t)(anim - 1u);
        st->ram[SMBC_FIREBALL_COUNTER]++;
      }
    }
  }
  /* smb.asm: ProcFireballs */
  fireball_obj_core(st, 0x00u);
  fireball_obj_core(st, 0x01u);
  /* smb.asm: ProcAirBubbles — water levels only */
  if (st->ram[SMBC_AREA_TYPE] != 0x00u) return;
  uint8_t x = 0x02u;
  for (;;) {
    st->ram[SMBC_OBJECT_OFFSET] = x;
    bubble_check(st, x);
    relative_bubble_position(st, x);
    get_bubble_offscreen_bits(st, x);
    draw_bubble(st, x);
    if (x == 0x00u) break;
    x--;
  }
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
