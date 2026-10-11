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

#define SMBC_PLAYER_GFX_TBL_OFFSETS_SIZE 16u
#define SMBC_PLAYER_GRAPHICS_TABLE_SIZE 208u
#define SMBC_SWIM_KICK_TILE_NUM_SIZE 2u
#define SMBC_INTERMEDIATE_PLAYER_DATA_SIZE 6u
#define SMBC_CHANGE_SIZE_OFFSET_ADDER_SIZE 20u

/* smb.asm: PlayerGfxTblOffsets — the graphics-table offset per pose
 * (walking through killed, then the shared small/intermediate/big
 * standing rows). */
extern const uint8_t
    SMBC_PLAYER_GFX_TBL_OFFSETS[SMBC_PLAYER_GFX_TBL_OFFSETS_SIZE];

/* smb.asm: PlayerGraphicsTable — the player's sprite tiles, two per
 * row: 96 bytes of big-player poses, 88 of small-player poses, then 24
 * shared standing/growing rows. */
extern const uint8_t
    SMBC_PLAYER_GRAPHICS_TABLE[SMBC_PLAYER_GRAPHICS_TABLE_SIZE];

/* smb.asm: SwimKickTileNum */
extern const uint8_t SMBC_SWIM_KICK_TILE_NUM[SMBC_SWIM_KICK_TILE_NUM_SIZE];

/* smb.asm: IntermediatePlayerData — the standing pose used on the
 * world/lives display: Y position, flip, attributes, X position,
 * unused byte, row count. */
extern const uint8_t
    SMBC_INTERMEDIATE_PLAYER_DATA[SMBC_INTERMEDIATE_PLAYER_DATA_SIZE];

/* smb.asm: ChangeSizeOffsetAdder */
extern const uint8_t
    SMBC_CHANGE_SIZE_OFFSET_ADDER[SMBC_CHANGE_SIZE_OFFSET_ADDER_SIZE];

#endif /* SMBC_DATA_SPRITES_H */
