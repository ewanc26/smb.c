/* core/runners.c — see core/runners.h for the contract. */
#include "runners.h"

#include "../data/objects.h"
#include "enemies.h"
#include "enemies.h"
#include "collide.h"
#include "move.h"
#include "screen.h"
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

/* smb.asm: ExplosionTiles */
static const uint8_t SMBC_EXPLOSION_TILES[3] = {0x68u, 0x67u, 0x66u};

/* smb.asm: DrawExplosion_Fireworks — four tiles around the
 * explosion centre with the flips spread across the quadrants */
static void draw_explosion_fireworks(CoreState *st, uint8_t x, uint8_t y,
                                     uint8_t frame) {
  uint8_t tile = SMBC_EXPLOSION_TILES[frame];
  y++;
  st->ram[SMBC_SPRITE_DATA + y] = tile;
  st->ram[SMBC_SPRITE_DATA + 4u + y] = tile;
  st->ram[SMBC_SPRITE_DATA + 8u + y] = tile;
  st->ram[SMBC_SPRITE_DATA + 12u + y] = tile;
  y--;
  (void)x;
  uint8_t rel_y = (uint8_t)(st->ram[SMBC_FIREBALL_REL_YPOS] - 0x04u);
  st->ram[SMBC_SPRITE_Y_POSITION + y] = rel_y;
  st->ram[SMBC_SPRITE_Y_POSITION + 8u + y] = rel_y;
  rel_y = (uint8_t)(rel_y + 0x08u);
  st->ram[SMBC_SPRITE_Y_POSITION + 4u + y] = rel_y;
  st->ram[SMBC_SPRITE_Y_POSITION + 12u + y] = rel_y;
  uint8_t rel_x = (uint8_t)(st->ram[SMBC_FIREBALL_REL_XPOS] - 0x04u);
  st->ram[SMBC_SPRITE_X_POSITION + y] = rel_x;
  st->ram[SMBC_SPRITE_X_POSITION + 4u + y] = rel_x;
  rel_x = (uint8_t)(rel_x + 0x08u);
  st->ram[SMBC_SPRITE_X_POSITION + 8u + y] = rel_x;
  st->ram[SMBC_SPRITE_X_POSITION + 12u + y] = rel_x;
  st->ram[SMBC_SPRITE_ATTRIBUTES + y] = 0x02u;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] = 0x82u;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 8u + y] = 0x42u;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 12u + y] = 0xc2u;
}

/* smb.asm: RunFireworks — count the explosion frames, draw the
 * current frame, and at the end award 500 with the blast sound */
void run_fireworks(CoreState *st, uint8_t x) {
  st->ram[SMBC_EXPLOSION_TIMER_COUNTER + x]--;
  if (st->ram[SMBC_EXPLOSION_TIMER_COUNTER + x] == 0x00u) {
    st->ram[SMBC_EXPLOSION_TIMER_COUNTER + x] = 0x08u;
    st->ram[SMBC_EXPLOSION_GFX_COUNTER + x]++;
    if (st->ram[SMBC_EXPLOSION_GFX_COUNTER + x] >= 0x03u) {
      /* smb.asm: FireworksSoundScore */
      st->ram[SMBC_ENEMY_FLAG + x] = 0x00u;
      st->ram[SMBC_SQUARE2_SOUND_QUEUE] |= 0x08u; /* Sfx_Blast */
      st->ram[SMBC_DIGIT_MODIFIER + 4u] = 0x05u;
      end_area_points(st);
      return;
    }
  }
  /* smb.asm: SetupExpl — the explosion rides the fireball's
   * relative coordinates */
  relative_enemy_position(st, x);
  st->ram[SMBC_FIREBALL_REL_YPOS] = st->ram[SMBC_ENEMY_REL_YPOS];
  st->ram[SMBC_FIREBALL_REL_XPOS] = st->ram[SMBC_ENEMY_REL_XPOS];
  uint8_t y = st->ram[SMBC_ENEMY_SPR_DATA_OFFSET + x];
  draw_explosion_fireworks(st, x, y,
                           st->ram[SMBC_EXPLOSION_GFX_COUNTER + x]);
}

/* smb.asm: StarFlagYPosAdder / StarFlagXPosAdder / StarFlagTileData */
static const uint8_t SMBC_STAR_FLAG_Y_POS_ADDER[4] = {0x00u, 0x00u, 0x08u, 0x08u};
static const uint8_t SMBC_STAR_FLAG_X_POS_ADDER[4] = {0x00u, 0x08u, 0x00u, 0x08u};
static const uint8_t SMBC_STAR_FLAG_TILE_DATA[4] = {0x54u, 0x55u, 0x56u, 0x57u};

/* smb.asm: DrawStarFlag — the four star sprites around the
 * relative position, palette $22 with background priority */
static void draw_star_flag(CoreState *st, uint8_t x) {
  relative_enemy_position(st, x);
  uint8_t y = st->ram[SMBC_ENEMY_SPR_DATA_OFFSET + x];
  for (int i = 3; i >= 0; i--) {
    st->ram[SMBC_SPRITE_Y_POSITION + y] =
        (uint8_t)(st->ram[SMBC_ENEMY_REL_YPOS] +
                  SMBC_STAR_FLAG_Y_POS_ADDER[i]);
    st->ram[SMBC_SPRITE_TILENUMBER + y] = SMBC_STAR_FLAG_TILE_DATA[i];
    st->ram[SMBC_SPRITE_ATTRIBUTES + y] = 0x22u;
    st->ram[SMBC_SPRITE_X_POSITION + y] =
        (uint8_t)(st->ram[SMBC_ENEMY_REL_XPOS] +
                  SMBC_STAR_FLAG_X_POS_ADDER[i]);
    y = (uint8_t)(y + 0x04u);
  }
}

/* smb.asm: GameTimerFireworks — the timer's last digit picks the
 * fireworks count: 5 for 1, 3 for 3, 1 for 6, none otherwise */
static void game_timer_fireworks(CoreState *st, uint8_t x) {
  uint8_t last = st->ram[SMBC_GAME_TIMER_DISPLAY + 2u];
  uint8_t state = 0x05u;
  if (last == 0x01u) {
    st->ram[SMBC_FIREWORKS_COUNTER] = state;
  } else if (last == 0x03u) {
    state = 0x03u;
    st->ram[SMBC_FIREWORKS_COUNTER] = state;
  } else if (last == 0x06u) {
    state = 0x00u;
    st->ram[SMBC_FIREWORKS_COUNTER] = state;
  } else {
    state = 0x00u; /* the ldy #$00 from the $06 compare path */
    st->ram[SMBC_FIREWORKS_COUNTER] = 0xffu;
  }
  st->ram[SMBC_ENEMY_STATE + x] = state;
  st->ram[SMBC_STAR_FLAG_TASK_CONTROL]++;
}

/* smb.asm: AwardGameTimerPoints — tick the timer down one interval
 * per frame with the tick sound every fourth frame, 50 points per
 * interval; advance when the timer hits zero */
static void award_game_timer_points(CoreState *st, uint8_t x) {
  (void)x;
  if ((st->ram[SMBC_GAME_TIMER_DISPLAY] |
       st->ram[SMBC_GAME_TIMER_DISPLAY + 1u] |
       st->ram[SMBC_GAME_TIMER_DISPLAY + 2u]) == 0x00u) {
    st->ram[SMBC_STAR_FLAG_TASK_CONTROL]++;
    return;
  }
  if ((st->ram[SMBC_FRAME_COUNTER] & 0x04u) != 0x00u) {
    st->ram[SMBC_SQUARE2_SOUND_QUEUE] |= 0x10u; /* Sfx_TimerTick */
  }
  st->ram[SMBC_DIGIT_MODIFIER + 5u] = 0xffu; /* -1 from the timer */
  digits_math_routine(st, 0x23u);
  st->ram[SMBC_DIGIT_MODIFIER + 5u] = 0x05u; /* +50 per interval */
  end_area_points(st);
}

/* smb.asm: RaiseFlagSetoffFWorks — raise the star to $72, then
 * queue fireworks in the frenzy buffer while the counter lasts */
