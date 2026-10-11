/* core/runners.c — see core/runners.h for the contract. */
#include "runners.h"

#include "../data/objects.h"
#include "enemies.h"
#include "enemies.h"
#include "collide.h"
#include "move.h"
#include "offscreen.h"
#include "../data/enemies.h"

/* smb.asm's enemy object constants used here */
#define SMBC_PIRANHA_PLANT 0x0du        /* smb.asm: PiranhaPlant */
#define SMBC_HAMMER_BRO 0x05u           /* smb.asm: HammerBro */
#define SMBC_FLYING_CHEEP_CHEEP 0x14u   /* smb.asm: FlyingCheepCheep */
#define SMBC_FLAGPOLE_FLAG_OBJECT 0x30u /* smb.asm: FlagpoleFlagObject */
#define SMBC_STAR_FLAG_OBJECT 0x31u     /* smb.asm: StarFlagObject */
#define SMBC_JUMPSPRING_OBJECT 0x32u    /* smb.asm: JumpspringObject */

#define SMBC_GOOMBA 0x06u          /* smb.asm: Goomba */
#define SMBC_POWER_UP_OBJECT 0x2eu /* smb.asm: PowerUpObject */

/* smb.asm: SteadM — move horizontally with a temporary speed adder
 * (the slow flag selects $e8/$18, a negative speed the second pair),
 * restoring the original speed afterwards */
static void steady_move(CoreState *st, uint8_t x, uint8_t slow) {
  uint8_t y = slow;
  uint8_t speed = st->ram[SMBC_ENEMY_X_SPEED + x];
  if ((int8_t)speed < 0) y = (uint8_t)(y + 2u);
  st->ram[SMBC_ENEMY_X_SPEED + x] =
      (uint8_t)(speed + SMBC_X_SPEED_ADDER_DATA[y]);
  move_enemy_horizontally(st, x);
  st->ram[SMBC_ENEMY_X_SPEED + x] = speed;
}

/* smb.asm: ChkKillGoomba — a stunned goomba whose interval timer has
 * counted down to $0e is erased rather than revived */
static void chk_kill_goomba(CoreState *st, uint8_t x, uint8_t timer) {
  if (timer != 0x0eu) return;
  if (st->ram[SMBC_ENEMY_ID + x] != SMBC_GOOMBA) return;
  erase_enemy_object(st, x);
}

/* smb.asm: ReviveStunned — the interval timer expiry revives a
 * stunned enemy with a pseudorandom direction (hard mode uses the
 * faster speeds); unexpired timers only check the goomba kill */
static void revive_stunned(CoreState *st, uint8_t x) {
  uint8_t timer = st->ram[SMBC_ENEMY_INTERVAL_TIMER + x];
  if (timer != 0x00u) {
    chk_kill_goomba(st, x, timer);
    return;
  }
  st->ram[SMBC_ENEMY_STATE + x] = 0x00u;
  uint8_t y = (uint8_t)(st->ram[SMBC_FRAME_COUNTER] & 0x01u);
  st->ram[SMBC_ENEMY_MOVING_DIR + x] = (uint8_t)(y + 1u);
  if (st->ram[SMBC_PRIMARY_HARD_MODE] != 0x00u) y = (uint8_t)(y + 2u);
  st->ram[SMBC_ENEMY_X_SPEED + x] = SMBC_REVIVED_X_SPEED[y];
}

void move_normal_enemy(CoreState *st, uint8_t x) {
  uint8_t state = st->ram[SMBC_ENEMY_STATE + x];
  if ((state & 0x40u) == 0x00u) {
    if ((state & 0x80u) != 0x00u) {
      steady_move(st, x, 0x00u);
      return;
    }
    if ((state & 0x20u) != 0x00u) {
      /* smb.asm: MoveDefeatedEnemy */
      move_d_enemy_vertically(st, x);
      move_enemy_horizontally(st, x);
      return;
    }
    uint8_t low = (uint8_t)(state & 0x07u);
    if (low == 0x00u) {
      steady_move(st, x, 0x00u);
      return;
    }
    if (low != 0x05u && low >= 0x03u) {
      revive_stunned(st, x);
      return;
    }
    /* states $01, $02 and $05 fall through to FallE in the asm */
  }
  /* smb.asm: FallE — gravity first, then horizontal movement for
   * state $02, slowed horizontal movement for d6 objects other than
   * power-ups */
  move_d_enemy_vertically(st, x);
  state = st->ram[SMBC_ENEMY_STATE + x];
  if (state == 0x02u) {
    move_enemy_horizontally(st, x);
    return;
  }
  if ((state & 0x40u) == 0x00u) {
    steady_move(st, x, 0x00u);
    return;
  }
  if (st->ram[SMBC_ENEMY_ID + x] == SMBC_POWER_UP_OBJECT) {
    steady_move(st, x, 0x00u);
    return;
  }
  steady_move(st, x, 0x01u); /* smb.asm: SlowM */
}

void move_jumping_enemy(CoreState *st, uint8_t x) {
  move_j_enemy_vertically(st, x);
  move_enemy_horizontally(st, x);
}

