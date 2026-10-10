/* data/palettes.h — palette VRAM buffer sets.
 *
 * Owns: the ROM-resident palette update sets from the disassembly's VRAM
 * BUFFER DATA section, in WriteBufferToScreen format: address high/low,
 * length byte, data, zero terminator. core/screen's update set writer
 * consumes them through the VRAM address table.
 */
#ifndef SMBC_DATA_PALETTES_H
#define SMBC_DATA_PALETTES_H

#include <stdint.h>

/* smb.asm: WaterPaletteData .. BowserPaletteData */
extern const uint8_t SMBC_WATER_PALETTE_DATA[36];
extern const uint8_t SMBC_GROUND_PALETTE_DATA[36];
extern const uint8_t SMBC_UNDERGROUND_PALETTE_DATA[36];
extern const uint8_t SMBC_CASTLE_PALETTE_DATA[36];
extern const uint8_t SMBC_DAY_SNOW_PALETTE_DATA[8];
extern const uint8_t SMBC_NIGHT_SNOW_PALETTE_DATA[8];
extern const uint8_t SMBC_MUSHROOM_PALETTE_DATA[8];
extern const uint8_t SMBC_BOWSER_PALETTE_DATA[8];

#endif /* SMBC_DATA_PALETTES_H */