static void raise_flag_setoff_fworks(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_ENEMY_Y_POSITION + x] >= 0x72u) {
    st->ram[SMBC_ENEMY_Y_POSITION + x]--;
    draw_star_flag(st, x);
    return;
  }
  int8_t counter = (int8_t)st->ram[SMBC_FIREWORKS_COUNTER];
  if (counter > 0x00) {
    st->ram[SMBC_ENEMY_FRENZY_BUFFER] = 0x16u; /* Fireworks */
    draw_star_flag(st, x);
    return;
  }
  /* smb.asm: DrawFlagSetTimer */
  draw_star_flag(st, x);
  st->ram[SMBC_ENEMY_INTERVAL_TIMER + x] = 0x06u;
  st->ram[SMBC_STAR_FLAG_TASK_CONTROL]++;
}

/* smb.asm: DelayToAreaEnd — hold until the interval expires and
 * the event music buffer drains, then advance past the table */
static void delay_to_area_end(CoreState *st, uint8_t x) {
  draw_star_flag(st, x);
  if (st->ram[SMBC_ENEMY_INTERVAL_TIMER + x] == 0x00u &&
      st->ram[SMBC_EVENT_MUSIC_BUFFER] == 0x00u) {
    st->ram[SMBC_STAR_FLAG_TASK_CONTROL]++;
  }
}

/* smb.asm: RunStarFlagObj — clear the frenzy buffer and run the
 * current task */
void run_star_flag_obj(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_FRENZY_BUFFER] = 0x00u;
  switch (st->ram[SMBC_STAR_FLAG_TASK_CONTROL]) {
    case 0x01u:
      game_timer_fireworks(st, x);
      break;
    case 0x02u:
      award_game_timer_points(st, x);
      break;
    case 0x03u:
      raise_flag_setoff_fworks(st, x);
      break;
    case 0x04u:
      delay_to_area_end(st, x);
      break;
    default: /* smb.asm: StarFlagExit */
      break;
  }
}

/* smb.asm: Jumpspring_Y_PosData */
static const uint8_t SMBC_JUMPSPRING_Y_POS_DATA[4] = {
    0x08u, 0x10u, 0x08u, 0x00u};

/* smb.asm: JumpspringHandler — ride the player with the spring's
 * compression frames, catch a fresh A press on the third frame
 * for the high bounce, launch on the fifth, and step the frame
 * control every four frames */
void jumpspring_handler(CoreState *st, uint8_t x) {
  get_enemy_offscreen_bits(st, x);
  if (st->ram[SMBC_TIMER_CONTROL] == 0x00u &&
      st->ram[SMBC_JUMPSPRING_ANIM_CTRL] != 0x00u) {
    uint8_t frame = (uint8_t)(st->ram[SMBC_JUMPSPRING_ANIM_CTRL] - 0x01u);
    if ((frame & 0x02u) != 0x00u) {
      st->ram[SMBC_PLAYER_Y_POSITION] -= 0x02u;
    } else {
      st->ram[SMBC_PLAYER_Y_POSITION] += 0x02u;
    }
    st->ram[SMBC_ENEMY_Y_POSITION + x] =
        (uint8_t)(st->ram[SMBC_JUMPSPRING_FIXED_Y_POS + x] +
                  SMBC_JUMPSPRING_Y_POS_DATA[frame]);
    if (frame >= 0x01u) {
      uint8_t a_btn =
          (uint8_t)(st->ram[SMBC_A_B_BUTTONS] & 0x80u); /* A_Button */
      if (a_btn != 0x00u &&
          (a_btn & st->ram[SMBC_PREV_A_B_BUTTONS]) == 0x00u) {
        st->ram[SMBC_JUMPSPRING_FORCE] = 0xf4u;
      }
    }
    if (frame == 0x03u) {
      /* smb.asm: BounceJS — launch with the spring force */
      st->ram[SMBC_PLAYER_Y_SPEED] = st->ram[SMBC_JUMPSPRING_FORCE];
      st->ram[SMBC_JUMPSPRING_ANIM_CTRL] = 0x00u;
    }
  }
  /* smb.asm: DrawJSpr */
  relative_enemy_position(st, x);
  enemy_gfx_handler(st, x);
  offscreen_bounds_check(st, x);
  if (st->ram[SMBC_JUMPSPRING_ANIM_CTRL] != 0x00u &&
      st->ram[SMBC_JUMPSPRING_TIMER] == 0x00u) {
    st->ram[SMBC_JUMPSPRING_TIMER] = 0x04u;
    st->ram[SMBC_JUMPSPRING_ANIM_CTRL]++;
  }
}

/* smb.asm: VineYPosAdder */
static const uint8_t SMBC_VINE_Y_POS_ADDER[2] = {0x00u, 0x30u};

/* smb.asm: SixSpriteStacker — stack six sprite rows eight pixels
 * apart vertically, then restore the OAM offset */
static uint8_t smbc_six_sprite_stacker(CoreState *st, uint8_t y, uint8_t v) {
  for (uint8_t i = 0x00u; i < 0x06u; i++) {
    st->ram[SMBC_SPRITE_DATA + y] = v;
    v = (uint8_t)(v + 0x08u);
    y = (uint8_t)(y + 0x04u);
  }
  return (uint8_t)(y - 0x18u);
}

/* smb.asm: DrawVine — a six-sprite staggered column, $e1 tiles
 * with $e0 for the topmost vine, rows past $64 pixels from the
 * start parked offscreen */
static void draw_vine(CoreState *st, uint8_t x, uint8_t which) {
  (void)x;
  uint8_t y_pos = (uint8_t)(st->ram[SMBC_ENEMY_REL_YPOS] +
                            SMBC_VINE_Y_POS_ADDER[which]);
  uint8_t slot = st->ram[SMBC_VINE_OBJ_OFFSET + which];
  uint8_t y = st->ram[SMBC_ENEMY_SPR_DATA_OFFSET + slot];
  y = smbc_six_sprite_stacker(st, y, y_pos);
  uint8_t rel_x = st->ram[SMBC_ENEMY_REL_XPOS];
  st->ram[SMBC_SPRITE_X_POSITION + y] = rel_x;
  st->ram[SMBC_SPRITE_X_POSITION + 8u + y] = rel_x;
  st->ram[SMBC_SPRITE_X_POSITION + 16u + y] = rel_x;
  rel_x = (uint8_t)(rel_x + 0x06u);
  st->ram[SMBC_SPRITE_X_POSITION + 4u + y] = rel_x;
  st->ram[SMBC_SPRITE_X_POSITION + 12u + y] = rel_x;
  st->ram[SMBC_SPRITE_X_POSITION + 20u + y] = rel_x;
  st->ram[SMBC_SPRITE_ATTRIBUTES + y] = 0x21u;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 8u + y] = 0x21u;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 16u + y] = 0x21u;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] = 0x61u;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 12u + y] = 0x61u;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 20u + y] = 0x61u;
  for (uint8_t i = 0x00u; i < 0x06u; i++) {
    st->ram[SMBC_SPRITE_TILENUMBER + y] = 0xe1u;
    y = (uint8_t)(y + 0x04u);
  }
  y = (uint8_t)(y - 0x18u);
  if (which == 0x00u) st->ram[SMBC_SPRITE_TILENUMBER + y] = 0xe0u;
  /* smb.asm: ChkFTop — park sprites too far from the start */
  for (uint8_t i = 0x00u; i < 0x06u; i++) {
    uint8_t dist = (uint8_t)(st->ram[SMBC_VINE_START_Y_POSITION] -
                             st->ram[SMBC_SPRITE_Y_POSITION + y]);
    if (dist >= 0x64u) st->ram[SMBC_SPRITE_Y_POSITION + y] = 0xf8u;
    y = (uint8_t)(y + 0x04u);
  }
}

/* smb.asm: VineHeightData */
static const uint8_t SMBC_VINE_HEIGHT_DATA[2] = {0x30u, 0x60u};

/* smb.asm: VineObjectHandler — the vine lives in the special
 * slot: grow one pixel every other frame pair to the cap, draw
 * each vine, kill them all if fully offscreen, and write the
 * climbing metatile at the tip */