void proc_move_red_p_troopa(CoreState *st, uint8_t x) {
  uint8_t any = (uint8_t)(st->ram[SMBC_ENEMY_Y_SPEED + x] |
                          st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x]);
  if (any == 0x00u) {
    st->ram[SMBC_ENEMY_YMF_DUMMY + x] = 0x00u;
    if (st->ram[SMBC_ENEMY_Y_POSITION + x] <
        st->ram[SMBC_RED_PTROOPA_ORIG_X_POS + x]) {
      if ((st->ram[SMBC_FRAME_COUNTER] & 0x07u) == 0x00u) {
        st->ram[SMBC_ENEMY_Y_POSITION + x]++;
      }
      return;
    }
  }
  /* smb.asm: MoveRedPTUpOrDown — towards the central coordinate */
  if (st->ram[SMBC_ENEMY_Y_POSITION + x] <
      st->ram[SMBC_RED_PTROOPA_CENTER_Y_POS + x]) {
    move_red_p_troopa(st, x, 0x00u); /* down */
  } else {
    move_red_p_troopa(st, x, 0x01u); /* up */
  }
}

/* smb.asm: MoveDefeatedEnemy — defeated enemies fall, then move
 * horizontally */
static void move_defeated_enemy(CoreState *st, uint8_t x) {
  move_d_enemy_vertically(st, x);
  move_enemy_horizontally(st, x);
}

/* smb.asm: XMoveCntr_Platform — every fourth frame, the secondary
 * counter (the enemy X speed, shared at $58) ramps towards the
 * maximum or back down to zero by the primary counter's d0; hitting
 * a limit advances the primary counter instead. XMoveCntr_GreenPTroopa
 * is this routine with a $13 maximum */
static void x_move_cntr(CoreState *st, uint8_t x, uint8_t max) {
  if ((st->ram[SMBC_FRAME_COUNTER] & 0x03u) != 0x00u) return;
  uint8_t secondary = st->ram[SMBC_ENEMY_X_SPEED + x];
  if ((st->ram[SMBC_X_MOVE_PRIMARY_COUNTER + x] & 0x01u) != 0x00u) {
    if (secondary == 0x00u) {
      st->ram[SMBC_X_MOVE_PRIMARY_COUNTER + x]++;
    } else {
      st->ram[SMBC_ENEMY_X_SPEED + x]--;
    }
  } else {
    if (secondary == max) {
      st->ram[SMBC_X_MOVE_PRIMARY_COUNTER + x]++;
    } else {
      st->ram[SMBC_ENEMY_X_SPEED + x]++;
    }
  }
}

/* smb.asm: MoveWithXMCntrs — the secondary counter is negated for
 * leftward movement by the primary counter's d1, the moving
 * direction is stored, and the counter is restored after the move */
static void move_with_xm_cntrs(CoreState *st, uint8_t x) {
  uint8_t saved = st->ram[SMBC_ENEMY_X_SPEED + x];
  uint8_t dir = 0x01u;
  if ((st->ram[SMBC_X_MOVE_PRIMARY_COUNTER + x] & 0x02u) == 0x00u) {
    st->ram[SMBC_ENEMY_X_SPEED + x] = (uint8_t)(~saved + 1u);
    dir = 0x02u;
  }
  st->ram[SMBC_ENEMY_MOVING_DIR + x] = dir;
  move_enemy_horizontally(st, x);
  st->ram[SMBC_ENEMY_X_SPEED + x] = saved;
}

void move_fly_green_p_troopa(CoreState *st, uint8_t x) {
  x_move_cntr(st, x, 0x13u);
  move_with_xm_cntrs(st, x);
  uint8_t adder = 0x01u; /* sway down */
  if ((st->ram[SMBC_FRAME_COUNTER] & 0x03u) != 0x00u) return;
  if ((st->ram[SMBC_FRAME_COUNTER] & 0x40u) == 0x00u) {
    adder = 0xffu; /* sway up */
  }
  st->ram[SMBC_ENEMY_Y_POSITION + x] += adder;
}

/* smb.asm: OffscreenBoundsCheck. The asm's carry chain is emulated
 * exactly: the cpy comparisons feed the adc/sbc pairs, so hammer bros
 * and piranha plants effectively add $39 before subtracting $48 (or
 * $10f when the add overflows a page), other objects subtract $48 or
 * $49 by identifier, and the right edge adds $49 whenever the left
 * page subtraction did not borrow */
