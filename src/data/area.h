/* data/area.h — area pointer offset tables.
 *
 * Owns: the per-world base offsets and the flat area pointer bytes that
 * FindAreaPointer (core/area) indexes to turn a (WorldNumber, AreaNumber)
 * pair into an area pointer. The enemy/area data address tables that
 * consume the pointer arrive with the area data itself.
 */
#ifndef SMBC_DATA_AREA_H
#define SMBC_DATA_AREA_H

#include <stdint.h>

#define SMBC_WORLD_ADDR_OFFSETS_SIZE 8u
#define SMBC_AREA_ADDR_OFFSETS_SIZE  36u

/* smb.asm: WorldAddrOffsets — base index into AreaAddrOffsets per world */
extern const uint8_t SMBC_WORLD_ADDR_OFFSETS[SMBC_WORLD_ADDR_OFFSETS_SIZE];

/* smb.asm: AreaAddrOffsets — area pointer bytes, worlds 1-8 laid out
 * contiguously (5, 5, 4, 5, 4, 4, 5, 4 entries) */
extern const uint8_t SMBC_AREA_ADDR_OFFSETS[SMBC_AREA_ADDR_OFFSETS_SIZE];

#endif /* SMBC_DATA_AREA_H */
