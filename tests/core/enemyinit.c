/* tests/core/enemyinit.c — exercises src/core/enemyinit.c: the
 * InitEnemyRoutines jump table and its per-identifier initialisers. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/enemies.h"
#include "../../src/core/enemyinit.h"
#include "../../src/core/frame.h"

static void init_at(Smbc *smbc, uint8_t x, uint8_t id) {
  smbc_init(smbc);
  smbc->state.ram[SMBC_ENEMY_ID + x] = id;
}

static void normal_enemy_speed(void) {
  /* smb.asm: InitNormalEnemy — $f8 normally, $f4 in primary hard mode,
   * with the tall bounding box, direction left and vertical reset */
  Smbc smbc;
  init_at(&smbc, 0x01u, 0x00u);
  smbc.state.ram[SMBC_ENEMY_Y_SPEED + 1] = 0x55u;
  run_init_enemy_routines(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 1] == 0xf8u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 1] == 0x03u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 1] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 1] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 1] == 0x00u);

  init_at(&smbc, 0x01u, 0x00u);
  smbc.state.ram[SMBC_PRIMARY_HARD_MODE] = 0x01u;
  run_init_enemy_routines(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 1] == 0xf4u);
}

static void goomba_small_bbox(void) {
  /* smb.asm: InitGoomba — normal speed, then the small bounding box */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x06u);
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xf8u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x09u);
}

static void red_koopa_state(void) {
  /* smb.asm: InitRedKoopa — normal init plus state one */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x03u);
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xf8u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0x01u);
}

static void hammer_bro_timer(void) {
  /* smb.asm: InitHammerBro — timer from the secondary hard mode table */
  Smbc smbc;
  init_at(&smbc, 0x02u, 0x05u);
  run_init_enemy_routines(&smbc.state, 0x02u);
  assert(smbc.state.ram[SMBC_HAMMER_THROWING_TIMER + 2] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 2] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_INTERVAL_TIMER + 2] == 0x80u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 2] == 0x0bu);

  init_at(&smbc, 0x02u, 0x05u);
  smbc.state.ram[SMBC_SECONDARY_HARD_MODE] = 0x01u;
  run_init_enemy_routines(&smbc.state, 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_INTERVAL_TIMER + 2] == 0x50u);
}

static void bloober_and_bullet_bill(void) {
  /* smb.asm: InitBloober / InitBulletBill — the bill sets direction and
   * bounding box only, leaving the vertical values alone */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x07u);
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_BLOOPER_MOVE_SPEED + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x09u);

  init_at(&smbc, 0x00u, 0x08u);
  smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] = 0x55u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x09u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0x55u);
}

static void cheep_cheep_lsfr(void) {
  /* smb.asm: InitCheepCheep — d4 of the slot's LSFR byte becomes the
   * movement flag; the vertical coordinate is the origin */
  Smbc smbc;
  init_at(&smbc, 0x01u, 0x0au);
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1] = 0xffu;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 1] = 0x60u;
  run_init_enemy_routines(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_CHEEP_CHEEP_MOVE_M_FLAG + 1] == 0x10u);
  assert(smbc.state.ram[SMBC_CHEEP_CHEEP_ORIG_Y_POS + 1] == 0x60u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 1] == 0x09u);

  init_at(&smbc, 0x01u, 0x0bu);
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1] = 0xefu;
  run_init_enemy_routines(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_CHEEP_CHEEP_MOVE_M_FLAG + 1] == 0x00u);
}

static void jump_gp_troopa(void) {
  /* smb.asm: InitJumpGPTroopa — TallBBox2 leaves the vertical speed */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x0eu);
  smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] = 0x55u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xf8u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x03u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0x55u);
}

static void red_p_troopa_centre(void) {
  /* smb.asm: InitRedPTroopa — the centre sits $30 below the origin, or
   * $20 above (via $e0) when the origin is in the lower half */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x0fu);
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x20u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_RED_PTROOPA_ORIG_X_POS + 0] == 0x20u);
  assert(smbc.state.ram[SMBC_RED_PTROOPA_CENTER_Y_POS + 0] == 0x50u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x03u);

  init_at(&smbc, 0x00u, 0x0fu);
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x90u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_RED_PTROOPA_CENTER_Y_POS + 0] == 0x70u);
}