void offscreen_bounds_check(CoreState *st, uint8_t x) {
  uint8_t id = st->ram[SMBC_ENEMY_ID + x];
  if (id == SMBC_FLYING_CHEEP_CHEEP) return;
  int a = st->ram[SMBC_SCREEN_LEFT_X_POS];
  int carry;
  if (id == SMBC_HAMMER_BRO || id == SMBC_PIRANHA_PLANT) {
    a += 0x38 + 1; /* adc #$38, carry set by the cpy */
    carry = a > 0xff;
    a &= 0xff;
  } else {
    carry = id >= SMBC_PIRANHA_PLANT; /* cpy #PiranhaPlant */
  }
  a -= 0x48 + (1 - carry); /* sbc #$48 */
  carry = a >= 0;
  uint8_t bound_x = (uint8_t)a;
  int page = st->ram[SMBC_SCREEN_LEFT_PAGE_LOC] - (1 - carry); /* sbc #$00 */
  carry = page >= 0;
  page &= 0xff;
  int r = st->ram[SMBC_SCREEN_RIGHT_X_POS] + 0x48 + carry; /* adc #$48 */
  uint8_t right_x = (uint8_t)r;
  int rpage = (st->ram[SMBC_SCREEN_RIGHT_PAGE_LOC] + (r > 0xff)) & 0xff;
  /* the compares: cmp on the low byte, sbc on the page, then bmi on
   * the result's d7 — emulated as byte operations, not a true
   * sixteen-bit compare */
  int obj_page = st->ram[SMBC_ENEMY_PAGE_LOC + x];
  int obj_x = st->ram[SMBC_ENEMY_X_POSITION + x];
  carry = obj_x >= bound_x; /* cmp $01 */
  if ((((obj_page - page - (1 - carry)) & 0xff) & 0x80) != 0) {
    erase_enemy_object(st, x); /* TooFar */
    return;
  }
  carry = obj_x >= right_x; /* cmp $03 */
  if ((((obj_page - rpage - (1 - carry)) & 0xff) & 0x80) != 0) {
    return; /* onscreen */
  }
  uint8_t state = st->ram[SMBC_ENEMY_STATE + x];
  if (state == SMBC_HAMMER_BRO) return; /* the spiny's egg state */
  if (id == SMBC_PIRANHA_PLANT || id == SMBC_FLAGPOLE_FLAG_OBJECT ||
      id == SMBC_STAR_FLAG_OBJECT || id == SMBC_JUMPSPRING_OBJECT) {
    return;
  }
  erase_enemy_object(st, x);
}

/* smb.asm: Floatdown — sink one pixel every other frame */
static void floatdown(CoreState *st, uint8_t x) {
  if ((st->ram[SMBC_FRAME_COUNTER] & 0x01u) != 0x00u) return;
  st->ram[SMBC_ENEMY_Y_POSITION + x]++;
}

/* smb.asm: ProcSwimmingB — the swim cycle. While d1 of the movement
 * counter is clear, the movement force (mirrored into the move
 * speed) counts up to two on every eighth frame, then back down;
 * reaching zero sets the float-down timer. With d1 set the bloober
 * floats down until the timer expires and it has caught up with the
 * player, restarting the cycle. The asm's adc into ChkNearPlayer
 * carries whatever the caller left; the port adds $10 flat */
static void proc_swimming_b(CoreState *st, uint8_t x) {
  uint8_t counter = st->ram[SMBC_X_MOVE_PRIMARY_COUNTER + x];
  if ((counter & 0x02u) != 0x00u) {
    /* smb.asm: ChkForFloatdown */
    if (st->ram[SMBC_ENEMY_INTERVAL_TIMER + x] != 0x00u) {
      floatdown(st, x);
      return;
    }
    /* smb.asm: ChkNearPlayer */
    if ((uint8_t)(st->ram[SMBC_ENEMY_Y_POSITION + x] + 0x10u) <
        st->ram[SMBC_PLAYER_Y_POSITION]) {
      floatdown(st, x);
      return;
    }
    st->ram[SMBC_X_MOVE_PRIMARY_COUNTER + x] = 0x00u;
    return;
  }
  if ((st->ram[SMBC_FRAME_COUNTER] & 0x07u) != 0x00u) return;
  if ((counter & 0x01u) == 0x00u) {
    uint8_t f = (uint8_t)(st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] + 0x01u);
    st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] = f;
    st->ram[SMBC_ENEMY_X_SPEED + x] = f; /* BlooperMoveSpeed */
    if (f == 0x02u) st->ram[SMBC_X_MOVE_PRIMARY_COUNTER + x]++;
    return;
  }
  /* smb.asm: SlowSwim */
  uint8_t f = (uint8_t)(st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] - 0x01u);
  st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] = f;
  st->ram[SMBC_ENEMY_X_SPEED + x] = f;
  if (f != 0x00u) return;
  st->ram[SMBC_X_MOVE_PRIMARY_COUNTER + x]++;
  st->ram[SMBC_ENEMY_INTERVAL_TIMER + x] = 0x02u;
}

