/* src/core/collide.c — smb.asm's bounding box and collision layer. */
#include "collide.h"

#include "enemies.h"
#include "offscreen.h"
#include "screen.h"

/* smb.asm: BoundBoxCtrlData — left/top/right/bottom offsets per
 * bound box control value */
static const uint8_t SMBC_BOUND_BOX_CTRL_DATA[48] = {
    0x02u, 0x08u, 0x0eu, 0x20u, 0x03u, 0x14u, 0x0du, 0x20u,
    0x02u, 0x14u, 0x0eu, 0x20u, 0x02u, 0x09u, 0x0eu, 0x15u,
    0x00u, 0x00u, 0x18u, 0x06u, 0x00u, 0x00u, 0x20u, 0x0du,
    0x00u, 0x00u, 0x30u, 0x0du, 0x00u, 0x00u, 0x08u, 0x08u,
    0x06u, 0x04u, 0x0au, 0x08u, 0x03u, 0x0eu, 0x0du, 0x14u,
    0x00u, 0x02u, 0x10u, 0x15u, 0x04u, 0x04u, 0x0cu, 0x1cu,
};

/* smb.asm: MoveBoundBoxOffscreen — all four coordinates offscreen */
static void move_bound_box_offscreen(CoreState *st, uint8_t x) {
  uint16_t base = SMBC_ENEMY_BBOX_COORD + (uint16_t)(x * 4u);
  st->ram[base] = 0xffu;
  st->ram[base + 1u] = 0xffu;
  st->ram[base + 2u] = 0xffu;
  st->ram[base + 3u] = 0xffu;
}

/* smb.asm: GetMaskedOffScrBits — the offscreen bits masked by which
 * side of the screen's left edge the object is on: beyond or
 * exactly at it keeps the caller's second mask, anywhere to the
 * right uses the first */
static void get_masked_off_scr_bits(CoreState *st, uint8_t x,
                                    uint8_t right_of_edge,
                                    uint8_t at_or_beyond) {
  uint8_t low = (uint8_t)(st->ram[SMBC_ENEMY_X_POSITION + x] -
                          st->ram[SMBC_SCREEN_LEFT_X_POS]);
  int16_t high = (int16_t)(st->ram[SMBC_ENEMY_PAGE_LOC + x] -
                           st->ram[SMBC_SCREEN_LEFT_PAGE_LOC] -
                           (low > st->ram[SMBC_ENEMY_X_POSITION + x] ? 1 : 0));
  uint8_t mask = at_or_beyond;
  if (high >= 0 && (high | low) != 0x00) mask = right_of_edge;
  st->ram[SMBC_ENEMY_OFFSCR_MASKED + x] =
      st->ram[SMBC_ENEMY_OFFSCREEN_BITS] & mask;
}

/* smb.asm: BoundingBoxCore — the four corners from the relative
 * coordinates plus the control data row. x arrives as the caller's
 * slot + 1 (the player's box leads the array) and y indexes the
 * relative coordinate arrays */
static void bounding_box_core(CoreState *st, uint8_t x, uint8_t y) {
  uint8_t rel_y = st->ram[SMBC_SPROBJ_REL_YPOS + y];
  uint8_t rel_x = st->ram[SMBC_SPROBJ_REL_XPOS + y];
  uint8_t ctrl = st->ram[SMBC_SPROBJ_BOUND_BOX_CTRL + x];
  const uint8_t *data = &SMBC_BOUND_BOX_CTRL_DATA[ctrl * 4u];
  uint16_t box = SMBC_BOUNDING_BOX_UL_CORNER + (uint16_t)(x * 4u);
  st->ram[box] = (uint8_t)(rel_x + data[0]);
  st->ram[box + 2u] = (uint8_t)(rel_x + data[2]);
  st->ram[box + 1u] = (uint8_t)(rel_y + data[1]);
  st->ram[box + 3u] = (uint8_t)(rel_y + data[3]);
}

/* smb.asm: CheckRightScreenBBox / CheckLeftScreenBBox — against the
 * screen's middle: on the right side a wrapped (sign-set) edge goes
 * to $ff, on the left a wrapped edge past $a0 goes to $00 */
static void check_right_screen_bbox(CoreState *st, uint8_t x, uint8_t y) {
  uint8_t mid_x = (uint8_t)(st->ram[SMBC_SCREEN_LEFT_X_POS] + 0x80u);
  uint8_t carry = mid_x < st->ram[SMBC_SCREEN_LEFT_X_POS] ? 1u : 0u;
  uint8_t mid_page =
      (uint8_t)(st->ram[SMBC_SCREEN_LEFT_PAGE_LOC] + carry);
  uint16_t obj = ((uint16_t)st->ram[SMBC_SPROBJ_PAGE_LOC + x] << 8) |
                 st->ram[SMBC_SPROBJ_X_POSITION + x];
  uint16_t mid = ((uint16_t)mid_page << 8) | mid_x;
  uint16_t ul = SMBC_BOUNDING_BOX_UL_XPOS + y;
  uint16_t dr = SMBC_BOUNDING_BOX_DR_XPOS + y;
  if (obj < mid) {
    /* left side */
    if ((st->ram[ul] & 0x80u) == 0x00u) return;
    if (st->ram[ul] < 0xa0u) return;
    if ((st->ram[dr] & 0x80u) != 0x00u) st->ram[dr] = 0x00u;
    st->ram[ul] = 0x00u;
    return;
  }
  if ((st->ram[dr] & 0x80u) == 0x00u) return;
  if ((st->ram[ul] & 0x80u) != 0x00u) st->ram[ul] = 0xffu;
  st->ram[dr] = 0xffu;
}

void get_enemy_bound_box(CoreState *st, uint8_t x) {
  get_masked_off_scr_bits(st, x, 0x48u, 0x44u);
  if (st->ram[SMBC_ENEMY_OFFSCR_MASKED + x] != 0x00u) {
    move_bound_box_offscreen(st, x);
    return;
  }
  /* smb.asm: SetupEOffsetFBBox */
  bounding_box_core(st, (uint8_t)(x + 1u), 0x01u);
  check_right_screen_bbox(st, (uint8_t)(x + 1u), (uint8_t)((x + 1u) * 4u));
}

uint8_t get_enemy_bound_box_ofs(CoreState *st, uint8_t slot,
                                uint8_t *all_set) {
  uint8_t masked = (uint8_t)(st->ram[SMBC_ENEMY_OFFSCREEN_BITS] & 0x0fu);
  *all_set = masked == 0x0fu ? 0x01u : 0x00u;
  return (uint8_t)(slot * 4u + 4u);
}

/* smb.asm: EnemyBGCXSpdData — the post-stun horizontal speeds,
 * right then left */
static const uint8_t SMBC_ENEMY_BG_CX_SPD_DATA[2] = {0x10u, 0xf0u};

void chk_to_stun_enemies(CoreState *st, uint8_t x, uint8_t a) {
  if (a >= 0x09u && a < 0x11u) {
    if (a < 0x0au || a >= SMBC_PIRANHA_PLANT) {
      /* smb.asm: Demote — erase all but the LSB */
      a = (uint8_t)(a & 0x01u);
      st->ram[SMBC_ENEMY_ID + x] = a;
    }
  }
  set_stun(st, x);
}

/* smb.asm: SetStun — the stun body shared by ChkToStunEnemies and
 * EnemyStomped, which jumps in here with the demote compares
 * skipped */
void set_stun(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_STATE + x] =
      (uint8_t)((st->ram[SMBC_ENEMY_STATE + x] & 0xf0u) | 0x02u);
  st->ram[SMBC_ENEMY_Y_POSITION + x]--;
  st->ram[SMBC_ENEMY_Y_POSITION + x]--;
  uint8_t speed = 0xfdu;
  if (st->ram[SMBC_ENEMY_ID + x] == SMBC_BLOOBER ||
      st->ram[SMBC_AREA_TYPE] == 0x00u) {
    speed = 0xffu; /* smb.asm: SetWYSpd — bloober or water area */
  }
  st->ram[SMBC_ENEMY_Y_SPEED + x] = speed;
  uint8_t dir = 0x01u; /* enemy right of player */
  if (player_enemy_diff(st, x) < 0) dir = 0x02u;
  uint8_t id = st->ram[SMBC_ENEMY_ID + x];
  if (id != SMBC_BULLET_BILL_CANNON && id != SMBC_BULLET_BILL_FRENZY) {
    st->ram[SMBC_ENEMY_MOVING_DIR + x] = dir;
  }
  st->ram[SMBC_ENEMY_X_SPEED + x] = SMBC_ENEMY_BG_CX_SPD_DATA[dir - 1u];
}

void setup_floatey_number(CoreState *st, uint8_t x, uint8_t score) {
  st->ram[SMBC_FLOATEY_NUM_CONTROL + x] = score;
  st->ram[SMBC_FLOATEY_NUM_TIMER + x] = 0x30u;
  st->ram[SMBC_FLOATEY_NUM_Y_POS + x] = st->ram[SMBC_ENEMY_Y_POSITION + x];
  st->ram[SMBC_FLOATEY_NUM_X_POS + x] = st->ram[SMBC_ENEMY_REL_XPOS];
}

