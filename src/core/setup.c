/* core/setup.c — see core/setup.h for the contract. */
#include "setup.h"

#include <stddef.h>

#include "frame.h"
#include "../data/music.h"
#include "../data/sprites.h"

void get_area_music(CoreState *state) {
  /* smb.asm: GetAreaMusic */
  if (state->ram[SMBC_OPER_MODE] == SMBC_MODE_TITLE) return; /* ExitGetM */
  uint8_t y = 0x05u; /* smb.asm: pipe intro by default */
  uint8_t entrance = state->ram[SMBC_PLAYER_ENTRANCE_CTRL];
  if (state->ram[SMBC_ALT_ENTRANCE_CONTROL] == 0x02u ||
      (entrance != 0x06u && entrance != 0x07u)) {
    /* smb.asm: ChkAreaType */
    y = state->ram[SMBC_AREA_TYPE];
    if (state->ram[SMBC_CLOUD_TYPE_OVERRIDE] != 0x00u) {
      y = 0x04u; /* smb.asm: cloud type override */
    }
  }
  /* smb.asm: StoreMusic */
  state->ram[SMBC_AREA_MUSIC_QUEUE] = SMBC_MUSIC_SELECT_DATA[y];
}

void primary_game_setup(Smbc *smbc) {
  /* smb.asm: PrimaryGameSetup — new game defaults, then falls through
   * into SecondaryGameSetup */
  CoreState *state = &smbc->state;
  state->ram[SMBC_FETCH_NEW_GAME_TIMER_FLAG] = 0x01u;
  state->ram[SMBC_PLAYER_SIZE] = 0x01u; /* player starts small */
  state->ram[SMBC_NUMBEROF_LIVES] = 0x02u;
  state->ram[SMBC_OFFSCR_NUMBEROF_LIVES] = 0x02u;
  secondary_game_setup(smbc);
}

void secondary_game_setup(Smbc *smbc) {
  /* smb.asm: SecondaryGameSetup */
  CoreState *state = &smbc->state;
  state->ram[SMBC_DISABLE_SCREEN_FLAG] = 0x00u; /* enable screen output */
  /* smb.asm: ClearVRLoop — clears the entire buffer page $0300-$03ff */
  for (size_t y = 0; y < 0x100u; y++) {
    state->ram[SMBC_VRAM_BUFFER1 - 1 + y] = 0x00u;
  }
  state->ram[SMBC_GAME_TIMER_EXPIRED_FLAG] = 0x00u;
  state->ram[SMBC_DISABLE_INTERMEDIATE] = 0x00u;
  state->ram[SMBC_BACKLOADING_FLAG] = 0x00u;
  state->ram[SMBC_BAL_PLATFORM_ALIGNMENT] = 0xffu;
  /* smb.asm: the lsr/and/ror/rol dance on Mirror_PPU_CTRL_REG1 — the net
   * effect is the mirror's nametable bit following ScreenLeft_PageLoc d0 */
  state->ram[SMBC_MIRROR_PPU_CTRL1] =
      (uint8_t)((state->ram[SMBC_MIRROR_PPU_CTRL1] & 0xfeu) |
                (state->ram[SMBC_SCREEN_LEFT_PAGE_LOC] & 0x01u));
  get_area_music(state);
  state->ram[SMBC_SPR_SHUFFLE_AMT + 2] = 0x38u;
  state->ram[SMBC_SPR_SHUFFLE_AMT + 1] = 0x48u;
  state->ram[SMBC_SPR_SHUFFLE_AMT] = 0x58u;
  for (size_t x = SMBC_DEFAULT_SPR_OFFSETS_SIZE; x-- > 0;) {
    state->ram[SMBC_SPR_DATA_OFFSET + x] = SMBC_DEFAULT_SPR_OFFSETS[x];
  }
  for (size_t y = SMBC_SPRITE0_DATA_SIZE; y-- > 0;) {
    state->ram[SMBC_SPRITE_DATA + y] = SMBC_SPRITE0_DATA[y];
  }
  /* smb.asm: jsr DoNothing2 / jsr DoNothing1 — residual code; the value it
   * writes to $06c9 is not used anywhere in the program */
  state->ram[0x06c9] = 0xffu;
  state->ram[SMBC_SPRITE0_HIT_FLAG]++; /* sprite 0 check flag */
  state->ram[SMBC_OPER_MODE_TASK]++;
  (void)smbc->ppu; /* setup touches RAM only */
}
