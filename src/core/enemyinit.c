/* core/enemyinit.c — see enemyinit.h */
#include "enemyinit.h"

#include "../data/enemies.h"
#include "../data/scenery.h" /* SMBC_BITMASKS */
#include "enemies.h"

/* smb.asm's sound effect constants used here */
#define SMBC_SFX_GROW_POWER_UP 0x02u /* smb.asm: Sfx_GrowPowerUp */
#define SMBC_SFX_GROW_VINE 0x04u     /* smb.asm: Sfx_GrowVine */

/* smb.asm's enemy object constants used here (see core/enemies.h for
 * the rest) */
#define SMBC_LAKITU 0x11u                /* smb.asm: Lakitu */
#define SMBC_SPINY 0x12u                 /* smb.asm: Spiny */
#define SMBC_BOWSER 0x2du                /* smb.asm: Bowser */
#define SMBC_STAR_FLAG_OBJECT 0x31u      /* smb.asm: StarFlagObject */
#define SMBC_BULLET_BILL_FRENZY_VAR 0x08u /* smb.asm: BulletBill_FrenzyVar */
#define SMBC_WORLD_2 0x01u               /* smb.asm: World2 (zero-based) */
#define SMBC_SFX_BLAST 0x08u             /* smb.asm: Sfx_Blast */
#define SMBC_SFX_BOWSER_FLAME 0x02u      /* smb.asm: Sfx_BowserFlame */

/* --- shared helpers ------------------------------------------------ */

/* smb.asm: SetBBox — bounding box control, moving direction left and
 * the vertical speed/force reset (InitVStf falls through) */
static void set_bbox(CoreState *st, uint8_t x, uint8_t bbox) {
  st->ram[SMBC_ENEMY_BOUND_BOX_CTRL + x] = bbox;
  st->ram[SMBC_ENEMY_MOVING_DIR + x] = 0x02u; /* left */
  st->ram[SMBC_ENEMY_Y_SPEED + x] = 0x00u;
  st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] = 0x00u;
}

/* smb.asm: TallBBox / SmallBBox */
static void tall_bbox(CoreState *st, uint8_t x) { set_bbox(st, x, 0x03u); }
static void small_bbox(CoreState *st, uint8_t x) { set_bbox(st, x, 0x09u); }

/* smb.asm: SetBBox2 — bounding box control only */
static void set_bbox2(CoreState *st, uint8_t x, uint8_t bbox) {
  st->ram[SMBC_ENEMY_BOUND_BOX_CTRL + x] = bbox;
}

/* smb.asm: PosPlatform — a signed sixteen-bit pixel offset applied to
 * the platform's position (the carry from the low add feeds the page
 * add, matching the asm's adc) */
static void pos_platform(CoreState *st, uint8_t x, uint8_t y) {
  uint16_t pos = ((uint16_t)st->ram[SMBC_ENEMY_PAGE_LOC + x] << 8) |
                 st->ram[SMBC_ENEMY_X_POSITION + x];
  int16_t ofs = (int16_t)((uint16_t)SMBC_PLAT_POS_DATA_LOW[y] |
                          ((uint16_t)SMBC_PLAT_POS_DATA_HIGH[y] << 8));
  pos = (uint16_t)(pos + (uint16_t)ofs);
  st->ram[SMBC_ENEMY_X_POSITION + x] = (uint8_t)pos;
  st->ram[SMBC_ENEMY_PAGE_LOC + x] = (uint8_t)(pos >> 8);
}

/* smb.asm: DuplicateEnemyObj — copy the object into the first empty
 * slot with the original's offset ORed into d7 of the new slot's flag
 * (the pair flag EnemiesAndLoopsCore checks). The asm scans without a
 * bound; the port stops at the six slots and leaves the object
 * unduplicated if all are full, which the spawn paths never allow */
static void duplicate_enemy_obj(CoreState *st, uint8_t x) {
  int y = -1;
  for (int s = 0; s < 0x06; s++) {
    if (st->ram[SMBC_ENEMY_FLAG + s] == 0x00u) {
      y = s;
      break;
    }
  }
  if (y < 0) return;
  st->ram[SMBC_DUPLICATE_OBJ_OFFSET] = (uint8_t)y;
  st->ram[SMBC_ENEMY_FLAG + y] = (uint8_t)(x | 0x80u);
  st->ram[SMBC_ENEMY_PAGE_LOC + y] = st->ram[SMBC_ENEMY_PAGE_LOC + x];
  st->ram[SMBC_ENEMY_X_POSITION + y] = st->ram[SMBC_ENEMY_X_POSITION + x];
  st->ram[SMBC_ENEMY_FLAG + x] = 0x01u;
  st->ram[SMBC_ENEMY_Y_HIGH_POS + y] = 0x01u;
  st->ram[SMBC_ENEMY_Y_POSITION + y] = st->ram[SMBC_ENEMY_Y_POSITION + x];
}

/* smb.asm: SPBBox — platforms get $06 outside castles when secondary
 * hard mode is clear, $05 otherwise */
static void sp_bbox(CoreState *st, uint8_t x) {
  uint8_t bbox = 0x05u;
  if (st->ram[SMBC_AREA_TYPE] != 0x03u &&
      st->ram[SMBC_SECONDARY_HARD_MODE] == 0x00u) {
    bbox = 0x06u;
  }
  st->ram[SMBC_ENEMY_BOUND_BOX_CTRL + x] = bbox;
}

/* smb.asm: CommonPlatCode — InitVStf then the platform bounding box */
static void common_plat_code(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_Y_SPEED + x] = 0x00u; /* InitVStf */
  st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] = 0x00u;
  sp_bbox(st, x);
}

