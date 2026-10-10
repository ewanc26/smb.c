/* tests/core/area.c — exercises src/core/area.c: GetAreaDataAddrs and
 * its stream resolution and header unpacking. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/area.h"
#include "../../src/data/areadata.h"
#include "../../src/core/frame.h"

static void world_1_1(void) {
  /* smb.asm: World1Areas[0] = $25 — type ground, offset five, into
   * E_GroundArea6 / L_GroundArea6 with the header $50 $21 */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x00u;
  smbc.state.ram[SMBC_AREA_NUMBER] = 0x00u;
  load_area_pointer(&smbc);
  get_area_data_addrs(&smbc);
  assert(smbc.state.ram[SMBC_AREA_POINTER] == 0x25u);
  assert(smbc.state.ram[SMBC_AREA_TYPE] == 0x01u);
  assert(smbc.state.ram[SMBC_AREA_ADDRS_L_OFFSET] == 0x05u);
  assert(smbc.state.enemy_data == SMBC_ENEMY_AREAS[11]);
  assert(smbc.state.area_data == SMBC_LEVEL_AREAS[8] + 2);
  assert(smbc.state.ram[SMBC_FOREGROUND_SCENERY] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_ENTRANCE_CTRL] == 0x02u);
  assert(smbc.state.ram[SMBC_GAME_TIMER_SETTING] == 0x01u);
  assert(smbc.state.ram[SMBC_TERRAIN_CONTROL] == 0x01u);
  assert(smbc.state.ram[SMBC_BACKGROUND_SCENERY] == 0x02u);
  assert(smbc.state.ram[SMBC_AREA_STYLE] == 0x00u);
  assert(smbc.state.ram[SMBC_CLOUD_TYPE_OVERRIDE] == 0x00u);
}

static void bg_colour_control(void) {
  /* smb.asm: World3Areas[2] = $20 (3-3) — L_GroundArea1's header byte
   * $94 has its low three bits at four, the background colour control */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x02u;
  smbc.state.ram[SMBC_AREA_NUMBER] = 0x02u;
  load_area_pointer(&smbc);
  get_area_data_addrs(&smbc);
  assert(smbc.state.ram[SMBC_AREA_POINTER] == 0x20u);
  assert(smbc.state.area_data == SMBC_LEVEL_AREAS[3] + 2);
  assert(smbc.state.ram[SMBC_BACKGROUND_COLOR_CTRL] == 0x04u);
  assert(smbc.state.ram[SMBC_FOREGROUND_SCENERY] == 0x00u);
  assert(smbc.state.ram[SMBC_PLAYER_ENTRANCE_CTRL] == 0x02u);
  assert(smbc.state.ram[SMBC_GAME_TIMER_SETTING] == 0x02u);
  assert(smbc.state.ram[SMBC_BACKGROUND_SCENERY] == 0x01u);
}

static void cloud_override(void) {
  /* smb.asm: the day cloud area's pointer $2b — L_GroundArea12's header
   * byte $c1 has style three, the cloud type override instead */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_POINTER] = 0x2bu;
  get_area_data_addrs(&smbc);
  assert(smbc.state.ram[SMBC_AREA_TYPE] == 0x01u);
  assert(smbc.state.area_data == SMBC_LEVEL_AREAS[14] + 2);
  assert(smbc.state.ram[SMBC_CLOUD_TYPE_OVERRIDE] == 0x03u);
  assert(smbc.state.ram[SMBC_AREA_STYLE] == 0x00u);
  assert(smbc.state.ram[SMBC_TERRAIN_CONTROL] == 0x01u);
}

static void world_8_4(void) {
  /* smb.asm: World8Areas[3] = $65 — castle type, the sixth castle */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x07u;
  smbc.state.ram[SMBC_AREA_NUMBER] = 0x03u;
  load_area_pointer(&smbc);
  get_area_data_addrs(&smbc);
  assert(smbc.state.ram[SMBC_AREA_POINTER] == 0x65u);
  assert(smbc.state.ram[SMBC_AREA_TYPE] == 0x03u);
  assert(smbc.state.enemy_data == SMBC_ENEMY_AREAS[5]);
  assert(smbc.state.area_data == SMBC_LEVEL_AREAS[33] + 2);
  assert(smbc.state.ram[SMBC_FOREGROUND_SCENERY] == 0x03u);
  assert(smbc.state.ram[SMBC_PLAYER_ENTRANCE_CTRL] == 0x03u);
  assert(smbc.state.ram[SMBC_TERRAIN_CONTROL] == 0x06u);
}

int main(void) {
  world_1_1();
  bg_colour_control();
  cloud_override();
  world_8_4();
  printf("tests/core/area: ok\n");
  return 0;
}