void move_bloober(CoreState *st, uint8_t x) {
  if ((st->ram[SMBC_ENEMY_STATE + x] & 0x20u) != 0x00u) {
    move_enemy_slow_vert(st, x); /* smb.asm: MoveDefeatedBloober */
    return;
  }
  uint8_t mask = SMBC_BLOOBER_BITMASKS[st->ram[SMBC_SECONDARY_HARD_MODE]];
  if ((st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1u + x] & mask) == 0x00u) {
    uint8_t dir;
    if ((x & 0x01u) != 0x00u) {
      dir = st->ram[SMBC_PLAYER_MOVING_DIR]; /* odd slots follow the player */
    } else {
      dir = 0x02u; /* left by default */
      if (player_enemy_diff(st, x) < 0) dir = 0x01u; /* player to the left */
    }
    st->ram[SMBC_ENEMY_MOVING_DIR + x] = dir;
  }
  /* smb.asm: BlooberSwim — the movement force bobs the bloober up
   * unless that would cross the status bar edge */
  proc_swimming_b(st, x);
  uint8_t new_y = (uint8_t)(st->ram[SMBC_ENEMY_Y_POSITION + x] -
                            st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x]);
  if (new_y >= 0x20u) st->ram[SMBC_ENEMY_Y_POSITION + x] = new_y;
  uint8_t speed = st->ram[SMBC_ENEMY_X_SPEED + x]; /* BlooperMoveSpeed */
  if (st->ram[SMBC_ENEMY_MOVING_DIR + x] == 0x01u) {
    uint16_t pos = (uint16_t)st->ram[SMBC_ENEMY_X_POSITION + x] + speed;
    st->ram[SMBC_ENEMY_X_POSITION + x] = (uint8_t)pos;
    st->ram[SMBC_ENEMY_PAGE_LOC + x] += (uint8_t)(pos >> 8);
  } else {
    int pos = (int)st->ram[SMBC_ENEMY_X_POSITION + x] - speed;
    st->ram[SMBC_ENEMY_X_POSITION + x] = (uint8_t)pos;
    st->ram[SMBC_ENEMY_PAGE_LOC + x] -= (pos < 0) ? 0x01u : 0x00u;
  }
}

void move_bullet_bill(CoreState *st, uint8_t x) {
  if ((st->ram[SMBC_ENEMY_STATE + x] & 0x20u) != 0x00u) {
    move_j_enemy_vertically(st, x);
    return;
  }
  st->ram[SMBC_ENEMY_X_SPEED + x] = 0xe8u;
  move_enemy_horizontally(st, x);
}

void move_swimming_cheep_cheep(CoreState *st, uint8_t x) {
  uint8_t masked = (uint8_t)(st->ram[SMBC_ENEMY_STATE + x] & 0x20u);
  if (masked != 0x00u) {
    move_enemy_slow_vert(st, x);
    return;
  }
  /* the asm stores the masked state as the vertical adder, which is
   * always zero on this path — the vertical drift is the carry or
   * borrow of the dummy accumulation alone */
  uint8_t state_adder = masked;
  uint8_t drain = SMBC_SWIM_CC_X_MOVE_DATA[st->ram[SMBC_ENEMY_ID + x] - 0x0au];
  int force = (int)st->ram[SMBC_ENEMY_X_MOVE_FORCE + x] - drain;
  st->ram[SMBC_ENEMY_X_MOVE_FORCE + x] = (uint8_t)force;
  int pos = (int)st->ram[SMBC_ENEMY_X_POSITION + x] - (force < 0 ? 1 : 0);
  st->ram[SMBC_ENEMY_X_POSITION + x] = (uint8_t)pos;
  st->ram[SMBC_ENEMY_PAGE_LOC + x] -= (pos < 0) ? 0x01u : 0x00u;
  if (x < 0x02u) return;
  uint8_t flag = st->ram[SMBC_CHEEP_CHEEP_MOVE_M_FLAG + x];
  if (flag < 0x10u) {
    /* smb.asm: CCSwimUpwards */
    int dummy = (int)st->ram[SMBC_ENEMY_YMF_DUMMY + x] - 0x20u;
    st->ram[SMBC_ENEMY_YMF_DUMMY + x] = (uint8_t)dummy;
    int y = (int)st->ram[SMBC_ENEMY_Y_POSITION + x] -
            ((dummy < 0 ? 1 : 0) + state_adder);
    st->ram[SMBC_ENEMY_Y_POSITION + x] = (uint8_t)y;
    st->ram[SMBC_ENEMY_Y_HIGH_POS + x] -= (y < 0) ? 0x01u : 0x00u;
  } else {
    uint16_t dummy = (uint16_t)st->ram[SMBC_ENEMY_YMF_DUMMY + x] + 0x20u;
    st->ram[SMBC_ENEMY_YMF_DUMMY + x] = (uint8_t)dummy;
    uint16_t y = (uint16_t)st->ram[SMBC_ENEMY_Y_POSITION + x] +
                 (uint8_t)(dummy >> 8) + state_adder;
    st->ram[SMBC_ENEMY_Y_POSITION + x] = (uint8_t)y;
    st->ram[SMBC_ENEMY_Y_HIGH_POS + x] += (uint8_t)(y >> 8);
  }
  /* smb.asm: ChkSwimYPos — flip the vertical direction fifteen
   * pixels away from the origin */
  uint8_t dir = 0x00u; /* upwards */
  int diff = (int)st->ram[SMBC_ENEMY_Y_POSITION + x] -
             st->ram[SMBC_CHEEP_CHEEP_ORIG_Y_POS + x];
  if (diff < 0) {
    dir = 0x10u; /* downwards */
    diff = -diff;
  }
  if (diff < 0x0f) return;
  st->ram[SMBC_CHEEP_CHEEP_MOVE_M_FLAG + x] = dir;
}