/* --- the normal enemy group ---------------------------------------- */

/* smb.asm: InitNormalEnemy — horizontal speed from the table indexed
 * by primary hard mode, then the tall bounding box */
static void init_normal_enemy(CoreState *st, uint8_t x) {
  uint8_t y = (st->ram[SMBC_PRIMARY_HARD_MODE] != 0x00u) ? 0x01u : 0x00u;
  st->ram[SMBC_ENEMY_X_SPEED + x] = SMBC_NORMAL_X_SPD_DATA[y]; /* SetESpd */
  tall_bbox(st, x);
}

/* smb.asm: InitGoomba */
static void init_goomba(CoreState *st, uint8_t x) {
  init_normal_enemy(st, x);
  small_bbox(st, x);
}

/* smb.asm: InitRedKoopa */
static void init_red_koopa(CoreState *st, uint8_t x) {
  init_normal_enemy(st, x);
  st->ram[SMBC_ENEMY_STATE + x] = 0x01u;
}

/* smb.asm: InitHammerBro — the asm indexes the timer table with the
 * raw secondary hard mode value */
static void init_hammer_bro(CoreState *st, uint8_t x) {
  st->ram[SMBC_HAMMER_THROWING_TIMER + x] = 0x00u;
  st->ram[SMBC_ENEMY_X_SPEED + x] = 0x00u;
  uint8_t y = st->ram[SMBC_SECONDARY_HARD_MODE];
  st->ram[SMBC_ENEMY_INTERVAL_TIMER + x] = SMBC_HBRO_WALKING_TIMER_DATA[y & 0x01u];
  set_bbox(st, x, 0x0bu);
}

/* smb.asm: InitHorizFlySwimEnemy — zero speed into SetESpd */
static void init_horiz_fly_swim_enemy(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_X_SPEED + x] = 0x00u;
  tall_bbox(st, x);
}

/* smb.asm: InitBloober */
static void init_bloober(CoreState *st, uint8_t x) {
  st->ram[SMBC_BLOOPER_MOVE_SPEED + x] = 0x00u;
  small_bbox(st, x);
}

/* smb.asm: InitRedPTroopa — the central vertical coordinate sits 48
 * pixels below the origin, or 32 above when the origin is in the lower
 * half (the asm's adc runs without a clc; the port adds cleanly) */
static void init_red_p_troopa(CoreState *st, uint8_t x) {
  uint8_t ypos = st->ram[SMBC_ENEMY_Y_POSITION + x];
  uint8_t adder = (ypos < 0x80u) ? 0x30u : 0xe0u;
  st->ram[SMBC_RED_PTROOPA_ORIG_X_POS + x] = ypos;
  st->ram[SMBC_RED_PTROOPA_CENTER_Y_POS + x] = (uint8_t)(ypos + adder);
  tall_bbox(st, x);
}

/* smb.asm: InitBulletBill — direction and bounding box only */
static void init_bullet_bill(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_MOVING_DIR + x] = 0x02u;
  st->ram[SMBC_ENEMY_BOUND_BOX_CTRL + x] = 0x09u;
}

/* smb.asm: InitCheepCheep — d4 of the slot's LSFR byte becomes the
 * movement flag; the vertical coordinate is saved as the origin */
static void init_cheep_cheep(CoreState *st, uint8_t x) {
  small_bbox(st, x);
  st->ram[SMBC_CHEEP_CHEEP_MOVE_M_FLAG + x] =
      (uint8_t)(st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + x] & 0x10u);
  st->ram[SMBC_CHEEP_CHEEP_ORIG_Y_POS + x] = st->ram[SMBC_ENEMY_Y_POSITION + x];
}

/* smb.asm: InitJumpGPTroopa — TallBBox2 stores the bounding box only */
static void init_jump_gp_troopa(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_MOVING_DIR + x] = 0x02u;
  st->ram[SMBC_ENEMY_X_SPEED + x] = 0xf8u;
  set_bbox2(st, x, 0x03u);
}

/* smb.asm: InitPodoboo — starts below the screen with the interval
 * timer set to one */
static void init_podoboo(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_Y_HIGH_POS + x] = 0x02u;
  st->ram[SMBC_ENEMY_Y_POSITION + x] = 0x02u;
  st->ram[SMBC_ENEMY_INTERVAL_TIMER + x] = 0x01u;
  st->ram[SMBC_ENEMY_STATE + x] = 0x00u;
  small_bbox(st, x);
}

/* smb.asm: InitPiranhaPlant */
static void init_piranha_plant(CoreState *st, uint8_t x) {
  st->ram[SMBC_PIRANHA_PLANT_Y_SPEED + x] = 0x01u;
  st->ram[SMBC_ENEMY_STATE + x] = 0x00u;
  st->ram[SMBC_PIRANHA_PLANT_MOVE_FLAG + x] = 0x00u;
  uint8_t ypos = st->ram[SMBC_ENEMY_Y_POSITION + x];
  st->ram[SMBC_PIRANHA_PLANT_DOWN_Y_POS + x] = ypos;
  st->ram[SMBC_PIRANHA_PLANT_UP_Y_POS + x] = (uint8_t)(ypos - 0x18u);
  set_bbox2(st, x, 0x09u);
}

/* smb.asm: InitRetainerObj — the princess or mushroom retainer waits
 * at a fixed height */
static void init_retainer_obj(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_Y_POSITION + x] = 0xb8u;
}

/* --- lakitu and the firebars --------------------------------------- */

/* smb.asm: SetupLakitu — clear the reappearance timer and set the
 * horizontal-flyer attributes (the redundant second TallBBox2 is
 * faithful to the asm) */