void vine_object_handler(CoreState *st, uint8_t x) {
  if (x != 0x05u) return;
  uint8_t flag = st->ram[SMBC_VINE_FLAG_OFFSET];
  uint8_t y = (uint8_t)(flag - 0x01u);
  if (st->ram[SMBC_VINE_HEIGHT] != SMBC_VINE_HEIGHT_DATA[y] &&
      (st->ram[SMBC_FRAME_COUNTER] & 0x02u) != 0x00u) {
    st->ram[SMBC_ENEMY_Y_POSITION + 5u]--;
    st->ram[SMBC_VINE_HEIGHT]++;
  }
  if (st->ram[SMBC_VINE_HEIGHT] < 0x08u) return;
  relative_enemy_position(st, x);
  get_enemy_offscreen_bits(st, x);
  for (y = 0x00u; y != flag; y++) draw_vine(st, x, y);
  if ((st->ram[SMBC_ENEMY_OFFSCREEN_BITS] & 0x0cu) != 0x00u) {
    /* smb.asm: KillVine */
    y = (uint8_t)(flag - 0x01u);
    for (;;) {
      erase_enemy_object(st, st->ram[SMBC_VINE_OBJ_OFFSET + y]);
      if (y == 0x00u) break;
      y--;
    }
    st->ram[SMBC_VINE_FLAG_OFFSET] = 0x00u;
    st->ram[SMBC_VINE_HEIGHT] = 0x00u;
  }
  /* smb.asm: WrCMTile */
  if (st->ram[SMBC_VINE_HEIGHT] < 0x20u) return;
  block_buffer_chk_enemy(st, 0x06u, 0x1bu, 0x01u);
  y = st->ram[SMBC_BB_Y_COORD];
  if (y >= 0xd0u) return;
  uint16_t ptr = (uint16_t)(st->ram[SMBC_BB_PTR_LO] |
                            ((uint16_t)st->ram[SMBC_BB_PTR_HI] << 8u));
  if (st->ram[ptr + y] != 0x00u) return;
  st->ram[ptr + y] = 0x26u;
}

/* smb.asm: PRandomRange */
static const uint8_t SMBC_P_RANDOM_RANGE[4] = {0x21u, 0x41u, 0x11u, 0x31u};

/* smb.asm: SetFlameTimer — cycle the flame timing table and, in
 * secondary hard mode, shorten the fuse */
static uint8_t set_flame_timer(CoreState *st) {
  uint8_t ctrl = st->ram[SMBC_BOWSER_FLAME_TIMER_CTRL];
  st->ram[SMBC_BOWSER_FLAME_TIMER_CTRL] =
      (uint8_t)((ctrl + 0x01u) & 0x07u);
  uint8_t v = SMBC_FLAME_TIMER_DATA[ctrl];
  if (st->ram[SMBC_SECONDARY_HARD_MODE] != 0x00u)
    v = (uint8_t)(v - 0x10u);
  return v;
}

/* smb.asm: KillAllEnemies — the defeated bowser past $e0 empties
 * every enemy slot and the frenzy buffer */
static void kill_all_enemies(CoreState *st) {
  for (int i = 4; i >= 0; i--) erase_enemy_object(st, (uint8_t)i);
  st->ram[SMBC_ENEMY_FRENZY_BUFFER] = 0x00u;
}

/* smb.asm: ProcessBowserHalf — one half of bowser: retainer draw,
 * then the bounding box and player collision while in the normal
 * state */
static void process_bowser_half(CoreState *st, uint8_t x) {
  st->ram[SMBC_BOWSER_GFX_FLAG]++;
  run_retainer_obj(st, x);
  if (st->ram[SMBC_ENEMY_STATE + x] != 0x00u) return;
  st->ram[SMBC_ENEMY_BOUND_BOX_CTRL + x] = 0x0au;
  get_enemy_bound_box(st, x);
  player_enemy_collision(st, x);
}

/* smb.asm: BowserGfxHandler — draw the front, mirror the state
 * into the rear object offset by the facing, and draw that too */
static void bowser_gfx_handler(CoreState *st, uint8_t x) {
  process_bowser_half(st, x);
  uint8_t ofs = (st->ram[SMBC_ENEMY_MOVING_DIR + x] & 0x01u) != 0x00u
                    ? 0xf0u
                    : 0x10u;
  uint8_t rear = st->ram[SMBC_DUPLICATE_OBJ_OFFSET];
  st->ram[SMBC_ENEMY_X_POSITION + rear] =
      (uint8_t)(st->ram[SMBC_ENEMY_X_POSITION + x] + ofs);
  st->ram[SMBC_ENEMY_Y_POSITION + rear] =
      (uint8_t)(st->ram[SMBC_ENEMY_Y_POSITION + x] + 0x08u);
  st->ram[SMBC_ENEMY_STATE + rear] = st->ram[SMBC_ENEMY_STATE + x];
  st->ram[SMBC_ENEMY_MOVING_DIR + rear] =
      st->ram[SMBC_ENEMY_MOVING_DIR + x];
  st->ram[SMBC_ENEMY_ID + rear] = SMBC_BOWSER;
  uint8_t saved = x;
  x = rear;
  process_bowser_half(st, x);
  (void)saved;
  st->ram[SMBC_BOWSER_GFX_FLAG] = 0x00u;
}

/* smb.asm: RunBowser — feet animation, facing resets, the drift
 * toward and away from the origin, hammers from world 6, jumps,
 * and the flame spit with the mouth toggle */
void run_bowser(CoreState *st, uint8_t x) {
  if ((st->ram[SMBC_ENEMY_STATE + x] & 0x20u) != 0x00u) {
    /* smb.asm: defeated — above $e0 the floater handles him,
     * below, clear the room */
    if (st->ram[SMBC_ENEMY_Y_POSITION + x] >= 0xe0u)
      kill_all_enemies(st);
    return;
  }
  st->ram[SMBC_ENEMY_FRENZY_BUFFER] = 0x00u;
  uint8_t timer_ctrl = st->ram[SMBC_TIMER_CONTROL];
  if (timer_ctrl == 0x00u &&
      (st->ram[SMBC_BOWSER_BODY_CONTROLS] & 0x80u) == 0x00u) {
    /* smb.asm: FeetTmr */
    if (--st->ram[SMBC_BOWSER_FEET_COUNTER] == 0x00u) {
      st->ram[SMBC_BOWSER_FEET_COUNTER] = 0x20u;
      st->ram[SMBC_BOWSER_BODY_CONTROLS] ^= 0x01u;
    }
    /* smb.asm: ResetMDr */
    if ((st->ram[SMBC_FRAME_COUNTER] & 0x0fu) == 0x00u)
      st->ram[SMBC_ENEMY_MOVING_DIR + x] = 0x02u;
    /* smb.asm: B_FaceP — chase the player while the frame timer
     * runs and bowser is left of the player */
    if (st->ram[SMBC_ENEMY_FRAME_TIMER + x] != 0x00u &&
        player_enemy_diff(st, x) < 0x00) {
      st->ram[SMBC_ENEMY_MOVING_DIR + x] = 0x01u;
      st->ram[SMBC_BOWSER_MOVEMENT_SPEED] = 0x02u;
      st->ram[SMBC_ENEMY_FRAME_TIMER + x] = 0x20u;
      st->ram[SMBC_BOWSER_FIRE_BREATH_TIMER] = 0x20u;
      if (st->ram[SMBC_ENEMY_X_POSITION + x] >= 0xc8u)
        goto hammer_chk;
    }
    /* smb.asm: GetPRCmp — the drift around the origin */
    if ((st->ram[SMBC_FRAME_COUNTER] & 0x03u) == 0x00u) {
      uint8_t epos = st->ram[SMBC_ENEMY_X_POSITION + x];
      if (epos == st->ram[SMBC_BOWSER_ORIG_X_POS]) {
        st->ram[SMBC_MAX_RANGE_FROM_ORIGIN] =
            SMBC_P_RANDOM_RANGE[st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + x] &
                                0x03u];
      }
      epos = (uint8_t)(epos + st->ram[SMBC_BOWSER_MOVEMENT_SPEED]);
      st->ram[SMBC_ENEMY_X_POSITION + x] = epos;
      if (st->ram[SMBC_ENEMY_MOVING_DIR + x] != 0x01u) {
        uint8_t speed = 0xffu;
        int8_t diff = (int8_t)(epos - st->ram[SMBC_BOWSER_ORIG_X_POS]);
        uint8_t d = (uint8_t)diff;
        if (diff < 0x00) {
          d = (uint8_t)(~d + 0x01u);
          speed = 0x01u;
        }
        if (d >= st->ram[SMBC_MAX_RANGE_FROM_ORIGIN])
          st->ram[SMBC_BOWSER_MOVEMENT_SPEED] = speed;
      }
    }
  }
