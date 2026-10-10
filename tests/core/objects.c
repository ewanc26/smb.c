/* tests/core/objects.c — exercises the area object decoder in
 * src/core/parser.c: ProcessAreaData/DecodeAreaData and the object
 * handlers, driven through the parser core task with crafted level
 * object streams. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/parser.h"

/* a fresh state with the three-slot object buffer emptied (negative
 * length counters), no terrain or scenery, and the given stream */
static void init_decoder(Smbc *smbc, const uint8_t *stream) {
  smbc_init(smbc);
  smbc->state.area_data = stream;
  for (uint8_t x = 0; x < 0x03u; x++) {
    smbc->state.ram[SMBC_AREA_OBJECT_LENGTH + x] = 0x80u;
  }
  smbc->state.ram[SMBC_AREA_PARSER_TASK_NUM] = 0x08u; /* a decoder task */
}

static void row_of_bricks(void) {
  /* smb.asm: a large object with d6-d4 = %010 — id 2, RowOfBricks.
   * The first free buffer slot (slot 2, the parser walks down) picks
   * it up on column 0 and renders the ground brick metatile */
  static const uint8_t stream[] = {0x02u, 0x25u, 0xfdu};
  Smbc smbc;
  init_decoder(&smbc, stream);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 2] == 0x51u);
  assert(smbc.state.ram[SMBC_AREA_OBJ_OFFSET_BUFFER + 2] == 0x00u);
  assert(smbc.state.ram[SMBC_AREA_DATA_OFFSET] == 0x02u);
  assert(smbc.state.ram[SMBC_AREA_OBJECT_LENGTH + 2] == 0x04u);

  /* the buffered object keeps rendering its columns from the offset
   * buffer until its length expires */
  smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] = 0x08u;
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 2] == 0x51u);
  assert(smbc.state.ram[SMBC_AREA_OBJECT_LENGTH + 2] == 0x03u);
}

static void page_control_object(void) {
  /* smb.asm: a row-13 object with d6 clear is page control — store
   * the new page and advance without rendering anything; the
   * advance's IncAreaObjOffset resets the page select behind it */
  static const uint8_t stream[] = {0x0du, 0x06u, 0xfdu};
  Smbc smbc;
  init_decoder(&smbc, stream);
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_AREA_OBJECT_PAGE_LOC] == 0x06u);
  assert(smbc.state.ram[SMBC_AREA_OBJECT_PAGE_SEL] == 0x00u);
  assert(smbc.state.ram[SMBC_AREA_DATA_OFFSET] == 0x02u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 0] == 0x00u);
}

static void object_behind_renderer(void) {
  /* smb.asm: CheckRear/SetBehind — an object whose page sits behind
   * the renderer is skipped, advancing the data pointer */
  static const uint8_t stream[] = {0x02u, 0x25u, 0xfdu};
  Smbc smbc;
  init_decoder(&smbc, stream);
  smbc.state.ram[SMBC_CURRENT_PAGE_LOC] = 0x01u;
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_AREA_DATA_OFFSET] == 0x02u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 2] == 0x00u);
  assert(smbc.state.ram[SMBC_AREA_OBJECT_LENGTH + 2] == 0x80u);
}

static void question_block_coin(void) {
  /* smb.asm: a small object (d6-d4 clear) with id 1 — the coin
   * question block, a single column with no length counter */
  static const uint8_t stream[] = {0x05u, 0x01u, 0xfdu};
  Smbc smbc;
  init_decoder(&smbc, stream);
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 5] == 0xc0u);
  assert(smbc.state.ram[SMBC_AREA_DATA_OFFSET] == 0x02u);
  assert(smbc.state.ram[SMBC_AREA_OBJECT_LENGTH + 2] == 0x80u);
}

static void brick_with_item_ground(void) {
  /* smb.asm: BrickWithItem id 4 (power-up) on a ground level indexes
   * the table from zero — the ground brick-with-lines metatile */
  static const uint8_t stream[] = {0x07u, 0x04u, 0xfdu};
  Smbc smbc;
  init_decoder(&smbc, stream);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 7] == 0x55u);
}

static void warp_pipe(void) {
  /* smb.asm: a vertical pipe with the usage control bit set — the id
   * is nullified, no piranha plant, the usable-pipe metatiles, and
   * the shaft running to the bottom from the pipe's height */
  static const uint8_t stream[] = {0x09u, 0x7bu, 0xfdu};
  Smbc smbc;
  init_decoder(&smbc, stream);
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 9] == 0x10u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 10] == 0x14u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 11] == 0x14u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 12] == 0x14u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_AREA_OBJECT_LENGTH + 2] == 0x00u);
}

