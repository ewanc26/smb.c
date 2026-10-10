/* data/chr.h — CHR-ROM pattern tables.
 *
 * Owns: the 8KB of graphics data from the donor cartridge. This is the one
 * asset the disassembly does not contain: smb.asm covers PRG code and data
 * only, so these bytes come from the ROM image itself (see chr.c for
 * provenance). Two pattern tables of 256 tiles each, 16 bytes per tile.
 *
 * Collaborators: a frontend renderer consumes the tiles; the PPU model
 * does not reference them directly.
 */
#ifndef SMBC_DATA_CHR_H
#define SMBC_DATA_CHR_H

#include <stdint.h>

#define SMBC_CHR_SIZE 0x2000u      /* 8KB: two 4KB pattern tables */
#define SMBC_CHR_TILE_COUNT 512u   /* 256 tiles per table */
#define SMBC_CHR_TILE_BYTES 16u    /* two 8-row bitplanes per tile */

extern const uint8_t SMBC_CHR_ROM[SMBC_CHR_SIZE];

#endif /* SMBC_DATA_CHR_H */