hammer_chk:
  /* smb.asm: HammerChk — jumps while the frame timer counts down,
   * else fall, throw, and rearm */
  if (timer_ctrl == 0x00u) {
    if (st->ram[SMBC_ENEMY_FRAME_TIMER + x] != 0x00u) {
      /* smb.asm: MakeBJump */
      if (st->ram[SMBC_ENEMY_FRAME_TIMER + x] == 0x01u) {
        st->ram[SMBC_ENEMY_Y_POSITION + x]--;
        st->ram[SMBC_ENEMY_Y_SPEED + x] = 0x00u;
        st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] = 0x00u;
        st->ram[SMBC_ENEMY_Y_SPEED + x] = 0xfeu;
      }
    } else {
      move_enemy_slow_vert(st, x);
      if (st->ram[SMBC_WORLD_NUMBER] >= 0x05u && /* World6 */
          (st->ram[SMBC_FRAME_COUNTER] & 0x03u) == 0x00u)
        spawn_hammer_obj(st, x);
      if (st->ram[SMBC_ENEMY_Y_POSITION + x] >= 0x80u) {
        st->ram[SMBC_ENEMY_FRAME_TIMER + x] =
            SMBC_P_RANDOM_RANGE[st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + x] &
                                0x03u];
      }
    }
  }
  /* smb.asm: ChkFireB — world 8 always spits, 6-7 never, and the
   * earlier worlds only on their flame cadence */
  uint8_t world = st->ram[SMBC_WORLD_NUMBER];
  if (world == 0x07u /* World8 */ || world < 0x05u /* World6 */) {
    if (st->ram[SMBC_BOWSER_FIRE_BREATH_TIMER] == 0x00u) {
      st->ram[SMBC_BOWSER_FIRE_BREATH_TIMER] = 0x20u;
      st->ram[SMBC_BOWSER_BODY_CONTROLS] ^= 0x80u;
      if ((st->ram[SMBC_BOWSER_BODY_CONTROLS] & 0x80u) == 0x00u) {
        st->ram[SMBC_BOWSER_FIRE_BREATH_TIMER] = set_flame_timer(st);
        st->ram[SMBC_ENEMY_FRENZY_BUFFER] = SMBC_BOWSER_FLAME;
      }
    }
  }
  bowser_gfx_handler(st, x);
}

/* smb.asm: PowerUpObjHandler — the power-up owns the last enemy
 * slot: rise out of the block for sixteen frames, then move by
 * type (mushrooms walk, the star bounces) with the full draw,
 * collide, and bounds suite */
void power_up_obj_handler(CoreState *st) {
  const uint8_t x = 0x05u;
  uint8_t state = st->ram[SMBC_ENEMY_STATE + 5u];
  if (state == 0x00u) return;
  if ((state & 0x80u) != 0x00u) {
    /* smb.asm: the grown power-up moves by type */
    if (st->ram[SMBC_TIMER_CONTROL] == 0x00u) {
      uint8_t type = st->ram[SMBC_POWER_UP_TYPE];
      if (type == 0x00u || type == 0x03u) {
        /* smb.asm: ShroomM */
        move_normal_enemy(st, x);
        enemy_to_bg_collision_det(st, x);
      } else if (type == 0x02u) {
        /* smb.asm: the star bounces */
        move_jumping_enemy(st, x);
        enemy_jump(st, x);
      }
    }
  } else {
    /* smb.asm: GrowThePowerUp — one pixel per four frames until
     * the state passes the sixteenth, then walk right */
    if ((st->ram[SMBC_FRAME_COUNTER] & 0x03u) == 0x00u) {
      st->ram[SMBC_ENEMY_Y_POSITION + 5u]--;
      uint8_t grown = st->ram[SMBC_ENEMY_STATE + 5u];
      st->ram[SMBC_ENEMY_STATE + 5u]++;
      if (grown >= 0x11u) {
        st->ram[SMBC_ENEMY_X_SPEED + x] = 0x10u;
        st->ram[SMBC_ENEMY_STATE + 5u] = 0x80u;
        /* smb.asm: the asl/rol pair — zero attribute, direction
         * right via the carry */
        st->ram[SMBC_ENEMY_SPR_ATTRIB + 5u] = 0x00u;
        st->ram[SMBC_ENEMY_MOVING_DIR + x] = 0x01u;
      }
    }
    if (st->ram[SMBC_ENEMY_STATE + 5u] < 0x06u) return;
  }
  /* smb.asm: RunPUSubs */
  relative_enemy_position(st, x);
  get_enemy_offscreen_bits(st, x);
  get_enemy_bound_box(st, x);
  draw_power_up(st, x);
  player_enemy_collision(st, x);
  offscreen_bounds_check(st, x);
}

/* smb.asm: FirebarPosLookupTbl — the sinusoid per fireball part */
static const uint8_t SMBC_FIREBAR_POS_LOOKUP[99] = {
    0x00u, 0x01u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x07u, 0x08u,
    0x00u, 0x03u, 0x06u, 0x09u, 0x0bu, 0x0du, 0x0eu, 0x0fu, 0x10u,
    0x00u, 0x04u, 0x09u, 0x0du, 0x10u, 0x13u, 0x16u, 0x17u, 0x18u,
    0x00u, 0x06u, 0x0cu, 0x12u, 0x16u, 0x1au, 0x1du, 0x1fu, 0x20u,
    0x00u, 0x07u, 0x0fu, 0x16u, 0x1cu, 0x21u, 0x25u, 0x27u, 0x28u,
    0x00u, 0x09u, 0x12u, 0x1bu, 0x21u, 0x27u, 0x2cu, 0x2fu, 0x30u,
    0x00u, 0x0bu, 0x15u, 0x1fu, 0x27u, 0x2eu, 0x33u, 0x37u, 0x38u,
    0x00u, 0x0cu, 0x18u, 0x24u, 0x2du, 0x35u, 0x3bu, 0x3eu, 0x40u,
    0x00u, 0x0eu, 0x1bu, 0x28u, 0x32u, 0x3bu, 0x42u, 0x46u, 0x48u,
    0x00u, 0x0fu, 0x1fu, 0x2du, 0x38u, 0x42u, 0x4au, 0x4eu, 0x50u,
    0x00u, 0x11u, 0x22u, 0x31u, 0x3eu, 0x49u, 0x51u, 0x56u, 0x58u};

/* smb.asm: FirebarMirrorData — quadrant mirrors by spinstate */
static const uint8_t SMBC_FIREBAR_MIRROR_DATA[4] = {
    0x01u, 0x03u, 0x02u, 0x00u};

/* smb.asm: FirebarTblOffsets — one row of nine per fireball */
static const uint8_t SMBC_FIREBAR_TBL_OFFSETS[12] = {
    0x00u, 0x09u, 0x12u, 0x1bu, 0x24u, 0x2du,
    0x36u, 0x3fu, 0x48u, 0x51u, 0x5au, 0x63u};

/* smb.asm: FirebarYPos — collision reach small/big */
static const uint8_t SMBC_FIREBAR_Y_POS[2] = {0x0cu, 0x18u};

/* smb.asm: FirebarSpin — the speed spins the low byte, the carry
 * returns through the high byte; counter-clockwise subtracts */
static uint8_t smbc_firebar_spin(CoreState *st, uint8_t x, uint8_t speed) {
  if (st->ram[SMBC_FIREBAR_SPIN_DIRECTION + x] == 0x00u) {
    uint8_t lo = st->ram[SMBC_FIREBAR_SPIN_STATE_LOW + x];
    uint16_t sum = (uint16_t)lo + speed;
    st->ram[SMBC_FIREBAR_SPIN_STATE_LOW + x] = (uint8_t)sum;
    return (uint8_t)(st->ram[SMBC_FIREBAR_SPIN_STATE_HIGH + x] +
                     ((sum >> 8u) & 0x01u));
  }
  uint8_t lo = st->ram[SMBC_FIREBAR_SPIN_STATE_LOW + x];
  uint8_t sub = (uint8_t)(lo - speed);
  st->ram[SMBC_FIREBAR_SPIN_STATE_LOW + x] = sub;
  return (uint8_t)(st->ram[SMBC_FIREBAR_SPIN_STATE_HIGH + x] -
                   (sub > lo ? 0x01u : 0x00u));
}