static void podoboo(void) {
  /* smb.asm: InitPodoboo — below the screen, interval timer one */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x0cu);
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_INTERVAL_TIMER + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x09u);
}

static void piranha_plant(void) {
  /* smb.asm: InitPiranhaPlant — the up position is 24 pixels above */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x0du);
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x40u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_PIRANHA_PLANT_Y_SPEED + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_PIRANHA_PLANT_MOVE_FLAG + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_PIRANHA_PLANT_DOWN_Y_POS + 0] == 0x40u);
  assert(smbc.state.ram[SMBC_PIRANHA_PLANT_UP_Y_POS + 0] == 0x28u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x09u);
}

static void retainer_obj(void) {
  /* smb.asm: InitRetainerObj */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x35u);
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0xb8u);
}

static void lakitu_setup_and_kill(void) {
  /* smb.asm: InitLakitu — a live frenzy buffer erases the slot,
   * otherwise the reappear timer clears and the flyer init runs */
  Smbc smbc;
  init_at(&smbc, 0x01u, 0x11u);
  smbc.state.ram[SMBC_LAKITU_REAPPEAR_TIMER] = 0x09u;
  run_init_enemy_routines(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_LAKITU_REAPPEAR_TIMER] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 1] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 1] == 0x03u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 1] == 0x11u);

  init_at(&smbc, 0x01u, 0x11u);
  smbc.state.ram[SMBC_ENEMY_FLAG + 1] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_FRENZY_BUFFER] = 0x12u;
  run_init_enemy_routines(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 1] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 1] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FRAME_TIMER + 1] == 0x00u);
}

static void short_firebar(void) {
  /* smb.asm: InitShortFirebar — four pixels down and right, the carry
   * moving the page; spin data indexed by identifier minus $1b */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x1du);
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0xfeu;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x20u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] = 0x01u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FIREBAR_SPIN_STATE_LOW + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_FIREBAR_SPIN_SPEED + 0] == 0x28u);
  assert(smbc.state.ram[SMBC_FIREBAR_SPIN_DIRECTION + 0] == 0x10u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x24u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x03u);
}

static void long_firebar_duplicates(void) {
  /* smb.asm: InitLongFirebar — the first empty slot gets a copy with
   * the original's offset ORed into d7 of its flag */
  Smbc smbc;
  init_at(&smbc, 0x02u, 0x1fu);
  smbc.state.ram[SMBC_ENEMY_FLAG + 2] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 2] = 0x30u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 2] = 0x40u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 2] = 0x01u;
  run_init_enemy_routines(&smbc.state, 0x02u);
  assert(smbc.state.ram[SMBC_DUPLICATE_OBJ_OFFSET] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x82u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x30u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x40u);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 2] == 0x01u);
  assert(smbc.state.ram[SMBC_FIREBAR_SPIN_SPEED + 2] == 0x28u);
}

static void bal_platform_chain(void) {
  /* smb.asm: InitBalPlatform — the alignment alternates between $ff
   * and the previous platform's slot offset */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x24u);
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x50u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x20u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] = 0x01u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x4eu);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0x00u); /* old alignment */
  assert(smbc.state.ram[SMBC_BAL_PLATFORM_ALIGNMENT] == 0xffu);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x20u); /* -8 then +8 */
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x06u);

  smbc.state.ram[SMBC_ENEMY_ID + 1] = 0x24u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 1] = 0x50u;
  run_init_enemy_routines(&smbc.state, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 1] == 0xffu);
  assert(smbc.state.ram[SMBC_BAL_PLATFORM_ALIGNMENT] == 0x01u);
}

static void vert_platform_mirror(void) {
  /* smb.asm: InitVertPlatform — the top mirrors around the centre */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x25u);
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0x30u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_Y_PLATFORM_TOP_Y_POS + 0] == 0x30u);
  assert(smbc.state.ram[SMBC_Y_PLATFORM_CENTER_Y_POS + 0] == 0x70u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x06u);

  init_at(&smbc, 0x00u, 0x25u);
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] = 0xa0u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_Y_PLATFORM_TOP_Y_POS + 0] == 0x60u);
  assert(smbc.state.ram[SMBC_Y_PLATFORM_CENTER_Y_POS + 0] == 0x60u);
}