/* smb.asm: InitVStf — initialise vertical speed and movement force */
static void init_v_stf(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_Y_SPEED + x] = 0x00u;
  st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] = 0x00u;
}

void move_podoboo(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_ENEMY_INTERVAL_TIMER + x] == 0x00u) {
    /* smb.asm: InitPodoboo, through SmallBBox and InitVStf */
    st->ram[SMBC_ENEMY_Y_HIGH_POS + x] = 0x02u;
    st->ram[SMBC_ENEMY_Y_POSITION + x] = 0x02u;
    st->ram[SMBC_ENEMY_STATE + x] = 0x00u;
    st->ram[SMBC_ENEMY_BOUND_BOX_CTRL + x] = 0x09u;
    st->ram[SMBC_ENEMY_MOVING_DIR + x] = 0x02u;
    init_v_stf(st, x);
    uint8_t r = st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1u + x];
    st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] = (uint8_t)(r | 0x80u);
    st->ram[SMBC_ENEMY_INTERVAL_TIMER + x] = (uint8_t)((r & 0x0fu) | 0x06u);
    st->ram[SMBC_ENEMY_Y_SPEED + x] = 0xf9u;
  }
  move_j_enemy_vertically(st, x);
}

/* smb.asm: SpawnHammerObj — the misc slot offset comes from the
 * LSFR's d2-d0, or its d3 when those are clear; the spawn is
 * refused (carry clear) when the misc slot is taken or its enemy
 * slot is still buffered. ObjectOffset restores x in the asm; the
 * caller's slot is the same value here */
static bool spawn_hammer_obj(CoreState *st, uint8_t x) {
  uint8_t y = (uint8_t)(st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1u] & 0x07u);
  if (y == 0x00u) y = (uint8_t)(st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1u] & 0x08u);
  if (st->ram[SMBC_MISC_STATE + y] != 0x00u) return false;
  if (st->ram[SMBC_ENEMY_FLAG + SMBC_HAMMER_ENEMY_OFS_DATA[y]] != 0x00u) {
    return false;
  }
  st->ram[SMBC_HAMMER_ENEMY_OFFSET + y] = x;
  st->ram[SMBC_MISC_STATE + y] = 0x90u;
  st->ram[SMBC_MISC_BOUND_BOX_CTRL + y] = 0x07u;
  return true;
}

void proc_hammer_bro(CoreState *st, uint8_t x) {
  if ((st->ram[SMBC_ENEMY_STATE + x] & 0x20u) != 0x00u) {
    move_defeated_enemy(st, x);
    return;
  }
  if (st->ram[SMBC_HAMMER_BRO_JUMP_TIMER + x] != 0x00u) {
    st->ram[SMBC_HAMMER_BRO_JUMP_TIMER + x]--;
    if ((st->ram[SMBC_ENEMY_OFFSCREEN_BITS] & 0x0cu) == 0x00u) {
      if (st->ram[SMBC_HAMMER_THROWING_TIMER + x] != 0x00u) {
        st->ram[SMBC_HAMMER_THROWING_TIMER + x]--; /* smb.asm: DecHT */
      } else {
        st->ram[SMBC_HAMMER_THROWING_TIMER + x] =
            SMBC_HAMMER_THROW_TMR_DATA[st->ram[SMBC_SECONDARY_HARD_MODE]];
        if (spawn_hammer_obj(st, x)) {
          st->ram[SMBC_ENEMY_STATE + x] |= 0x08u;
        }
      }
    }
  } else {
    /* smb.asm: HammerBroJumpCode */
    if ((st->ram[SMBC_ENEMY_STATE + x] & 0x07u) != 0x01u) {
      uint8_t bitmask = 0x00u;
      uint8_t speed = 0xfau;
      uint8_t y_pos = st->ram[SMBC_ENEMY_Y_POSITION + x];
      if ((y_pos & 0x80u) == 0x00u) {
        speed = 0xfdu;
        bitmask = 0x01u;
        if (y_pos >= 0x70u) {
          bitmask = 0x00u;
          if ((st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1u + x] & 0x01u) == 0x00u) {
            speed = 0xfau;
          }
        }
      }
      st->ram[SMBC_ENEMY_Y_SPEED + x] = speed;
      st->ram[SMBC_ENEMY_STATE + x] |= 0x01u;
      uint8_t offset = (uint8_t)(bitmask &
                                 st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + 2u + x]);
      if (st->ram[SMBC_SECONDARY_HARD_MODE] == 0x00u) offset = 0x00u;
      st->ram[SMBC_ENEMY_FRAME_TIMER + x] = SMBC_HAMMER_BRO_JUMP_L_DATA[offset];
      st->ram[SMBC_HAMMER_BRO_JUMP_TIMER + x] =
          (uint8_t)(st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1u + x] | 0xc0u);
    }
  }
  /* smb.asm: MoveHammerBroXDir */
  uint8_t shimmy = 0xfcu; /* left */
  if ((st->ram[SMBC_FRAME_COUNTER] & 0x40u) != 0x00u) shimmy = 0x04u;
  st->ram[SMBC_ENEMY_X_SPEED + x] = shimmy;
  uint8_t dir = 0x01u; /* face right */
  if (player_enemy_diff(st, x) >= 0) {
    dir = 0x02u; /* face left */
    if (st->ram[SMBC_ENEMY_INTERVAL_TIMER + x] == 0x00u) {
      st->ram[SMBC_ENEMY_X_SPEED + x] = 0xf8u; /* walk left at the player */
    }
  }
  st->ram[SMBC_ENEMY_MOVING_DIR + x] = dir;
}