static void setup_lakitu(CoreState *st, uint8_t x) {
  st->ram[SMBC_LAKITU_REAPPEAR_TIMER] = 0x00u;
  init_horiz_fly_swim_enemy(st, x);
  set_bbox2(st, x, 0x03u); /* TallBBox2 */
}

/* smb.asm: InitLakitu — a live frenzy buffer kills the new lakitu */
static void init_lakitu(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_ENEMY_FRENZY_BUFFER] != 0x00u) { /* KillLakitu */
    st->ram[SMBC_ENEMY_FLAG + x] = 0x00u;
    st->ram[SMBC_ENEMY_ID + x] = 0x00u;
    st->ram[SMBC_ENEMY_STATE + x] = 0x00u;
    st->ram[SMBC_FLOATEY_NUM_CONTROL + x] = 0x00u;
    st->ram[SMBC_ENEMY_INTERVAL_TIMER + x] = 0x00u;
    st->ram[SMBC_SHELL_CHAIN_COUNTER + x] = 0x00u;
    st->ram[SMBC_ENEMY_SPR_ATTRIB + x] = 0x00u;
    st->ram[SMBC_ENEMY_FRAME_TIMER + x] = 0x00u;
    return;
  }
  setup_lakitu(st, x);
}

/* smb.asm: InitShortFirebar — spin state, speed and direction from the
 * tables indexed by identifier minus $1b, then four pixels down and
 * right (the carry moves the page) */
static void init_short_firebar(CoreState *st, uint8_t x) {
  st->ram[SMBC_FIREBAR_SPIN_STATE_LOW + x] = 0x00u;
  uint8_t y = (uint8_t)(st->ram[SMBC_ENEMY_ID + x] - 0x1bu);
  st->ram[SMBC_FIREBAR_SPIN_SPEED + x] = SMBC_FIREBAR_SPIN_SPD_DATA[y & 0x07u];
  st->ram[SMBC_FIREBAR_SPIN_DIRECTION + x] = SMBC_FIREBAR_SPIN_DIR_DATA[y & 0x07u];
  st->ram[SMBC_ENEMY_Y_POSITION + x] += 0x04u;
  uint16_t xsum = (uint16_t)st->ram[SMBC_ENEMY_X_POSITION + x] + 0x04u;
  st->ram[SMBC_ENEMY_X_POSITION + x] = (uint8_t)xsum;
  st->ram[SMBC_ENEMY_PAGE_LOC + x] += (uint8_t)(xsum >> 8);
  set_bbox2(st, x, 0x03u); /* TallBBox2 */
}

/* smb.asm: InitLongFirebar — the duplicate is the firebar's second
 * half; init then continues as a short firebar */
static void init_long_firebar(CoreState *st, uint8_t x) {
  duplicate_enemy_obj(st, x);
  init_short_firebar(st, x);
}

/* --- platforms ------------------------------------------------------ */

/* smb.asm: InitBalPlatform — raised two pixels, nudged twelve left
 * outside secondary hard mode, and chained to the previous balance
 * platform through the alignment variable */
static void init_bal_platform(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_Y_POSITION + x] -= 0x02u;
  if (st->ram[SMBC_SECONDARY_HARD_MODE] == 0x00u) {
    pos_platform(st, x, 0x02u); /* -8 pixels */
  }
  uint8_t y = 0xffu;
  uint8_t align = st->ram[SMBC_BAL_PLATFORM_ALIGNMENT];
  st->ram[SMBC_ENEMY_STATE + x] = align;
  if ((align & 0x80u) != 0x00u) y = x; /* old alignment negative: chain */
  st->ram[SMBC_BAL_PLATFORM_ALIGNMENT] = y;
  st->ram[SMBC_ENEMY_MOVING_DIR + x] = 0x00u;
  pos_platform(st, x, 0x00u); /* +8 pixels */
  common_plat_code(st, x);    /* the asm falls through */
}

/* smb.asm: InitVertPlatform — the top position mirrors around the
 * vertical centre, then the shared platform code */
static void init_vert_platform(CoreState *st, uint8_t x) {
  uint8_t ypos = st->ram[SMBC_ENEMY_Y_POSITION + x];
  uint8_t adder = 0x40u;
  uint8_t top = ypos;
  if (ypos >= 0x80u) {
    top = (uint8_t)(~ypos + 0x01u); /* two's complement */
    adder = 0xc0u;
  }
  st->ram[SMBC_Y_PLATFORM_TOP_Y_POS + x] = top;
  st->ram[SMBC_Y_PLATFORM_CENTER_Y_POS + x] = (uint8_t)(ypos + adder);
  common_plat_code(st, x); /* the asm falls through */
}

/* smb.asm: InitDropPlatform */
static void init_drop_platform(CoreState *st, uint8_t x) {
  st->ram[SMBC_PLATFORM_COLLISION_FLAG + x] = 0xffu;
  common_plat_code(st, x);
}

/* smb.asm: InitHoriPlatform */
static void init_hori_platform(CoreState *st, uint8_t x) {
  st->ram[SMBC_X_MOVE_SECONDARY_COUNTER + x] = 0x00u;
  common_plat_code(st, x);
}

/* smb.asm: CommonSmallLift — twelve pixels across and the small
 * platform bounding box */
static void common_small_lift(CoreState *st, uint8_t x) {
  pos_platform(st, x, 0x01u); /* +12 pixels */
  st->ram[SMBC_ENEMY_BOUND_BOX_CTRL + x] = 0x04u;
}

/* smb.asm: PlatLiftUp (falls through to CommonSmallLift) */
static void plat_lift_up(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] = 0x10u;
  st->ram[SMBC_ENEMY_Y_SPEED + x] = 0xffu;
  common_small_lift(st, x);
}

