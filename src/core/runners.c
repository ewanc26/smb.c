/* core/runners.c — see core/runners.h for the contract. */
#include "runners.h"

#include "../data/objects.h"
#include "enemies.h"
#include "move.h"

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