/* smb.asm: GetFirebarPosition — the spinstate oscillates over the
 * lookup row for this part: low nybble for the horizontal adder,
 * plus eight for the vertical, mirrored per quadrant */
static void smbc_get_firebar_position(uint8_t spin, uint8_t part,
                                      uint8_t *h_adder, uint8_t *v_adder,
                                      uint8_t *mirror) {
  uint8_t a = (uint8_t)(spin & 0x0fu);
  if (a >= 0x09u) a = (uint8_t)((a ^ 0x0fu) + 0x01u);
  *h_adder = SMBC_FIREBAR_POS_LOOKUP[SMBC_FIREBAR_TBL_OFFSETS[part] + a];
  uint8_t v = (uint8_t)((spin + 0x08u) & 0x0fu);
  if (v >= 0x09u) v = (uint8_t)((v ^ 0x0fu) + 0x01u);
  *v_adder = SMBC_FIREBAR_POS_LOOKUP[SMBC_FIREBAR_TBL_OFFSETS[part] + v];
  *mirror = SMBC_FIREBAR_MIRROR_DATA[spin >> 3u];
}

/* smb.asm: DrawFirebar — tile $64/$65 alternating every four
 * frames, both flips set every eight */
static void smbc_draw_firebar(CoreState *st, uint8_t y) {
  uint8_t f = st->ram[SMBC_FRAME_COUNTER];
  st->ram[SMBC_SPRITE_TILENUMBER + y] = (uint8_t)(((f >> 2u) & 0x01u) ^ 0x64u);
  st->ram[SMBC_SPRITE_ATTRIBUTES + y] =
      (uint8_t)(0x02u | (((f >> 2u) & 0x02u) != 0x00u ? 0xc0u : 0x00u));
}

/* smb.asm: FirebarCollision — draw the fireball tile, then the
 * radial proximity check against the player: under eight pixels
 * on both axes hurts, pushing the shared movement direction away
 * from the side that hit; the OAM offset advances regardless */
static void smbc_firebar_collision(CoreState *st, uint8_t *oam_ofs,
                                   uint8_t spr_x, uint8_t spr_y) {
  uint8_t y = *oam_ofs;
  smbc_draw_firebar(st, y);
  if (st->ram[SMBC_STAR_INVINCIBLE_TIMER] == 0x00u &&
      st->ram[SMBC_TIMER_CONTROL] == 0x00u &&
      st->ram[SMBC_PLAYER_Y_HIGH_POS] == 0x01u) {
    uint8_t size_flag = 0x00u;
    uint8_t py = st->ram[SMBC_PLAYER_Y_POSITION];
    if (st->ram[SMBC_PLAYER_SIZE] != 0x00u ||
        st->ram[SMBC_CROUCHING_FLAG] != 0x00u) {
      /* smb.asm: AdjSm — small or crouching reaches lower */
      size_flag = 0x02u;
      py = (uint8_t)(py + 0x18u);
    }
    for (;;) {
      uint8_t dy = py >= spr_y ? (uint8_t)(py - spr_y)
                               : (uint8_t)(spr_y - py);
      if (dy < 0x08u && spr_x < 0xf0u) {
        /* smb.asm: the player sprite's centre against the
         * fireball */
        uint8_t pcx =
            (uint8_t)(st->ram[SMBC_SPRITE_X_POSITION + 4u] + 0x04u);
        uint8_t pdx = pcx >= spr_x ? (uint8_t)(pcx - spr_x)
                                   : (uint8_t)(spr_x - pcx);
        if (pdx < 0x08u) {
          /* smb.asm: ChgSDir — away from the side that hit */
          st->ram[SMBC_ENEMY_MOVING_DIR] = pcx >= spr_x ? 0x01u : 0x02u;
          injure_player(st);
          break;
        }
      }
      if (size_flag == 0x02u) break;
      py = (uint8_t)(st->ram[SMBC_PLAYER_Y_POSITION] +
                     SMBC_FIREBAR_Y_POS[size_flag]);
      size_flag++;
    }
  }
  *oam_ofs = (uint8_t)(y + 0x04u);
}

/* smb.asm: DrawFirebar_Collision — place one fireball from its
 * adders (mirrored per quadrant), park it at $f8 if it strays
 * $59 pixels from the centre, then run the collision check */
static void smbc_draw_firebar_collision(CoreState *st, uint8_t *oam_ofs,
                                        uint8_t h_adder, uint8_t v_adder,
                                        uint8_t mirror) {
  uint8_t y = *oam_ofs;
  uint8_t h = h_adder;
  if ((mirror & 0x01u) == 0x00u) h = (uint8_t)(0x01u + (h ^ 0xffu));
  uint8_t spr_x = (uint8_t)(h + st->ram[SMBC_ENEMY_REL_XPOS]);
  st->ram[SMBC_SPRITE_X_POSITION + y] = spr_x;
  uint8_t dx = spr_x >= st->ram[SMBC_ENEMY_REL_XPOS]
                   ? (uint8_t)(spr_x - st->ram[SMBC_ENEMY_REL_XPOS])
                   : (uint8_t)(st->ram[SMBC_ENEMY_REL_XPOS] - spr_x);
  uint8_t spr_y;
  if (dx >= 0x59u) {
    spr_y = 0xf8u;
  } else if (st->ram[SMBC_ENEMY_REL_YPOS] == 0xf8u) {
    spr_y = 0xf8u;
  } else {
    uint8_t v = v_adder;
    if ((mirror & 0x02u) == 0x00u) v = (uint8_t)(0x01u + (v ^ 0xffu));
    spr_y = (uint8_t)(v + st->ram[SMBC_ENEMY_REL_YPOS]);
  }
  st->ram[SMBC_SPRITE_Y_POSITION + y] = spr_y;
  smbc_firebar_collision(st, oam_ofs, spr_x, spr_y);
}

/* smb.asm: ProcFirebar — spin the bar (skipping the horizontal
 * state for the short bars), place the head, then each ball from
 * its lookup row; the fifth ball of a long bar borrows the
 * duplicate object's OAM offset */
static void smbc_proc_firebar(CoreState *st, uint8_t x) {
  get_enemy_offscreen_bits(st, x);
  if ((st->ram[SMBC_ENEMY_OFFSCREEN_BITS] & 0x08u) != 0x00u) return;
  if (st->ram[SMBC_TIMER_CONTROL] == 0x00u) {
    uint8_t hi = smbc_firebar_spin(st, x,
                                   st->ram[SMBC_FIREBAR_SPIN_SPEED + x]);
    st->ram[SMBC_FIREBAR_SPIN_STATE_HIGH + x] = (uint8_t)(hi & 0x1fu);
  }
  uint8_t spin = st->ram[SMBC_FIREBAR_SPIN_STATE_HIGH + x];
  if (st->ram[SMBC_ENEMY_ID + x] >= 0x1fu &&
      (spin == 0x08u || spin == 0x18u)) {
    /* smb.asm: SkpFSte — never rest horizontal */
    spin++;
    st->ram[SMBC_FIREBAR_SPIN_STATE_HIGH + x] = spin;
  }
  relative_enemy_position(st, x);
  uint8_t oam_ofs = st->ram[SMBC_ENEMY_SPR_DATA_OFFSET + x];
  st->ram[SMBC_SPRITE_Y_POSITION + oam_ofs] = st->ram[SMBC_ENEMY_REL_YPOS];
  st->ram[SMBC_SPRITE_X_POSITION + oam_ofs] = st->ram[SMBC_ENEMY_REL_XPOS];
  /* smb.asm: the head sits at the centre and collides from the
   * coordinates just written */
  smbc_firebar_collision(st, &oam_ofs, st->ram[SMBC_ENEMY_REL_XPOS],
                         st->ram[SMBC_ENEMY_REL_YPOS]);
  uint8_t h, v, mirror;
  uint8_t max_part =
      st->ram[SMBC_ENEMY_ID + x] >= 0x1fu ? 0x0bu : 0x05u;
  for (uint8_t part = 0x00u;;) {
    smbc_get_firebar_position(spin, part, &h, &v, &mirror);
    smbc_draw_firebar_collision(st, &oam_ofs, h, v, mirror);
    if (part == 0x04u) {
      /* smb.asm: NextFbar — the long bar's second half takes the
       * duplicate object's sprite offset */
      uint8_t dup = st->ram[SMBC_DUPLICATE_OBJ_OFFSET];
      oam_ofs = st->ram[SMBC_ENEMY_SPR_DATA_OFFSET + dup];
    }
    part++;
    if (part >= max_part) break;
  }
}

