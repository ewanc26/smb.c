/* core/move.c — see core/move.h for the contract. */
#include "move.h"

/* smb.asm: MoveObjectHorizontally — the generic horizontal mover,
 * applied here to an enemy object. The speed's low nybble feeds the
 * fractional movement force counter; the high nybble (sign-extended)
 * is the whole-pixel adder, and the page location absorbs the carry */
uint8_t move_enemy_horizontally(CoreState *st, uint8_t x) {
  uint8_t speed = st->ram[SMBC_ENEMY_X_SPEED + x];
  uint8_t low = (uint8_t)(speed << 4);
  uint8_t high = (uint8_t)(speed >> 4);
  if (high >= 0x08u) high |= 0xf0u;
  uint8_t page_add = (high & 0x80u) != 0x00u ? 0xffu : 0x00u;
  uint16_t force = (uint16_t)st->ram[SMBC_ENEMY_X_MOVE_FORCE + x] + low;
  st->ram[SMBC_ENEMY_X_MOVE_FORCE + x] = (uint8_t)force;
  uint8_t carry = (uint8_t)(force >> 8);
  uint16_t pos = (uint16_t)st->ram[SMBC_ENEMY_X_POSITION + x] + high + carry;
  st->ram[SMBC_ENEMY_X_POSITION + x] = (uint8_t)pos;
  st->ram[SMBC_ENEMY_PAGE_LOC + x] =
      (uint8_t)(st->ram[SMBC_ENEMY_PAGE_LOC + x] + page_add +
                (uint8_t)(pos >> 8));
  return (uint8_t)(high + carry);
}

/* smb.asm: ImposeGravity — the vertical physics core. The movement
 * force dummy absorbs the force counter, the speed (plus that carry)
 * moves the object, then the force counter grows by the downward
 * amount and the speed is clamped to the maximum. When the up flag is
 * set the upward amount (twice the downward) undoes the growth and
 * the speed is clamped to the negative maximum instead */
static void impose_gravity(CoreState *st, uint8_t x, uint8_t down,
                           uint8_t up, uint8_t max, uint8_t up_flag) {
  uint16_t dummy = (uint16_t)st->ram[SMBC_ENEMY_YMF_DUMMY + x] +
                   st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x];
  st->ram[SMBC_ENEMY_YMF_DUMMY + x] = (uint8_t)dummy;
  uint8_t carry = (uint8_t)(dummy >> 8);
  uint8_t add = ((int8_t)st->ram[SMBC_ENEMY_Y_SPEED + x] < 0) ? 0xffu : 0x00u;
  uint16_t pos = (uint16_t)st->ram[SMBC_ENEMY_Y_POSITION + x] +
                 st->ram[SMBC_ENEMY_Y_SPEED + x] + carry;
  st->ram[SMBC_ENEMY_Y_POSITION + x] = (uint8_t)pos;
  st->ram[SMBC_ENEMY_Y_HIGH_POS + x] =
      (uint8_t)(st->ram[SMBC_ENEMY_Y_HIGH_POS + x] + add +
                (uint8_t)(pos >> 8));
  uint16_t force = (uint16_t)st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] + down;
  st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] = (uint8_t)force;
  uint8_t speed =
      (uint8_t)(st->ram[SMBC_ENEMY_Y_SPEED + x] + (uint8_t)(force >> 8));
  st->ram[SMBC_ENEMY_Y_SPEED + x] = speed;
  if ((int8_t)speed >= (int8_t)max &&
      st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] >= 0x80u) {
    st->ram[SMBC_ENEMY_Y_SPEED + x] = max;
    st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] = 0x00u;
  }
  if (up_flag == 0x00u) return;
  uint8_t negmax = (uint8_t)(~(max)+1u);
  uint8_t sub =
      (uint16_t)st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] < (uint16_t)up ? 1u : 0u;
  st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] =
      (uint8_t)(st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] - up);
  speed = (uint8_t)(st->ram[SMBC_ENEMY_Y_SPEED + x] - sub);
  st->ram[SMBC_ENEMY_Y_SPEED + x] = speed;
  if ((int8_t)speed < (int8_t)negmax &&
      st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] < 0x80u) {
    st->ram[SMBC_ENEMY_Y_SPEED + x] = negmax;
    st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] = 0xffu;
  }
}

/* smb.asm: ImposeGravitySprObj — the downwards-only entry */
static void impose_gravity_spr_obj(CoreState *st, uint8_t x, uint8_t down,
                                   uint8_t max) {
  impose_gravity(st, x, down, 0x00u, max, 0x00u);
}

void move_d_enemy_vertically(CoreState *st, uint8_t x) {
  uint8_t down = 0x3du;
  if (st->ram[SMBC_ENEMY_STATE + x] == 0x05u) down = 0x20u;
  impose_gravity_spr_obj(st, x, down, 0x03u);
}

void move_falling_platform(CoreState *st, uint8_t x) {
  impose_gravity_spr_obj(st, x, 0x20u, 0x03u);
}

void move_j_enemy_vertically(CoreState *st, uint8_t x) {
  impose_gravity_spr_obj(st, x, 0x1cu, 0x03u);
}

void move_drop_platform(CoreState *st, uint8_t x) {
  impose_gravity_spr_obj(st, x, 0x7fu, 0x02u);
}

void move_enemy_slow_vert(CoreState *st, uint8_t x) {
  impose_gravity_spr_obj(st, x, 0x0fu, 0x02u);
}

/* smb.asm: MoveRedPTroopaDown / MoveRedPTroopaUp / MoveRedPTroopa,
 * which falls into RedPTroopaGrav */
void move_red_p_troopa(CoreState *st, uint8_t x, uint8_t dir) {
  impose_gravity(st, x, 0x03u, 0x06u, 0x02u, dir);
}

/* smb.asm: MovePlatformDown / MovePlatformUp, which fall into
 * RedPTroopaGrav. The asm's comparison against object $29 is residual
 * (that object never reaches this code) and is not ported */
void move_platform_vert(CoreState *st, uint8_t x, uint8_t dir) {
  impose_gravity(st, x, 0x05u, 0x0au, 0x03u, dir);
}