void shell_or_block_defeat(CoreState *st, uint8_t x) {
  uint8_t a = st->ram[SMBC_ENEMY_ID + x];
  if (a == SMBC_PIRANHA_PLANT) {
    a = (uint8_t)(st->ram[SMBC_ENEMY_Y_POSITION + x] + 0x18u);
    st->ram[SMBC_ENEMY_Y_POSITION + x] = a;
  }
  chk_to_stun_enemies(st, x, a);
  st->ram[SMBC_ENEMY_STATE + x] =
      (uint8_t)((st->ram[SMBC_ENEMY_STATE + x] & 0x1fu) | 0x20u);
  uint8_t score = 0x02u; /* 200 points */
  uint8_t id = st->ram[SMBC_ENEMY_ID + x];
  if (id == SMBC_HAMMER_BRO) score = 0x06u; /* 1000 */
  if (id == SMBC_GOOMBA) score = 0x01u;     /* 100 */
  setup_floatey_number(st, x, score);
  st->ram[SMBC_SQUARE1_SOUND_QUEUE] = SMBC_SFX_ENEMY_SMACK;
}

uint8_t spr_object_collision_core(CoreState *st, uint8_t x, uint8_t y) {
  for (uint8_t pass = 0x02u; pass != 0x00u; pass--) {
    uint8_t ul_x = st->ram[SMBC_BOUNDING_BOX_UL_CORNER + x];
    uint8_t ul_y = st->ram[SMBC_BOUNDING_BOX_UL_CORNER + y];
    uint8_t lr_x = st->ram[SMBC_BOUNDING_BOX_LR_CORNER + x];
    uint8_t lr_y = st->ram[SMBC_BOUNDING_BOX_LR_CORNER + y];
    uint8_t hit = 0x00u;
    if (ul_y < ul_x) {
      if (ul_y == lr_x) {
        hit = 0x01u;
      } else if (ul_y > lr_x) {
        /* smb.asm: the first box's bottom above its own top, or
         * past the second's top, is a vertical wrap collision */
        hit = lr_y < ul_y || lr_y >= ul_x;
      } else {
        /* smb.asm: SecondBoxVerticalChk */
        hit = lr_x < ul_x || lr_y >= ul_x;
      }
    } else {
      /* smb.asm: FirstBoxGreater */
      if (ul_y == ul_x || ul_y <= lr_x) {
        hit = 0x01u;
      } else if (ul_y > lr_y) {
        hit = lr_y >= ul_x;
      }
    }
    if (hit == 0x00u) return 0x00u;
    x++;
    y++;
  }
  return 0x01u;
}

uint8_t player_collision_core(CoreState *st, uint8_t y) {
  return spr_object_collision_core(st, 0x00u, y);
}

/* smb.asm: RXSpd — the speed-reversal tail of EnemyTurnAround,
 * also jumped to directly by ChkForBump_HammerBroJ */
void rx_spd(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_X_SPEED + x] =
      (uint8_t)(~st->ram[SMBC_ENEMY_X_SPEED + x] + 1u);
  st->ram[SMBC_ENEMY_MOVING_DIR + x] ^= 0x03u;
}

void enemy_turn_around(CoreState *st, uint8_t x) {
  uint8_t id = st->ram[SMBC_ENEMY_ID + x];
  if (id == SMBC_PIRANHA_PLANT || id == SMBC_LAKITU ||
      id == SMBC_HAMMER_BRO) {
    return;
  }
  if (id != SMBC_SPINY && id != SMBC_GREEN_PARATROOPA_JUMP && id >= 0x07u) {
    return;
  }
  rx_spd(st, x);
}

void proc_enemy_collisions(CoreState *st, uint8_t x, uint8_t y) {
  if (((st->ram[SMBC_ENEMY_STATE + y] | st->ram[SMBC_ENEMY_STATE + x]) &
       0x20u) != 0x00u) {
    return;
  }
  if (st->ram[SMBC_ENEMY_STATE + x] >= 0x06u) {
    /* the second enemy is a shell or stunned: it kills the first */
    if (st->ram[SMBC_ENEMY_ID + x] == SMBC_HAMMER_BRO) return;
    if ((st->ram[SMBC_ENEMY_STATE + y] & 0x80u) != 0x00u) {
      /* smb.asm: the colliding enemy is itself a moving shell —
       * kill this one too, then fall through for it below */
      setup_floatey_number(st, x, 0x06u); /* 1000 points */
      shell_or_block_defeat(st, x);
    }
    shell_or_block_defeat(st, y);
    uint8_t score =
        (uint8_t)(st->ram[SMBC_SHELL_CHAIN_COUNTER + x] + 0x04u);
    setup_floatey_number(st, y, score);
    st->ram[SMBC_SHELL_CHAIN_COUNTER + x]++;
    return;
  }
  if (st->ram[SMBC_ENEMY_STATE + y] >= 0x06u) {
    /* smb.asm: ProcSecondEnemyColl — the first is the shell */
    if (st->ram[SMBC_ENEMY_ID + y] == SMBC_HAMMER_BRO) return;
    shell_or_block_defeat(st, x);
    uint8_t score =
        (uint8_t)(st->ram[SMBC_SHELL_CHAIN_COUNTER + y] + 0x04u);
    setup_floatey_number(st, x, score);
    st->ram[SMBC_SHELL_CHAIN_COUNTER + y]++;
    return;
  }
  /* smb.asm: MoveEOfs — two walking enemies turn around */
  enemy_turn_around(st, y);
  enemy_turn_around(st, x);
}

/* smb.asm: SetBitsMask / ClearBitsMask — one bit per slot, slot 0
 * taking d7 */
static const uint8_t SMBC_SET_BITS_MASK[7] = {
    0x80u, 0x40u, 0x20u, 0x10u, 0x08u, 0x04u, 0x02u};
static const uint8_t SMBC_CLEAR_BITS_MASK[7] = {
    0x7fu, 0xbfu, 0xdfu, 0xefu, 0xf7u, 0xfbu, 0xfdu};

void enemies_collision(CoreState *st, uint8_t x) {
  if ((st->ram[SMBC_FRAME_COUNTER] & 0x01u) == 0x00u) return;
  if (st->ram[SMBC_AREA_TYPE] == 0x00u) return; /* water area */
  uint8_t id = st->ram[SMBC_ENEMY_ID + x];
  if (id >= 0x15u || id == SMBC_LAKITU || id == SMBC_PIRANHA_PLANT) return;
  if (st->ram[SMBC_ENEMY_OFFSCR_MASKED + x] != 0x00u) return;
  uint8_t all_set = 0x00u;
  uint8_t bbox = get_enemy_bound_box_ofs(st, x, &all_set);
  if (x == 0x00u) return; /* no other enemies to compare */
  for (uint8_t slot = (uint8_t)(x - 1u);; slot--) {
    uint8_t other_id = st->ram[SMBC_ENEMY_ID + slot];
    uint8_t ok = st->ram[SMBC_ENEMY_FLAG + slot] != 0x00u &&
                 other_id < 0x15u && other_id != SMBC_LAKITU &&
                 other_id != SMBC_PIRANHA_PLANT &&
                 st->ram[SMBC_ENEMY_OFFSCR_MASKED + slot] == 0x00u;
    if (ok) {
      uint8_t other_bbox = (uint8_t)(slot * 4u + 4u);
      if (spr_object_collision_core(st, other_bbox, bbox)) {
        if (((st->ram[SMBC_ENEMY_STATE + x] |
              st->ram[SMBC_ENEMY_STATE + slot]) &
             0x80u) != 0x00u) {
          proc_enemy_collisions(st, x, slot);
        } else if ((st->ram[SMBC_ENEMY_COLLISION_BITS + slot] &
                    SMBC_SET_BITS_MASK[x]) == 0x00u) {
          st->ram[SMBC_ENEMY_COLLISION_BITS + slot] |=
              SMBC_SET_BITS_MASK[x];
          proc_enemy_collisions(st, x, slot);
        }
      } else {
        st->ram[SMBC_ENEMY_COLLISION_BITS + slot] &=
            SMBC_CLEAR_BITS_MASK[x];
      }
    }
    if (slot == 0x00u) break;
  }
}

/* smb.asm: StompedEnemyPtsData — 200, 600, 500, 600 */
static const uint8_t SMBC_STOMPED_ENEMY_PTS_DATA[4] = {
    0x02u, 0x06u, 0x05u, 0x06u};
/* smb.asm: DemotedKoopaXSpdData */
static const uint8_t SMBC_DEMOTED_KOOPA_X_SPD_DATA[2] = {0x08u, 0xf8u};
/* smb.asm: RevivalRateData */
static const uint8_t SMBC_REVIVAL_RATE_DATA[2] = {0x10u, 0x0bu};

uint8_t check_player_vertical(CoreState *st) {
  if (st->ram[SMBC_PLAYER_OFFSCREEN_BITS] >= 0xf0u) return 0x00u;
  if (st->ram[SMBC_PLAYER_Y_HIGH_POS] != 0x01u) return 0x00u;
  return st->ram[SMBC_PLAYER_Y_POSITION] < 0xd0u;
}

uint8_t init_vstf(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_Y_SPEED + x] = 0x00u;
  st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] = 0x00u;
  return 0x00u;
}

uint8_t enemy_face_player(CoreState *st, uint8_t x) {
  uint8_t dir = 0x01u; /* enemy right of player: move right */
  if (player_enemy_diff(st, x) < 0) dir = 0x02u;
  st->ram[SMBC_ENEMY_MOVING_DIR + x] = dir;
  return (uint8_t)(dir - 1u);
}

