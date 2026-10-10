/* core/parser.c — see core/parser.h for the contract. */
#include "parser.h"

#include "../data/metatiles.h"
#include "../data/scenery.h"

#define SMBC_WORLD8 0x07u /* smb.asm: World8 */

/* smb.asm: ProcessAreaData — pending the area object data atom */
static void process_area_data(CoreState *st) {
  (void)st; /* TODO(port): smb.asm: ProcessAreaData (data/areadata) */
}

/* smb.asm: GetBlockBufferAddr — the block buffer address for a column
 * position: buffer 1 for columns $00-$0f, buffer 2 for $10-$1f */
static uint16_t get_block_buffer_addr(uint8_t col_pos) {
  return (uint16_t)(SMBC_BLOCK_BUFFER_1 + (col_pos & 0x10u) / 0x10u * 0xd0u +
                    (col_pos & 0x0fu));
}

/* smb.asm: RenderSceneryTerrain — clear the metatile buffer, lay in the
 * background scenery (chosen every third page), the foreground scenery,
 * and the terrain ceiling/floor from the render bits; then the tail end
 * runs the object decoder and stores the column into the block buffer,
 * dropping metatiles under their attribute bound. */
static void render_scenery_terrain(CoreState *st) {
  for (uint8_t x = 0; x <= 0x0cu; x++) { /* ClrMTBuf */
    st->ram[SMBC_METATILE_BUFFER + x] = 0x00u;
  }
  uint8_t bg = st->ram[SMBC_BACKGROUND_SCENERY];
  if (bg != 0x00u) {
    uint8_t page = st->ram[SMBC_CURRENT_PAGE_LOC];
    while (page >= 0x03u) page = (uint8_t)(page - 0x03u); /* ThirdP */
    uint8_t x = (uint8_t)((uint8_t)(page << 4) +
                          SMBC_B_SCENE_DATA_OFFSETS[bg - 1u] +
                          st->ram[SMBC_CURRENT_COLUMN_POS]);
    uint8_t sd = SMBC_BACK_SCENERY_DATA[x];
    if (sd != 0x00u) {
      uint8_t low = (uint8_t)((sd & 0x0fu) - 1u); /* shapes are 1-based */
      uint8_t sx = (uint8_t)(low * 3u);
      uint8_t y = (uint8_t)(sd >> 4); /* the buffer height */
      uint8_t count = 0x03u;
      for (;;) { /* SceLoop1 */
        st->ram[SMBC_METATILE_BUFFER + y] = SMBC_BACK_SCENERY_METATILES[sx];
        sx++;
        y++;
        if (y == 0x0bu) break;
        if (--count == 0x00u) break;
      }
    }
  }
  uint8_t fg = st->ram[SMBC_FOREGROUND_SCENERY];
  if (fg != 0x00u) {
    uint8_t y = SMBC_F_SCENE_DATA_OFFSETS[fg - 1u];
    for (uint8_t x = 0x00u; x < 0x0du; x++) { /* SceLoop2 */
      uint8_t d = SMBC_FORE_SCENERY_DATA[y];
      y++;
      if (d != 0x00u) st->ram[SMBC_METATILE_BUFFER + x] = d;
    }
  }
  /* RendTerr: the terrain metatile — castle wall in world 8's water */
  uint8_t terrain;
  uint8_t area_type = st->ram[SMBC_AREA_TYPE];
  if (area_type == 0x00u && st->ram[SMBC_WORLD_NUMBER] == SMBC_WORLD8) {
    terrain = 0x62u;
  } else {
    terrain = SMBC_TERRAIN_METATILES[area_type];
  }
  if (st->ram[SMBC_CLOUD_TYPE_OVERRIDE] != 0x00u) {
    terrain = 0x88u; /* the cloud block */
  }
  uint8_t x = 0x00u;
  uint8_t y = (uint8_t)(st->ram[SMBC_TERRAIN_CONTROL] * 2u);
  for (;;) { /* TerrLoop: the ceiling byte, then the floor byte */
    uint8_t bits = SMBC_TERRAIN_RENDER_BITS[y];
    y++;
    if (st->ram[SMBC_CLOUD_TYPE_OVERRIDE] != 0x00u && x != 0x00u) {
      bits &= 0x08u; /* clouds only keep the d3 floor edge */
    }
    for (uint8_t by = 0x00u; by < 0x08u; by++) { /* TerrBChk */
      if ((SMBC_BITMASKS[by] & bits) != 0x00u) {
        st->ram[SMBC_METATILE_BUFFER + x] = terrain;
      }
      x++;
      if (x == 0x0du) break;
      if (area_type == 0x02u && x == 0x0bu) {
        terrain = 0x54u; /* underground: ground level at the bottom */
      }
    }
    if (x == 0x0du) break; /* RendBBuf */
  }
  process_area_data(st); /* RendBBuf */
  uint16_t addr = get_block_buffer_addr(st->ram[SMBC_BLOCK_BUFFER_COLUMN_POS]);
  uint8_t by = 0x00u;
  for (x = 0x00u; x < 0x0du; x++) { /* ChkMTLow */
    uint8_t mt = st->ram[SMBC_METATILE_BUFFER + x];
    if (mt < SMBC_BLOCK_BUFF_LOW_BOUNDS[(mt & 0xc0u) >> 6]) {
      mt = 0x00u;
    }
    st->ram[addr + by] = mt;
    by = (uint8_t)(by + 0x10u);
  }
}

/* smb.asm: AreaParserCore — with the backloading flag set the decoder
 * runs once to skip ahead, then the column renders as usual (the
 * original falls through; there is no return between) */
static void area_parser_core(CoreState *st) {
  if (st->ram[SMBC_BACKLOADING_FLAG] != 0x00u) {
    process_area_data(st);
  }
  render_scenery_terrain(st);
}

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
