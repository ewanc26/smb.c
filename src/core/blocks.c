/* core/blocks.c — see core/blocks.h for the contract. */
#include "blocks.h"

#include "../data/metatiles.h"
#include "move.h"
#include "offscreen.h"

/* smb.asm: PutBlockMetatile — style selects the BlockGfxData quad, y
 * the vram buffer offset plus one; bbuf_low and orig_y are the $06/$02
 * temps the caller set from the block object */
static void put_block_metatile(CoreState *st, uint8_t style, uint8_t y,
                               uint8_t bbuf_low, uint8_t orig_y) {
  uint8_t x = (uint8_t)(style << 2u);
  /* the odd-page block buffer maps to name table 1 */
  uint8_t hi = bbuf_low < 0xd0u ? 0x20u : 0x24u;
  uint8_t lo = (uint8_t)((bbuf_low & 0x0fu) << 1u);
  /* the vertical offset plus 32 pixels for the status bar, doubled
   * twice; the rotated-out bits and the low-byte carry rise into the
   * name table high byte */
  uint8_t a = (uint8_t)(orig_y + 0x20u);
  uint8_t rot = (uint8_t)(a >> 7u);
  a = (uint8_t)(a << 1u);
  rot = (uint8_t)((rot << 1u) | (a >> 7u));
  a = (uint8_t)(a << 1u);
  uint16_t sum = (uint16_t)a + lo;
  lo = (uint8_t)sum;
  hi = (uint8_t)(rot + (sum >> 8u) + hi);
  st->ram[SMBC_VRAM_BUFFER1 + 2u + y] = SMBC_BLOCK_GFX_DATA[x];
  st->ram[SMBC_VRAM_BUFFER1 + 3u + y] = SMBC_BLOCK_GFX_DATA[x + 1u];
  st->ram[SMBC_VRAM_BUFFER1 + 7u + y] = SMBC_BLOCK_GFX_DATA[x + 2u];
  st->ram[SMBC_VRAM_BUFFER1 + 8u + y] = SMBC_BLOCK_GFX_DATA[x + 3u];
  st->ram[SMBC_VRAM_BUFFER1 + y] = lo;
  st->ram[SMBC_VRAM_BUFFER1 + 5u + y] = (uint8_t)(lo + 0x20u);
  st->ram[SMBC_VRAM_BUFFER1 - 1u + y] = hi;
  st->ram[SMBC_VRAM_BUFFER1 + 4u + y] = hi;
  st->ram[SMBC_VRAM_BUFFER1 + 1u + y] = 0x02u;
  st->ram[SMBC_VRAM_BUFFER1 + 6u + y] = 0x02u;
  st->ram[SMBC_VRAM_BUFFER1 + 9u + y] = 0x00u;
}

/* smb.asm: WriteBlockMetatile — pick the replacement style for the
 * metatile, queue the write, and advance the vram buffer offset */
static void write_block_metatile(CoreState *st, uint8_t mt,
                                 uint8_t bbuf_low, uint8_t orig_y) {
  uint8_t style;
  if (mt == 0x00u) {
    style = 0x03u; /* blank */
  } else if (mt == 0x58u || mt == 0x51u) {
    style = 0x00u; /* brick with line */
  } else if (mt == 0x5du || mt == 0x52u) {
    style = 0x01u; /* brick without line */
  } else {
    style = 0x02u; /* empty block */
  }
  uint8_t y = (uint8_t)(st->ram[SMBC_VRAM_BUFFER1_OFFSET] + 1u);
  put_block_metatile(st, style, y, bbuf_low, orig_y);
  /* smb.asm: MoveVOffset / SetVRAMOffset */
  st->ram[SMBC_VRAM_BUFFER1_OFFSET] = (uint8_t)(y - 1u + 10u);
}

/* smb.asm: ReplaceBlockMetatile — the write plus the residual
 * bookkeeping the original never reads */