void move_piranha_plant(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_ENEMY_STATE + x] == 0x00u &&
      st->ram[SMBC_ENEMY_FRAME_TIMER + x] == 0x00u) {
    if (st->ram[SMBC_PIRANHA_PLANT_MOVE_FLAG + x] == 0x00u) {
      if ((int8_t)st->ram[SMBC_PIRANHA_PLANT_Y_SPEED + x] >= 0) {
        /* smb.asm: ChkPlayerNearPipe — the saved low byte of
         * PlayerEnemyDiff, two's complemented when the plant is
         * left of the player */
        uint8_t diff = (uint8_t)(st->ram[SMBC_ENEMY_X_POSITION + x] -
                                 st->ram[SMBC_PLAYER_X_POSITION]);
        if (player_enemy_diff(st, x) < 0) diff = (uint8_t)(~diff + 1u);
        if (diff < 0x21u) goto put_in_pipe;
      }
      /* smb.asm: ReversePlantSpeed */
      st->ram[SMBC_PIRANHA_PLANT_Y_SPEED + x] =
          (uint8_t)(~st->ram[SMBC_PIRANHA_PLANT_Y_SPEED + x] + 1u);
      st->ram[SMBC_PIRANHA_PLANT_MOVE_FLAG + x]++;
    }
    /* smb.asm: SetupToMovePPlant — the down coordinate when moving
     * down, the up coordinate when rising */
    uint8_t target = (int8_t)st->ram[SMBC_PIRANHA_PLANT_Y_SPEED + x] >= 0
                         ? st->ram[SMBC_PIRANHA_PLANT_DOWN_Y_POS + x]
                         : st->ram[SMBC_PIRANHA_PLANT_UP_Y_POS + x];
    /* smb.asm: RiseFallPiranhaPlant — every other frame, and never
     * under the master timer control */
    if ((st->ram[SMBC_FRAME_COUNTER] & 0x01u) != 0x00u &&
        st->ram[SMBC_TIMER_CONTROL] == 0x00u) {
      st->ram[SMBC_ENEMY_Y_POSITION + x] +=
          st->ram[SMBC_PIRANHA_PLANT_Y_SPEED + x];
      if (st->ram[SMBC_ENEMY_Y_POSITION + x] == target) {
        st->ram[SMBC_PIRANHA_PLANT_MOVE_FLAG + x] = 0x00u;
        st->ram[SMBC_ENEMY_FRAME_TIMER + x] = 0x40u;
      }
    }
  }
put_in_pipe:
  st->ram[SMBC_ENEMY_SPR_ATTRIB + x] = 0x20u;
}

/* smb.asm: PlayerLakituDiff — the speed lakitu (or a thrown spiny)
 * should fly at. The horizontal player distance caps at $3c; beyond
 * it a lakitu moving away from the player turns around, slowly if
 * already moving right. The capped distance, quartered, is then
 * subtracted from an adjustment picked by movement class — running
 * player with fast scroll, moving player, or neither — with a spiny
 * or a vertically-moving enemy keeping the richer classes. The
 * asm's SPixelLak loop subtracts once more than the pixel count */
static uint8_t player_lakitu_diff(CoreState *st, uint8_t x) {
  uint8_t side = 0x00u; /* enemy right of player */
  uint8_t diff = (uint8_t)(st->ram[SMBC_ENEMY_X_POSITION + x] -
                           st->ram[SMBC_PLAYER_X_POSITION]);
  if (player_enemy_diff(st, x) < 0) {
    side = 0x01u;
    diff = (uint8_t)(~diff + 1u);
  }
  if (diff >= 0x3cu) {
    diff = 0x3cu;
    if (st->ram[SMBC_ENEMY_ID + x] == SMBC_LAKITU &&
        side != st->ram[SMBC_LAKITU_MOVE_DIRECTION + x]) {
      if (st->ram[SMBC_LAKITU_MOVE_DIRECTION + x] != 0x00u) {
        st->ram[SMBC_ENEMY_X_SPEED + x]--; /* LakituMoveSpeed */
        if (st->ram[SMBC_ENEMY_X_SPEED + x] != 0x00u) {
          return st->ram[SMBC_ENEMY_X_SPEED + x];
        }
      }
      st->ram[SMBC_LAKITU_MOVE_DIRECTION + x] = side; /* smb.asm: SetLMovD */
    }
  }
  diff = (uint8_t)((diff & 0x3cu) >> 2);
  uint8_t adj = 0x00u;
  if (st->ram[SMBC_PLAYER_X_SPEED] != 0x00u &&
      st->ram[SMBC_SCROLL_AMOUNT] != 0x00u) {
    adj = 0x01u;
    if (st->ram[SMBC_PLAYER_X_SPEED] >= 0x19u &&
        st->ram[SMBC_SCROLL_AMOUNT] >= 0x02u) {
      adj = 0x02u;
    }
  }
  /* smb.asm: ChkSpinyO */
  if (st->ram[SMBC_ENEMY_ID + x] == SMBC_SPINY) {
    if (st->ram[SMBC_PLAYER_X_SPEED] == 0x00u &&
        st->ram[SMBC_ENEMY_Y_SPEED + x] == 0x00u) {
      adj = 0x00u;
    }
  } else if (st->ram[SMBC_ENEMY_Y_SPEED + x] == 0x00u) {
    adj = 0x00u;
  }
  return (uint8_t)(SMBC_LAKITU_DIFF_ADJ_DATA[adj] - diff - 0x01u);
}