void enemy_stomped(CoreState *st, uint8_t x) {
  uint8_t id = st->ram[SMBC_ENEMY_ID + x];
  if (id == SMBC_SPINY) {
    /* smb.asm: spiny stomps bounce back to InjurePlayer — handled
     * by the caller, never reached here */
    return;
  }
  st->ram[SMBC_SQUARE1_SOUND_QUEUE] = SMBC_SFX_ENEMY_STOMP;
  uint8_t ofs;
  if (id == SMBC_FLYING_CHEEP_CHEEP || id == SMBC_BULLET_BILL_FRENZY ||
      id == SMBC_BULLET_BILL_CANNON || id == SMBC_PODOBOO) {
    ofs = 0x00u;
  } else if (id == SMBC_HAMMER_BRO) {
    ofs = 0x01u;
  } else if (id == SMBC_LAKITU) {
    ofs = 0x02u;
  } else if (id == SMBC_BLOOBER) {
    ofs = 0x03u;
  } else {
    /* smb.asm: anything else — goombas and koopas included — falls
     * through to the demote check */
    chk_for_demote_koopa(st, x);
    return;
  }
  setup_floatey_number(st, x, SMBC_STOMPED_ENEMY_PTS_DATA[ofs]);
  uint8_t dir = st->ram[SMBC_ENEMY_MOVING_DIR + x];
  set_stun(st, x);
  st->ram[SMBC_ENEMY_MOVING_DIR + x] = dir;
  st->ram[SMBC_ENEMY_STATE + x] = 0x20u;
  st->ram[SMBC_ENEMY_X_SPEED + x] = init_vstf(st, x);
  st->ram[SMBC_PLAYER_Y_SPEED] = 0xfdu;
}

void chk_for_demote_koopa(CoreState *st, uint8_t x) {
  uint8_t id = st->ram[SMBC_ENEMY_ID + x];
  if (id < 0x09u) {
    handle_stomped_shell_e(st, x);
    return;
  }
  id = (uint8_t)(id & 0x01u);
  st->ram[SMBC_ENEMY_ID + x] = id;
  st->ram[SMBC_ENEMY_STATE + x] = 0x00u;
  setup_floatey_number(st, x, 0x03u); /* 400 points */
  init_vstf(st, x);
  uint8_t ofs = enemy_face_player(st, x);
  st->ram[SMBC_ENEMY_X_SPEED + x] = SMBC_DEMOTED_KOOPA_X_SPD_DATA[ofs];
  st->ram[SMBC_PLAYER_Y_SPEED] = 0xfcu;
}

void handle_stomped_shell_e(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_STATE + x] = 0x04u;
  st->ram[SMBC_STOMP_CHAIN_COUNTER]++;
  uint8_t score = (uint8_t)(st->ram[SMBC_STOMP_CHAIN_COUNTER] +
                            st->ram[SMBC_STOMP_TIMER]);
  setup_floatey_number(st, x, score);
  st->ram[SMBC_STOMP_TIMER]++;
  st->ram[SMBC_ENEMY_INTERVAL_TIMER + x] =
      SMBC_REVIVAL_RATE_DATA[st->ram[SMBC_PRIMARY_HARD_MODE] & 0x01u];
  st->ram[SMBC_PLAYER_Y_SPEED] = 0xfcu;
}

void force_injury(CoreState *st) {
  uint8_t status = st->ram[SMBC_PLAYER_STATUS];
  if (status == 0x00u) {
    /* smb.asm: KillPlayer */
    st->ram[SMBC_PLAYER_X_SPEED] = 0x00u;
    st->ram[SMBC_EVENT_MUSIC_QUEUE] = 0x01u; /* death music */
    st->ram[SMBC_PLAYER_Y_SPEED] = 0xfcu;
    st->ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x0bu;
    st->ram[SMBC_PLAYER_STATE] = 0x01u;
    st->ram[SMBC_TIMER_CONTROL] = 0xffu;
    st->ram[SMBC_SCROLL_AMOUNT] = 0x00u;
    return;
  }
  st->ram[SMBC_PLAYER_STATUS] = 0x00u; /* shrink to small */
  st->ram[SMBC_INJURY_TIMER] = 0x08u;
  st->ram[SMBC_SQUARE1_SOUND_QUEUE] = 0x10u; /* pipedown/injury */
  get_player_colors_core(st);
  st->ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x0au;
  st->ram[SMBC_PLAYER_STATE] = 0x01u;
  st->ram[SMBC_TIMER_CONTROL] = 0xffu;
  st->ram[SMBC_SCROLL_AMOUNT] = 0x00u;
}

void injure_player(CoreState *st) {
  if (st->ram[SMBC_INJURY_TIMER] != 0x00u) return;
  force_injury(st);
}

uint8_t chk_for_player_injury(CoreState *st, uint8_t x) {
  int8_t y_speed = (int8_t)st->ram[SMBC_PLAYER_Y_SPEED];
  if (y_speed > 0) {
    /* smb.asm: falling — the enemy is stomped */
    enemy_stomped(st, x);
    return 0x01u;
  }
  if (st->ram[SMBC_ENEMY_ID + x] >= SMBC_BLOOBER) {
    uint8_t adj = (uint8_t)(st->ram[SMBC_PLAYER_Y_POSITION] + 0x0cu);
    if (adj < st->ram[SMBC_ENEMY_Y_POSITION + x]) {
      enemy_stomped(st, x);
      return 0x01u;
    }
  }
  if (st->ram[SMBC_STOMP_TIMER] != 0x00u) {
    enemy_stomped(st, x);
    return 0x01u;
  }
  if (st->ram[SMBC_INJURY_TIMER] != 0x00u) return 0x00u;
  /* smb.asm: a moving enemy facing the player turns around first */
  if (st->ram[SMBC_SPROBJ_REL_XPOS] < st->ram[SMBC_ENEMY_REL_XPOS]) {
    if (st->ram[SMBC_ENEMY_MOVING_DIR + x] == 0x01u) {
      enemy_turn_around(st, x);
    }
  } else {
    if (st->ram[SMBC_ENEMY_MOVING_DIR + x] != 0x01u) {
      enemy_turn_around(st, x);
    }
  }
  injure_player(st);
  return 0x00u;
}

/* smb.asm: KickedShellPtsData */
static const uint8_t SMBC_KICKED_SHELL_PTS_DATA[3] = {0x0au, 0x06u, 0x04u};
/* smb.asm: KickedShellXSpdData */
static const uint8_t SMBC_KICKED_SHELL_X_SPD_DATA[2] = {0x30u, 0xd0u};

void handle_power_up_collision(CoreState *st, uint8_t x) {
  erase_enemy_object(st, x);
  setup_floatey_number(st, x, 0x06u); /* 1000 points */
  st->ram[SMBC_SQUARE2_SOUND_QUEUE] = SMBC_SFX_POWER_UP_GRAB;
  uint8_t type = st->ram[SMBC_POWER_UP_TYPE];
  if (type >= 0x02u) {
    if (type == 0x03u) {
      /* smb.asm: SetFor1Up — the floatey becomes a 1-up */
      st->ram[SMBC_FLOATEY_NUM_CONTROL + x] = 0x0bu;
      return;
    }
    st->ram[SMBC_STAR_INVINCIBLE_TIMER] = 0x23u;
    st->ram[SMBC_AREA_MUSIC_QUEUE] = SMBC_STAR_POWER_MUSIC;
    return;
  }
  uint8_t status = st->ram[SMBC_PLAYER_STATUS];
  if (status == 0x00u) {
    /* smb.asm: UpToSuper */
    st->ram[SMBC_PLAYER_STATUS] = 0x01u;
    st->ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x09u;
  } else if (status == 0x01u) {
    st->ram[SMBC_PLAYER_STATUS] = 0x02u;
    get_player_colors_core(st);
    st->ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x0cu;
  } else {
    return;
  }
  /* smb.asm: UpToFiery — SetPRout with player state 0 */
  st->ram[SMBC_PLAYER_STATE] = 0x00u;
  st->ram[SMBC_TIMER_CONTROL] = 0xffu;
  st->ram[SMBC_SCROLL_AMOUNT] = 0x00u;
}

void handle_pe_collisions(CoreState *st, uint8_t x) {
  if ((st->ram[SMBC_ENEMY_COLLISION_BITS + x] & 0x01u) != 0x00u) return;
  if (st->ram[SMBC_ENEMY_OFFSCR_MASKED + x] != 0x00u) return;
  st->ram[SMBC_ENEMY_COLLISION_BITS + x] |= 0x01u;
  uint8_t id = st->ram[SMBC_ENEMY_ID + x];
  if (id == SMBC_SPINY || id == SMBC_PIRANHA_PLANT || id == SMBC_PODOBOO ||
      id == SMBC_BULLET_BILL_CANNON || id >= 0x15u ||
      st->ram[SMBC_AREA_TYPE] == 0x00u) {
    /* smb.asm: spiny and cannon bullet bill go through the
     * stomp-or-injury decision; piranha plants, podoboos,
     * objects >= $15 and water contacts injure directly */
    if (id == SMBC_SPINY || id == SMBC_BULLET_BILL_CANNON) {
      chk_for_player_injury(st, x);
    } else {
      injure_player(st);
    }
    return;
  }
  if ((st->ram[SMBC_ENEMY_STATE + x] & 0x80u) != 0x00u) {
    chk_for_player_injury(st, x);
    return;
  }
  uint8_t state = st->ram[SMBC_ENEMY_STATE + x] & 0x07u;
  if (state < 0x02u) {
    chk_for_player_injury(st, x);
    return;
  }
  if (id == SMBC_GOOMBA) return; /* defeated goomba: leave */
  /* smb.asm: kicking a stunned enemy into a moving shell */
  st->ram[SMBC_SQUARE1_SOUND_QUEUE] = SMBC_SFX_ENEMY_SMACK;
  st->ram[SMBC_ENEMY_STATE + x] |= 0x80u;
  uint8_t ofs = enemy_face_player(st, x);
  st->ram[SMBC_ENEMY_X_SPEED + x] = SMBC_KICKED_SHELL_X_SPD_DATA[ofs];
  uint8_t score = (uint8_t)(0x03u + st->ram[SMBC_STOMP_CHAIN_COUNTER]);
  uint8_t timer = st->ram[SMBC_ENEMY_INTERVAL_TIMER + x];
  if (timer < 0x03u) score = SMBC_KICKED_SHELL_PTS_DATA[timer];
  setup_floatey_number(st, x, score);
}