void replace_block_metatile(CoreState *st, uint8_t x) {
  write_block_metatile(st, st->ram[SMBC_BLOCK_METATILE + x],
                       st->ram[SMBC_BLOCK_BBUF_LOW + x],
                       st->ram[SMBC_BLOCK_ORIG_Y_POS + x]);
  st->ram[SMBC_BLOCK_RESIDUAL_COUNTER]++;
  st->ram[SMBC_BLOCK_REP_FLAG + x]--;
}

/* smb.asm: DestroyBlockMetatile — the head-bump blank write: the
 * block buffer position comes straight from the $06/$02 scratch the
 * collision routine left behind */
void destroy_block_metatile(CoreState *st) {
  write_block_metatile(st, 0x00u, st->ram[SMBC_BB_PTR_LO],
                       st->ram[SMBC_BB_Y_COORD]);
}

/* smb.asm: RemoveCoin_Axe — write the blank (or water blank)
 * metatile over the block buffer spot the $06/$02 scratch points at,
 * then set the vram address controller to the $0341 set */
void remove_coin_axe(CoreState *st) {
  uint8_t style = st->ram[SMBC_AREA_TYPE] != 0x00u ? 0x03u : 0x04u;
  put_block_metatile(st, style, 0x41u, st->ram[SMBC_BB_PTR_LO],
                     st->ram[SMBC_BB_Y_COORD]);
  st->ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x06u;
}

/* smb.asm: BlockObjMT_Updater — both block objects, second first; a
 * busy vram buffer or a clear flag skips the slot */
void block_obj_mt_updater(CoreState *st) {
  for (int x = 1; x >= 0; x--) {
    if (st->ram[SMBC_VRAM_BUFFER1] != 0x00u) continue;
    if (st->ram[SMBC_BLOCK_REP_FLAG + x] == 0x00u) continue;
    uint8_t bbuf_low = st->ram[SMBC_BLOCK_BBUF_LOW + x];
    uint8_t orig_y = st->ram[SMBC_BLOCK_ORIG_Y_POS + x];
    /* smb.asm: sta ($06),y — the metatile back into the block buffer */
    st->ram[(uint16_t)(0x0500u + bbuf_low + orig_y)] =
        st->ram[SMBC_BLOCK_METATILE + x];
    replace_block_metatile(st, (uint8_t)x);
    st->ram[SMBC_BLOCK_REP_FLAG + x] = 0x00u;
  }
}

/* smb.asm: DefaultBlockObjTiles — the two-by-two block object's sprite
 * tiles, brick with line (sprite tiles, not background tiles) */
static const uint8_t SMBC_DEFAULT_BLOCK_OBJ_TILES[4] = {
    0x85u, 0x85u, 0x86u, 0x86u};

/* smb.asm: DumpTwoSpr / DumpFourSpr — one byte into the sprite rows.
 * The callers add one to the base offset first so the writes land in
 * the tile-number bytes */
static void dump_two_spr(CoreState *st, uint8_t y, uint8_t a) {
  st->ram[SMBC_SPRITE_DATA + y] = a;
  st->ram[SMBC_SPRITE_DATA + 4u + y] = a;
}

static void dump_four_spr(CoreState *st, uint8_t y, uint8_t a) {
  dump_two_spr(st, y, a);
  st->ram[SMBC_SPRITE_DATA + 8u + y] = a;
  st->ram[SMBC_SPRITE_DATA + 12u + y] = a;
}

/* smb.asm: DrawSpriteObject — one row of two sprites: the tile pair
 * optionally swapped for a horizontal flip ($03 d1), the attributes,
 * the shared vertical and staggered horizontal coordinates, then the
 * $02 scratch and Y advance to the next row. Returns the new Y */
