/* hw/render.h — PPU picture generation.
 *
 * Owns: turning the PPU's register, VRAM and OAM state into a 256×240
 * frame of NES colour indices ($00-$3F, the 2C02's 6-bit palette
 * space). Background tiles decode through the pattern tables, name
 * tables and attribute tables exactly as the hardware assembles them;
 * sprites come from OAM with the original flips, palettes, priority
 * and ordering. Output is deterministic for identical PPU state.
 *
 * Not owned: scaling, windowing, RGB conversion and presentation —
 * those belong to the host frontend, which consumes the index buffer
 * and may map it through its own 2C02 palette approximation.
 *
 * Scroll resolution: each scanline composes against the newest scroll
 * write the raster clock had reached when it was made (the PPU's
 * scroll log), so the status-bar split — scroll (0,0) written during
 * vblank, gameplay scroll written after the sprite-0 hit — renders
 * faithfully. Writes tagged at scanline 241+ take effect from the top
 * of the frame, as the hardware's vertical reload provides.
 *
 * Deliberate approximations (documented per #32):
 *   - 8×8 sprites only: SMB never sets $2000 d5, so 8×16 sprites are
 *     not modelled.
 *   - $2001 d0 (grayscale) and the emphasis bits are not applied;
 *     the game never uses them.
 *   - Sprite Y is rendered at OAM byte + 1, matching the 2C02's
 *     one-scanline delay.
 */
#ifndef SMBC_HW_RENDER_H
#define SMBC_HW_RENDER_H

#include "ppu.h"

#define SMBC_RENDER_WIDTH 256u
#define SMBC_RENDER_HEIGHT 240u
#define SMBC_RENDER_PIXELS (SMBC_RENDER_WIDTH * SMBC_RENDER_HEIGHT)

/* Compose the frame into fb, which must hold SMBC_RENDER_PIXELS bytes.
 * Each byte is a 2C02 palette index ($00-$3F). When sprite0_scanline
 * is non-NULL it receives the first scanline on which an opaque
 * sprite-0 pixel overlaps an opaque background pixel, or -1 when no
 * overlap occurs — the transition the original game's split-screen
 * scroll waits on. */
void ppu_render_frame(const Ppu *ppu, uint8_t *fb, int *sprite0_scanline);

/* The sprite-0 overlap alone: the first scanline where sprite 0's
 * opaque pixels meet opaque background, or -1. This is the evaluation
 * the translated Sprite0Hit wait advances the raster clock to; it runs
 * against mid-frame PPU state, before the split scroll write exists,
 * exactly as the hardware evaluates it during the frame. */
int ppu_sprite0_hit_scanline(const Ppu *ppu);

#endif /* SMBC_HW_RENDER_H */