static void platform_bbox_rules(void) {
  /* smb.asm: SPBBox — $06 outside castles with secondary hard mode
   * clear, $05 otherwise */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x29u); /* drop platform */
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_PLATFORM_COLLISION_FLAG + 0] == 0xffu);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x06u);

  init_at(&smbc, 0x00u, 0x28u); /* horizontal platform, castle */
  smbc.state.ram[SMBC_AREA_TYPE] = 0x03u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_X_MOVE_SECONDARY_COUNTER + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x05u);

  init_at(&smbc, 0x00u, 0x28u); /* secondary hard mode */
  smbc.state.ram[SMBC_SECONDARY_HARD_MODE] = 0x01u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x05u);
}

static void small_lifts(void) {
  /* smb.asm: PlatLiftUp / PlatLiftDown — twelve pixels across, small
   * bounding box */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x2bu);
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x20u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x10u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0xffu);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x2cu);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x04u);

  init_at(&smbc, 0x00u, 0x2cu);
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x20u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0xf0u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x2cu);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x04u);
}

static void large_lifts(void) {
  /* smb.asm: LargeLiftUp / LargeLiftDown — the platform bounding box
   * overwrites the small one */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x26u);
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x20u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0xffu);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x06u);

  init_at(&smbc, 0x00u, 0x27u);
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] = 0x20u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0xf0u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x06u);
}

static void bowser_pair(void) {
  /* smb.asm: InitBowser — the duplicate is the second sprite; the
   * original offset becomes the front offset */
  Smbc smbc;
  init_at(&smbc, 0x02u, 0x2du);
  smbc.state.ram[SMBC_ENEMY_FLAG + 2] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 2] = 0x80u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 2] = 0x40u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 2] = 0x03u;
  run_init_enemy_routines(&smbc.state, 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x82u);
  assert(smbc.state.ram[SMBC_BOWSER_FRONT_OFFSET] == 0x02u);
  assert(smbc.state.ram[SMBC_BOWSER_BODY_CONTROLS] == 0x00u);
  assert(smbc.state.ram[SMBC_BRIDGE_COLLAPSE_OFFSET] == 0x00u);
  assert(smbc.state.ram[SMBC_BOWSER_ORIG_X_POS] == 0x80u);
  assert(smbc.state.ram[SMBC_BOWSER_FIRE_BREATH_TIMER] == 0xdfu);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 2] == 0xdfu);
  assert(smbc.state.ram[SMBC_BOWSER_FEET_COUNTER] == 0x20u);
  assert(smbc.state.ram[SMBC_ENEMY_FRAME_TIMER + 2] == 0x20u);
  assert(smbc.state.ram[SMBC_BOWSER_HIT_POINTS] == 0x05u);
  assert(smbc.state.ram[SMBC_BOWSER_MOVEMENT_SPEED] == 0x02u);
}

static void power_up_jump(void) {
  /* smb.asm: PwrUpJmp — slot five directly; the type follows the
   * player's status below star/1-up, fiery forcing the flower */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x2eu);
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 5] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 5] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 5] == 0x03u);
  assert(smbc.state.ram[SMBC_POWER_UP_TYPE] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_SPR_ATTRIB + 5] == 0x20u);
  assert(smbc.state.ram[SMBC_SQUARE2_SOUND_QUEUE] == 0x02u);

  init_at(&smbc, 0x00u, 0x2eu);
  smbc.state.ram[SMBC_PLAYER_STATUS] = 0x02u; /* fiery */
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_POWER_UP_TYPE] == 0x01u);

  init_at(&smbc, 0x00u, 0x2eu);
  smbc.state.ram[SMBC_POWER_UP_TYPE] = 0x02u; /* star: untouched */
  smbc.state.ram[SMBC_PLAYER_STATUS] = 0x01u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_POWER_UP_TYPE] == 0x02u);
}