void player_enemy_collision(CoreState *st, uint8_t x) {
  if ((st->ram[SMBC_FRAME_COUNTER] & 0x01u) != 0x00u) return;
  if (!check_player_vertical(st)) return;
  if (st->ram[SMBC_ENEMY_OFFSCR_MASKED + x] != 0x00u) return;
  if (st->ram[SMBC_GAME_ENGINE_SUBROUTINE] != 0x08u) return;
  if ((st->ram[SMBC_ENEMY_STATE + x] & 0x20u) != 0x00u) return;
  uint8_t all_set = 0x00u;
  uint8_t bbox = get_enemy_bound_box_ofs(st, x, &all_set);
  if (!player_collision_core(st, bbox)) {
    st->ram[SMBC_ENEMY_COLLISION_BITS + x] &= 0xfeu;
    return;
  }
  if (st->ram[SMBC_ENEMY_ID + x] == SMBC_POWER_UP_OBJECT) {
    handle_power_up_collision(st, x);
    return;
  }
  if (st->ram[SMBC_STAR_INVINCIBLE_TIMER] != 0x00u) {
    shell_or_block_defeat(st, x);
    return;
  }
  handle_pe_collisions(st, x);
}

/* smb.asm: BlockBuffer_X_Adder / BlockBuffer_Y_Adder */
static const uint8_t SMBC_BLOCK_BUFFER_X_ADDER[28] = {
    0x08u, 0x03u, 0x0cu, 0x02u, 0x02u, 0x0du, 0x0du, 0x08u,
    0x03u, 0x0cu, 0x02u, 0x02u, 0x0du, 0x0du, 0x08u, 0x03u,
    0x0cu, 0x02u, 0x02u, 0x0du, 0x0du, 0x08u, 0x00u, 0x10u,
    0x04u, 0x14u, 0x04u, 0x04u};
static const uint8_t SMBC_BLOCK_BUFFER_Y_ADDER[28] = {
    0x04u, 0x20u, 0x20u, 0x08u, 0x18u, 0x08u, 0x18u, 0x02u,
    0x20u, 0x20u, 0x08u, 0x18u, 0x08u, 0x18u, 0x12u, 0x20u,
    0x20u, 0x18u, 0x18u, 0x18u, 0x18u, 0x18u, 0x14u, 0x14u,
    0x06u, 0x06u, 0x08u, 0x10u};

void get_block_buffer_addr(CoreState *st, uint8_t column) {
  /* smb.asm: BlockBufferAddr — low bytes {00, d0}, both high $05 */
  uint16_t addr = SMBC_BLOCK_BUFFER_1 +
                  (uint16_t)((column >> 4u) * 0xd0u) + (column & 0x0fu);
  st->ram[SMBC_BB_PTR_LO] = (uint8_t)(addr & 0xffu);
  st->ram[SMBC_BB_PTR_HI] = (uint8_t)(addr >> 8u);
}

uint8_t block_buffer_chk_enemy(CoreState *st, uint8_t x, uint8_t y_ofs,
                               uint8_t return_x) {
  /* the asm adds one to X, the enemy slots following the player
   * in the SprObject arrays */
  uint8_t slot = (uint8_t)(x + 1u);
  uint16_t sum =
      (uint16_t)st->ram[SMBC_SPROBJ_X_POSITION + slot] +
      SMBC_BLOCK_BUFFER_X_ADDER[y_ofs];
  st->ram[SMBC_BB_X_COORD] = (uint8_t)sum;
  uint8_t page =
      (uint8_t)((st->ram[SMBC_SPROBJ_PAGE_LOC + slot] + (sum >> 8u)) & 0x01u);
  uint8_t column = (uint8_t)(((sum & 0xffu) >> 4u) | (page << 4u));
  get_block_buffer_addr(st, column);
  uint8_t row = (uint8_t)(
      ((st->ram[SMBC_SPROBJ_Y_POSITION + slot] +
        SMBC_BLOCK_BUFFER_Y_ADDER[y_ofs]) &
       0xf0u) -
      0x20u);
  st->ram[SMBC_BB_Y_COORD] = row;
  uint16_t ptr =
      (uint16_t)(st->ram[SMBC_BB_PTR_LO] | (st->ram[SMBC_BB_PTR_HI] << 8u));
  uint8_t metatile = st->ram[ptr + row];
  st->ram[SMBC_BB_METATILE] = metatile;
  uint8_t coord = return_x != 0x00u ? st->ram[SMBC_SPROBJ_X_POSITION + slot]
                                    : st->ram[SMBC_SPROBJ_Y_POSITION + slot];
  st->ram[SMBC_BB_LOW_NYBBLE] = (uint8_t)(coord & 0x0fu);
  return metatile;
}

uint8_t chk_under_enemy(CoreState *st, uint8_t x) {
  return block_buffer_chk_enemy(st, x, 0x15u, 0x00u);
}

uint8_t chk_for_non_solids(uint8_t metatile) {
  return metatile != 0x26u && metatile != 0xc2u && metatile != 0xc3u &&
         metatile != 0x5fu && metatile != 0x60u;
}

uint8_t subt_enemy_y_pos(CoreState *st, uint8_t x) {
  return (uint8_t)(st->ram[SMBC_ENEMY_Y_POSITION + x] + 0x3eu) >= 0x44u;
}

void enemy_landing(CoreState *st, uint8_t x) {
  init_vstf(st, x);
  st->ram[SMBC_ENEMY_Y_POSITION + x] =
      (uint8_t)((st->ram[SMBC_ENEMY_Y_POSITION + x] & 0xf0u) | 0x08u);
}

/* smb.asm: EnemyBGCStateData */
static const uint8_t SMBC_ENEMY_BGC_STATE_DATA[6] = {
    0x01u, 0x01u, 0x02u, 0x02u, 0x02u, 0x05u};
/* smb.asm: HammerBroJumpLData */
static const uint8_t SMBC_HAMMER_BRO_JUMP_L_DATA[2] = {0x20u, 0x37u};

void set_hj(CoreState *st, uint8_t x, uint8_t y_speed, uint8_t bitmask) {
  st->ram[SMBC_ENEMY_Y_SPEED + x] = y_speed;
  st->ram[SMBC_ENEMY_STATE + x] |= 0x01u;
  uint8_t ofs = st->ram[SMBC_SECONDARY_HARD_MODE] != 0x00u
                    ? (uint8_t)(bitmask & st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + 2u + x])
                    : 0x00u;
  st->ram[SMBC_ENEMY_FRAME_TIMER + x] = SMBC_HAMMER_BRO_JUMP_L_DATA[ofs];
  st->ram[SMBC_HAMMER_BRO_JUMP_TIMER + x] =
      (uint8_t)(st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1u + x] | 0xc0u);
}

void chk_for_bump_hammer_bro_j(CoreState *st, uint8_t x) {
  if (x != 0x05u && (st->ram[SMBC_ENEMY_STATE + x] & 0x80u) != 0x00u) {
    st->ram[SMBC_SQUARE1_SOUND_QUEUE] = SMBC_SFX_BUMP;
  }
  if (st->ram[SMBC_ENEMY_ID + x] == SMBC_HAMMER_BRO) {
    set_hj(st, x, 0xfau, 0x00u);
    return;
  }
  rx_spd(st, x);
}

void land_enemy_init_state(CoreState *st, uint8_t x) {
  enemy_landing(st, x);
  if ((st->ram[SMBC_ENEMY_STATE + x] & 0x80u) != 0x00u) {
    st->ram[SMBC_ENEMY_STATE + x] &= 0xbfu;
    return;
  }
  st->ram[SMBC_ENEMY_STATE + x] = 0x00u;
}

void proc_enemy_direction(CoreState *st, uint8_t x) {
  uint8_t id = st->ram[SMBC_ENEMY_ID + x];
  if (id == SMBC_GOOMBA) {
    land_enemy_init_state(st, x);
    return;
  }
  if (id == SMBC_SPINY) {
    st->ram[SMBC_ENEMY_MOVING_DIR + x] = 0x01u;
    st->ram[SMBC_ENEMY_X_SPEED + x] = 0x08u;
    if ((st->ram[SMBC_FRAME_COUNTER] & 0x07u) == 0x00u) {
      land_enemy_init_state(st, x);
      return;
    }
  }
  /* smb.asm: InvtD — the inverted facing values */
  uint8_t dir = 0x01u;
  if (player_enemy_diff(st, x) < 0) dir = 0x02u;
  if (dir != st->ram[SMBC_ENEMY_MOVING_DIR + x]) {
    land_enemy_init_state(st, x);
    return;
  }
  chk_for_bump_hammer_bro_j(st, x);
}