/* smb.asm: PlatLiftDown (falls through to CommonSmallLift) */
static void plat_lift_down(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] = 0xf0u;
  st->ram[SMBC_ENEMY_Y_SPEED + x] = 0x00u;
  common_small_lift(st, x);
}

/* smb.asm: LargeLiftUp / LargeLiftDown — the small lift setup, then
 * the platform bounding box overwrites the small one */
static void large_lift_up(CoreState *st, uint8_t x) {
  plat_lift_up(st, x);
  sp_bbox(st, x);
}

static void large_lift_down(CoreState *st, uint8_t x) {
  plat_lift_down(st, x);
  sp_bbox(st, x);
}

/* --- bowser, the power-up and the vine ------------------------------ */

/* smb.asm: InitBowser — the duplicate is bowser's second sprite; the
 * original offset becomes BowserFront_Offset */
static void init_bowser(CoreState *st, uint8_t x) {
  duplicate_enemy_obj(st, x);
  st->ram[SMBC_BOWSER_FRONT_OFFSET] = x;
  st->ram[SMBC_BOWSER_BODY_CONTROLS] = 0x00u;
  st->ram[SMBC_BRIDGE_COLLAPSE_OFFSET] = 0x00u;
  st->ram[SMBC_BOWSER_ORIG_X_POS] = st->ram[SMBC_ENEMY_X_POSITION + x];
  st->ram[SMBC_BOWSER_FIRE_BREATH_TIMER] = 0xdfu;
  st->ram[SMBC_ENEMY_MOVING_DIR + x] = 0xdfu; /* and in moving direction */
  st->ram[SMBC_BOWSER_FEET_COUNTER] = 0x20u;
  st->ram[SMBC_ENEMY_FRAME_TIMER + x] = 0x20u;
  st->ram[SMBC_BOWSER_HIT_POINTS] = 0x05u;
  st->ram[SMBC_BOWSER_MOVEMENT_SPEED] = 0x02u; /* lsr of the five */
}

/* smb.asm: PwrUpJmp — a residual jump point operating directly on the
 * sixth slot; the type comes from the player's status unless a star or
 * 1-up is already loaded */
static void pwr_up_jmp(CoreState *st) {
  st->ram[SMBC_ENEMY_STATE + 5] = 0x01u;
  st->ram[SMBC_ENEMY_FLAG + 5] = 0x01u;
  st->ram[SMBC_ENEMY_BOUND_BOX_CTRL + 5] = 0x03u;
  uint8_t type = st->ram[SMBC_POWER_UP_TYPE];
  if (type < 0x02u) {
    type = st->ram[SMBC_PLAYER_STATUS];
    if (type >= 0x02u) type >>= 1; /* fiery forces the fire flower */
    st->ram[SMBC_POWER_UP_TYPE] = type;
  }
  st->ram[SMBC_ENEMY_SPR_ATTRIB + 5] = 0x20u; /* PutBehind */
  st->ram[SMBC_SQUARE2_SOUND_QUEUE] = SMBC_SFX_GROW_POWER_UP;
}

void setup_vine(CoreState *st, uint8_t x, uint8_t block_offset) {
  uint8_t y = block_offset;
  st->ram[SMBC_ENEMY_ID + x] = SMBC_VINE_OBJECT;
  st->ram[SMBC_ENEMY_FLAG + x] = 0x01u;
  st->ram[SMBC_ENEMY_PAGE_LOC + x] = st->ram[SMBC_BLOCK_PAGE_LOC + y];
  st->ram[SMBC_ENEMY_X_POSITION + x] = st->ram[SMBC_BLOCK_X_POSITION + y];
  st->ram[SMBC_ENEMY_Y_POSITION + x] = st->ram[SMBC_BLOCK_Y_POSITION + y];
  y = st->ram[SMBC_VINE_FLAG_OFFSET];
  if (y == 0x00u) {
    st->ram[SMBC_VINE_START_Y_POSITION] = st->ram[SMBC_ENEMY_Y_POSITION + x];
  }
  st->ram[SMBC_VINE_OBJ_OFFSET + y] = x;
  st->ram[SMBC_VINE_FLAG_OFFSET]++;
  st->ram[SMBC_SQUARE2_SOUND_QUEUE] = SMBC_SFX_GROW_VINE;
}

/* smb.asm: EndFrenzy — wake any lakitu on the slots, then empty the
 * frenzy buffer and disable this object */
static void end_frenzy(CoreState *st, uint8_t x) {
  for (int y = 0x05; y >= 0; y--) {
    if (st->ram[SMBC_ENEMY_ID + y] == SMBC_LAKITU) {
      st->ram[SMBC_ENEMY_STATE + y] = 0x01u;
    }
  }
  st->ram[SMBC_ENEMY_FRENZY_BUFFER] = 0x00u;
  st->ram[SMBC_ENEMY_FLAG + x] = 0x00u;
}

/* --- the frenzy generators ----------------------------------------- */

/* smb.asm: PlayerEnemyDiff — the sixteen-bit horizontal difference,
 * enemy minus player; the low byte goes to *diff and the high byte is
 * returned (its d7 is the sign the callers test) */
static uint8_t player_enemy_diff(CoreState *st, uint8_t x, uint8_t *diff) {
  uint16_t enemy = ((uint16_t)st->ram[SMBC_ENEMY_PAGE_LOC + x] << 8) |
                   st->ram[SMBC_ENEMY_X_POSITION + x];
  uint16_t player = ((uint16_t)st->ram[SMBC_PLAYER_PAGE_LOC] << 8) |
                    st->ram[SMBC_PLAYER_X_POSITION];
  uint16_t d = (uint16_t)(enemy - player);
  *diff = (uint8_t)d;
  return (uint8_t)(d >> 8);
}

