/* tests/core/parser.c — exercises src/core/parser.c: the task counter,
 * the column position bookkeeping, and the attribute buffer flush. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/parser.h"
#include "../../src/core/state.h"

static void task_counter(void) {
  Smbc smbc;
  smbc_init(&smbc);
  /* an unset counter defaults to eight and runs the cycle */
  smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] = 0x00u;
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] == 0x07u);
  /* the cycle's end flushes the attributes and lands on zero */
  for (int i = 0; i < 7; i++) area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] == 0x00u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x06u);
}

static void column_positions(void) {
  Smbc smbc;
  smbc_init(&smbc);
  /* ordinary column: position and block buffer advance, page stays */
  smbc.state.ram[SMBC_CURRENT_COLUMN_POS] = 0x03u;
  smbc.state.ram[SMBC_BLOCK_BUFFER_COLUMN_POS] = 0x1eu;
  smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] = 0x05u; /* a column-inc task */
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_CURRENT_COLUMN_POS] == 0x04u);
  assert(smbc.state.ram[SMBC_BLOCK_BUFFER_COLUMN_POS] == 0x1fu);
  /* the block buffer column wraps on its own */
  smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] = 0x01u; /* the other inc */
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_CURRENT_COLUMN_POS] == 0x05u);
  assert(smbc.state.ram[SMBC_BLOCK_BUFFER_COLUMN_POS] == 0x00u);
  /* a wrapped column is a new page */
  smbc.state.ram[SMBC_CURRENT_COLUMN_POS] = 0x0fu;
  smbc.state.ram[SMBC_CURRENT_PAGE_LOC] = 0x07u;
  smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] = 0x05u;
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_CURRENT_COLUMN_POS] == 0x00u);
  assert(smbc.state.ram[SMBC_CURRENT_PAGE_LOC] == 0x08u);
}

static void attribute_flush(void) {
  /* same name-table half: no borrow, no high-byte flip */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_CURRENT_NT_ADDR_LOW] = 0x4au;
  smbc.state.ram[SMBC_CURRENT_NT_ADDR_HIGH] = 0x24u;
  for (int x = 0; x < 7; x++)
    smbc.state.ram[SMBC_ATTRIBUTE_BUFFER + x] = (uint8_t)(0x10u + x);
  smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] = 0x01u; /* last task */
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER2] == 0x27u); /* $2400's attributes */
  /* low: (6 >> 2) + $c0 + carry(bit1 of 6) = $c2, then + 8 */
  assert(smbc.state.ram[SMBC_VRAM_BUFFER2 + 1] == 0xcau);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER2 + 2] == 0x01u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER2 + 3] == 0x10u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER2 + 27] == 0x16u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER2 + 28] == 0x00u); /* terminator */
  assert(smbc.state.ram[SMBC_VRAM_BUFFER2_OFFSET] == 0x1cu);
  assert(smbc.state.ram[SMBC_ATTRIBUTE_BUFFER] == 0x00u); /* cleared */

  /* a low address under $04 borrows across the name tables */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_CURRENT_NT_ADDR_LOW] = 0x02u;
  smbc.state.ram[SMBC_CURRENT_NT_ADDR_HIGH] = 0x24u;
  smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] = 0x01u;
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER2] == 0x23u); /* $24^$04 = $20, d2 clear */
  /* low: (0x1e >> 2) + $c0 + carry(bit1 of 0x1e) = 7 + $c0 + 1 = $c8, +8 */
  assert(smbc.state.ram[SMBC_VRAM_BUFFER2 + 1] == 0xd0u);
}