void chk_landed_enemy_state(CoreState *st, uint8_t x) {
  uint8_t state = st->ram[SMBC_ENEMY_STATE + x];
  if (state == 0x00u) {
    do_enemy_side_check(st, x);
    return;
  }
  if (state == 0x05u) {
    proc_enemy_direction(st, x);
    return;
  }
  if (state >= 0x03u) return;
  if (state != 0x02u) {
    proc_enemy_direction(st, x);
    return;
  }
  uint8_t timer = st->ram[SMBC_ENEMY_ID + x] == SMBC_SPINY ? 0x00u : 0x10u;
  st->ram[SMBC_ENEMY_INTERVAL_TIMER + x] = timer;
  st->ram[SMBC_ENEMY_STATE + x] = 0x03u;
  enemy_landing(st, x);
}

void land_enemy_properly(CoreState *st, uint8_t x) {
  uint8_t nyb = (uint8_t)(st->ram[SMBC_BB_LOW_NYBBLE] - 0x08u);
  if (nyb >= 0x05u) {
    chk_for_red_koopa(st, x);
    return;
  }
  if ((st->ram[SMBC_ENEMY_STATE + x] & 0x40u) != 0x00u) {
    land_enemy_init_state(st, x);
    return;
  }
  if ((st->ram[SMBC_ENEMY_STATE + x] & 0x80u) != 0x00u) {
    do_enemy_side_check(st, x);
    return;
  }
  chk_landed_enemy_state(st, x);
}

void chk_for_red_koopa(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_ENEMY_ID + x] == SMBC_RED_KOOPA &&
      st->ram[SMBC_ENEMY_STATE + x] == 0x00u) {
    chk_for_bump_hammer_bro_j(st, x);
    return;
  }
  uint8_t state = st->ram[SMBC_ENEMY_STATE + x];
  if ((state & 0x80u) != 0x00u) {
    st->ram[SMBC_ENEMY_STATE + x] = (uint8_t)(state | 0x40u);
    return;
  }
  st->ram[SMBC_ENEMY_STATE + x] = SMBC_ENEMY_BGC_STATE_DATA[state];
}

void do_enemy_side_check(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_ENEMY_Y_POSITION + x] < 0x20u) return;
  st->ram[SMBC_SIDE_CHECK_DIR] = 0x02u;
  for (uint8_t y = 0x16u; y < 0x18u; y++) {
    if (st->ram[SMBC_SIDE_CHECK_DIR] == st->ram[SMBC_ENEMY_MOVING_DIR + x]) {
      uint8_t metatile = block_buffer_chk_enemy(st, x, y, 0x01u);
      if (metatile != 0x00u && chk_for_non_solids(metatile)) {
        chk_for_bump_hammer_bro_j(st, x);
        /* the asm's turn-around does not return here; the loop
         * continues with $eb/y exhausted past it, but a second
         * identical probe at the other offset cannot match the
         * now-inverted direction, so falling out is equivalent */
      }
    }
    st->ram[SMBC_SIDE_CHECK_DIR]--;
  }
}

void enemy_jump(CoreState *st, uint8_t x) {
  if (!subt_enemy_y_pos(st, x)) {
    do_enemy_side_check(st, x);
    return;
  }
  if ((uint8_t)(st->ram[SMBC_ENEMY_Y_SPEED + x] + 0x02u) < 0x03u) {
    do_enemy_side_check(st, x);
    return;
  }
  uint8_t metatile = chk_under_enemy(st, x);
  if (metatile == 0x00u || !chk_for_non_solids(metatile)) {
    do_enemy_side_check(st, x);
    return;
  }
  enemy_landing(st, x);
  st->ram[SMBC_ENEMY_Y_SPEED + x] = 0xfdu;
  do_enemy_side_check(st, x);
}

void kill_enemy_above_block(CoreState *st, uint8_t x) {
  shell_or_block_defeat(st, x);
  st->ram[SMBC_ENEMY_Y_SPEED + x] = 0xfcu;
}

void hammer_bro_bg_coll(CoreState *st, uint8_t x) {
  uint8_t metatile = chk_under_enemy(st, x);
  if (metatile == 0x00u) {
    st->ram[SMBC_ENEMY_STATE + x] |= 0x01u;
    return;
  }
  if (metatile == 0x23u) {
    kill_enemy_above_block(st, x);
    return;
  }
  if (st->ram[SMBC_ENEMY_FRAME_TIMER + x] != 0x00u) {
    st->ram[SMBC_ENEMY_STATE + x] |= 0x01u;
    return;
  }
  st->ram[SMBC_ENEMY_STATE + x] &= 0x88u;
  enemy_landing(st, x);
  do_enemy_side_check(st, x);
}

void handle_e_to_bg_collision(CoreState *st, uint8_t x, uint8_t metatile) {
  if (!chk_for_non_solids(metatile)) {
    /* smb.asm: NoEToBGCollision — blank, coins and hidden blocks
     * fall through */
    chk_for_red_koopa(st, x);
    return;
  }
  if (metatile != 0x23u) {
    land_enemy_properly(st, x);
    return;
  }
  /* smb.asm: erase the $23 blank so this does not retrigger */
  uint16_t ptr =
      (uint16_t)(st->ram[SMBC_BB_PTR_LO] | (st->ram[SMBC_BB_PTR_HI] << 8u));
  st->ram[ptr + st->ram[SMBC_BB_Y_COORD]] = 0x00u;
  uint8_t id = st->ram[SMBC_ENEMY_ID + x];
  if (id < 0x15u) {
    if (id == SMBC_GOOMBA) kill_enemy_above_block(st, x);
    /* smb.asm: GiveOEPoints — 100 points for hitting the block
     * beneath the enemy */
    setup_floatey_number(st, x, 0x01u);
  }
  chk_to_stun_enemies(st, x, id);
}

void enemy_to_bg_collision_det(CoreState *st, uint8_t x) {
  if ((st->ram[SMBC_ENEMY_STATE + x] & 0x20u) != 0x00u) return;
  if (!subt_enemy_y_pos(st, x)) return;
  uint8_t id = st->ram[SMBC_ENEMY_ID + x];
  if (id == SMBC_SPINY && st->ram[SMBC_ENEMY_Y_POSITION + x] < 0x25u) return;
  if (id == SMBC_GREEN_PARATROOPA_JUMP) {
    enemy_jump(st, x);
    return;
  }
  if (id == SMBC_HAMMER_BRO) {
    hammer_bro_bg_coll(st, x);
    return;
  }
  if (id != SMBC_SPINY && id != SMBC_POWER_UP_OBJECT && id >= 0x07u) return;
  uint8_t metatile = chk_under_enemy(st, x);
  if (metatile != 0x00u) {
    handle_e_to_bg_collision(st, x, metatile);
    return;
  }
  chk_for_red_koopa(st, x);
}

/* smb.asm: EnemyGraphicsTable — six tiles per frame, the frames
 * stacked per object; the offsets below index into it */