static void vine_setup(void) {
  /* smb.asm: Setup_Vine — position from the block buffer, the start
   * height stored only for the first vine */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_BLOCK_PAGE_LOC + 1] = 0x02u;
  smbc.state.ram[SMBC_BLOCK_X_POSITION + 1] = 0x30u;
  smbc.state.ram[SMBC_BLOCK_Y_POSITION + 1] = 0x50u;
  setup_vine(&smbc.state, 0x03u, 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 3] == 0x2fu);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 3] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 3] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 3] == 0x30u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 3] == 0x50u);
  assert(smbc.state.ram[SMBC_VINE_START_Y_POSITION] == 0x50u);
  assert(smbc.state.ram[SMBC_VINE_OBJ_OFFSET + 0] == 0x03u);
  assert(smbc.state.ram[SMBC_VINE_FLAG_OFFSET] == 0x01u);
  assert(smbc.state.ram[SMBC_SQUARE2_SOUND_QUEUE] == 0x04u);

  /* a second vine leaves the start height alone */
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 4] = 0x00u;
  setup_vine(&smbc.state, 0x04u, 0x01u);
  assert(smbc.state.ram[SMBC_VINE_START_Y_POSITION] == 0x50u);
  assert(smbc.state.ram[SMBC_VINE_OBJ_OFFSET + 1] == 0x04u);
  assert(smbc.state.ram[SMBC_VINE_FLAG_OFFSET] == 0x02u);
}

static void end_frenzy_wakes_lakitu(void) {
  /* smb.asm: EndFrenzy — any lakitu on the slots gets state one; the
   * buffer empties and this object disables */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x18u);
  smbc.state.ram[SMBC_ENEMY_FLAG + 0] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_ID + 3] = 0x11u;
  smbc.state.ram[SMBC_ENEMY_FRENZY_BUFFER] = 0x12u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 3] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_FRENZY_BUFFER] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x00u);
}

static void no_init_leaves_everything(void) {
  /* smb.asm: NoInitCode */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x04u);
  smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] = 0x55u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x55u);
}

static void frenzy_buffer_and_timer(void) {
  /* smb.asm: InitEnemyFrenzy — the identifier always goes into the
   * frenzy buffer; a live timer stops the generators there */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x12u);
  smbc.state.ram[SMBC_FRENZY_ENEMY_TIMER] = 0x30u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FRENZY_BUFFER] == 0x12u);
  assert(smbc.state.ram[SMBC_FRENZY_ENEMY_TIMER] == 0x30u);
  assert(smbc.state.ram[SMBC_LAKITU_REAPPEAR_TIMER] == 0x00u);

  /* smb.asm: LakituAndSpinyHandler — the timer sets to $80 and the
   * reappearance timer counts up while lakitu is absent */
  init_at(&smbc, 0x00u, 0x12u);
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FRENZY_ENEMY_TIMER] == 0x80u);
  assert(smbc.state.ram[SMBC_LAKITU_REAPPEAR_TIMER] == 0x01u);

  /* the special use slot is excluded */
  init_at(&smbc, 0x05u, 0x12u);
  run_init_enemy_routines(&smbc.state, 0x05u);
  assert(smbc.state.ram[SMBC_FRENZY_ENEMY_TIMER] == 0x00u);
}

static void lakitu_respawn(void) {
  /* smb.asm: LakituAndSpinyHandler — after seven counts lakitu is
   * created in the first empty slot from the top, at the right
   * extent with the FinishFlame tail */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x12u);
  smbc.state.ram[SMBC_LAKITU_REAPPEAR_TIMER] = 0x06u;
  smbc.state.ram[SMBC_ENEMY_FLAG + 4] = 0x01u; /* slot 4 busy, 3 free */
  smbc.state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0xf0u;
  smbc.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x01u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 3] == 0x11u);
  assert(smbc.state.ram[SMBC_LAKITU_REAPPEAR_TIMER] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 3] == 0x20u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 3] == 0x10u);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 3] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 3] == 0x08u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 3] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 3] == 0x00u);
}

static void spiny_hatch(void) {
  /* smb.asm: CreateSpiny — the frenzy object's own slot becomes the
   * spiny, eight pixels above lakitu */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x12u);
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x60u;
  smbc.state.ram[SMBC_ENEMY_ID + 2] = 0x11u; /* lakitu in slot 2 */
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 2] = 0x70u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 2] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 2] = 0x50u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x70u);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x48u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x09u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0xfdu);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0x05u);

  /* a player above $2c stops the throw */
  init_at(&smbc, 0x00u, 0x12u);
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x20u;
  smbc.state.ram[SMBC_ENEMY_ID + 2] = 0x11u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x00u);

  /* a non-normal lakitu state stops the throw */
  init_at(&smbc, 0x00u, 0x12u);
  smbc.state.ram[SMBC_PLAYER_Y_POSITION] = 0x60u;
  smbc.state.ram[SMBC_ENEMY_ID + 2] = 0x11u;
  smbc.state.ram[SMBC_ENEMY_STATE + 2] = 0x01u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x00u);
}

