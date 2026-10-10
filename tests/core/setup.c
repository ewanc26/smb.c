/* tests/core/setup.c — exercises src/core/setup.c: the secondary game
 * setup's page clear, flag resets and sprite seeding, and the area music
 * selection's override order. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/core/frame.h"
#include "../../src/core/setup.h"
#include "../../src/core/state.h"
#include "../../src/data/music.h"
#include "../../src/data/sprites.h"

int main(void) {
  /* SecondaryGameSetup */
  Smbc smbc;
  smbc_init(&smbc);
  /* dirty everything the setup must reset */
  memset(&smbc.state.ram[SMBC_VRAM_BUFFER1 - 1], 0xaa, 0x100u);
  smbc.state.ram[SMBC_GAME_TIMER_EXPIRED_FLAG] = 0x01u;
  smbc.state.ram[SMBC_DISABLE_INTERMEDIATE] = 0x01u;
  smbc.state.ram[SMBC_BACKLOADING_FLAG] = 0x01u;
  smbc.state.ram[SMBC_DISABLE_SCREEN_FLAG] = 0x01u;
  smbc.state.ram[SMBC_SCREEN_LEFT_PAGE_LOC] = 0x05u; /* odd page */
  smbc.state.ram[SMBC_MIRROR_PPU_CTRL1] = 0x90u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME; /* past boot's title */
  secondary_game_setup(&smbc);

  for (int i = 0; i < 0x100; i++) {
    /* smb.asm: BalPlatformAlignment at $03a0 lives in the cleared page and
     * is set to $ff right after the clear */
    uint8_t want = (i == 0xa0) ? 0xffu : 0x00u;
    assert(smbc.state.ram[SMBC_VRAM_BUFFER1 - 1 + i] == want);
  }
  assert(smbc.state.ram[SMBC_GAME_TIMER_EXPIRED_FLAG] == 0x00u);
  assert(smbc.state.ram[SMBC_DISABLE_INTERMEDIATE] == 0x00u);
  assert(smbc.state.ram[SMBC_BACKLOADING_FLAG] == 0x00u);
  assert(smbc.state.ram[SMBC_BAL_PLATFORM_ALIGNMENT] == 0xffu);
  /* nametable bit of the mirror follows the page location's d0 */
  assert(smbc.state.ram[SMBC_MIRROR_PPU_CTRL1] == 0x91u);
  /* music queued from the ground area type */
  assert(smbc.state.ram[SMBC_AREA_MUSIC_QUEUE] == SMBC_MUSIC_GROUND);
  /* sprite shuffler seeded */
  assert(smbc.state.ram[SMBC_SPR_SHUFFLE_AMT] == 0x58u);
  assert(smbc.state.ram[SMBC_SPR_SHUFFLE_AMT + 1] == 0x48u);
  assert(smbc.state.ram[SMBC_SPR_SHUFFLE_AMT + 2] == 0x38u);
  /* default offsets and sprite 0 template loaded */
  for (int i = 0; i < (int)SMBC_DEFAULT_SPR_OFFSETS_SIZE; i++) {
    assert(smbc.state.ram[SMBC_SPR_DATA_OFFSET + i] ==
           SMBC_DEFAULT_SPR_OFFSETS[i]);
  }
  for (int i = 0; i < (int)SMBC_SPRITE0_DATA_SIZE; i++) {
    assert(smbc.state.ram[SMBC_SPRITE_DATA + i] == SMBC_SPRITE0_DATA[i]);
  }
  assert(smbc.state.ram[0x06c9] == 0xffu); /* smb.asm: DoNothing1 */
  assert(smbc.state.ram[SMBC_SPRITE0_HIT_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_OPER_MODE_TASK] == 0x01u);

  /* GetAreaMusic: title screen leaves the queue alone */
  CoreState state;
  state_init(&state);
  state.ram[SMBC_AREA_MUSIC_QUEUE] = 0xaau;
  get_area_music(&state);
  assert(state.ram[SMBC_AREA_MUSIC_QUEUE] == 0xaau);

  /* game mode, alt entrance $02 with cloud override: cloud wins */
  state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  state.ram[SMBC_ALT_ENTRANCE_CONTROL] = 0x02u;
  state.ram[SMBC_AREA_TYPE] = 0x01u;
  state.ram[SMBC_CLOUD_TYPE_OVERRIDE] = 0x01u;
  get_area_music(&state);
  assert(state.ram[SMBC_AREA_MUSIC_QUEUE] == SMBC_MUSIC_CLOUD);

  /* pipe intro entrance ctrl $06 beats even the cloud override */
  state.ram[SMBC_ALT_ENTRANCE_CONTROL] = 0x00u;
  state.ram[SMBC_PLAYER_ENTRANCE_CTRL] = 0x06u;
  get_area_music(&state);
  assert(state.ram[SMBC_AREA_MUSIC_QUEUE] == SMBC_MUSIC_PIPE_INTRO);

  /* plain area types: water and castle */
  state.ram[SMBC_CLOUD_TYPE_OVERRIDE] = 0x00u;
  state.ram[SMBC_PLAYER_ENTRANCE_CTRL] = 0x00u; /* off the pipe intro */
  state.ram[SMBC_AREA_TYPE] = 0x00u;
  get_area_music(&state);
  assert(state.ram[SMBC_AREA_MUSIC_QUEUE] == SMBC_MUSIC_WATER);
  state.ram[SMBC_AREA_TYPE] = 0x03u;
  get_area_music(&state);
  assert(state.ram[SMBC_AREA_MUSIC_QUEUE] == SMBC_MUSIC_CASTLE);

  printf("tests/core/setup: ok\n");
  return 0;
}