static void pipe_piranha_plant(void) {
  /* smb.asm: outside 1-1 a pipe's first column spawns a piranha
   * plant into the first empty enemy slot */
  static const uint8_t stream[] = {0x09u, 0x7bu, 0xfdu};
  Smbc smbc;
  init_decoder(&smbc, stream);
  smbc.state.ram[SMBC_AREA_NUMBER] = 0x01u;
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 0] == 0x0du);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 0] == 0x08u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 0] == 0xb0u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_HIGH_POS + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_PIRANHA_PLANT_Y_SPEED + 0] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_STATE + 0] == 0x00u);
  assert(smbc.state.ram[SMBC_PIRANHA_PLANT_DOWN_Y_POS + 0] == 0xb0u);
  assert(smbc.state.ram[SMBC_PIRANHA_PLANT_UP_Y_POS + 0] == 0x98u);
  assert(smbc.state.ram[SMBC_ENEMY_BOUND_BOX_CTRL + 0] == 0x09u);
}

static void flagpole(void) {
  /* smb.asm: the row-13 flagpole object — ball, shaft, base block,
   * and the flag written into the sixth enemy slot */
  static const uint8_t stream[] = {0x2du, 0x41u, 0xfdu};
  Smbc smbc;
  init_decoder(&smbc, stream);
  smbc.state.ram[SMBC_CURRENT_COLUMN_POS] = 0x02u;
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 0] == 0x24u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 1] == 0x25u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 9] == 0x25u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 10] == 0x61u);
  assert(smbc.state.ram[SMBC_ENEMY_ID + 5] == 0x30u);
  assert(smbc.state.ram[SMBC_ENEMY_FLAG + 5] == 0x01u);
  assert(smbc.state.ram[SMBC_ENEMY_X_POSITION + 5] == 0x18u);
  assert(smbc.state.ram[SMBC_ENEMY_PAGE_LOC + 5] == 0x00u);
  assert(smbc.state.ram[SMBC_ENEMY_Y_POSITION + 5] == 0x30u);
  assert(smbc.state.ram[SMBC_FLAGPOLE_FNUM_Y_POS] == 0xb0u);
}

static void hole_whirlpool(void) {
  /* smb.asm: Hole_Empty in a water area — the pit metatiles plus a
   * whirlpool data entry two blocks wider than the hole */
  static const uint8_t stream[] = {0x0cu, 0x02u, 0xfdu};
  Smbc smbc;
  init_decoder(&smbc, stream);
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 8] == 0x87u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 12] == 0x87u);
  assert(smbc.state.ram[SMBC_WHIRLPOOL_LEFT_EXTENT + 0] == 0xf0u);
  assert(smbc.state.ram[SMBC_WHIRLPOOL_PAGE_LOC + 0] == 0xffu);
  assert(smbc.state.ram[SMBC_WHIRLPOOL_LENGTH + 0] == 0x40u);
  assert(smbc.state.ram[SMBC_WHIRLPOOL_OFFSET] == 0x01u);
}

static void alter_area_attributes(void) {
  /* smb.asm: the row-14 attribute object with d6 clear rewrites the
   * terrain control and background scenery */
  static const uint8_t stream[] = {0x1eu, 0x25u, 0xfdu};
  Smbc smbc;
  init_decoder(&smbc, stream);
  smbc.state.ram[SMBC_CURRENT_COLUMN_POS] = 0x01u;
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_TERRAIN_CONTROL] == 0x05u);
  assert(smbc.state.ram[SMBC_BACKGROUND_SCENERY] == 0x02u);

  /* d6 set with a value of four or more sets the background colour
   * control and nullifies the foreground scenery */
  static const uint8_t stream2[] = {0x1eu, 0x45u, 0xfdu};
  init_decoder(&smbc, stream2);
  smbc.state.ram[SMBC_CURRENT_COLUMN_POS] = 0x01u;
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_BACKGROUND_COLOR_CTRL] == 0x05u);
  assert(smbc.state.ram[SMBC_FOREGROUND_SCENERY] == 0x00u);
}

static void loop_command(void) {
  /* smb.asm: the row-13 loop command ($4b with d6) sets the loop
   * command flag and renders nothing */
  static const uint8_t stream[] = {0x0du, 0x4bu, 0xfdu};
  Smbc smbc;
  init_decoder(&smbc, stream);
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_LOOP_COMMAND] == 0x01u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 0] == 0x00u);
}

static void scroll_lock(void) {
  /* smb.asm: the row-13 scroll lock object inverts the scroll lock */
  static const uint8_t stream[] = {0x0du, 0x47u, 0xfdu};
  Smbc smbc;
  init_decoder(&smbc, stream);
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_SCROLL_LOCK] == 0x01u);
}

static void endless_rope(void) {
  /* smb.asm: the row-15 endless rope renders from the top of the
   * screen to the bottom */
  static const uint8_t stream[] = {0x0fu, 0x00u, 0xfdu};
  Smbc smbc;
  init_decoder(&smbc, stream);
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 0] == 0x40u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 12] == 0x40u);
}

int main(void) {
  row_of_bricks();
  page_control_object();
  object_behind_renderer();
  question_block_coin();
  brick_with_item_ground();
  warp_pipe();
  pipe_piranha_plant();
  flagpole();
  hole_whirlpool();
  alter_area_attributes();
  loop_command();
  scroll_lock();
  endless_rope();
  printf("tests/core/objects: ok\n");
  return 0;
}