/* smb.asm: PlayerLakituDiff — returns an adjusted difference value.
 * The adjusters live in $01-$03 of zero page on the NES, filled by the
 * callers from LakituDiffAdj or PRDiffAdjustData; the port takes them
 * as a three-byte array. The lakitu speed side effects only apply when
 * the slot holds lakitu itself */
static uint8_t player_lakitu_diff(CoreState *st, uint8_t x,
                                  const uint8_t adj[3]) {
  uint8_t diff;
  uint8_t side = ((player_enemy_diff(st, x, &diff) & 0x80u) != 0x00u)
                     ? 0x01u /* left of the player */
                     : 0x00u;
  if (side != 0x00u) diff = (uint8_t)(~diff + 1u); /* two's complement */
  if (diff >= 0x3cu) {
    diff = 0x3cu;
    if (st->ram[SMBC_ENEMY_ID + x] == SMBC_LAKITU &&
        side != st->ram[SMBC_LAKITU_MOVE_DIRECTION + x]) {
      if (st->ram[SMBC_LAKITU_MOVE_DIRECTION + x] == 0x00u) {
        st->ram[SMBC_LAKITU_MOVE_DIRECTION + x] = side;
      } else {
        st->ram[SMBC_LAKITU_MOVE_SPEED + x]--;
        if (st->ram[SMBC_LAKITU_MOVE_SPEED + x] != 0x00u) {
          return st->ram[SMBC_LAKITU_MOVE_SPEED + x]; /* ExMoveLak */
        }
        st->ram[SMBC_LAKITU_MOVE_DIRECTION + x] = side; /* SetLMovD */
      }
    }
  }
  /* ChkPSpeed — the middle four bits of the difference, divided by
   * four, with the adjuster offset from the player's speed and the
   * scroll amount */
  diff = (diff & 0x3cu) >> 2;
  uint8_t y = 0x00u;
  if (st->ram[SMBC_PLAYER_X_SPEED] != 0x00u &&
      st->ram[SMBC_SCROLL_AMOUNT] != 0x00u) {
    y = 0x01u;
    if (st->ram[SMBC_PLAYER_X_SPEED] >= 0x19u &&
        st->ram[SMBC_SCROLL_AMOUNT] >= 0x02u) {
      y = 0x02u;
    }
  }
  if (st->ram[SMBC_ENEMY_ID + x] == SMBC_SPINY) {
    /* a thrown spiny keeps the offset while the player is moving */
    if (st->ram[SMBC_PLAYER_X_SPEED] == 0x00u &&
        st->ram[SMBC_ENEMY_Y_SPEED + x] == 0x00u) {
      y = 0x00u;
    }
  } else if (st->ram[SMBC_ENEMY_Y_SPEED + x] == 0x00u) {
    y = 0x00u;
  }
  /* SubDifAdj / SPixelLak — one subtraction per pixel of difference,
   * plus one (the asm's loop runs once for y = 0) */
  uint8_t a = adj[y];
  for (int i = diff; i >= 0; i--) a--;
  return a;
}

/* smb.asm: FinishFlame — the shared tail of PutAtRightExtent: bounding
 * box $08, the vertical high byte and buffer flag set, the horizontal
 * movement force and enemy state cleared. Returns zero (A after the
 * lsr), which the bullet bill path stores */
static uint8_t finish_flame(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_BOUND_BOX_CTRL + x] = 0x08u;
  st->ram[SMBC_ENEMY_Y_HIGH_POS + x] = 0x01u;
  st->ram[SMBC_ENEMY_FLAG + x] = 0x01u;
  st->ram[SMBC_ENEMY_X_MOVE_FORCE + x] = 0x00u;
  st->ram[SMBC_ENEMY_STATE + x] = 0x00u;
  return 0x00u;
}

/* smb.asm: PutAtRightExtent — the vertical coordinate in ypos and the
 * horizontal position thirty-two pixels beyond the screen's right
 * edge, then the shared tail */
static uint8_t put_at_right_extent(CoreState *st, uint8_t x,
                                   uint8_t ypos) {
  st->ram[SMBC_ENEMY_Y_POSITION + x] = ypos;
  uint16_t xsum = (uint16_t)st->ram[SMBC_SCREEN_RIGHT_X_POS] + 0x20u;
  st->ram[SMBC_ENEMY_X_POSITION + x] = (uint8_t)xsum;
  st->ram[SMBC_ENEMY_PAGE_LOC + x] =
      (uint8_t)(st->ram[SMBC_SCREEN_RIGHT_PAGE_LOC] + (xsum >> 8));
  return finish_flame(st, x); /* the asm tail-jumps here */
}

/* smb.asm: LakituAndSpinyHandler — respawns lakitu after the
 * reappearance timer, or turns this frenzy object's own slot into a
 * spiny thrown from the live lakitu */
