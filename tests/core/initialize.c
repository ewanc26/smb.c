/* tests/core/initialize.c — exercises src/core/initialize.c: InitializeArea's
 * RAM clear, timer reset, page selection, screen/nametable setup, hard-mode
 * rule and mode-task advance. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/core/area.h"
#include "../../src/core/frame.h"
#include "../../src/core/initialize.h"
#include "../../src/core/state.h"
#include "../../src/data/music.h"

/* a live GameMode area with world 1-1's pointer ($25, ground type) */
static void entered(Smbc *smbc) {
  smbc_init(smbc);
  smbc->state.ram[SMBC_AREA_POINTER] = 0x25u;
  smbc->state.ram[SMBC_WORLD_NUMBER] = 0x00u;
  smbc->state.ram[SMBC_AREA_NUMBER] = 0x00u;
  smbc->state.ram[SMBC_OPER_MODE_TASK] = 0x00u;
}

static void get_screen_position_carries(void) {
  Smbc smbc;
  entered(&smbc);
  smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x00u;
  smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x05u;
  assert(get_screen_position(&smbc.state) == 0x05u);
  assert(smbc.state.ram[SMBC_SCREEN_RIGHT_X_POS] == 0xffu);
  assert(smbc.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] == 0x05u);

  smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] = 0x01u;
  assert(get_screen_position(&smbc.state) == 0x06u);
  assert(smbc.state.ram[SMBC_SCREEN_RIGHT_X_POS] == 0x00u);
  assert(smbc.state.ram[SMBC_SCREEN_RIGHT_PAGE_LOC] == 0x06u);
}

static void clears_memory_and_timers(void) {
  Smbc smbc;
  entered(&smbc);
  memset(smbc.state.ram, 0xff, sizeof smbc.state.ram);
  smbc.state.ram[SMBC_AREA_POINTER] = 0x25u;
  initialize_area(&smbc);
  /* working RAM below $074c is cleared */
  assert(smbc.state.ram[0x0000] == 0x00u);
  assert(smbc.state.ram[0x06ff] == 0x00u);
  assert(smbc.state.ram[0x0700] == 0x00u);
  assert(smbc.state.ram[0x074b] == 0x00u);
  /* timers $0780-$07a1 are cleared */
  assert(smbc.state.ram[SMBC_TIMERS] == 0x00u);
  assert(smbc.state.ram[SMBC_TIMERS + 0x21u] == 0x00u);
}

static void even_page_sets_plain_nametable(void) {
  Smbc smbc;
  entered(&smbc);
  /* halfway page $04 is even, so the right page is even and the name
   * table is the plain $2080 */
  smbc.state.ram[SMBC_HALFWAY_PAGE] = 0x04u;
  initialize_area(&smbc);
  assert(smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] == 0x04u);
  assert(smbc.state.ram[SMBC_CURRENT_PAGE_LOC] == 0x04u);
  assert(smbc.state.ram[SMBC_BACKLOADING_FLAG] == 0x04u);
  assert(smbc.state.ram[SMBC_SCREEN_LEFT_X_POS] == 0x00u);
  assert(smbc.state.ram[SMBC_CURRENT_NT_ADDR_HIGH] == 0x20u);
  assert(smbc.state.ram[SMBC_CURRENT_NT_ADDR_LOW] == 0x80u);
  assert(smbc.state.ram[SMBC_BLOCK_BUFFER_COLUMN_POS] == 0x00u);
}

static void odd_page_sets_upper_nametable(void) {
  Smbc smbc;
  entered(&smbc);
  smbc.state.ram[SMBC_HALFWAY_PAGE] = 0x05u; /* odd */
  initialize_area(&smbc);
  assert(smbc.state.ram[SMBC_CURRENT_NT_ADDR_HIGH] == 0x24u);
  assert(smbc.state.ram[SMBC_BLOCK_BUFFER_COLUMN_POS] == 0x10u);
}

static void saved_entry_page_overrides_halfway(void) {
  Smbc smbc;
  entered(&smbc);
  smbc.state.ram[SMBC_HALFWAY_PAGE] = 0x04u;
  smbc.state.ram[SMBC_ALT_ENTRANCE_CONTROL] = 0x01u;
  smbc.state.ram[SMBC_ENTRANCE_PAGE] = 0x07u;
  initialize_area(&smbc);
  assert(smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] == 0x07u);
  assert(smbc.state.ram[SMBC_CURRENT_PAGE_LOC] == 0x07u);
}