static const uint8_t SMBC_ENEMY_GRAPHICS_TABLE[270] = {
    0xfcu, 0xfcu, 0xaau, 0xabu, 0xacu, 0xadu, /* buzzy beetle 1 */
    0xfcu, 0xfcu, 0xaeu, 0xafu, 0xb0u, 0xb1u, /* buzzy beetle 2 */
    0xfcu, 0xa5u, 0xa6u, 0xa7u, 0xa8u, 0xa9u, /* koopa troopa 1 */
    0xfcu, 0xa0u, 0xa1u, 0xa2u, 0xa3u, 0xa4u, /* koopa troopa 2 */
    0x69u, 0xa5u, 0x6au, 0xa7u, 0xa8u, 0xa9u, /* paratroopa 1 */
    0x6bu, 0xa0u, 0x6cu, 0xa2u, 0xa3u, 0xa4u, /* paratroopa 2 */
    0xfcu, 0xfcu, 0x96u, 0x97u, 0x98u, 0x99u, /* spiny 1 */
    0xfcu, 0xfcu, 0x9au, 0x9bu, 0x9cu, 0x9du, /* spiny 2 */
    0xfcu, 0xfcu, 0x8fu, 0x8eu, 0x8eu, 0x8fu, /* spiny's egg 1 */
    0xfcu, 0xfcu, 0x95u, 0x94u, 0x94u, 0x95u, /* spiny's egg 2 */
    0xfcu, 0xfcu, 0xdcu, 0xdcu, 0xdfu, 0xdfu, /* bloober 1 */
    0xdcu, 0xdcu, 0xddu, 0xddu, 0xdeu, 0xdeu, /* bloober 2 */
    0xfcu, 0xfcu, 0xb2u, 0xb3u, 0xb4u, 0xb5u, /* cheep-cheep 1 */
    0xfcu, 0xfcu, 0xb6u, 0xb3u, 0xb7u, 0xb5u, /* cheep-cheep 2 */
    0xfcu, 0xfcu, 0x70u, 0x71u, 0x72u, 0x73u, /* goomba */
    0xfcu, 0xfcu, 0x6eu, 0x6eu, 0x6fu, 0x6fu, /* koopa shell, down 1 */
    0xfcu, 0xfcu, 0x6du, 0x6du, 0x6fu, 0x6fu, /* koopa shell, down 2 */
    0xfcu, 0xfcu, 0x6fu, 0x6fu, 0x6eu, 0x6eu, /* koopa shell, up 1 */
    0xfcu, 0xfcu, 0x6fu, 0x6fu, 0x6du, 0x6du, /* koopa shell, up 2 */
    0xfcu, 0xfcu, 0xf4u, 0xf4u, 0xf5u, 0xf5u, /* buzzy shell, up 1 */
    0xfcu, 0xfcu, 0xf4u, 0xf4u, 0xf5u, 0xf5u, /* buzzy shell, up 2 */
    0xfcu, 0xfcu, 0xf5u, 0xf5u, 0xf4u, 0xf4u, /* buzzy shell, down 1 */
    0xfcu, 0xfcu, 0xf5u, 0xf5u, 0xf4u, 0xf4u, /* buzzy shell, down 2 */
    0xfcu, 0xfcu, 0xfcu, 0xfcu, 0xefu, 0xefu, /* defeated goomba */
    0xb9u, 0xb8u, 0xbbu, 0xbau, 0xbcu, 0xbcu, /* lakitu 1 */
    0xfcu, 0xfcu, 0xbdu, 0xbdu, 0xbcu, 0xbcu, /* lakitu 2 */
    0x7au, 0x7bu, 0xdau, 0xdbu, 0xd8u, 0xd8u, /* princess */
    0xcdu, 0xcdu, 0xceu, 0xceu, 0xcfu, 0xcfu, /* mushroom retainer */
    0x7du, 0x7cu, 0xd1u, 0x8cu, 0xd3u, 0xd2u, /* hammer bro 1 */
    0x7du, 0x7cu, 0x89u, 0x88u, 0x8bu, 0x8au, /* hammer bro 2 */
    0xd5u, 0xd4u, 0xe3u, 0xe2u, 0xd3u, 0xd2u, /* hammer bro 3 */
    0xd5u, 0xd4u, 0xe3u, 0xe2u, 0x8bu, 0x8au, /* hammer bro 4 */
    0xe5u, 0xe5u, 0xe6u, 0xe6u, 0xebu, 0xebu, /* piranha plant 1 */
    0xecu, 0xecu, 0xedu, 0xedu, 0xeeu, 0xeeu, /* piranha plant 2 */
    0xfcu, 0xfcu, 0xd0u, 0xd0u, 0xd7u, 0xd7u, /* podoboo */
    0xbfu, 0xbeu, 0xc1u, 0xc0u, 0xc2u, 0xfcu, /* bowser front 1 */
    0xc4u, 0xc3u, 0xc6u, 0xc5u, 0xc8u, 0xc7u, /* bowser rear 1 */
    0xbfu, 0xbeu, 0xcau, 0xc9u, 0xc2u, 0xfcu, /* bowser front 2 */
    0xc4u, 0xc3u, 0xc6u, 0xc5u, 0xccu, 0xcbu, /* bowser rear 2 */
    0xfcu, 0xfcu, 0xe8u, 0xe7u, 0xeau, 0xe9u, /* bullet bill */
    0xf2u, 0xf2u, 0xf3u, 0xf3u, 0xf2u, 0xf2u, /* jumpspring 1 */
    0xf1u, 0xf1u, 0xf1u, 0xf1u, 0xfcu, 0xfcu, /* jumpspring 2 */
    0xf0u, 0xf0u, 0xfcu, 0xfcu, 0xfcu, 0xfcu, /* jumpspring 3 */
};
/* smb.asm: EnemyGfxTableOffsets */
static const uint8_t SMBC_ENEMY_GFX_TABLE_OFFSETS[27] = {
    0x0cu, 0x0cu, 0x00u, 0x0cu, 0x0cu, 0xa8u, 0x54u, 0x3cu,
    0xeau, 0x18u, 0x48u, 0x48u, 0xccu, 0xc0u, 0x18u, 0x18u,
    0x18u, 0x90u, 0x24u, 0xffu, 0x48u, 0x9cu, 0xd2u, 0xd8u,
    0xf0u, 0xf6u, 0xfcu};
/* smb.asm: EnemyAttributeData */
static const uint8_t SMBC_ENEMY_ATTRIBUTE_DATA[27] = {
    0x01u, 0x02u, 0x03u, 0x02u, 0x01u, 0x01u, 0x03u, 0x03u,
    0x03u, 0x01u, 0x01u, 0x02u, 0x02u, 0x21u, 0x01u, 0x02u,
    0x01u, 0x01u, 0x02u, 0xffu, 0x02u, 0x02u, 0x01u, 0x01u,
    0x02u, 0x02u, 0x02u};
/* smb.asm: EnemyAnimTimingBMask */
static const uint8_t SMBC_ENEMY_ANIM_TIMING_B_MASK[2] = {0x08u, 0x18u};
/* smb.asm: JumpspringFrameOffsets */
static const uint8_t SMBC_JUMPSPRING_FRAME_OFFSETS[5] = {
    0x18u, 0x19u, 0x1au, 0x19u, 0x18u};

/* smb.asm: DumpTwoSpr through DumpSixSpr — write A into the
 * sprite data at the row offsets */
static void smbc_dump_six_spr(CoreState *st, uint8_t y, uint8_t a) {
  st->ram[SMBC_SPRITE_DATA + 20u + y] = a;
  st->ram[SMBC_SPRITE_DATA + 16u + y] = a;
  st->ram[SMBC_SPRITE_DATA + 12u + y] = a;
  st->ram[SMBC_SPRITE_DATA + 8u + y] = a;
  st->ram[SMBC_SPRITE_DATA + 4u + y] = a;
  st->ram[SMBC_SPRITE_DATA + y] = a;
}

/* smb.asm: MoveColOffscreen — park a column's rows at $f8 */
static void smbc_move_col_offscreen(CoreState *st, uint8_t y) {
  st->ram[SMBC_SPRITE_Y_POSITION + y] = 0xf8u;
  st->ram[SMBC_SPRITE_Y_POSITION + 8u + y] = 0xf8u;
}

/* smb.asm: MoveESprRowOffscreen — park a row pair at $f8 */
static void smbc_move_e_spr_row_offscreen(CoreState *st, uint8_t x, uint8_t a) {
  uint8_t y = (uint8_t)(a + st->ram[SMBC_ENEMY_SPR_DATA_OFFSET + x]);
  st->ram[SMBC_SPRITE_DATA + 4u + y] = 0xf8u;
  st->ram[SMBC_SPRITE_DATA + y] = 0xf8u;
}

/* smb.asm: MoveESprColOffscreen — park a column at $f8 */
static void smbc_move_e_spr_col_offscreen(CoreState *st, uint8_t x, uint8_t a) {
  uint8_t y = (uint8_t)(a + st->ram[SMBC_ENEMY_SPR_DATA_OFFSET + x]);
  smbc_move_col_offscreen(st, y);
  st->ram[SMBC_SPRITE_DATA + 16u + y] = 0xf8u;
}

/* smb.asm: DrawSpriteObject — one row of two sprites: the tile
 * pair optionally swapped for a horizontal flip (d1 of $03),
 * the attributes, the shared vertical and staggered horizontal
 * coordinates, then $02 and Y advance to the next row */
static uint8_t smbc_draw_sprite_object(CoreState *st, uint8_t y, uint8_t *t,
                                       uint8_t tile0, uint8_t tile1) {
  if ((t[3] & 0x02u) != 0x00u) {
    st->ram[SMBC_SPRITE_TILENUMBER + 4u + y] = tile0;
    st->ram[SMBC_SPRITE_TILENUMBER + y] = tile1;
    st->ram[SMBC_SPRITE_ATTRIBUTES + y] = (uint8_t)(0x40u | t[4]);
    st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] = (uint8_t)(0x40u | t[4]);
  } else {
    st->ram[SMBC_SPRITE_TILENUMBER + y] = tile0;
    st->ram[SMBC_SPRITE_TILENUMBER + 4u + y] = tile1;
    st->ram[SMBC_SPRITE_ATTRIBUTES + y] = t[4];
    st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] = t[4];
  }
  st->ram[SMBC_SPRITE_Y_POSITION + y] = t[2];
  st->ram[SMBC_SPRITE_Y_POSITION + 4u + y] = t[2];
  st->ram[SMBC_SPRITE_X_POSITION + y] = t[5];
  st->ram[SMBC_SPRITE_X_POSITION + 4u + y] = (uint8_t)(t[5] + 0x08u);
  t[2] = (uint8_t)(t[2] + 0x08u);
  return (uint8_t)(y + 0x08u);
}

/* smb.asm: DrawEnemyObjRow — one graphics-table row into the
 * sprite buffer */
static uint8_t smbc_draw_enemy_obj_row(CoreState *st, uint8_t y, uint8_t *t,
                                       uint8_t x_ofs) {
  return smbc_draw_sprite_object(st, y, t, SMBC_ENEMY_GRAPHICS_TABLE[x_ofs],
                                 SMBC_ENEMY_GRAPHICS_TABLE[x_ofs + 1u]);
}