static void lakitu_and_spiny_handler(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_FRENZY_ENEMY_TIMER] != 0x00u) return;
  if (x >= 0x05u) return; /* not on the special use slot */
  st->ram[SMBC_FRENZY_ENEMY_TIMER] = 0x80u;
  int lakitu = -1;
  for (int y = 0x04; y >= 0; y--) {
    if (st->ram[SMBC_ENEMY_ID + y] == SMBC_LAKITU) {
      lakitu = y;
      break;
    }
  }
  if (lakitu < 0) {
    /* no lakitu onscreen — count towards a reappearance */
    st->ram[SMBC_LAKITU_REAPPEAR_TIMER]++;
    if (st->ram[SMBC_LAKITU_REAPPEAR_TIMER] < 0x07u) return;
    int slot = -1;
    for (int y = 0x04; y >= 0; y--) {
      if (st->ram[SMBC_ENEMY_FLAG + y] == 0x00u) {
        slot = y;
        break;
      }
    }
    if (slot < 0) return;
    uint8_t lx = (uint8_t)slot;
    st->ram[SMBC_ENEMY_STATE + lx] = 0x00u;
    st->ram[SMBC_ENEMY_ID + lx] = SMBC_LAKITU;
    setup_lakitu(st, lx);
    put_at_right_extent(st, lx, 0x20u);
    return;
  }
  /* smb.asm: CreateSpiny — the spiny hatches in this object's slot.
   * The adjusted difference the asm computes is discarded on the NES
   * too (SmallBBox leaves zero in A), but the call is kept for its
   * faithful structure */
  uint8_t ly = (uint8_t)lakitu;
  if (st->ram[SMBC_PLAYER_Y_POSITION] < 0x2cu) return;
  if (st->ram[SMBC_ENEMY_STATE + ly] != 0x00u) return;
  st->ram[SMBC_ENEMY_PAGE_LOC + x] = st->ram[SMBC_ENEMY_PAGE_LOC + ly];
  st->ram[SMBC_ENEMY_X_POSITION + x] = st->ram[SMBC_ENEMY_X_POSITION + ly];
  st->ram[SMBC_ENEMY_Y_HIGH_POS + x] = 0x01u;
  st->ram[SMBC_ENEMY_Y_POSITION + x] =
      (uint8_t)(st->ram[SMBC_ENEMY_Y_POSITION + ly] - 0x08u);
  uint8_t ofs = st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + x] & 0x03u;
  const uint8_t adj[3] = {SMBC_PR_DIFF_ADJUST_DATA[ofs + 8u],
                          SMBC_PR_DIFF_ADJUST_DATA[ofs + 4u],
                          SMBC_PR_DIFF_ADJUST_DATA[ofs]};
  (void)player_lakitu_diff(st, x, adj);
  small_bbox(st, x); /* SmallBBox / SetSpSpd */
  st->ram[SMBC_ENEMY_X_SPEED + x] = 0x00u; /* A was stomped to zero */
  st->ram[SMBC_ENEMY_MOVING_DIR + x] = 0x01u;
  st->ram[SMBC_ENEMY_Y_SPEED + x] = 0xfdu;
  st->ram[SMBC_ENEMY_FLAG + x] = 0x01u;
  st->ram[SMBC_ENEMY_STATE + x] = 0x05u; /* the egg state */
}

/* smb.asm: InitFlyingCheepCheep — spawn a flying cheep-cheep from off
 * the screen, position and speed chosen by the LSFR and the player's
 * motion */
static void init_flying_cheep_cheep(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_FRENZY_ENEMY_TIMER] != 0x00u) return;
  small_bbox(st, x);
  uint8_t y = st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1u + x] & 0x03u;
  st->ram[SMBC_FRENZY_ENEMY_TIMER] = SMBC_FLYCC_TIMER_DATA[y];
  y = 0x03u; /* three onscreen normally, four in secondary hard mode */
  if (st->ram[SMBC_SECONDARY_HARD_MODE] != 0x00u) y = 0x04u;
  if (x >= y) return;
  uint8_t s00 = st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + x] & 0x03u;
  st->ram[SMBC_ENEMY_Y_SPEED + x] = 0xfbu;
  uint8_t a = 0x00u;
  uint8_t pxs = st->ram[SMBC_PLAYER_X_SPEED];
  if (pxs != 0x00u) {
    a = 0x04u;
    if (pxs >= 0x19u) a <<= 1;
  }
  uint8_t saved = a;               /* the asm's pha */
  s00 = (uint8_t)(s00 + a);        /* clc; adc $00 */
  if ((st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1u + x] & 0x03u) != 0x00u) {
    /* RSeed — the third LSFR part's lower nybble overwrites $00 */
    s00 = st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + 2u + x] & 0x0fu;
  }
  uint8_t idx = (uint8_t)(saved +
      (st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + x] & 0x03u)); /* adc $01 */
  st->ram[SMBC_ENEMY_X_SPEED + x] = SMBC_FLYCC_X_SPEED_DATA[idx];
  st->ram[SMBC_ENEMY_MOVING_DIR + x] = 0x01u;
  uint8_t posidx = idx;
  if (pxs == 0x00u) {
    /* a still player: d1 of $00 reverses the cheep-cheep */
    posidx = s00;
    if ((s00 & 0x02u) != 0x00u) {
      st->ram[SMBC_ENEMY_X_SPEED + x] =
          (uint8_t)(~st->ram[SMBC_ENEMY_X_SPEED + x] + 1u);
      st->ram[SMBC_ENEMY_MOVING_DIR + x]++;
    }
  }
  uint16_t px = ((uint16_t)st->ram[SMBC_PLAYER_PAGE_LOC] << 8) |
                st->ram[SMBC_PLAYER_X_POSITION];
  uint16_t ex = ((posidx & 0x02u) != 0x00u)
                    ? (uint16_t)(px + SMBC_FLYCC_X_POSITION_DATA[posidx])
                    : (uint16_t)(px - SMBC_FLYCC_X_POSITION_DATA[posidx]);
  st->ram[SMBC_ENEMY_X_POSITION + x] = (uint8_t)ex;
  st->ram[SMBC_ENEMY_PAGE_LOC + x] = (uint8_t)(ex >> 8);
  st->ram[SMBC_ENEMY_FLAG + x] = 0x01u;
  st->ram[SMBC_ENEMY_Y_HIGH_POS + x] = 0x01u;
  st->ram[SMBC_ENEMY_Y_POSITION + x] = 0xf8u; /* below the screen */
}

