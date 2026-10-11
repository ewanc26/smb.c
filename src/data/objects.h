#ifndef SMBC_DATA_OBJECTS_H
#define SMBC_DATA_OBJECTS_H

/* data/objects.h — area object render tables (smb.asm: the lookup tables
 * referenced by the DecodeAreaData object handlers, lines 3472-3551 and
 * 3753-3764 and 7357-7367). The decoder and handlers live in core/parser.c. */

#include <stdint.h>

/* smb.asm: FrenzyIDData — frenzy enemy ids for the row-13 AreaFrenzy
 * objects (FlyCheepCheepFrenzy, BBill_CCheep_Frenzy, Stop_Frenzy) */
extern const uint8_t SMBC_FRENZY_ID_DATA[3];

/* smb.asm: PulleyRopeMetatiles — left pulley, rope, right pulley */
extern const uint8_t SMBC_PULLEY_ROPE_METATILES[3];

/* smb.asm: CastleMetatiles — the 55 metatiles of the castle object,
 * column-major in bands of five */
extern const uint8_t SMBC_CASTLE_METATILES[55];

/* smb.asm: SidePipeShaftData/SidePipeTopPart/SidePipeBottomPart —
 * the sideways (intro/exit) pipe parts by horizontal length */
extern const uint8_t SMBC_SIDE_PIPE_SHAFT_DATA[4];
extern const uint8_t SMBC_SIDE_PIPE_TOP_PART[4];
extern const uint8_t SMBC_SIDE_PIPE_BOTTOM_PART[4];

/* smb.asm: VerticalPipeData — four pipe varieties (shaft bottom,
 * shaft top, lip left, lip right); warp pipes use the base set */
extern const uint8_t SMBC_VERTICAL_PIPE_DATA[8];

/* smb.asm: CoinMetatileData — the coin metatile by area type */
extern const uint8_t SMBC_COIN_METATILE_DATA[4];

/* smb.asm: C_ObjectRow/C_ObjectMetatile — the "C objects" (castle
 * bridge, axe, rope chain): row and metatile indexed by id - 2 */
extern const uint8_t SMBC_C_OBJECT_ROW[3];
extern const uint8_t SMBC_C_OBJECT_METATILE[3];

/* smb.asm: SolidBlockMetatiles — the solid block metatile by area
 * type; the first entry is the underwater/water brick metatile */
extern const uint8_t SMBC_SOLID_BLOCK_METATILES[4];

/* smb.asm: BrickMetatiles — brick by area type, with the fifth entry
 * ($88) substituted for the cloud-type override */
extern const uint8_t SMBC_BRICK_METATILES[5];

/* smb.asm: StaircaseHeightData/StaircaseRowData — reverse staircase
 * geometry, indexed by the staircase control counter */
extern const uint8_t SMBC_STAIRCASE_HEIGHT_DATA[9];
extern const uint8_t SMBC_STAIRCASE_ROW_DATA[9];

/* smb.asm: HoleMetatiles — the hole metatile by area type; only water
 * area types ($87) leave any trace, the rest are empty */
extern const uint8_t SMBC_HOLE_METATILES[4];

/* smb.asm: BrickQBlockMetatiles — question block and brick-with-item
 * metatiles, selected by object id plus the area-type offset */
extern const uint8_t SMBC_BRICK_Q_BLOCK_METATILES[14];

/* smb.asm: XOffscreenBitsData / YOffscreenBitsData — the sprite-row
 * masks describing how much of an object is off a screen edge;
 * DefaultXOnscreenOfs / DefaultYOnscreenOfs are the fallback table
 * offsets and HighPosUnitData the vertical unit edge coordinates */
extern const uint8_t SMBC_X_OFFSCREEN_BITS_DATA[16];
extern const uint8_t SMBC_Y_OFFSCREEN_BITS_DATA[9];
extern const uint8_t SMBC_DEFAULT_X_ONSCREEN_OFS[3];
extern const uint8_t SMBC_DEFAULT_Y_ONSCREEN_OFS[3];
extern const uint8_t SMBC_HIGH_POS_UNIT_DATA[2];

/* smb.asm: XSpeedAdderData — the temporary speed adder used to slow
 * horizontal movement, indexed by direction and the slow flag;
 * RevivedXSpeed — the speed a revived stunned enemy gets, by
 * direction and primary hard mode */
extern const uint8_t SMBC_X_SPEED_ADDER_DATA[4];
extern const uint8_t SMBC_REVIVED_X_SPEED[4];

#endif /* SMBC_DATA_OBJECTS_H */
