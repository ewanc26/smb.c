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

/* smb.asm: ColorRotatePalette — the background colour cycled through one
 * palette slot every eighth frame */
extern const uint8_t SMBC_COLOR_ROTATE_PALETTE[6];

/* smb.asm: BlankPalette — a palette-3 update set (address $3f $0c,
 * length $04) whose data bytes are patched in by ColorRotation */
extern const uint8_t SMBC_BLANK_PALETTE[8];

/* smb.asm: Palette3Data — four palette-3 sets chosen by AreaType */
extern const uint8_t SMBC_PALETTE_3_DATA[16];

#endif /* SMBC_DATA_PALETTES_H */