static uint8_t draw_sprite_object(CoreState *st, uint8_t y, uint8_t *t,
                                  uint8_t tile0, uint8_t tile1) {
  if ((t[3] & 0x02u) != 0x00u) {
    st->ram[SMBC_SPRITE_TILENUMBER + 4u + y] = tile0;
    st->ram[SMBC_SPRITE_TILENUMBER + y] = tile1;
    st->ram[SMBC_SPRITE_ATTRIBUTES + y] = (uint8_t)(0x40u | t[4]);
    st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] = (uint8_t)(0x40u | t[4]);
  } else {
    st->ram[SMBC_SPRITE_TILENUMBER + y] = tile0;
    st->ram[SMBC_SPRITE_TILENUMBER + 4u + y] = tile1;
    st->ram[SMBC_SPRITE_ATTRIBUTES + y] = t[4];
    st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] = t[4];
  }
  st->ram[SMBC_SPRITE_Y_POSITION + y] = t[2];
  st->ram[SMBC_SPRITE_Y_POSITION + 4u + y] = t[2];
  st->ram[SMBC_SPRITE_X_POSITION + y] = t[5];
  st->ram[SMBC_SPRITE_X_POSITION + 4u + y] = (uint8_t)(t[5] + 0x08u);
  t[2] = (uint8_t)(t[2] + 0x08u);
  return (uint8_t)(y + 0x08u);
}

/* smb.asm: DrawOneSpriteRow — the left tile via $00, the right in A */
static uint8_t draw_one_sprite_row(CoreState *st, uint8_t y, uint8_t *t,
                                   uint8_t left, uint8_t right) {
  t[0] = left;
  return draw_sprite_object(st, y, t, t[0], right);
}

/* smb.asm: DrawBlock — the block object's relative coordinates and the
 * default tiles fill two rows of sprites; then the lineless tiles, the
 * used-block ($c4) variant and the offscreen masking adjust them */
void draw_block(CoreState *st, uint8_t block) {
  uint8_t t[6] = {0x00u, 0x00u, 0x00u, 0x00u, 0x03u, 0x00u};
  t[2] = st->ram[SMBC_BLOCK_REL_YPOS];
  t[5] = st->ram[SMBC_BLOCK_REL_XPOS];
  t[3] = 0x01u; /* lsr of $03 — the flip bit is never used here */
  uint8_t y = st->ram[SMBC_BLOCK_SPR_DATA_OFFSET + block];
  uint8_t ofs = 0x00u;
  do {
    y = draw_one_sprite_row(st, y, t, SMBC_DEFAULT_BLOCK_OBJ_TILES[ofs],
                            SMBC_DEFAULT_BLOCK_OBJ_TILES[ofs + 1u]);
    ofs = (uint8_t)(ofs + 2u);
  } while (ofs != 0x04u);
  y = st->ram[SMBC_BLOCK_SPR_DATA_OFFSET + block];
  /* outside ground-type areas the lined brick tiles become lineless */
  if (st->ram[SMBC_AREA_TYPE] != 0x01u) {
    st->ram[SMBC_SPRITE_TILENUMBER + y] = 0x86u;
    st->ram[SMBC_SPRITE_TILENUMBER + 4u + y] = 0x86u;
  }
  if (st->ram[SMBC_BLOCK_METATILE + block] == 0xc4u) {
    dump_four_spr(st, (uint8_t)(y + 1u), 0x87u);
    uint8_t attrib = 0x03u;
    if (st->ram[SMBC_AREA_TYPE] != 0x01u) attrib = (uint8_t)(0x03u >> 1u);
    st->ram[SMBC_SPRITE_ATTRIBUTES + y] = attrib;
    st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + y] = (uint8_t)(attrib | 0x40u);
    st->ram[SMBC_SPRITE_ATTRIBUTES + 12u + y] =
        (uint8_t)(attrib | 0x40u | 0x80u);
    st->ram[SMBC_SPRITE_ATTRIBUTES + 8u + y] =
        (uint8_t)((attrib | 0x40u | 0x80u) & 0x83u);
  }
  uint8_t off = st->ram[SMBC_BLOCK_OFFSCREEN_BITS];
  if ((off & 0x04u) != 0x00u) {
    st->ram[SMBC_SPRITE_Y_POSITION + 4u + y] = 0xf8u;
    st->ram[SMBC_SPRITE_Y_POSITION + 12u + y] = 0xf8u;
  }
  if ((off & 0x08u) != 0x00u) {
    st->ram[SMBC_SPRITE_Y_POSITION + y] = 0xf8u;
    st->ram[SMBC_SPRITE_Y_POSITION + 8u + y] = 0xf8u;
  }
}