/* smb.asm: RunFirebarObj */
void run_firebar_obj(CoreState *st, uint8_t x) {
  smbc_proc_firebar(st, x);
  offscreen_bounds_check(st, x);
}

/* smb.asm: DrawSmallPlatform — six sprites in two stacked rows of
 * three girder tiles; either row parks at $f8 while it sits in
 * the status bar, and each column parks on its offscreen bit */
static void smbc_draw_small_platform(CoreState *st, uint8_t x) {
  uint8_t y = st->ram[SMBC_ENEMY_SPR_DATA_OFFSET + x];
  for (uint8_t i = 0x00u; i < 0x06u; i++) {
    st->ram[SMBC_SPRITE_TILENUMBER + (uint8_t)(i * 4u) + y] = 0x5bu;
    st->ram[SMBC_SPRITE_ATTRIBUTES + (uint8_t)(i * 4u) + y] = 0x02u;
  }
  uint8_t rel_x = st->ram[SMBC_ENEMY_REL_XPOS];
  st->ram[SMBC_SPRITE_X_POSITION + y] = rel_x;
  st->ram[SMBC_SPRITE_X_POSITION + 12u + y] = rel_x;
  st->ram[SMBC_SPRITE_X_POSITION + 4u + y] = (uint8_t)(rel_x + 0x08u);
  st->ram[SMBC_SPRITE_X_POSITION + 16u + y] = (uint8_t)(rel_x + 0x08u);
  st->ram[SMBC_SPRITE_X_POSITION + 8u + y] = (uint8_t)(rel_x + 0x10u);
  st->ram[SMBC_SPRITE_X_POSITION + 20u + y] = (uint8_t)(rel_x + 0x10u);
  uint8_t top = st->ram[SMBC_ENEMY_Y_POSITION + x];
  if (top < 0x20u) top = 0xf8u;
  st->ram[SMBC_SPRITE_Y_POSITION + y] = top;
  st->ram[SMBC_SPRITE_Y_POSITION + 4u + y] = top;
  st->ram[SMBC_SPRITE_Y_POSITION + 8u + y] = top;
  uint8_t bot =
      (uint8_t)(st->ram[SMBC_ENEMY_Y_POSITION + x] + 0x80u);
  if (bot < 0x20u) bot = 0xf8u;
  st->ram[SMBC_SPRITE_Y_POSITION + 12u + y] = bot;
  st->ram[SMBC_SPRITE_Y_POSITION + 16u + y] = bot;
  st->ram[SMBC_SPRITE_Y_POSITION + 20u + y] = bot;
  uint8_t bits = st->ram[SMBC_ENEMY_OFFSCREEN_BITS];
  if ((bits & 0x08u) != 0x00u) {
    st->ram[SMBC_SPRITE_Y_POSITION + y] = 0xf8u;
    st->ram[SMBC_SPRITE_Y_POSITION + 12u + y] = 0xf8u;
  }
  if ((bits & 0x04u) != 0x00u) {
    st->ram[SMBC_SPRITE_Y_POSITION + 4u + y] = 0xf8u;
    st->ram[SMBC_SPRITE_Y_POSITION + 16u + y] = 0xf8u;
  }
  if ((bits & 0x02u) != 0x00u) {
    st->ram[SMBC_SPRITE_Y_POSITION + 8u + y] = 0xf8u;
    st->ram[SMBC_SPRITE_Y_POSITION + 20u + y] = 0xf8u;
  }
}

/* smb.asm: DrawLargePlatform — six girder tiles in one row, the
 * last two parked in castles and secondary hard mode; the cloud
 * override swaps the tile for the puff */
static void smbc_draw_large_platform(CoreState *st, uint8_t x) {
  uint8_t y = st->ram[SMBC_ENEMY_SPR_DATA_OFFSET + x];
  smbc_six_sprite_stacker(st, (uint8_t)(y + 0x03u),
                          st->ram[SMBC_ENEMY_REL_XPOS]);
  uint8_t y_pos = st->ram[SMBC_ENEMY_Y_POSITION + x];
  st->ram[SMBC_SPRITE_Y_POSITION + y] = y_pos;
  st->ram[SMBC_SPRITE_Y_POSITION + 4u + y] = y_pos;
  st->ram[SMBC_SPRITE_Y_POSITION + 8u + y] = y_pos;
  st->ram[SMBC_SPRITE_Y_POSITION + 12u + y] = y_pos;
  uint8_t last = y_pos;
  if (st->ram[SMBC_AREA_TYPE] == 0x03u ||
      st->ram[SMBC_SECONDARY_HARD_MODE] != 0x00u) {
    last = 0xf8u;
  }
  st->ram[SMBC_SPRITE_Y_POSITION + 16u + y] = last;
  st->ram[SMBC_SPRITE_Y_POSITION + 20u + y] = last;
  uint8_t tile =
      st->ram[SMBC_CLOUD_TYPE_OVERRIDE] != 0x00u ? 0x75u : 0x5bu;
  for (uint8_t i = 0x00u; i < 0x06u; i++) {
    st->ram[SMBC_SPRITE_TILENUMBER + (uint8_t)(i * 4u) + y] = tile;
    st->ram[SMBC_SPRITE_ATTRIBUTES + (uint8_t)(i * 4u) + y] = 0x02u;
  }
  /* smb.asm: SChk2-SChk6 — each sprite parks on its horizontal
   * offscreen bit, d7 down to d2 */
  uint8_t xbits = get_x_offscreen_bits(st, (uint8_t)(x + 1u));
  for (uint8_t i = 0x00u; i < 0x06u; i++) {
    if ((xbits & (0x80u >> i)) != 0x00u) {
      st->ram[SMBC_SPRITE_Y_POSITION + (uint8_t)(i * 4u) + y] = 0xf8u;
    }
  }
  if ((st->ram[SMBC_ENEMY_OFFSCREEN_BITS] & 0x80u) != 0x00u) {
    /* smb.asm: MoveSixSpritesOffscreen */
    for (uint8_t i = 0x00u; i < 0x06u; i++) {
      st->ram[SMBC_SPRITE_Y_POSITION + (uint8_t)(i * 4u) + y] = 0xf8u;
    }
  }
}

/* smb.asm: MoveLiftPlatforms — the shared lift drift: movement
 * force accumulates into the dummy, the speed rides the carry */
static void smbc_move_lift_platforms(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_TIMER_CONTROL] != 0x00u) return;
  uint16_t sum = (uint16_t)st->ram[SMBC_ENEMY_YMF_DUMMY + x] +
                 st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x];
  st->ram[SMBC_ENEMY_YMF_DUMMY + x] = (uint8_t)sum;
  st->ram[SMBC_ENEMY_Y_POSITION + x] =
      (uint8_t)(st->ram[SMBC_ENEMY_Y_POSITION + x] +
                st->ram[SMBC_ENEMY_Y_SPEED + x] + ((sum >> 8u) & 0x01u));
}

/* smb.asm: MoveSmallPlatform + ChkSmallPlatCollision — drift,
 * then carry the player if the collision flag holds a counter */
static void smbc_move_small_platform(CoreState *st, uint8_t x) {
  smbc_move_lift_platforms(st, x);
  uint8_t flag = st->ram[SMBC_PLATFORM_COLLISION_FLAG + x];
  if (flag != 0x00u) {
    position_player_on_s_plat(st, x, flag);
  }
}

/* smb.asm: RunSmallPlatform */
void run_small_platform(CoreState *st, uint8_t x) {
  get_enemy_offscreen_bits(st, x);
  relative_enemy_position(st, x);
  small_platform_bound_box(st, x);
  small_platform_collision(st, x);
  relative_enemy_position(st, x);
  smbc_draw_small_platform(st, x);
  smbc_move_small_platform(st, x);
  offscreen_bounds_check(st, x);
}

/* smb.asm: RunLargePlatform — the movement subroutines dispatch
 * per object $24-$2a unless the master timer is set */
void run_large_platform(CoreState *st, uint8_t x) {
  get_enemy_offscreen_bits(st, x);
  relative_enemy_position(st, x);
  large_platform_bound_box(st, x);
  large_platform_collision(st, x);
  if (st->ram[SMBC_TIMER_CONTROL] == 0x00u) {
    large_platform_subroutines(st, x);
  }
  relative_enemy_position(st, x);
  smbc_draw_large_platform(st, x);
  offscreen_bounds_check(st, x);
}

