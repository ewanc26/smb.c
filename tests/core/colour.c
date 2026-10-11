/* tests/core/colour.c — exercises src/core/colour.c: ColorRotation's
 * every-eighth-frame background palette cycle and its buffer bookkeeping. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/colour.h"
#include "../../src/core/frame.h"
#include "../../src/core/state.h"
#include "../../src/data/palettes.h"

static void not_eighth_frame_does_nothing(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x03u;
  smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] = 0x00u;
  smbc.state.ram[SMBC_COLOR_ROTATE_OFFSET] = 0x00u;
  color_rotation(&smbc.state);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] == 0x00u);
  assert(smbc.state.ram[SMBC_COLOR_ROTATE_OFFSET] == 0x00u);
}

static void full_buffer_leaves(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x00u;
  smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] = 0x31u;
  smbc.state.ram[SMBC_COLOR_ROTATE_OFFSET] = 0x02u;
  color_rotation(&smbc.state);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] == 0x31u);
  assert(smbc.state.ram[SMBC_COLOR_ROTATE_OFFSET] == 0x02u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1] == 0x00u);
}

static void ground_area_writes_palette_set(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x08u; /* low three bits zero */
  smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] = 0x00u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u; /* ground */
  smbc.state.ram[SMBC_COLOR_ROTATE_OFFSET] = 0x00u;
  color_rotation(&smbc.state);

  /* the blank palette-3 set lands whole, then the area's four bytes are
   * patched into slots three through six, then the cycling colour
   * overwrites slot four */
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 0] == 0x3fu);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 1] == 0x0cu);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 2] == 0x04u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 3] == 0x0fu);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 4] == 0x27u); /* ColorRotate[0] */
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 5] == 0x17u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 6] == 0x0fu);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 7] == 0x00u);

  /* the buffer grew by seven and the cycle advanced by one */
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] == 0x07u);
  assert(smbc.state.ram[SMBC_COLOR_ROTATE_OFFSET] == 0x01u);
}

static void area_type_selects_the_set(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x00u;
  smbc.state.ram[SMBC_VRAM_BUFFER1_OFFSET] = 0x00u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x03u; /* fourth set */
  smbc.state.ram[SMBC_COLOR_ROTATE_OFFSET] = 0x05u; /* ColorRotate[5] */
  color_rotation(&smbc.state);
  /* Palette3Data[12..15] = 0f 07 17 00, the cycling byte 0x17 in slot 4 */
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 3] == 0x0fu);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 4] == 0x17u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 5] == 0x17u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER1 + 6] == 0x00u);
  /* the cycle offset wrapped back to zero at six */
  assert(smbc.state.ram[SMBC_COLOR_ROTATE_OFFSET] == 0x00u);
}

static void cycle_offset_wraps_at_six(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_FRAME_COUNTER] = 0x00u;
  smbc.state.ram[SMBC_COLOR_ROTATE_OFFSET] = 0x05u;
  color_rotation(&smbc.state);
  assert(smbc.state.ram[SMBC_COLOR_ROTATE_OFFSET] == 0x00u);
}

int main(void) {
  not_eighth_frame_does_nothing();
  full_buffer_leaves();
  ground_area_writes_palette_set();
  area_type_selects_the_set();
  cycle_offset_wraps_at_six();
  puts("tests/core/colour: ok");
  return 0;
}
