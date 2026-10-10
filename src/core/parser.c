/* core/parser.c — see core/parser.h for the contract. */
#include "parser.h"

#include "../data/metatiles.h"

/* smb.asm: IncrementColumnPos */
static void increment_column_pos(CoreState *st) {
  uint8_t col = (uint8_t)(st->ram[SMBC_CURRENT_COLUMN_POS] + 1u);
  if ((col & 0x0fu) == 0x00u) { /* a wrapped column is a new page */
    col = 0x00u;
    st->ram[SMBC_CURRENT_PAGE_LOC]++;
  }
  st->ram[SMBC_CURRENT_COLUMN_POS] = col;
  st->ram[SMBC_BLOCK_BUFFER_COLUMN_POS] =
      (uint8_t)(st->ram[SMBC_BLOCK_BUFFER_COLUMN_POS] + 1u) & 0x1fu;
}

/* smb.asm: RenderAreaGraphics — draw one column of metatiles from the
 * metatile buffer into VRAM buffer 2 as 26 tile bytes, OR-ing each
 * metatile's attribute bits into the attribute buffer. The two top bits
 * of a metatile number select its palette's attribute bits; the rest is
 * the metatile index, four bytes per metatile (UL, LL, UR, LR), and the
 * task number's parity picks the left or right half. */
static void render_area_graphics(CoreState *st) {
  uint8_t col_lsb = st->ram[SMBC_CURRENT_COLUMN_POS] & 0x01u; /* $05 */
  uint8_t buf_off = st->ram[SMBC_VRAM_BUFFER2_OFFSET];        /* $00 */
  st->ram[SMBC_VRAM_BUFFER2 + buf_off] = st->ram[SMBC_CURRENT_NT_ADDR_HIGH];
  st->ram[SMBC_VRAM_BUFFER2 + buf_off + 1] = st->ram[SMBC_CURRENT_NT_ADDR_LOW];
  st->ram[SMBC_VRAM_BUFFER2 + buf_off + 2] = 0x9au; /* 26 bytes, +32 stride */
  uint8_t attr_row = 0x00u; /* $04 */
  /* ((task & 1) ^ 1) << 1: the metatile half this pass draws */
  uint8_t half =
      (uint8_t)(((st->ram[SMBC_AREA_PARSER_TASK_NUM] & 0x01u) ^ 0x01u) << 1);
  for (uint8_t row = 0x00u; row < 0x0du; row++) { /* DrawMTLoop */
    uint8_t mt = st->ram[SMBC_METATILE_BUFFER + row];
    uint8_t attr = mt & 0xc0u; /* $03: attribute table bits */
    const uint8_t *table = SMBC_METATILE_GRAPHICS
        [(uint8_t)((mt & 0xc0u) >> 6)]; /* asl/rol/rol: bits to d1-d0 */
    uint8_t tile_off = (uint8_t)(half + (uint8_t)(mt << 2)); /* asl asl */
    st->ram[SMBC_VRAM_BUFFER2 + buf_off + 3] = table[tile_off];
    st->ram[SMBC_VRAM_BUFFER2 + buf_off + 4] = table[(uint8_t)(tile_off + 1u)];
    if (col_lsb == 0x00u) { /* left attribute square */
      if ((row & 0x01u) == 0x00u) {
        attr >>= 6; /* upper left: three rol through carry, d1-d0 */
      } else {
        attr >>= 2; /* LLeft: lower left square, d5-d4 */
        attr_row++; /* falls through into NextMTRow */
      }
    } else if ((row & 0x01u) == 0x00u) {
      attr >>= 4; /* RightCheck: upper right square, d3-d2 */
    } else {
      attr_row++; /* NextMTRow: lower right keeps d7-d6, next row */
    }
    uint8_t slot = attr_row;
    st->ram[SMBC_ATTRIBUTE_BUFFER + slot] |= attr; /* SetAttrib */
    buf_off = (uint8_t)(buf_off + 2u);
  }
  buf_off = (uint8_t)(buf_off + 3u); /* past the address and length */
  st->ram[SMBC_VRAM_BUFFER2 + buf_off] = 0x00u; /* null terminator */
  st->ram[SMBC_VRAM_BUFFER2_OFFSET] = buf_off;
  /* advance the name table address one column, wrapping under the status
   * bar and into the other name table */
  uint8_t low = (uint8_t)(st->ram[SMBC_CURRENT_NT_ADDR_LOW] + 1u);
  st->ram[SMBC_CURRENT_NT_ADDR_LOW] = low;
  if ((low & 0x1fu) == 0x00u) {
    st->ram[SMBC_CURRENT_NT_ADDR_LOW] = 0x80u;
    st->ram[SMBC_CURRENT_NT_ADDR_HIGH] ^= 0x04u;
  }
  st->ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x06u; /* SetVRAMCtrl */
}

