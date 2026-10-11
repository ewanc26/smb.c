/* core/area.c — see core/area.h for the contract. */
#include "area.h"

#include "../data/areadata.h"
#include "../data/area.h"
#include "frame.h"
#include "state.h"

uint8_t find_area_pointer(const CoreState *state) {
  /* smb.asm: FindAreaPointer — offset from world variable, plus the area
   * number, indexes the flat area table. */
  uint8_t y = SMBC_WORLD_ADDR_OFFSETS[state->ram[SMBC_WORLD_NUMBER]];
  return SMBC_AREA_ADDR_OFFSETS[y + state->ram[SMBC_AREA_NUMBER]];
}

void load_area_pointer(Smbc *smbc) {
  uint8_t a = find_area_pointer(&smbc->state);
  smbc->state.ram[SMBC_AREA_POINTER] = a;
  /* smb.asm: GetAreaType — and #%01100000, then asl/rol/rol/rol rotates
   * %0xx00000 into %000000xx; equivalent to a shift. */
  smbc->state.ram[SMBC_AREA_TYPE] = (a >> 5) & 0x03u;
}

uint8_t get_screen_position(CoreState *st) {
  /* smb.asm: GetScreenPosition — the right boundary is the screen's left
   * boundary advanced by 255 pixels; the page carries when the low byte
   * is at least one. A holds the resulting right page location on exit,
   * which callers (InitializeArea, ScrollHandler) reuse. */
  uint16_t sum = (uint16_t)st->ram[SMBC_SCREEN_LEFT_X_POS] + 0xffu;
  st->ram[SMBC_SCREEN_RIGHT_X_POS] = (uint8_t)sum;
  sum = (uint16_t)st->ram[SMBC_SCREEN_LEFT_PAGE_LOC] + (uint8_t)(sum >> 8);
  st->ram[SMBC_SCREEN_RIGHT_PAGE_LOC] = (uint8_t)sum;
  return (uint8_t)sum;
}

void get_area_data_addrs(Smbc *smbc) {
  CoreState *st = &smbc->state;
  /* smb.asm: GetAreaDataAddrs — the area pointer's 2 MSB select the table
   * base (after GetAreaType rotates them down), the 5 LSB offset into it */
  uint8_t a = st->ram[SMBC_AREA_POINTER];
  uint8_t area_type = (a >> 5) & 0x03u;
  st->ram[SMBC_AREA_TYPE] = area_type;
  uint8_t offset = a & 0x1fu;
  st->ram[SMBC_AREA_ADDRS_L_OFFSET] = offset;
  st->enemy_data = SMBC_ENEMY_AREAS[SMBC_ENEMY_ADDR_H_OFFSETS[area_type] + offset];
  const uint8_t *area =
      SMBC_LEVEL_AREAS[SMBC_AREA_DATA_H_OFFSETS[area_type] + offset];

  /* the header's first byte: foreground scenery or background colour
   * control, the player entrance control, and the game timer setting */
  uint8_t b = area[0];
  uint8_t low = b & 0x07u;
  if (low >= 0x04u) {
    st->ram[SMBC_BACKGROUND_COLOR_CTRL] = low;
    low = 0x00u;
  }
  st->ram[SMBC_FOREGROUND_SCENERY] = low;
  st->ram[SMBC_PLAYER_ENTRANCE_CTRL] = (b & 0x38u) >> 3;
  st->ram[SMBC_GAME_TIMER_SETTING] = (b >> 6) & 0x03u;

  /* the second byte: terrain control, background scenery, and the area
   * style — a style of three is the cloud override instead */
  b = area[1];
  st->ram[SMBC_TERRAIN_CONTROL] = b & 0x0fu;
  st->ram[SMBC_BACKGROUND_SCENERY] = (b & 0x30u) >> 4;
  uint8_t style = (b >> 6) & 0x03u;
  if (style == 0x03u) {
    st->ram[SMBC_CLOUD_TYPE_OVERRIDE] = style;
    style = 0x00u;
  }
  st->ram[SMBC_AREA_STYLE] = style;

  st->area_data = area + 2; /* past the header */
}