static void area_graphics(void) {
  /* the black metatile (palette 0, index 1: all $27 tiles) down a
   * column; even task number draws the right half */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_CURRENT_NT_ADDR_LOW] = 0x44u;
  smbc.state.ram[SMBC_CURRENT_NT_ADDR_HIGH] = 0x24u;
  smbc.state.ram[SMBC_CURRENT_COLUMN_POS] = 0x02u; /* even: left square */
  for (int row = 0; row < 13; row++)
    smbc.state.ram[SMBC_METATILE_BUFFER + row] = 0x01u;
  smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] = 0x02u; /* a graphics task */
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER2] == 0x24u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER2 + 1] == 0x44u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER2 + 2] == 0x9au);
  for (int i = 0; i < 26; i++)
    assert(smbc.state.ram[SMBC_VRAM_BUFFER2 + 3 + i] == 0x27u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER2 + 29] == 0x00u);
  assert(smbc.state.ram[SMBC_VRAM_BUFFER2_OFFSET] == 0x1du);
  /* the column advanced; palette 0 contributes no attribute bits */
  assert(smbc.state.ram[SMBC_CURRENT_NT_ADDR_LOW] == 0x45u);
  assert(smbc.state.ram[SMBC_CURRENT_NT_ADDR_HIGH] == 0x24u);
  assert(smbc.state.ram[SMBC_ATTRIBUTE_BUFFER] == 0x00u);

  /* palette 3 bits, odd column: upper right at d3-d2, lower right at
   * d7-d6 — both halves of the same attribute cell; the asm's SetAttrib
   * indexes the row loaded before NextMTRow's increment */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_CURRENT_NT_ADDR_LOW] = 0x5fu;
  smbc.state.ram[SMBC_CURRENT_NT_ADDR_HIGH] = 0x23u;
  smbc.state.ram[SMBC_CURRENT_COLUMN_POS] = 0x03u; /* odd: right square */
  smbc.state.ram[SMBC_METATILE_BUFFER + 0] = 0xc0u; /* palette 3, blank */
  smbc.state.ram[SMBC_METATILE_BUFFER + 1] = 0xc0u;
  smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] = 0x06u; /* a graphics task */
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_ATTRIBUTE_BUFFER + 0] == 0xccu); /* d3-d2 | d7-d6 */
  assert(smbc.state.ram[SMBC_ATTRIBUTE_BUFFER + 1] == 0x00u);
  /* low $5f + 1 wraps: back under the status bar, other name table */
  assert(smbc.state.ram[SMBC_CURRENT_NT_ADDR_LOW] == 0x80u);
  assert(smbc.state.ram[SMBC_CURRENT_NT_ADDR_HIGH] == 0x27u);
}

static void scenery_terrain(void) {
  /* flat ground: the floor's two rows of ground metatile, stored into
   * the block buffer with the low-bound drop */
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_TERRAIN_CONTROL] = 0x01u; /* no ceiling, floor 2 */
  smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] = 0x08u; /* a decoder task */
  area_parser_task_handler(&smbc.state);
  for (int x = 0; x < 11; x++)
    assert(smbc.state.ram[SMBC_METATILE_BUFFER + x] == 0x00u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 11] == 0x54u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 12] == 0x54u);
  assert(smbc.state.ram[SMBC_BLOCK_BUFFER_1] == 0x00u); /* rows drop out */
  assert(smbc.state.ram[SMBC_BLOCK_BUFFER_1 + 0xb0] == 0x54u);
  assert(smbc.state.ram[SMBC_BLOCK_BUFFER_1 + 0xc0] == 0x54u);

  /* clouds on the first page: a cloud right at buffer height nine */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x01u;
  smbc.state.ram[SMBC_BACKGROUND_SCENERY] = 0x01u;
  smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] = 0x08u;
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 9] == 0x82u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 10] == 0x85u);

  /* foreground water: surface, then water, then the water metatile */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u;
  smbc.state.ram[SMBC_FOREGROUND_SCENERY] = 0x01u;
  smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] = 0x08u;
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER] == 0x86u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 5] == 0x87u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 11] == 0x69u);

  /* world 8's water uses the castle wall metatile throughout */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_AREA_TYPE] = 0x00u;
  smbc.state.ram[SMBC_WORLD_NUMBER] = 0x07u;
  smbc.state.ram[SMBC_TERRAIN_CONTROL] = 0x0fu; /* solid top to bottom */
  smbc.state.ram[SMBC_AREA_PARSER_TASK_NUM] = 0x08u;
  area_parser_task_handler(&smbc.state);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER] == 0x62u);
  assert(smbc.state.ram[SMBC_METATILE_BUFFER + 12] == 0x62u);
}

int main(void) {
  task_counter();
  column_positions();
  attribute_flush();
  area_graphics();
  scenery_terrain();
  printf("tests/core/parser: ok\n");
  return 0;
}