static void area_lengths_decrement_and_renderer_primed(void) {
  Smbc smbc;
  entered(&smbc);
  /* the working RAM is cleared first, so the three empty area objects
   * decrement from zero to $ff, and the renderer is primed for twelve
   * column sets */
  initialize_area(&smbc);
  assert(smbc.state.ram[SMBC_AREA_OBJECT_LENGTH] == 0xffu);
  assert(smbc.state.ram[SMBC_AREA_OBJECT_LENGTH + 1u] == 0xffu);
  assert(smbc.state.ram[SMBC_AREA_OBJECT_LENGTH + 2u] == 0xffu);
  assert(smbc.state.ram[SMBC_COLUMN_SETS] == 0x0bu);
}

static void halfway_forces_entrance_control(void) {
  Smbc smbc;
  entered(&smbc);
  smbc.state.ram[SMBC_HALFWAY_PAGE] = 0x04u;
  initialize_area(&smbc);
  assert(smbc.state.ram[SMBC_PLAYER_ENTRANCE_CTRL] == 0x02u);
  assert(smbc.state.ram[SMBC_AREA_MUSIC_QUEUE] == SMBC_MUSIC_SILENCE);
  assert(smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x01u);
}

static void no_halfway_leaves_entrance_control(void) {
  Smbc smbc;
  entered(&smbc);
  smbc.state.ram[SMBC_HALFWAY_PAGE] = 0x00u;
  /* world 1-1's header entrance control is two; the halfway overwrite
   * must not fire, so the header value stands */
  initialize_area(&smbc);
  assert(smbc.state.ram[SMBC_PLAYER_ENTRANCE_CTRL] == 0x02u);
}

static void secondary_hard_mode_from_world(void) {
  Smbc smbc;
  entered(&smbc);
  smbc.state.ram[SMBC_SECONDARY_HARD_MODE] = 0x00u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x04u; /* world 5 */
  smbc.state.ram[SMBC_LEVEL_NUMBER] = 0x02u; /* level 3 */
  initialize_area(&smbc);
  assert(smbc.state.ram[SMBC_SECONDARY_HARD_MODE] == 0x01u);
}

static void secondary_hard_mode_not_before_world_5(void) {
  Smbc smbc;
  entered(&smbc);
  smbc.state.ram[SMBC_SECONDARY_HARD_MODE] = 0x00u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x02u; /* world 3 */
  smbc.state.ram[SMBC_LEVEL_NUMBER] = 0x03u;
  initialize_area(&smbc);
  assert(smbc.state.ram[SMBC_SECONDARY_HARD_MODE] == 0x00u);
}

static void secondary_hard_mode_from_primary(void) {
  Smbc smbc;
  entered(&smbc);
  smbc.state.ram[SMBC_SECONDARY_HARD_MODE] = 0x00u;
  smbc.state.ram[SMBC_PRIMARY_HARD_MODE] = 0x01u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x00u; /* world 1-1 otherwise */
  initialize_area(&smbc);
  assert(smbc.state.ram[SMBC_SECONDARY_HARD_MODE] == 0x01u);
}

static void game_init_clears_and_seeds(void) {
  Smbc smbc;
  entered(&smbc);
  memset(smbc.state.ram, 0xff, sizeof smbc.state.ram);
  smbc.state.ram[SMBC_AREA_POINTER] = 0x25u;
  smbc.state.ram[SMBC_OPER_MODE_TASK] = 0x00u;
  initialize_game(&smbc);
  /* memory cleared as far as $076f */
  assert(smbc.state.ram[0x076f] == 0x00u);
  /* the stack at $0160-$01ff is skipped */
  assert(smbc.state.ram[0x0160] == 0xffu);
  assert(smbc.state.ram[0x01ff] == 0xffu);
  /* the sound memory $07b0-$07cf is cleared separately */
  assert(smbc.state.ram[SMBC_SOUND_MEMORY] == 0x00u);
  assert(smbc.state.ram[SMBC_SOUND_MEMORY + 0x1fu] == 0x00u);
  /* above the sound memory is untouched */
  assert(smbc.state.ram[SMBC_SOUND_MEMORY + 0x20u] == 0xffu);
  /* demo timer seeded for the title screen */
  assert(smbc.state.ram[SMBC_DEMO_TIMER] == 0x18u);
  /* fall-through into InitializeArea: the mode task advanced */
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x01u);
}

int main(void) {
  get_screen_position_carries();
  clears_memory_and_timers();
  even_page_sets_plain_nametable();
  odd_page_sets_upper_nametable();
  saved_entry_page_overrides_halfway();
  area_lengths_decrement_and_renderer_primed();
  halfway_forces_entrance_control();
  no_halfway_leaves_entrance_control();
  secondary_hard_mode_from_world();
  secondary_hard_mode_not_before_world_5();
  secondary_hard_mode_from_primary();
  game_init_clears_and_seeds();
  puts("tests/core/initialize: ok");
  return 0;
}
