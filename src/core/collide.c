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

void enemy_turn_around(CoreState *st, uint8_t x) {
  uint8_t id = st->ram[SMBC_ENEMY_ID + x];
  if (id == SMBC_PIRANHA_PLANT || id == SMBC_LAKITU ||
      id == SMBC_HAMMER_BRO) {
    return;
  }
  if (id != SMBC_SPINY && id != SMBC_GREEN_PARATROOPA_JUMP && id >= 0x07u) {
    return;
  }
  st->ram[SMBC_ENEMY_X_SPEED + x] =
      (uint8_t)(~st->ram[SMBC_ENEMY_X_SPEED + x] + 1u);
  st->ram[SMBC_ENEMY_MOVING_DIR + x] ^= 0x03u;
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
