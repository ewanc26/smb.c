/* host/palette.h — 2C02 RGB approximation.
 *
 * Owns: mapping the renderer's 6-bit NES colour indices to RGB triples
 * the host can present. This is the frontend's palette approximation —
 * the core only ever deals in indices (hw/render.h documents the
 * split). The table is the canonical 2C02 emphasis-neutral values.
 *
 * Not owned: anything about how the pixels are scaled or presented. */
#ifndef SMBC_HOST_PALETTE_H
#define SMBC_HOST_PALETTE_H

#include <stdint.h>

/* 64 entries of {R, G, B}. */
extern const uint8_t SMBC_2C02_RGB[64][3];

#endif /* SMBC_HOST_PALETTE_H */