/* smb.asm: StopPlatforms — zero both platforms' vertical
 * momentum */
static void smbc_stop_platforms(CoreState *st, uint8_t x, uint8_t y) {
  uint8_t zero = init_vstf(st, x);
  st->ram[SMBC_ENEMY_Y_SPEED + y] = zero;
  st->ram[SMBC_ENEMY_Y_MOVE_FORCE + y] = zero;
}

/* smb.asm: SetupPlatformRope — the name table address for the
 * rope half a tile right of the platform (a full tile in
 * secondary hard mode), one row lower while the platform rises */
static void smbc_setup_platform_rope(CoreState *st, uint8_t y, uint8_t speed,
                                     uint8_t *low, uint8_t *high) {
  uint16_t xp = (uint16_t)st->ram[SMBC_ENEMY_X_POSITION + y] + 0x08u;
  if (st->ram[SMBC_SECONDARY_HARD_MODE] == 0x00u) xp += 0x10u;
  uint8_t page =
      (uint8_t)(st->ram[SMBC_ENEMY_PAGE_LOC + y] + (uint8_t)(xp >> 8u));
  uint8_t hpart = (uint8_t)(((uint8_t)xp & 0xf0u) >> 3u);
  uint8_t ycoord = st->ram[SMBC_ENEMY_Y_POSITION + y];
  if ((int8_t)speed < 0) ycoord = (uint8_t)(ycoord + 0x08u);
  uint8_t rot = (uint8_t)(ycoord << 2u);
  uint8_t carry_hi = ycoord >> 6u; /* the two bits rotated out */
  uint8_t hi = (uint8_t)(carry_hi | 0x20u);
  hi = (uint8_t)(hi | ((page & 0x01u) << 2u));
  uint8_t lo = (uint8_t)((rot & 0xe0u) + hpart);
  if (st->ram[SMBC_ENEMY_Y_POSITION + y] >= 0xe8u) lo &= 0xbfu;
  *low = lo;
  *high = hi;
}

/* smb.asm: InitPlatformFall — both platforms let go, a 1000-point
 * floatey number where the player stands, the moving direction as
 * the falling flag */
static void smbc_init_platform_fall(CoreState *st, uint8_t y) {
  get_enemy_offscreen_bits(st, y);
  setup_floatey_number(st, y, 0x06u);
  st->ram[SMBC_FLOATEY_NUM_X_POS + y] = st->ram[SMBC_PLAYER_REL_XPOS];
  st->ram[SMBC_FLOATEY_NUM_Y_POS + y] = st->ram[SMBC_PLAYER_Y_POSITION];
  st->ram[SMBC_ENEMY_MOVING_DIR + y] = 0x01u;
}

/* smb.asm: PlatformFall — both platforms drop together, carrying
 * the player */
static void smbc_platform_fall(CoreState *st, uint8_t x, uint8_t y) {
  move_falling_platform(st, x);
  move_falling_platform(st, y);
  uint8_t flag = st->ram[SMBC_PLATFORM_COLLISION_FLAG + x];
  if ((int8_t)flag >= 0) {
    position_player_on_v_plat(st, flag, st->ram[SMBC_ENEMY_Y_POSITION + flag]);
  }
}

/* smb.asm: DrawEraseRope — the rope tiles follow the platforms:
 * drawn above the descending one, erased above the ascending */
static void smbc_draw_erase_rope(CoreState *st, uint8_t x) {
  if ((st->ram[SMBC_ENEMY_Y_SPEED + x] |
       st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x]) == 0x00u) {
    return;
  }
  uint8_t ofs = st->ram[SMBC_VRAM_BUFFER1_OFFSET];
  if (ofs >= 0x20u) return;
  uint8_t speed = st->ram[SMBC_ENEMY_Y_SPEED + x];
  uint8_t low, high;
  smbc_setup_platform_rope(st, x, speed, &low, &high);
  st->ram[SMBC_VRAM_BUFFER1 + ofs] = high;
  st->ram[SMBC_VRAM_BUFFER1 + 1u + ofs] = low;
  st->ram[SMBC_VRAM_BUFFER1 + 2u + ofs] = 0x02u;
  if ((int8_t)speed < 0) {
    st->ram[SMBC_VRAM_BUFFER1 + 3u + ofs] = 0x24u;
    st->ram[SMBC_VRAM_BUFFER1 + 4u + ofs] = 0x24u;
  } else {
    st->ram[SMBC_VRAM_BUFFER1 + 3u + ofs] = 0xa2u;
    st->ram[SMBC_VRAM_BUFFER1 + 4u + ofs] = 0xa3u;
  }
  uint8_t other = st->ram[SMBC_ENEMY_STATE + x];
  smbc_setup_platform_rope(st, other, (uint8_t)~speed, &low, &high);
  st->ram[SMBC_VRAM_BUFFER1 + 5u + ofs] = high;
  st->ram[SMBC_VRAM_BUFFER1 + 6u + ofs] = low;
  st->ram[SMBC_VRAM_BUFFER1 + 7u + ofs] = 0x02u;
  if ((int8_t)speed >= 0) {
    st->ram[SMBC_VRAM_BUFFER1 + 8u + ofs] = 0x24u;
    st->ram[SMBC_VRAM_BUFFER1 + 9u + ofs] = 0x24u;
  } else {
    st->ram[SMBC_VRAM_BUFFER1 + 8u + ofs] = 0xa2u;
    st->ram[SMBC_VRAM_BUFFER1 + 9u + ofs] = 0xa3u;
  }
  st->ram[SMBC_VRAM_BUFFER1 + 10u + ofs] = 0x00u;
  st->ram[SMBC_VRAM_BUFFER1_OFFSET] = (uint8_t)(ofs + 10u);
}

/* smb.asm: BalancePlatform — the rope pair: whichever platform
 * the player stands on sinks, the other rises, both stop short of
 * $2d; a stand at the limit lets both fall */
static void smbc_balance_platform(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_ENEMY_Y_HIGH_POS + x] == 0x03u) {
    erase_enemy_object(st, x);
    return;
  }
  int8_t state = (int8_t)st->ram[SMBC_ENEMY_STATE + x];
  if (state < 0) return;
  uint8_t y = (uint8_t)state;
  uint8_t flag = st->ram[SMBC_PLATFORM_COLLISION_FLAG + x];
  if (st->ram[SMBC_ENEMY_MOVING_DIR + x] != 0x00u) {
    smbc_platform_fall(st, x, y);
    return;
  }
  /* smb.asm: ChkForFall — above the limit, check for a stand */
  if (st->ram[SMBC_ENEMY_Y_POSITION + x] <= 0x2du) {
    if (flag == y) {
      smbc_init_platform_fall(st, y);
      return;
    }
    st->ram[SMBC_ENEMY_Y_POSITION + x] = 0x2fu;
    smbc_stop_platforms(st, x, y);
    return;
  }
  if (st->ram[SMBC_ENEMY_Y_POSITION + y] <= 0x2du) {
    if (flag == x) {
      smbc_init_platform_fall(st, y);
      return;
    }
    st->ram[SMBC_ENEMY_Y_POSITION + y] = 0x2fu;
    smbc_stop_platforms(st, x, y);
    return;
  }
  /* smb.asm: ChkToMoveBalPlat — the seesaw */
  uint8_t old_pos = st->ram[SMBC_ENEMY_Y_POSITION + x];
  if ((int8_t)flag >= 0) {
    if (flag == x) {
      move_platform_vert(st, x, 0x00u);
    } else {
      move_platform_vert(st, x, 0x01u);
    }
  } else {
    uint16_t force = (uint16_t)st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] + 0x05u;
    uint8_t speed =
        (uint8_t)(st->ram[SMBC_ENEMY_Y_SPEED + x] + (uint8_t)(force >> 8u));
    if ((int8_t)speed < 0) {
      move_platform_vert(st, x, 0x00u);
    } else if (speed != 0x00u || (uint8_t)force >= 0x0bu) {
      move_platform_vert(st, x, 0x01u);
    } else {
      smbc_stop_platforms(st, x, y);
    }
  }
  /* smb.asm: DoOtherPlatform — the other side mirrors the move */
  uint8_t diff =
      (uint8_t)(old_pos - st->ram[SMBC_ENEMY_Y_POSITION + x]);
  st->ram[SMBC_ENEMY_Y_POSITION + y] =
      (uint8_t)(st->ram[SMBC_ENEMY_Y_POSITION + y] + diff);
  if ((int8_t)flag >= 0) {
    position_player_on_v_plat(st, flag,
                              st->ram[SMBC_ENEMY_Y_POSITION + flag]);
  }
  smbc_draw_erase_rope(st, x);
}