void move_lakitu(CoreState *st, uint8_t x) {
  if ((st->ram[SMBC_ENEMY_STATE + x] & 0x20u) != 0x00u) {
    move_d_enemy_vertically(st, x);
    return;
  }
  uint8_t speed;
  if (st->ram[SMBC_ENEMY_STATE + x] != 0x00u) {
    st->ram[SMBC_LAKITU_MOVE_DIRECTION + x] = 0x00u;
    st->ram[SMBC_ENEMY_FRENZY_BUFFER] = 0x00u;
    speed = 0x10u;
  } else {
    st->ram[SMBC_ENEMY_FRENZY_BUFFER] = SMBC_SPINY;
    speed = player_lakitu_diff(st, x);
  }
  st->ram[SMBC_ENEMY_X_SPEED + x] = speed; /* LakituMoveSpeed */
  uint8_t dir = 0x01u; /* right */
  if ((st->ram[SMBC_LAKITU_MOVE_DIRECTION + x] & 0x01u) == 0x00u) {
    st->ram[SMBC_ENEMY_X_SPEED + x] = (uint8_t)(~speed + 1u);
    dir = 0x02u; /* left */
  }
  st->ram[SMBC_ENEMY_MOVING_DIR + x] = dir;
  move_enemy_horizontally(st, x);
}

void move_flying_cheep_cheep(CoreState *st, uint8_t x) {
  if ((st->ram[SMBC_ENEMY_STATE + x] & 0x20u) != 0x00u) {
    st->ram[SMBC_ENEMY_SPR_ATTRIB + x] = 0x00u;
    move_j_enemy_vertically(st, x);
    return;
  }
  move_enemy_horizontally(st, x);
  set_x_move_amt(st, x, 0x0du, 0x05u); /* smb.asm: SetXMoveAmt */
  uint8_t y = (uint8_t)(st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] >> 4);
  if (y > 0x09u) y = 0x09u; /* the code bytes beyond are not ported */
  uint8_t r = (uint8_t)(st->ram[SMBC_ENEMY_Y_POSITION + x] -
                        SMBC_PRANDOM_SUBTRACTER[y]);
  if ((r & 0x80u) != 0x00u) r = (uint8_t)(~r + 1u); /* smb.asm: AddCCF */
  if (r < 0x08u) {
    st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] += 0x10u;
    y = (uint8_t)(st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] >> 4);
  }
  /* smb.asm: BPGet — residual; the priority is overwritten before
   * the next frame is drawn */
  if (y > 0x04u) y = 0x04u;
  st->ram[SMBC_ENEMY_SPR_ATTRIB + x] = SMBC_FLY_CC_B_PRIORITY[y];
}

/* smb.asm: NoMoveCode — ids $09 and $13 have no mover */
static void no_move_code(void) {}

void enemy_movement_subs(CoreState *st, uint8_t x) {
  switch (st->ram[SMBC_ENEMY_ID + x]) {
    case 0x00u: /* GreenKoopa */
    case 0x01u:
    case 0x02u: /* BuzzyBeetle */
    case 0x03u: /* RedKoopa */
    case 0x04u:
    case 0x06u: /* Goomba */
    case 0x12u: /* Spiny */
      move_normal_enemy(st, x);
      break;
    case 0x05u:
      proc_hammer_bro(st, x);
      break;
    case 0x07u:
      move_bloober(st, x);
      break;
    case 0x08u:
      move_bullet_bill(st, x);
      break;
    case 0x09u:
    case 0x13u:
      no_move_code();
      break;
    case 0x0au:
    case 0x0bu:
      move_swimming_cheep_cheep(st, x);
      break;
    case 0x0cu:
      move_podoboo(st, x);
      break;
    case 0x0du:
      move_piranha_plant(st, x);
      break;
    case 0x0eu:
      move_jumping_enemy(st, x);
      break;
    case 0x0fu:
      proc_move_red_p_troopa(st, x);
      break;
    case 0x10u:
      move_fly_green_p_troopa(st, x);
      break;
    case 0x11u:
      move_lakitu(st, x);
      break;
    case 0x14u:
      move_flying_cheep_cheep(st, x);
      break;
    default:
      break;
  }
}

