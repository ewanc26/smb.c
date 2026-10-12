/* hw/render.c — see hw/render.h for the contract. */
#include "render.h"

#include <stdbool.h>
#include <stddef.h>

/* Side-effect-free VRAM/pattern read: the cartridge's CHR answers the
 * pattern tables when nominated, the flat image otherwise; name-table
 * and palette addresses resolve through the same mirroring the
 * register path applies. */
static uint8_t render_vram(const Ppu *ppu, uint16_t addr) {
  addr &= 0x3fffu;
  if (addr < 0x2000u && ppu->chr != NULL) return ppu->chr[addr];
  if (addr >= 0x3f00u) return ppu->vram[0x3f00u | (addr & 0x1fu)];
  if (addr >= 0x3000u) addr = (uint16_t)(addr & 0x2fffu);
  if (addr >= 0x2000u) addr = (uint16_t)(0x2000u | (addr & 0x07ffu));
  return ppu->vram[addr];
}

/* The 2-bit palette an attribute-table byte assigns to a pixel's
 * 16×16 block. */
static uint8_t render_bg_palette(const Ppu *ppu, uint16_t nt_base,
                                 uint8_t tx, uint8_t ty) {
  uint8_t attr =
      render_vram(ppu, (uint16_t)(nt_base + 0x03c0u + (ty / 32u) * 8u +
                                  tx / 32u));
  uint8_t shift = (uint8_t)(((ty & 0x10u) ? 4u : 0u) |
                            ((tx & 0x10u) ? 2u : 0u));
  return (uint8_t)((attr >> shift) & 0x03u);
}

/* One pixel of a pattern-table tile: 0-3, where 0 is transparent. */
static uint8_t render_tile_pixel(const Ppu *ppu, uint16_t pattern_base,
                                 uint8_t tile, uint8_t col, uint8_t row) {
  uint16_t addr = (uint16_t)(pattern_base + tile * 16u + row);
  uint8_t lo = render_vram(ppu, addr);
  uint8_t hi = render_vram(ppu, (uint16_t)(addr + 8u));
  uint8_t bit = (uint8_t)(7u - col);
  return (uint8_t)(((lo >> bit) & 0x01u) | (((hi >> bit) & 0x01u) << 1));
}

/* The background layer for one scanline: palette index per pixel and
 * an opaque mask, honouring the scroll pair, the base name-table bits
 * and the $2001 d3 / left-column (d1) enables. */
static void render_bg_scanline(const Ppu *ppu, uint8_t y, uint8_t *pix,
                               bool *opaque) {
  uint16_t bg_base = (ppu->ctrl1 & 0x10u) ? 0x1000u : 0x0000u;
  unsigned fx0 = (unsigned)ppu->scroll_x + (ppu->ctrl1 & 0x01u) * 256u;
  unsigned fy = (unsigned)y + ppu->scroll_y + ((ppu->ctrl1 >> 1) & 0x01u) * 240u;
  unsigned nt_y = (fy / 240u) & 0x01u;
  uint8_t ty = (uint8_t)(fy % 240u);
  for (unsigned x = 0; x < SMBC_RENDER_WIDTH; x++) {
    pix[x] = 0x00u;
    opaque[x] = false;
    if ((ppu->ctrl2 & 0x08u) == 0x00u) continue; /* background off */
    if (x < 8u && (ppu->ctrl2 & 0x02u) == 0x00u) continue; /* clipped */
    unsigned fx = (fx0 + x) & 0x01ffu;
    uint8_t tx = (uint8_t)(fx % 256u);
    uint16_t nt_base =
        (uint16_t)(0x2000u + nt_y * 0x0800u + (fx / 256u) * 0x0400u);
    uint8_t tile = render_vram(
        ppu, (uint16_t)(nt_base + (ty / 8u) * 32u + tx / 8u));
    uint8_t p = render_tile_pixel(ppu, bg_base, tile, (uint8_t)(tx & 7u),
                                  (uint8_t)(ty & 7u));
    if (p == 0x00u) continue;
    uint8_t pal = render_bg_palette(ppu, nt_base, tx, ty);
    pix[x] = render_vram(ppu, (uint16_t)(0x3f00u + pal * 4u + p));
    opaque[x] = true;
  }
}

void ppu_render_frame(const Ppu *ppu, uint8_t *fb, int *sprite0_scanline) {
  uint8_t backdrop = render_vram(ppu, 0x3f00u);
  uint16_t spr_base = (ppu->ctrl1 & 0x08u) ? 0x1000u : 0x0000u;
  bool hit = false;
  int hit_y = -1;
  uint8_t bg_pix[SMBC_RENDER_WIDTH];
  bool bg_opaque[SMBC_RENDER_WIDTH];

  for (uint8_t y = 0; y < SMBC_RENDER_HEIGHT; y++) {
    render_bg_scanline(ppu, y, bg_pix, bg_opaque);
    /* sprites compose back to front so the lowest OAM index — the
     * original's highest priority — lands last and wins */
    uint8_t spr_pix[SMBC_RENDER_WIDTH];
    bool spr_opaque[SMBC_RENDER_WIDTH];
    bool spr_behind[SMBC_RENDER_WIDTH];
    for (unsigned x = 0; x < SMBC_RENDER_WIDTH; x++) {
      fb[(unsigned)y * SMBC_RENDER_WIDTH + x] =
          bg_opaque[x] ? bg_pix[x] : backdrop;
      spr_opaque[x] = false;
      spr_behind[x] = false;
    }
    if ((ppu->ctrl2 & 0x10u) != 0x00u) { /* sprites enabled */
      for (int i = 63; i >= 0; i--) {
        uint8_t sy = ppu->oam[i * 4];
        uint8_t tile = ppu->oam[i * 4 + 1];
        uint8_t attr = ppu->oam[i * 4 + 2];
        uint8_t sx = ppu->oam[i * 4 + 3];
        int row = (int)y - ((int)sy + 1); /* the 2C02's one-scanline delay */
        if (row < 0 || row >= 8) continue;
        if ((attr & 0x80u) != 0x00u) row = 7 - row; /* vertical flip */
        for (unsigned cx = 0; cx < 8u; cx++) {
          unsigned x = (unsigned)sx + cx;
          if (x >= SMBC_RENDER_WIDTH) break;
          if (x < 8u && (ppu->ctrl2 & 0x04u) == 0x00u) continue; /* clipped */
          uint8_t col = (uint8_t)cx;
          if ((attr & 0x40u) != 0x00u) col = (uint8_t)(7u - col); /* h-flip */
          uint8_t p = render_tile_pixel(ppu, spr_base, tile, col,
                                        (uint8_t)row);
          if (p == 0x00u) continue;
          /* sprite 0's first opaque overlap with the background is the
           * hit the status-bar split waits on */
          if (i == 0 && bg_opaque[x] && !hit && x < 255u) {
            hit = true;
            hit_y = y;
          }
          spr_pix[x] = render_vram(
              ppu, (uint16_t)(0x3f10u + (attr & 0x03u) * 4u + p));
          spr_opaque[x] = true;
          spr_behind[x] = (attr & 0x20u) != 0x00u;
        }
      }
      for (unsigned x = 0; x < SMBC_RENDER_WIDTH; x++) {
        if (!spr_opaque[x]) continue;
        if (spr_behind[x] && bg_opaque[x]) continue;
        fb[(unsigned)y * SMBC_RENDER_WIDTH + x] = spr_pix[x];
      }
    }
  }
  if (sprite0_scanline != NULL) *sprite0_scanline = hit ? hit_y : -1;
}