static void flying_cheep_cheep(void) {
  /* smb.asm: InitFlyingCheepCheep — timer, speed and position from the
   * LSFR parts and the player's motion */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x14u);
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 0] = 0x00u;
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1] = 0x01u;
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 2] = 0x05u;
  smbc.state.ram[SMBC_PLAYER_X_POSITION] = 0x80u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FRENZY_ENEMY_TIMER] == 0x60u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_SPEED + 0] == 0xfbu);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0x0eu);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x01u);
  /* $00 = 5 from the third LSFR part; d1 clear so the position
   * subtracts FlyCCXPositionData[5] = $50 from the player's $80 */
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x30u);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0xf8u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x09u);

  /* the slot cap: three onscreen without secondary hard mode */
  init_at(&smbc, 0x03u, 0x14u);
  run_init_enemy_routines(&smbc.state, 0x03u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 3] == 0x00u);

  /* secondary hard mode allows the fourth slot */
  init_at(&smbc, 0x03u, 0x14u);
  smbc.state.ram[SMBC_SECONDARY_HARD_MODE] = 0x01u;
  run_init_enemy_routines(&smbc.state, 0x03u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 3] == 0x01u);
}

static void bowser_flame_from_mouth(void) {
  /* smb.asm: InitBowserFlame / SpawnFromMouth — fourteen pixels ahead
   * of bowser, eight below, with the movement force pulling toward the
   * pseudorandom target height */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x15u);
  smbc.state.ram[SMBC_BOWSER_FRONT_OFFSET] = 0x02u;
  smbc.state.ram[SMBC_ENEMY_ID + 2] = 0x2du;
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 2] = 0x60u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 2] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_Y_POSITION + 2] = 0x40u;
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 0] = 0x03u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x52u);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x48u);
  assert(smbc.state.ram[SMBC_ENEMY_YMF_DUMMY + 0] == 0x03u);
  /* target $90 >= current $48, so the force pulls down */
  assert(smbc.state.ram[SMBC_ENEMY_Y_MOVE_FORCE + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_FRENZY_BUFFER] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x08u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_NOISE_SOUND_QUEUE] == 0x02u);
}

static void bowser_flame_from_extent(void) {
  /* smb.asm: InitBowserFlame — without bowser, the flame waits on the
   * timer table then spawns at the right extent */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x15u);
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 0] = 0x01u;
  smbc.state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0xf0u;
  smbc.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x01u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FRENZY_ENEMY_TIMER] == 0xdfu); /* $bf+$20 */
  assert(smbc.state.ram[SMBC_BOWSER_FLAME_TIMER_CTRL] == 0x01u);
  assert(smbc.state.ram[SMBC_BOWSER_FLAME_PRANDOM_OFS + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x80u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x10u);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x08u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);

  /* secondary hard mode shortens the timer by sixteen */
  init_at(&smbc, 0x00u, 0x15u);
  smbc.state.ram[SMBC_SECONDARY_HARD_MODE] = 0x01u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FRENZY_ENEMY_TIMER] == 0xcfu);
}

static void fireworks(void) {
  /* smb.asm: InitFireworks — the counter indexes the offset tables;
   * the position anchors forty-eight pixels left of the star flag */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x16u);
  smbc.state.ram[SMBC_FIREWORKS_COUNTER] = 0x06u;
  smbc.state.ram[SMBC_ENEMY_ID + 1] = 0x31u; /* star flag object */
  smbc.state.ram[SMBC_ENEMY_X_POSITION + 1] = 0x90u;
  smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 1] = 0x01u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_FRENZY_ENEMY_TIMER] == 0x20u);
  assert(smbc.state.ram[SMBC_FIREWORKS_COUNTER] == 0x05u);
  /* $190 - $30 + FireworksXPosData[5] ($20) = $180 */
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x80u);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x30u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_EXPLOSION_GFX_COUNTER + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_EXPLOSION_TIMER_COUNTER + 0] == 0x08u);

  /* no star flag object: the port leaves instead of the asm's spin */
  init_at(&smbc, 0x00u, 0x16u);
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x00u);
}

