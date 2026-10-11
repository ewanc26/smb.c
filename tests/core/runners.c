/* tests/core/runners.c — exercises src/core/runners.c: the offscreen
 * bounds check. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/runners.h"

/* screen spanning $0020-$0110 */
static void set_screen(Smbc *smbc) {
  smbc->state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x00u;
  smbc->state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x20u;
  smbc->state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x01u;
  smbc->state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0x10u;
}

static void set_enemy(Smbc *smbc, uint8_t id, uint8_t state, uint8_t page,
                      uint8_t x) {
  smbc->state.ram[SMBC_ENEMY_ID + 0] = id;
  smbc->state.ram[SMBC_ENEMY_STATE + 0] = state;
  smbc->state.ram[SMBC_ENEMY_FLAG + 0] = 0x01u;
  smbc->state.ram[SMBC_ENEMY_PAGE_LOC + 0] = page;
  smbc->state.ram[SMBC_ENEMY_X_POSITION + 0] = x;
}

static uint8_t erased(const Smbc *smbc) {
  return (uint8_t)(smbc->state.ram[SMBC_ENEMY_FLAG + 0] == 0x00u);
}

static void onscreen_survives(void) {
  /* an object within the bounds is left alone; the left bound wraps
   * below zero here, exercising the page borrow */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  set_enemy(&smbc, 0x06u, 0x01u, 0x00u, 0x80u); /* green koopa troopa */
  offscreen_bounds_check(&smbc.state, 0x00u);
  assert(!erased(&smbc));
}

static void too_far_left_erased(void) {
  /* a goomba seventy-three pixels left of the left edge is erased:
   * identifiers below $0d subtract $49 */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  set_enemy(&smbc, 0x06u, 0x01u, 0x00u, 0x00u);
  smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x80u; /* $80 - $49 = $37 */
  offscreen_bounds_check(&smbc.state, 0x00u);
  assert(erased(&smbc));
}

static void hammer_bro_limit(void) {
  /* hammer bros add $39 and then subtract $49 (the adc did not
   * overflow): with the left edge at $80 the bound is $70, sixteen
   * pixels back */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x80u;
  set_enemy(&smbc, 0x05u, 0x01u, 0x00u, 0x6fu); /* one past the bound */
  offscreen_bounds_check(&smbc.state, 0x00u);
  assert(erased(&smbc));

  smbc_init(&smbc);
  set_screen(&smbc);
  smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x80u;
  set_enemy(&smbc, 0x05u, 0x01u, 0x00u, 0x70u); /* exactly at the bound */
  offscreen_bounds_check(&smbc.state, 0x00u);
  assert(!erased(&smbc));
}

static void too_far_right_erased(void) {
  /* past the right edge plus the margin (the adc adds $49 when the
   * left subtraction borrowed) an ordinary object is erased */
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  set_enemy(&smbc, 0x06u, 0x01u, 0x01u, 0xa0u); /* right bound: $159 */
  offscreen_bounds_check(&smbc.state, 0x00u);
  assert(erased(&smbc));
}

static void right_side_exemptions(void) {
  /* the spiny's egg state, piranha plants, flagpole flags, star
   * flags and jumpsprings are never erased on the right */
  static const struct {
    uint8_t id;
    uint8_t state;
  } cases[] = {
      {0x09u, 0x05u}, /* spiny in the egg state */
      {0x0du, 0x01u}, /* piranha plant */
      {0x30u, 0x01u}, /* flagpole flag */
      {0x31u, 0x01u}, /* star flag */
      {0x32u, 0x01u}, /* jumpspring */
  };
  for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    Smbc smbc;
    smbc_init(&smbc);
    set_screen(&smbc);
    set_enemy(&smbc, cases[i].id, cases[i].state, 0x01u, 0xa0u);
    offscreen_bounds_check(&smbc.state, 0x00u);
    assert(!erased(&smbc));
  }
}

static void flying_cheep_cheep_exempt(void) {
  Smbc smbc;
  smbc_init(&smbc);
  set_screen(&smbc);
  set_enemy(&smbc, 0x14u, 0x01u, 0x00u, 0x00u); /* far left */
  offscreen_bounds_check(&smbc.state, 0x00u);
  assert(!erased(&smbc));
}

int main(void) {
  onscreen_survives();
  too_far_left_erased();
  hammer_bro_limit();
  too_far_right_erased();
  right_side_exemptions();
  flying_cheep_cheep_exempt();
  puts("tests/core/runners: ok");
  return 0;
}
