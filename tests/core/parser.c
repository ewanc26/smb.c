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

int main(void) {
  task_counter();
  column_positions();
  attribute_flush();
  printf("tests/core/parser: ok\n");
  return 0;
}
