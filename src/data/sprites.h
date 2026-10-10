/* data/sprites.h — sprite data offset tables.
 *
 * Owns: the default sprite data offsets and the sprite 0 template,
 * translated verbatim from the disassembly. Seeded into RAM by
 * core/setup's SecondaryGameSetup.
 */
#ifndef SMBC_DATA_SPRITES_H
#define SMBC_DATA_SPRITES_H

#include <stdint.h>

#define SMBC_DEFAULT_SPR_OFFSETS_SIZE 15u
#define SMBC_SPRITE0_DATA_SIZE 4u

/* smb.asm: DefaultSprOffsets */
extern const uint8_t SMBC_DEFAULT_SPR_OFFSETS[SMBC_DEFAULT_SPR_OFFSETS_SIZE];

/* smb.asm: Sprite0Data */
extern const uint8_t SMBC_SPRITE0_DATA[SMBC_SPRITE0_DATA_SIZE];

#endif /* SMBC_DATA_SPRITES_H */