/* smb.asm: RunRetainerObj — the mushroom retainer only needs its
 * position resolved and drawn */
void run_retainer_obj(CoreState *st, uint8_t x) {
  get_enemy_offscreen_bits(st, x);
  relative_enemy_position(st, x);
  enemy_gfx_handler(st, x);
}

/* smb.asm: RunNormalEnemies — draw, bound, collide, move, and
 * bounds-check the ordinary enemies */
void run_normal_enemies(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_SPR_ATTRIB + x] = 0x00u;
  get_enemy_offscreen_bits(st, x);
  relative_enemy_position(st, x);
  enemy_gfx_handler(st, x);
  get_enemy_bound_box(st, x);
  enemy_to_bg_collision_det(st, x);
  enemies_collision(st, x);
  player_enemy_collision(st, x);
  if (st->ram[SMBC_TIMER_CONTROL] == 0x00u) enemy_movement_subs(st, x);
  offscreen_bounds_check(st, x);
}

/* smb.asm: ProcBowserFlame — drift the flame left with the borrow
 * chaining from the move force through the page, bob toward the
 * random target, and draw the three flame sprites */
void proc_bowser_flame(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_TIMER_CONTROL] == 0x00u) {
    uint8_t force = st->ram[SMBC_SECONDARY_HARD_MODE] != 0x00u ? 0x60u : 0x40u;
    uint16_t sub = (uint16_t)st->ram[SMBC_ENEMY_X_MOVE_FORCE + x] - force;
    st->ram[SMBC_ENEMY_X_MOVE_FORCE + x] = (uint8_t)sub;
    sub = (uint16_t)st->ram[SMBC_ENEMY_X_POSITION + x] - 0x01u -
          ((sub & 0xff00u) != 0x0000u ? 0x01u : 0x00u);
    st->ram[SMBC_ENEMY_X_POSITION + x] = (uint8_t)sub;
    sub = (uint16_t)st->ram[SMBC_ENEMY_PAGE_LOC + x] -
          ((sub & 0xff00u) != 0x0000u ? 0x01u : 0x00u);
    st->ram[SMBC_ENEMY_PAGE_LOC + x] = (uint8_t)sub;
    uint8_t y_pos = st->ram[SMBC_ENEMY_Y_POSITION + x];
    if (y_pos !=
        SMBC_FLAME_Y_POS_DATA[st->ram[SMBC_BOWSER_FLAME_PRANDOM_OFS + x]]) {
      st->ram[SMBC_ENEMY_Y_POSITION + x] =
          (uint8_t)(y_pos + st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x]);
    }
  }
  /* smb.asm: SetGfxF */
  relative_enemy_position(st, x);
  if (st->ram[SMBC_ENEMY_STATE + x] != 0x00u) return;
  uint8_t tile = 0x51u;
  uint8_t attr = (st->ram[SMBC_FRAME_COUNTER] & 0x02u) != 0x00u ? 0x82u : 0x02u;
  uint8_t y = st->ram[SMBC_ENEMY_SPR_DATA_OFFSET + x];
  for (uint8_t i = 0x00u; i < 0x03u; i++) {
    st->ram[SMBC_SPRITE_Y_POSITION + y] = st->ram[SMBC_ENEMY_REL_YPOS];
    st->ram[SMBC_SPRITE_TILENUMBER + y] = tile;
    tile++;
    st->ram[SMBC_SPRITE_ATTRIBUTES + y] = attr;
    st->ram[SMBC_SPRITE_X_POSITION + y] = st->ram[SMBC_ENEMY_REL_XPOS];
    st->ram[SMBC_ENEMY_REL_XPOS] += 0x08u;
    y = (uint8_t)(y + 0x04u);
  }
  get_enemy_offscreen_bits(st, x);
  y = st->ram[SMBC_ENEMY_SPR_DATA_OFFSET + x];
  uint8_t bits = st->ram[SMBC_ENEMY_OFFSCREEN_BITS];
  if ((bits & 0x01u) != 0x00u) {
    st->ram[SMBC_SPRITE_Y_POSITION + 12u + y] = 0xf8u; /* residual */
  }
  if ((bits & 0x02u) != 0x00u) {
    st->ram[SMBC_SPRITE_Y_POSITION + 8u + y] = 0xf8u;
  }
  if ((bits & 0x04u) != 0x00u) {
    st->ram[SMBC_SPRITE_Y_POSITION + 4u + y] = 0xf8u;
  }
  if ((bits & 0x08u) != 0x00u) {
    st->ram[SMBC_SPRITE_Y_POSITION + y] = 0xf8u;
  }
}

/* smb.asm: RunBowserFlame — process, then the shared pipeline */
void run_bowser_flame(CoreState *st, uint8_t x) {
  proc_bowser_flame(st, x);
  get_enemy_offscreen_bits(st, x);
  relative_enemy_position(st, x);
  get_enemy_bound_box(st, x);
  player_enemy_collision(st, x);
  offscreen_bounds_check(st, x);
}