/* smb.asm: SetFlameTimer — the next timer value from the table, with
 * the control counter wrapped to three bits */
static uint8_t set_flame_timer(CoreState *st) {
  uint8_t y = st->ram[SMBC_BOWSER_FLAME_TIMER_CTRL];
  st->ram[SMBC_BOWSER_FLAME_TIMER_CTRL] = (uint8_t)((y + 1u) & 0x07u);
  return SMBC_FLAME_TIMER_DATA[y];
}

/* smb.asm: InitBowserFlame — with bowser onscreen the flame comes from
 * his mouth, otherwise from beyond the right extent after a timer */
static void init_bowser_flame(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_FRENZY_ENEMY_TIMER] != 0x00u) return;
  st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] = 0x00u;
  st->ram[SMBC_NOISE_SOUND_QUEUE] |= SMBC_SFX_BOWSER_FLAME;
  uint8_t by = st->ram[SMBC_BOWSER_FRONT_OFFSET];
  if (st->ram[SMBC_ENEMY_ID + by] == SMBC_BOWSER) {
    /* SpawnFromMouth */
    st->ram[SMBC_ENEMY_X_POSITION + x] =
        (uint8_t)(st->ram[SMBC_ENEMY_X_POSITION + by] - 0x0eu);
    st->ram[SMBC_ENEMY_PAGE_LOC + x] = st->ram[SMBC_ENEMY_PAGE_LOC + by];
    st->ram[SMBC_ENEMY_Y_POSITION + x] =
        (uint8_t)(st->ram[SMBC_ENEMY_Y_POSITION + by] + 0x08u);
    uint8_t y = st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + x] & 0x03u;
    st->ram[SMBC_ENEMY_YMF_DUMMY + x] = y;
    uint8_t target = SMBC_FLAME_Y_POS_DATA[y];
    /* the movement force pulls toward the target height */
    st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] =
        SMBC_FLAME_YMF_ADDER_DATA[
            (target < st->ram[SMBC_ENEMY_Y_POSITION + x]) ? 0x00u : 0x01u];
    st->ram[SMBC_ENEMY_FRENZY_BUFFER] = 0x00u;
    (void)finish_flame(st, x);
    return;
  }
  uint8_t timer = (uint8_t)(set_flame_timer(st) + 0x20u);
  if (st->ram[SMBC_SECONDARY_HARD_MODE] != 0x00u) timer -= 0x10u;
  st->ram[SMBC_FRENZY_ENEMY_TIMER] = timer;
  uint8_t y = st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + x] & 0x03u;
  st->ram[SMBC_BOWSER_FLAME_PRANDOM_OFS + x] = y;
  (void)put_at_right_extent(st, x, SMBC_FLAME_Y_POS_DATA[y]);
}

/* smb.asm: InitFireworks — an explosion near the star flag object; the
 * asm spins forever when none exists, so the port just leaves */
static void init_fireworks(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_FRENZY_ENEMY_TIMER] != 0x00u) return;
  st->ram[SMBC_FRENZY_ENEMY_TIMER] = 0x20u;
  st->ram[SMBC_FIREWORKS_COUNTER]--;
  int star = -1;
  for (int y = 0x05; y >= 0; y--) {
    if (st->ram[SMBC_ENEMY_ID + y] == SMBC_STAR_FLAG_OBJECT) {
      star = y;
      break;
    }
  }
  if (star < 0) return;
  uint16_t starx = (((uint16_t)st->ram[SMBC_ENEMY_PAGE_LOC + star] << 8) |
                    st->ram[SMBC_ENEMY_X_POSITION + star]) -
                   0x30u;
  uint8_t y = (uint8_t)(st->ram[SMBC_FIREWORKS_COUNTER] +
                        st->ram[SMBC_ENEMY_STATE + star]);
  y &= 0x07u; /* the asm reads out of bounds past five; never in-game */
  uint16_t fx = starx + SMBC_FIREWORKS_X_POS_DATA[y];
  st->ram[SMBC_ENEMY_X_POSITION + x] = (uint8_t)fx;
  st->ram[SMBC_ENEMY_PAGE_LOC + x] = (uint8_t)(fx >> 8);
  st->ram[SMBC_ENEMY_Y_POSITION + x] = SMBC_FIREWORKS_Y_POS_DATA[y];
  st->ram[SMBC_ENEMY_Y_HIGH_POS + x] = 0x01u;
  st->ram[SMBC_ENEMY_FLAG + x] = 0x01u;
  st->ram[SMBC_EXPLOSION_GFX_COUNTER + x] = 0x00u;
  st->ram[SMBC_EXPLOSION_TIMER_COUNTER + x] = 0x08u;
}

/* smb.asm: BulletBillCheepCheep — a swimming cheep-cheep in water
 * levels, a bullet bill (with the blast sound, at most one frenzy
 * variant onscreen) elsewhere; the BitMFilter spreads the vertical
 * positions */