/* smb.asm: DrawBrickChunks — the shattered brick's four chunks. The
 * tiles and attributes come first (palette bits from $00), then the
 * two chunks' Y and staggered X coordinates, then the offscreen
 * masking shared with DrawBlock */
void draw_brick_chunks(CoreState *st, uint8_t block) {
  uint8_t palette = 0x02u;
  uint8_t tile = 0x75u;
  if (st->ram[SMBC_GAME_ENGINE_SUBROUTINE] != 0x05u) {
    palette = 0x03u;
    tile = 0x84u;
  }
  uint8_t y = st->ram[SMBC_BLOCK_SPR_DATA_OFFSET + block];
  dump_four_spr(st, (uint8_t)(y + 1u), tile);
  uint8_t a = (uint8_t)(st->ram[SMBC_FRAME_COUNTER] << 4u);
  a = (uint8_t)((a & 0xc0u) | palette);
  dump_four_spr(st, (uint8_t)(y + 2u), a);
  /* smb.asm: Y coordinate of the first two sprites */
  dump_two_spr(st, y, st->ram[SMBC_BLOCK_REL_YPOS]);
  st->ram[SMBC_SPRITE_X_POSITION + y] = st->ram[SMBC_BLOCK_REL_XPOS];
  uint8_t orig_rel =
      (uint8_t)(st->ram[SMBC_BLOCK_ORIG_XPOS + block] -
                st->ram[SMBC_SCREEN_LEFT_X_POS]);
  /* sec/sbc Block_Rel_XPos, adc $00, adc #$06 — the sbc carry feeds the
   * adc, faithful to the asm's residual arithmetic */
  uint8_t carry = orig_rel >= st->ram[SMBC_BLOCK_REL_XPOS] ? 1u : 0u;
  uint8_t x1 = (uint8_t)(orig_rel - st->ram[SMBC_BLOCK_REL_XPOS]);
  x1 = (uint8_t)(x1 + orig_rel + carry + 0x06u);
  st->ram[SMBC_SPRITE_X_POSITION + 4u + y] = x1;
  st->ram[SMBC_SPRITE_Y_POSITION + 8u + y] = st->ram[SMBC_BLOCK_REL_YPOS + 1u];
  st->ram[SMBC_SPRITE_Y_POSITION + 12u + y] =
      st->ram[SMBC_BLOCK_REL_YPOS + 1u];
  st->ram[SMBC_SPRITE_X_POSITION + 8u + y] = st->ram[SMBC_BLOCK_REL_XPOS + 1u];
  carry = orig_rel >= st->ram[SMBC_BLOCK_REL_XPOS + 1u] ? 1u : 0u;
  uint8_t x3 = (uint8_t)(orig_rel - st->ram[SMBC_BLOCK_REL_XPOS + 1u]);
  x3 = (uint8_t)(x3 + orig_rel + carry + 0x06u);
  st->ram[SMBC_SPRITE_X_POSITION + 12u + y] = x3;
  /* smb.asm: ChkLeftCo — the left column masked off */
  uint8_t off = st->ram[SMBC_BLOCK_OFFSCREEN_BITS];
  if ((off & 0x08u) != 0x00u) {
    st->ram[SMBC_SPRITE_Y_POSITION + y] = 0xf8u;
    st->ram[SMBC_SPRITE_Y_POSITION + 8u + y] = 0xf8u;
  }
  /* smb.asm: d7 set — park the top row */
  if ((off & 0x80u) != 0x00u) dump_two_spr(st, y, 0xf8u);
  /* smb.asm: ChnkOfs — bpl leaves when d7 is clear; otherwise the right
   * half is parked once it reaches the left half */
  if ((orig_rel & 0x80u) == 0x00u) return;
  if (st->ram[SMBC_SPRITE_X_POSITION + y] <
      st->ram[SMBC_SPRITE_X_POSITION + 4u + y]) {
    return;
  }
  st->ram[SMBC_SPRITE_Y_POSITION + 4u + y] = 0xf8u;
  st->ram[SMBC_SPRITE_Y_POSITION + 12u + y] = 0xf8u;
}

