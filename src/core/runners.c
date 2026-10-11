/* core/runners.c — see core/runners.h for the contract. */
#include "runners.h"

#include "enemies.h"

/* smb.asm's enemy object constants used here */
#define SMBC_PIRANHA_PLANT 0x0du        /* smb.asm: PiranhaPlant */
#define SMBC_HAMMER_BRO 0x05u           /* smb.asm: HammerBro */
#define SMBC_FLYING_CHEEP_CHEEP 0x14u   /* smb.asm: FlyingCheepCheep */
#define SMBC_FLAGPOLE_FLAG_OBJECT 0x30u /* smb.asm: FlagpoleFlagObject */
#define SMBC_STAR_FLAG_OBJECT 0x31u     /* smb.asm: StarFlagObject */
#define SMBC_JUMPSPRING_OBJECT 0x32u    /* smb.asm: JumpspringObject */

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