static void bullet_bill_cheep_cheep(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_FRENZY_ENEMY_TIMER] != 0x00u) return;
  if (st->ram[SMBC_AREA_TYPE] == 0x00u) {
    if (x >= 0x03u) return; /* the first three slots only */
    uint8_t y = (st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + x] >= 0xaau)
                    ? 0x01u
                    : 0x00u;
    if (st->ram[SMBC_WORLD_NUMBER] != SMBC_WORLD_2) y++;
    y &= 0x01u;
    st->ram[SMBC_ENEMY_ID + x] = SMBC_SWIMCC_ID_DATA[y];
  } else {
    for (int y = 0x00; y < 0x05; y++) {
      if (st->ram[SMBC_ENEMY_FLAG + y] != 0x00u &&
          st->ram[SMBC_ENEMY_ID + y] == SMBC_BULLET_BILL_FRENZY_VAR) {
        return;
      }
    }
    st->ram[SMBC_SQUARE2_SOUND_QUEUE] |= SMBC_SFX_BLAST;
    st->ram[SMBC_ENEMY_ID + x] = SMBC_BULLET_BILL_FRENZY_VAR;
  }
  /* Set17ID */
  if (st->ram[SMBC_BITM_FILTER] == 0xffu) {
    st->ram[SMBC_BITM_FILTER] = 0x00u;
  }
  uint8_t y = st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + x] & 0x07u;
  while ((st->ram[SMBC_BITM_FILTER] & SMBC_BITMASKS[y]) != 0x00u) {
    y = (uint8_t)((y + 1u) & 0x07u);
  }
  st->ram[SMBC_BITM_FILTER] |= SMBC_BITMASKS[y];
  uint8_t a = put_at_right_extent(st, x, SMBC_ENEMY17_Y_POS_DATA[y]);
  st->ram[SMBC_ENEMY_YMF_DUMMY + x] = a; /* zero after the lsr */
  st->ram[SMBC_FRENZY_ENEMY_TIMER] = 0x20u;
  checkpoint_enemy_id(st, x); /* the new enemy gets its own init */
}

/* smb.asm: InitEnemyFrenzy — the identifier goes into the frenzy
 * buffer, then the generator indexed by identifier minus $12 */
static void init_enemy_frenzy(CoreState *st, uint8_t x) {
  uint8_t id = st->ram[SMBC_ENEMY_ID + x];
  st->ram[SMBC_ENEMY_FRENZY_BUFFER] = id;
  switch ((uint8_t)(id - 0x12u)) {
  case 0x00u: lakitu_and_spiny_handler(st, x); break;
  case 0x02u: init_flying_cheep_cheep(st, x); break;
  case 0x03u: init_bowser_flame(st, x); break;
  case 0x04u: init_fireworks(st, x); break;
  case 0x05u: bullet_bill_cheep_cheep(st, x); break;
  default: break; /* NoFrenzyCode */
  }
}

void run_init_enemy_routines(CoreState *st, uint8_t x) {
  /* smb.asm's jump engine table, fifty-four entries by identifier:
   *
   *   $00-$0f: InitNormalEnemy ×3, InitRedKoopa, NoInitCode,
   *            InitHammerBro, InitGoomba, InitBloober, InitBulletBill,
   *            NoInitCode, InitCheepCheep ×2, InitPodoboo,
   *            InitPiranhaPlant, InitJumpGPTroopa, InitRedPTroopa
   *   $10-$1f: InitHorizFlySwimEnemy, InitLakitu, InitEnemyFrenzy,
   *            NoInitCode, InitEnemyFrenzy ×4, EndFrenzy, NoInitCode ×2,
   *            InitShortFirebar ×4, InitLongFirebar
   *   $20-$2f: NoInitCode ×4, InitBalPlatform, InitVertPlatform,
   *            LargeLiftUp, LargeLiftDown, InitHoriPlatform,
   *            InitDropPlatform, InitHoriPlatform, PlatLiftUp,
   *            PlatLiftDown, InitBowser, PwrUpJmp, Setup_Vine
   *   $30-$36: NoInitCode ×5, InitRetainerObj, EndOfEnemyInitCode */
  switch (st->ram[SMBC_ENEMY_ID + x]) {
  case 0x00u:
  case 0x01u:
  case 0x02u: init_normal_enemy(st, x); break;
  case 0x03u: init_red_koopa(st, x); break;
  case 0x05u: init_hammer_bro(st, x); break;
  case 0x06u: init_goomba(st, x); break;
  case 0x07u: init_bloober(st, x); break;
  case 0x08u: init_bullet_bill(st, x); break;
  case 0x0au:
  case 0x0bu: init_cheep_cheep(st, x); break;
  case 0x0cu: init_podoboo(st, x); break;
  case 0x0du: init_piranha_plant(st, x); break;
  case 0x0eu: init_jump_gp_troopa(st, x); break;
  case 0x0fu: init_red_p_troopa(st, x); break;
  case 0x10u: init_horiz_fly_swim_enemy(st, x); break;
  case 0x11u: init_lakitu(st, x); break;
  case 0x12u:
  case 0x14u:
  case 0x15u:
  case 0x16u:
  case 0x17u: init_enemy_frenzy(st, x); break;
  case 0x18u: end_frenzy(st, x); break;
  case 0x1bu:
  case 0x1cu:
  case 0x1du:
  case 0x1eu: init_short_firebar(st, x); break;
  case 0x1fu: init_long_firebar(st, x); break;
  case 0x24u: init_bal_platform(st, x); break;
  case 0x25u: init_vert_platform(st, x); break;
  case 0x26u: large_lift_up(st, x); break;
  case 0x27u: large_lift_down(st, x); break;
  case 0x28u:
  case 0x2au: init_hori_platform(st, x); break;
  case 0x29u: init_drop_platform(st, x); break;
  case 0x2bu: plat_lift_up(st, x); break;
  case 0x2cu: plat_lift_down(st, x); break;
  case 0x2du: init_bowser(st, x); break;
  case 0x2eu: pwr_up_jmp(st); break;
  case 0x2fu:
    /* reached from the interpreter's vine fallback, where the asm's
     * block buffer offset in Y is long stale; the block code will call
     * setup_vine with a real offset when it is ported */
    setup_vine(st, x, 0x00u);
    break;
  case 0x35u: init_retainer_obj(st, x); break;
  default: break; /* NoInitCode / EndOfEnemyInitCode */
  }
}