/* smb.asm: InitBlock_XY_Pos — the player's position plus eight
 * pixels, snapped to the 16-pixel grid, becomes the block object's */
void init_block_xy_pos(CoreState *st, uint8_t x) {
  uint16_t pos = (uint16_t)st->ram[SMBC_PLAYER_X_POSITION] + 0x08u;
  st->ram[SMBC_BLOCK_X_POSITION + x] = (uint8_t)pos & 0xf0u;
  uint8_t page =
      (uint8_t)(st->ram[SMBC_PLAYER_PAGE_LOC] + (uint8_t)(pos >> 8));
  st->ram[SMBC_BLOCK_PAGE_LOC + x] = page;
  st->ram[SMBC_BLOCK_PAGE_LOC2 + x] = page;
  st->ram[SMBC_BLOCK_Y_HIGH_POS + x] = st->ram[SMBC_PLAYER_Y_HIGH_POS];
}

/* smb.asm: BlockObjectsCore — state zero leaves the block asleep; a
 * state low nybble of one is a bouncing block (chunk A only), anything
 * else is a pair of brick chunks (chunks A and B). After the movement
 * the object is either parked at the bottom of the screen, marked for
 * metatile replacement on landing, or killed */
void block_objects_core(CoreState *st, uint8_t block) {
  uint8_t state = st->ram[SMBC_BLOCK_STATE + block];
  if (state == 0x00u) return; /* beq UpdSte — store zero and leave */
  uint8_t nybble = (uint8_t)(state & 0x0fu);
  if (nybble == 0x01u) {
    /* BouncingBlockHandler — chunk A only */
    impose_gravity_block(st, (uint8_t)(8u + block));
    relative_block_position(st, block);
    get_block_offscreen_bits(st, block);
    draw_block(st, block);
    if ((st->ram[SMBC_BLOCK_Y_POSITION + block] & 0x0fu) >= 0x05u) {
      /* bcs UpdSte — still above the amount, keep the state */
      st->ram[SMBC_BLOCK_STATE + block] = nybble;
      return;
    }
    /* the low nybble wrapped: mark for replacement and kill */
    st->ram[SMBC_BLOCK_REP_FLAG + block] = 0x01u;
    st->ram[SMBC_BLOCK_STATE + block] = 0x00u;
    return;
  }
  /* brick chunks: both chunks obey gravity and horizontal movement */
  impose_gravity_block(st, (uint8_t)(8u + block));
  (void)move_enemy_horizontally(st, (uint8_t)(8u + block));
  impose_gravity_block(st, (uint8_t)(10u + block));
  (void)move_enemy_horizontally(st, (uint8_t)(10u + block));
  relative_block_position(st, block);
  get_block_offscreen_bits(st, block);
  draw_brick_chunks(st, block);
  if (st->ram[SMBC_BLOCK_Y_HIGH_POS + block] == 0x00u) {
    /* beq UpdSte — the low nybble is written back unchanged */
    st->ram[SMBC_BLOCK_STATE + block] = nybble;
    return;
  }
  /* the bottom chunk at or past the bottom of the screen is parked */
  if (st->ram[SMBC_BLOCK_Y_POSITION + block + 2u] > 0xf0u) {
    st->ram[SMBC_BLOCK_Y_POSITION + block + 2u] = 0xf0u;
  }
  if (st->ram[SMBC_BLOCK_Y_POSITION + block] >= 0xf0u) {
    /* bcs KillBlock — the top chunk reached the bottom */
    st->ram[SMBC_BLOCK_STATE + block] = 0x00u;
    return;
  }
  st->ram[SMBC_BLOCK_STATE + block] = nybble;
}
