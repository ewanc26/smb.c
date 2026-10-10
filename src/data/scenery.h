/* data/scenery.h — scenery and terrain tables.
 *
 * Owns: the background/foreground scenery data and metatiles, the
 * terrain metatiles and render bits, the bitmasks the terrain loop
 * tests against, and the block buffer store bounds — smb.asm's
 * BackSceneryData through BlockBuffLowBounds, translated verbatim.
 * Consumed by core/parser's RenderSceneryTerrain.
 */
#ifndef SMBC_DATA_SCENERY_H
#define SMBC_DATA_SCENERY_H

#include <stdint.h>

#define SMBC_B_SCENE_DATA_OFFSETS_SIZE 3u
#define SMBC_BACK_SCENERY_DATA_SIZE    144u /* 48 columns, 3 scenery sets */
#define SMBC_BACK_SCENERY_METATILES_SIZE 36u /* 12 shapes, 3 metatiles each */
#define SMBC_F_SCENE_DATA_OFFSETS_SIZE 3u
#define SMBC_FORE_SCENERY_DATA_SIZE    39u /* 13 rows, 3 scenery sets */
#define SMBC_TERRAIN_METATILES_SIZE    4u  /* one per area type */
#define SMBC_TERRAIN_RENDER_BITS_SIZE  32u /* 16 controls, ceiling + floor */
#define SMBC_BITMASKS_SIZE             8u
#define SMBC_BLOCK_BUFF_LOW_BOUNDS_SIZE 4u

/* smb.asm: BSceneDataOffsets */
extern const uint8_t SMBC_B_SCENE_DATA_OFFSETS[SMBC_B_SCENE_DATA_OFFSETS_SIZE];

/* smb.asm: BackSceneryData — per-column scenery selectors: high nybble is
 * the metatile buffer height, low nybble the shape (1-based) */
extern const uint8_t SMBC_BACK_SCENERY_DATA[SMBC_BACK_SCENERY_DATA_SIZE];

/* smb.asm: BackSceneryMetatiles */
extern const uint8_t SMBC_BACK_SCENERY_METATILES[SMBC_BACK_SCENERY_METATILES_SIZE];

/* smb.asm: FSceneDataOffsets */
extern const uint8_t SMBC_F_SCENE_DATA_OFFSETS[SMBC_F_SCENE_DATA_OFFSETS_SIZE];

/* smb.asm: ForeSceneryData */
extern const uint8_t SMBC_FORE_SCENERY_DATA[SMBC_FORE_SCENERY_DATA_SIZE];

/* smb.asm: TerrainMetatiles */
extern const uint8_t SMBC_TERRAIN_METATILES[SMBC_TERRAIN_METATILES_SIZE];

/* smb.asm: TerrainRenderBits — per TerrainControl: ceiling byte, floor byte */
extern const uint8_t SMBC_TERRAIN_RENDER_BITS[SMBC_TERRAIN_RENDER_BITS_SIZE];

/* smb.asm: Bitmasks */
extern const uint8_t SMBC_BITMASKS[SMBC_BITMASKS_SIZE];

/* smb.asm: BlockBuffLowBounds — metatiles below these (per attribute bits)
 * are not stored in the block buffer */
extern const uint8_t SMBC_BLOCK_BUFF_LOW_BOUNDS[SMBC_BLOCK_BUFF_LOW_BOUNDS_SIZE];

#endif /* SMBC_DATA_SCENERY_H */