void enemy_gfx_handler(CoreState *st, uint8_t x) {
  /* smb.asm scratch: $02 rel Y, $03 moving dir, $04 attributes,
   * $05 rel X, $eb sprite data offset, $ec alt state, $ed saved
   * state, $ef saved object value */
  uint8_t t[6] = {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u};
  t[2] = st->ram[SMBC_ENEMY_Y_POSITION + x];
  t[5] = st->ram[SMBC_ENEMY_REL_XPOS];
  uint8_t spr_ofs = st->ram[SMBC_ENEMY_SPR_DATA_OFFSET + x];
  st->ram[SMBC_VERTICAL_FLIP_FLAG] = 0x00u;
  t[3] = st->ram[SMBC_ENEMY_MOVING_DIR + x];
  t[4] = st->ram[SMBC_ENEMY_SPR_ATTRIB + x];
  uint8_t a = st->ram[SMBC_ENEMY_ID + x];
  if (a == SMBC_PIRANHA_PLANT &&
      (int8_t)st->ram[SMBC_PIRANHA_PLANT_Y_SPEED + x] >= 0 &&
      st->ram[SMBC_ENEMY_FRAME_TIMER + x] != 0x00u) {
    return;
  }
  uint8_t saved_state = st->ram[SMBC_ENEMY_STATE + x];
  uint8_t ec = (uint8_t)(saved_state & 0x1fu);
  if (a == SMBC_RETAINER_OBJECT) {
    ec = 0x00u;
    t[3] = 0x01u;
    a = 0x15u;
  }
  if (a == SMBC_BULLET_BILL_CANNON) {
    t[2]--;
    a = 0x03u;
    if (st->ram[SMBC_ENEMY_FRAME_TIMER + x] != 0x00u) a |= 0x20u;
    t[4] = a;
    ec = 0x00u;
    saved_state = 0x00u;
    a = 0x08u;
  }
  if (a == SMBC_JUMPSPRING_OBJECT) {
    ec = 0x03u;
    a = SMBC_JUMPSPRING_FRAME_OFFSETS[st->ram[SMBC_JUMPSPRING_ANIM_CTRL]];
  }
  uint8_t ef = a;
  if (ef == SMBC_PODOBOO &&
      (int8_t)st->ram[SMBC_ENEMY_Y_SPEED + x] >= 0) {
    st->ram[SMBC_VERTICAL_FLIP_FLAG]++;
  }
  if (st->ram[SMBC_BOWSER_GFX_FLAG] != 0x00u) {
    ef = st->ram[SMBC_BOWSER_GFX_FLAG] == 0x01u ? 0x16u : 0x17u;
  }
  if (ef == SMBC_GOOMBA) {
    uint8_t state = st->ram[SMBC_ENEMY_STATE + x];
    if (state >= 0x02u) ec = 0x04u;
    if ((state & 0x20u) == 0x00u && st->ram[SMBC_TIMER_CONTROL] == 0x00u &&
        (st->ram[SMBC_FRAME_COUNTER] & 0x08u) == 0x00u) {
      t[3] ^= 0x03u;
    }
  }
  /* smb.asm: CheckBowserFront */
  t[4] |= SMBC_ENEMY_ATTRIBUTE_DATA[ef];
  uint8_t gfx_ofs = SMBC_ENEMY_GFX_TABLE_OFFSETS[ef];
  if (st->ram[SMBC_BOWSER_GFX_FLAG] != 0x00u) {
    if (st->ram[SMBC_BOWSER_GFX_FLAG] == 0x01u) {
      if ((st->ram[SMBC_BOWSER_BODY_CONTROLS] & 0x80u) != 0x00u) gfx_ofs = 0xdeu;
      if ((saved_state & 0x20u) != 0x00u) {
        /* smb.asm: FlipBowserOver */
        st->ram[SMBC_VERTICAL_FLIP_FLAG] = gfx_ofs;
      }
    } else {
      if ((st->ram[SMBC_BOWSER_BODY_CONTROLS] & 0x01u) != 0x00u) gfx_ofs = 0xe4u;
      if ((saved_state & 0x20u) != 0x00u) {
        t[2] = (uint8_t)(t[2] - 0x10u);
        st->ram[SMBC_VERTICAL_FLIP_FLAG] = gfx_ofs;
      }
    }
  } else if (gfx_ofs == 0x24u) {
    /* smb.asm: CheckForSpiny — the egg frame */
    if (ec == 0x05u) {
      gfx_ofs = 0x30u;
      t[3] = 0x02u;
      ec = 0x05u;
    }
    goto hammer_bro_chk;
  } else if (gfx_ofs == 0x90u) {
    /* smb.asm: CheckForLakitu — the throwing frame */
    if ((saved_state & 0x20u) == 0x00u &&
        st->ram[SMBC_FRENZY_ENEMY_TIMER] < 0x10u) {
      gfx_ofs = 0x96u;
    }
    goto defeated_chk;
  } else {
    /* smb.asm: CheckUpsideDownShell */
    if (ef < 0x04u && ec >= 0x02u) {
      gfx_ofs = 0x5au;
      if (ef == SMBC_BUZZY_BEETLE) {
        gfx_ofs = 0x7eu;
        t[2]++;
      }
    }
    /* smb.asm: CheckRightSideUpShell */
    if (ec == 0x04u) {
      gfx_ofs = 0x72u; /* right-side-up buzzy beetle shell */
      t[2]++;
      if (ef != SMBC_BUZZY_BEETLE) {
        gfx_ofs = 0x66u; /* right-side-up koopa shell */
        t[2]++;
      }
    }
    /* smb.asm: CheckForDefdGoomba — only reached through the
     * shell branches above, i.e. when $ec == $04 */
    if (ec == 0x04u && ef == SMBC_GOOMBA &&
        (saved_state & 0x20u) == 0x00u) {
      gfx_ofs = 0x8au;
      t[2]--;
    }
  }
hammer_bro_chk:;
  /* smb.asm: CheckForHammerBro / CheckForBloober — find whether
   * the animation-timing branch runs */
  uint8_t animate = 0x00u;
  if (ef == SMBC_HAMMER_BRO) {
    if (saved_state == 0x00u) {
      animate = 0x01u;
    } else if ((saved_state & 0x08u) != 0x00u) {
      gfx_ofs = 0xb4u;
      animate = 0x01u;
    }
  } else if (gfx_ofs == 0x48u) {
    animate = 0x01u; /* cheep-cheep animates freely */
  } else {
    uint8_t timer = st->ram[SMBC_ENEMY_INTERVAL_TIMER + x];
    if (timer < 0x05u) {
      if (gfx_ofs != 0x3cu) {
        animate = 0x01u;
      } else if (timer != 0x01u) {
        /* smb.asm: the bloober bobs three pixels */
        t[2] = (uint8_t)(t[2] + 0x03u);
        goto animation_stop;
      }
    }
  }
  if (animate != 0x00u) {
    /* smb.asm: CheckToAnimateEnemy */
    if (ef == SMBC_GOOMBA || ef == 0x08u || ef == SMBC_PODOBOO || ef >= 0x18u) {
      goto defeated_chk;
    }
    uint8_t mask_idx = 0x00u;
    if (ef == 0x15u) {
      mask_idx = 0x01u; /* residual iny */
      if (st->ram[SMBC_WORLD_NUMBER] >= 0x08u) goto defeated_chk;
      gfx_ofs = 0xa2u;
      ec = 0x03u;
      goto defeated_chk;
    }
    if ((st->ram[SMBC_FRAME_COUNTER] &
         SMBC_ENEMY_ANIM_TIMING_B_MASK[mask_idx]) != 0x00u) {
      goto defeated_chk;
    }
    goto animation_stop;
  }
  goto defeated_chk;
animation_stop:;
  /* smb.asm: CheckAnimationStop — the second frame */
  if ((saved_state & 0xa0u) == 0x00u && st->ram[SMBC_TIMER_CONTROL] == 0x00u) {
    gfx_ofs = (uint8_t)(gfx_ofs + 0x06u);
  }
defeated_chk:;
  /* smb.asm: CheckDefeatedState */
  if ((saved_state & 0x20u) != 0x00u && ef >= 0x04u) {
    st->ram[SMBC_VERTICAL_FLIP_FLAG] = 0x01u;
    ec = 0x00u;
  }
  /* smb.asm: DrawEnemyObject — three rows of two sprites */
  uint8_t y = spr_ofs;
  y = smbc_draw_enemy_obj_row(st, y, t, gfx_ofs);
  y = smbc_draw_enemy_obj_row(st, y, t, (uint8_t)(gfx_ofs + 0x02u));
  y = smbc_draw_enemy_obj_row(st, y, t, (uint8_t)(gfx_ofs + 0x04u));
  if (ef == 0x08u) goto offscr_chk;
  y = spr_ofs;
  if (st->ram[SMBC_VERTICAL_FLIP_FLAG] != 0x00u) {
    /* smb.asm: CheckForVerticalFlip */
    st->ram[SMBC_SPRITE_ATTRIBUTES + y] |= 0x80u;
    smbc_dump_six_spr(st, (uint8_t)(y + 0x02u),
                      st->ram[SMBC_SPRITE_ATTRIBUTES + y]);
    uint8_t row = y;
    if (ef != SMBC_HAMMER_BRO && ef != SMBC_LAKITU && ef < 0x15u) {
      row = (uint8_t)(y + 0x08u);
    }
    /* smb.asm: FlipEnemyVertically — swap the row pair with the
     * third row's tiles */
    uint8_t swap0 = st->ram[SMBC_SPRITE_TILENUMBER + row];
    uint8_t swap1 = st->ram[SMBC_SPRITE_TILENUMBER + 4u + row];
    st->ram[SMBC_SPRITE_TILENUMBER + row] =
        st->ram[SMBC_SPRITE_TILENUMBER + 16u + y];
    st->ram[SMBC_SPRITE_TILENUMBER + 4u + row] =
        st->ram[SMBC_SPRITE_TILENUMBER + 20u + y];
    st->ram[SMBC_SPRITE_TILENUMBER + 20u + y] = swap1;
    st->ram[SMBC_SPRITE_TILENUMBER + 16u + y] = swap0;
  }
  /* smb.asm: CheckForESymmetry */
  if (st->ram[SMBC_BOWSER_GFX_FLAG] != 0x00u) goto offscr_chk;
  if (ef == 0x05u) goto offscr_chk;
  uint8_t mirror = 0x00u;
  if (ef == SMBC_BLOOBER || ef == SMBC_PIRANHA_PLANT || ef == SMBC_PODOBOO) {
    mirror = 0x01u;
  } else if (ef == SMBC_SPINY) {
    if (ec == 0x05u) mirror = 0x01u;
  } else {
    if (ef == 0x15u) {
      st->ram[SMBC_SPRITE_ATTRIBUTES + 20u + y] = 0x42u;
    }
    if (ec >= 0x02u) mirror = 0x01u;
  }
  if (mirror != 0x00u && st->ram[SMBC_BOWSER_GFX_FLAG] == 0x00u) {
    /* smb.asm: MirrorEnemyGfx */
    uint8_t attr = (uint8_t)(st->ram[SMBC_SPRITE_ATTRIBUTES + y] & 0xa3u);
    st->ram[SMBC_SPRITE_ATTRIBUTES + y] = attr;
    st->ram[SMBC_SPRITE_ATTRIBUTES + 8u + y] = attr;
    st->ram[SMBC_SPRITE_ATTRIBUTES + 16u + y] = attr;
    attr |= 0x40u;
    if (ec == 0x05u) attr |= 0x80u;
    st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] = attr;
    st->ram[SMBC_SPRITE_ATTRIBUTES + 12u + y] = attr;
    st->ram[SMBC_SPRITE_ATTRIBUTES + 20u + y] = attr;
    if (ec == 0x04u) {
      attr = (uint8_t)(st->ram[SMBC_SPRITE_ATTRIBUTES + 8u + y] | 0x80u);
      st->ram[SMBC_SPRITE_ATTRIBUTES + 8u + y] = attr;
      st->ram[SMBC_SPRITE_ATTRIBUTES + 16u + y] = attr;
      attr |= 0x40u;
      st->ram[SMBC_SPRITE_ATTRIBUTES + 12u + y] = attr;
      st->ram[SMBC_SPRITE_ATTRIBUTES + 20u + y] = attr;
    }
  }
  /* smb.asm: CheckToMirrorLakitu */
  if (ef == SMBC_LAKITU) {
    if (st->ram[SMBC_VERTICAL_FLIP_FLAG] != 0x00u) {
      st->ram[SMBC_SPRITE_ATTRIBUTES + y] &= 0x81u;
      st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] |= 0x41u;
    } else {
      st->ram[SMBC_SPRITE_ATTRIBUTES + 16u + y] &= 0x81u;
      st->ram[SMBC_SPRITE_ATTRIBUTES + 20u + y] |= 0x41u;
      if (st->ram[SMBC_FRENZY_ENEMY_TIMER] < 0x10u) {
        uint8_t attr = st->ram[SMBC_SPRITE_ATTRIBUTES + 20u + y];
        st->ram[SMBC_SPRITE_ATTRIBUTES + 12u + y] = attr;
        st->ram[SMBC_SPRITE_ATTRIBUTES + 8u + y] =
            (uint8_t)(attr & 0x81u);
        goto offscr_chk;
      }
    }
  }
  /* smb.asm: CheckToMirrorJSpring */
  if (ef >= 0x18u) {
    st->ram[SMBC_SPRITE_ATTRIBUTES + 8u + y] = 0x82u;
    st->ram[SMBC_SPRITE_ATTRIBUTES + 16u + y] = 0x82u;
    st->ram[SMBC_SPRITE_ATTRIBUTES + 12u + y] = 0xc2u;
    st->ram[SMBC_SPRITE_ATTRIBUTES + 20u + y] = 0xc2u;
  }