/* smb.asm: AreaParserCore — pending the area object data atom */
static void area_parser_core(CoreState *st) {
  (void)st; /* TODO(port): smb.asm: AreaParserCore (data/areadata) */
}

/* smb.asm: RenderAttributeTables — flush the attribute buffer into VRAM
 * buffer 2, one byte per attribute cell, and clear the buffer behind it */
static void render_attribute_tables(CoreState *st) {
  uint8_t low = st->ram[SMBC_CURRENT_NT_ADDR_LOW];
  /* sec; sbc #$04 — the borrow flags a name-table boundary crossing */
  bool borrow = (uint8_t)(low & 0x1fu) < 0x04u;
  low = (uint8_t)((low & 0x1fu) - 0x04u) & 0x1fu;
  uint8_t high = st->ram[SMBC_CURRENT_NT_ADDR_HIGH];
  if (borrow) {
    high ^= 0x04u; /* crossed into the other name table's half */
  }
  high = (uint8_t)((high & 0x04u) | 0x23u);
  /* low / 4 + $c0 selects the attribute row; the two lsrs leave the old
   * bit 1 in the carry the adc consumes */
  low = (uint8_t)((low >> 2) + 0xc0u + ((low >> 1) & 0x01u));
  uint8_t y = st->ram[SMBC_VRAM_BUFFER2_OFFSET];
  for (uint8_t x = 0; x < 0x07u; x++) { /* AttribLoop */
    st->ram[SMBC_VRAM_BUFFER2 + y] = high;
    low = (uint8_t)(low + 0x08u); /* start below the status bar */
    st->ram[SMBC_VRAM_BUFFER2 + y + 1] = low;
    st->ram[SMBC_VRAM_BUFFER2 + y + 2] = 0x01u; /* one byte per write */
    st->ram[SMBC_VRAM_BUFFER2 + y + 3] = st->ram[SMBC_ATTRIBUTE_BUFFER + x];
    st->ram[SMBC_ATTRIBUTE_BUFFER + x] = 0x00u; /* clear behind */
    y = (uint8_t)(y + 4u);
  }
  st->ram[SMBC_VRAM_BUFFER2 + y] = 0x00u; /* null terminator */
  st->ram[SMBC_VRAM_BUFFER2_OFFSET] = y;
  st->ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x06u; /* SetVRAMCtrl */
}

static void run_task(CoreState *st, uint8_t task) {
  switch (task) { /* smb.asm: AreaParserTasks, jump table order */
    case 0: increment_column_pos(st); break;
    case 1: render_area_graphics(st); break;
    case 2: render_area_graphics(st); break;
    case 3: area_parser_core(st); break;
    case 4: increment_column_pos(st); break;
    case 5: render_area_graphics(st); break;
    case 6: render_area_graphics(st); break;
    default: area_parser_core(st); break;
  }
}

void area_parser_task_handler(CoreState *st) {
  if (st->ram[SMBC_AREA_PARSER_TASK_NUM] == 0x00u) {
    st->ram[SMBC_AREA_PARSER_TASK_NUM] = 0x08u; /* eight tasks by default */
  }
  run_task(st, (uint8_t)(st->ram[SMBC_AREA_PARSER_TASK_NUM] - 1u));
  st->ram[SMBC_AREA_PARSER_TASK_NUM]--;
  if (st->ram[SMBC_AREA_PARSER_TASK_NUM] == 0x00u) {
    render_attribute_tables(st); /* the cycle flushes its attributes */
  }
}