/* smb.asm: YMovingPlatform — sink to the top bound an eighth of
 * the frames, then gravity between the bounds, carrying the
 * player */
static void smbc_y_moving_platform(CoreState *st, uint8_t x) {
  if ((st->ram[SMBC_ENEMY_Y_SPEED + x] |
       st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x]) == 0x00u) {
    st->ram[SMBC_ENEMY_YMF_DUMMY + x] = 0x00u;
    if (st->ram[SMBC_ENEMY_Y_POSITION + x] <
        st->ram[SMBC_Y_PLATFORM_TOP_Y_POS + x]) {
      if ((st->ram[SMBC_FRAME_COUNTER] & 0x07u) == 0x00u) {
        st->ram[SMBC_ENEMY_Y_POSITION + x]++;
      }
      goto collision;
    }
  }
  if (st->ram[SMBC_ENEMY_Y_POSITION + x] <
      st->ram[SMBC_Y_PLATFORM_CENTER_Y_POS + x]) {
    move_platform_vert(st, x, 0x00u);
  } else {
    move_platform_vert(st, x, 0x01u);
  }
collision:;
  uint8_t flag = st->ram[SMBC_PLATFORM_COLLISION_FLAG + x];
  if ((int8_t)flag >= 0) {
    position_player_on_v_plat(st, x, st->ram[SMBC_ENEMY_Y_POSITION + x]);
  }
}

/* smb.asm: XMoveCntr_Platform — every fourth frame the secondary
 * counter walks toward the preset maximum while the primary is
 * even, back to zero while odd, flipping the primary at each
 * end */
static void smbc_x_move_cntr_platform(CoreState *st, uint8_t x, uint8_t max) {
  if ((st->ram[SMBC_FRAME_COUNTER] & 0x03u) != 0x00u) return;
  uint8_t sec = st->ram[SMBC_X_MOVE_SECONDARY_COUNTER + x];
  if ((st->ram[SMBC_X_MOVE_PRIMARY_COUNTER + x] & 0x01u) == 0x00u) {
    if (sec == max) {
      st->ram[SMBC_X_MOVE_PRIMARY_COUNTER + x]++;
    } else {
      st->ram[SMBC_X_MOVE_SECONDARY_COUNTER + x]++;
    }
  } else {
    if (sec == 0x00u) {
      st->ram[SMBC_X_MOVE_PRIMARY_COUNTER + x]++;
    } else {
      st->ram[SMBC_X_MOVE_SECONDARY_COUNTER + x]--;
    }
  }
}

/* smb.asm: MoveWithXMCntrs — d1 of the primary picks the
 * direction; the secondary counter negates for leftward motion
 * and is restored after the move, the residual delta returned */
static uint8_t smbc_move_with_xm_cntrs(CoreState *st, uint8_t x) {
  uint8_t saved = st->ram[SMBC_X_MOVE_SECONDARY_COUNTER + x];
  uint8_t dir = 0x01u;
  if ((st->ram[SMBC_X_MOVE_PRIMARY_COUNTER + x] & 0x02u) == 0x00u) {
    st->ram[SMBC_X_MOVE_SECONDARY_COUNTER + x] =
        (uint8_t)(0x01u + (saved ^ 0xffu));
    dir = 0x02u;
  }
  st->ram[SMBC_ENEMY_MOVING_DIR + x] = dir;
  uint8_t adder = move_enemy_horizontally(st, x);
  st->ram[SMBC_X_MOVE_SECONDARY_COUNTER + x] = saved;
  return adder;
}

/* smb.asm: PositionPlayerOnHPlat — the horizontal carry: the
 * move delta slides the player and their page, remembered in the
 * scroll register */
static void smbc_position_player_on_h_plat(CoreState *st, uint8_t x,
                                           uint8_t adder) {
  uint16_t sum = (uint16_t)st->ram[SMBC_PLAYER_X_POSITION] + adder;
  st->ram[SMBC_PLAYER_X_POSITION] = (uint8_t)sum;
  if ((int8_t)adder < 0) {
    st->ram[SMBC_PLAYER_PAGE_LOC] =
        (uint8_t)(st->ram[SMBC_PLAYER_PAGE_LOC] -
                  (((sum >> 8u) & 0x01u) == 0x00u ? 0x01u : 0x00u));
  } else {
    st->ram[SMBC_PLAYER_PAGE_LOC] =
        (uint8_t)(st->ram[SMBC_PLAYER_PAGE_LOC] + ((sum >> 8u) & 0x01u));
  }
  st->ram[SMBC_PLATFORM_X_SCROLL] = adder;
  position_player_on_v_plat(st, x, st->ram[SMBC_ENEMY_Y_POSITION + x]);
}

/* smb.asm: XMovingPlatform */
static void smbc_x_moving_platform(CoreState *st, uint8_t x) {
  smbc_x_move_cntr_platform(st, x, 0x0eu);
  uint8_t adder = smbc_move_with_xm_cntrs(st, x);
  if ((int8_t)st->ram[SMBC_PLATFORM_COLLISION_FLAG + x] < 0) return;
  smbc_position_player_on_h_plat(st, x, adder);
}

/* smb.asm: DropPlatform — once touched, straight down */
static void smbc_drop_platform(CoreState *st, uint8_t x) {
  if ((int8_t)st->ram[SMBC_PLATFORM_COLLISION_FLAG + x] < 0) return;
  move_drop_platform(st, x);
  position_player_on_v_plat(st, x, st->ram[SMBC_ENEMY_Y_POSITION + x]);
}

/* smb.asm: RightPlatform — motionless until stood on, then
 * steady rightward */
static void smbc_right_platform(CoreState *st, uint8_t x) {
  uint8_t adder = move_enemy_horizontally(st, x);
  if ((int8_t)st->ram[SMBC_PLATFORM_COLLISION_FLAG + x] < 0) return;
  st->ram[SMBC_ENEMY_X_SPEED + x] = 0x10u;
  smbc_position_player_on_h_plat(st, x, adder);
}

/* smb.asm: MoveLargeLiftPlat — the shared lift drift, then the
 * carry check */
static void smbc_move_large_lift_plat(CoreState *st, uint8_t x) {
  smbc_move_lift_platforms(st, x);
  uint8_t flag = st->ram[SMBC_PLATFORM_COLLISION_FLAG + x];
  if ((int8_t)flag >= 0) {
    position_player_on_v_plat(st, x, st->ram[SMBC_ENEMY_Y_POSITION + x]);
  }
}

/* smb.asm: LargePlatformSubroutines — the jump table for objects
 * $24-$2a */
void large_platform_subroutines(CoreState *st, uint8_t x) {
  switch ((uint8_t)(st->ram[SMBC_ENEMY_ID + x] - 0x24u)) {
  case 0x00u: smbc_balance_platform(st, x); break;
  case 0x01u: smbc_y_moving_platform(st, x); break;
  case 0x02u:
  case 0x03u: smbc_move_large_lift_plat(st, x); break;
  case 0x04u: smbc_x_moving_platform(st, x); break;
  case 0x05u: smbc_drop_platform(st, x); break;
  case 0x06u: smbc_right_platform(st, x); break;
  default: break;
  }
}

/* smb.asm: WarpZoneObject — the invisible anchor at the top of a
 * warp zone: once the scroll locks and the player is at the very
 * top (vertical coordinate and high byte sharing no set bits),
 * it releases the lock, raises the warp control, and erases
 * itself */
void warp_zone_object(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_SCROLL_LOCK] == 0x00u) return;
  uint8_t a = (uint8_t)(st->ram[SMBC_PLAYER_Y_POSITION] &
                        st->ram[SMBC_PLAYER_Y_HIGH_POS]);
  if (a != 0x00u) return;
  st->ram[SMBC_SCROLL_LOCK] = a;
  st->ram[SMBC_WARP_ZONE_CONTROL]++;
  erase_enemy_object(st, x);
}