static void bullet_bill_on_land(void) {
  /* smb.asm: BulletBillCheepCheep — outside water, one frenzy bullet
   * bill at a time, with the blast sound and the identifier
   * checkpoint's own init running afterwards */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x17u);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 0] = 0x00u;
  smbc.state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0x20u;
  smbc.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = 0x00u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x08u);
  assert(smbc.state.ram[SMBC_SQUARE2_SOUND_QUEUE] == 0x08u);
  assert(smbc.state.ram[SMBC_BITM_FILTER] == 0x01u);
  /* Enemy17YPosData[0] = $40, then eight pixels from the checkpoint */
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x48u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x40u);
  assert(smbc.state.ram[SMBC_ENEMY_OFFSCR_BITS_MASKED + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x09u);
  assert(smbc.state.ram[SMBC_FRENZY_ENEMY_TIMER] == 0x20u);

  /* a live frenzy bullet bill blocks another */
  init_at(&smbc, 0x00u, 0x17u);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_FLAG + 2] = 0x01u;
  smbc.state.ram[SMBC_ENEMY_ID + 2] = 0x08u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x17u);
  assert(smbc.state.ram[SMBC_SQUARE2_SOUND_QUEUE] == 0x00u);
}

static void swim_cheep_cheep_in_water(void) {
  /* smb.asm: BulletBillCheepCheep — water levels spawn a swimming
   * cheep-cheep, the identifier from the LSFR and the world number */
  Smbc smbc;
  init_at(&smbc, 0x00u, 0x17u);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x00u;
  smbc.state.ram[SMBC_PSEUDO_RANDOM_BIT_REG + 0] = 0xaau;
  smbc.state.ram[SMBC_SCREEN_RIGHT_X_POS] = 0x00u;
  run_init_enemy_routines(&smbc.state, 0x00u);
  /* $aa >= $aa and not world two: y = 2, masked to 0 — first id */
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x0au);
  /* $aa & 7 = 2: Enemy17YPosData[2] = $90, then eight from the
   * checkpoint */
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0x98u);
  assert(smbc.state.ram[SMBC_BITM_FILTER] == 0x04u);
  /* then the cheep-cheep init ran through the checkpoint */
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x09u);
  assert(smbc.state.ram[SMBC_CHEEP_CHEEP_ORIG_Y_POS + 0] == 0x98u);

  /* past the third slot in water: nothing */
  init_at(&smbc, 0x03u, 0x17u);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u;
  run_init_enemy_routines(&smbc.state, 0x03u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 3] == 0x17u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 3] == 0x00u);
}

static void interpreter_runs_init(void) {
  /* integration: a goomba spawned by the interpreter now gets its
   * speed, bounding box and direction from the jump table */
  static const uint8_t stream[] = {0x29u, 0x06u, 0xffu};
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.enemy_data = stream;
  enemies_and_loops_core(&smbc.state, 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_SPEED + 0] == 0xf8u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x09u);
  assert(smbc.state.ram[SMBC_ENEMY_MOVING_DIR + 0] == 0x02u);
}

int main(void) {
  normal_enemy_speed();
  goomba_small_bbox();
  red_koopa_state();
  hammer_bro_timer();
  bloober_and_bullet_bill();
  cheep_cheep_lsfr();
  jump_gp_troopa();
  red_p_troopa_centre();
  podoboo();
  piranha_plant();
  retainer_obj();
  lakitu_setup_and_kill();
  short_firebar();
  long_firebar_duplicates();
  bal_platform_chain();
  vert_platform_mirror();
  platform_bbox_rules();
  small_lifts();
  large_lifts();
  bowser_pair();
  power_up_jump();
  vine_setup();
  end_frenzy_wakes_lakitu();
  no_init_leaves_everything();
  frenzy_buffer_and_timer();
  lakitu_respawn();
  spiny_hatch();
  flying_cheep_cheep();
  bowser_flame_from_mouth();
  bowser_flame_from_extent();
  fireworks();
  bullet_bill_on_land();
  swim_cheep_cheep_in_water();
  interpreter_runs_init();
  printf("tests/core/enemyinit: ok\n");
  return 0;
}
