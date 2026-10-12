/* data/metatiles.h — metatile graphics tables.
 *
 * Owns: the four metatile palettes, translated verbatim from the
 * disassembly's METATILE GRAPHICS TABLE. Each metatile is four bytes:
 * upper-left, lower-left, upper-right, lower-right tile indices. The
 * address table of the original (MetatileGraphics_Low/High) is replaced by
 * a single array of pointers here. Consumers (the area parser) arrive with
 * GameMode; see issue #2.
 */
#ifndef SMBC_DATA_METATILES_H
#define SMBC_DATA_METATILES_H

#include <stddef.h>
#include <stdint.h>

#define SMBC_METATILE_SIZE 4u
#define SMBC_PALETTE0_MTILES_COUNT 39u
#define SMBC_PALETTE1_MTILES_COUNT 46u
#define SMBC_PALETTE2_MTILES_COUNT 10u
#define SMBC_PALETTE3_MTILES_COUNT 6u

/* smb.asm: Palette0_MTiles .. Palette3_MTiles */
extern const uint8_t
    SMBC_PALETTE0_MTILES[SMBC_PALETTE0_MTILES_COUNT * SMBC_METATILE_SIZE];
extern const uint8_t
    SMBC_PALETTE1_MTILES[SMBC_PALETTE1_MTILES_COUNT * SMBC_METATILE_SIZE];
extern const uint8_t
    SMBC_PALETTE2_MTILES[SMBC_PALETTE2_MTILES_COUNT * SMBC_METATILE_SIZE];
extern const uint8_t
    SMBC_PALETTE3_MTILES[SMBC_PALETTE3_MTILES_COUNT * SMBC_METATILE_SIZE];

#define SMBC_METATILE_GRAPHICS_COUNT 4u

/* smb.asm: MetatileGraphics_Low / MetatileGraphics_High, folded into one
 * pointer table */
extern const uint8_t *const SMBC_METATILE_GRAPHICS[SMBC_METATILE_GRAPHICS_COUNT];

/* metatile counts per palette, matching SMBC_METATILE_GRAPHICS */
extern const uint8_t SMBC_METATILE_COUNTS[SMBC_METATILE_GRAPHICS_COUNT];

#endif /* SMBC_DATA_METATILES_H */

/* smb.asm: BlockGfxData — the four tiles of each replacement style:
 * brick with line, brick without, empty block, blank, and brick with
 * coins (the two brick-with-coins entries share the first style).
 * Read by PutBlockMetatile in core/blocks and RemBridge in core/victory */
extern const uint8_t SMBC_BLOCK_GFX_DATA[20];