offscr_chk:;
  /* smb.asm: SprObjectOffscrChk — park columns and rows at $f8
   * per the shared offscreen bits */
  uint8_t bits = st->ram[SMBC_ENEMY_OFFSCREEN_BITS];
  if ((bits & 0x04u) != 0x00u) smbc_move_e_spr_col_offscreen(st, x, 0x04u);
  if ((bits & 0x08u) != 0x00u) smbc_move_e_spr_col_offscreen(st, x, 0x00u);
  if ((bits & 0x20u) != 0x00u) smbc_move_e_spr_row_offscreen(st, x, 0x10u);
  if ((bits & 0x40u) != 0x00u) smbc_move_e_spr_row_offscreen(st, x, 0x08u);
  if ((bits & 0x80u) != 0x00u) {
    smbc_move_e_spr_row_offscreen(st, x, 0x00u);
    if (st->ram[SMBC_ENEMY_ID + x] != SMBC_PODOBOO &&
        st->ram[SMBC_ENEMY_Y_HIGH_POS + x] == 0x02u) {
      erase_enemy_object(st, x);
    }
  }
}

/* smb.asm: PowerUpGfxTable — mushroom, fire flower, star, 1-up */
static const uint8_t SMBC_POWER_UP_GFX_TABLE[16] = {
    0x76u, 0x77u, 0x78u, 0x79u, 0xd6u, 0xd6u, 0xd9u, 0xd9u,
    0x8du, 0x8du, 0xe4u, 0xe4u, 0x76u, 0x77u, 0x78u, 0x79u};

/* smb.asm: PowerUpAttributes */
static const uint8_t SMBC_POWER_UP_ATTRIBUTES[4] = {
    0x02u, 0x01u, 0x02u, 0x01u};

/* smb.asm: DrawPowerUp — two rows from the type's tile quad; the
 * fire flower and star cycle palettes every two frames and flip
 * their right side, then the shared offscreen check parks any
 * exposed columns and rows */
void draw_power_up(CoreState *st, uint8_t x) {
  uint8_t t[6] = {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u};
  uint8_t y = st->ram[SMBC_ENEMY_SPR_DATA_OFFSET + 5u];
  t[2] = (uint8_t)(st->ram[SMBC_ENEMY_REL_YPOS] + 0x08u);
  t[5] = st->ram[SMBC_ENEMY_REL_XPOS];
  uint8_t type = st->ram[SMBC_POWER_UP_TYPE];
  t[4] = (uint8_t)(SMBC_POWER_UP_ATTRIBUTES[type] |
                   st->ram[SMBC_ENEMY_SPR_ATTRIB + 5u]);
  t[3] = 0x01u; /* smb.asm: init d1 of flip control */
  uint8_t ofs = (uint8_t)(type << 2u);
  y = smbc_draw_sprite_object(st, y, t, SMBC_POWER_UP_GFX_TABLE[ofs],
                              SMBC_POWER_UP_GFX_TABLE[ofs + 1u]);
  y = smbc_draw_sprite_object(st, y, t, SMBC_POWER_UP_GFX_TABLE[ofs + 2u],
                              SMBC_POWER_UP_GFX_TABLE[ofs + 3u]);
  y = st->ram[SMBC_ENEMY_SPR_DATA_OFFSET + 5u];
  if (type != 0x00u && type != 0x03u) {
    /* smb.asm: the flower and star flash through the palettes */
    uint8_t pal = (uint8_t)(((st->ram[SMBC_FRAME_COUNTER] >> 1u) & 0x03u) |
                            st->ram[SMBC_ENEMY_SPR_ATTRIB + 5u]);
    st->ram[SMBC_SPRITE_ATTRIBUTES + y] = pal;
    st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] = pal;
    if (type != 0x01u) { /* the star flashes the bottom row too */
      st->ram[SMBC_SPRITE_ATTRIBUTES + 8u + y] = pal;
      st->ram[SMBC_SPRITE_ATTRIBUTES + 12u + y] = pal;
    }
    st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] |= 0x40u;
    st->ram[SMBC_SPRITE_ATTRIBUTES + 12u + y] |= 0x40u;
  }
  /* smb.asm: SprObjectOffscrChk */
  uint8_t bits = st->ram[SMBC_ENEMY_OFFSCREEN_BITS];
  if ((bits & 0x04u) != 0x00u) smbc_move_e_spr_col_offscreen(st, x, 0x04u);
  if ((bits & 0x08u) != 0x00u) smbc_move_e_spr_col_offscreen(st, x, 0x00u);
  if ((bits & 0x20u) != 0x00u) smbc_move_e_spr_row_offscreen(st, x, 0x10u);
  if ((bits & 0x40u) != 0x00u) smbc_move_e_spr_row_offscreen(st, x, 0x08u);
  if ((bits & 0x80u) != 0x00u) {
    smbc_move_e_spr_row_offscreen(st, x, 0x00u);
    if (st->ram[SMBC_ENEMY_ID + x] != SMBC_PODOBOO &&
        st->ram[SMBC_ENEMY_Y_HIGH_POS + x] == 0x02u) {
      erase_enemy_object(st, x);
    }
  }
}
