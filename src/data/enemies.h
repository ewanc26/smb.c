/* data/enemies.h — the loop command and loopback tables.
 *
 * Owns: smb.asm's LoopCmdWorldNumber/LoopCmdPageNumber/LoopCmdYPosition
 * (where each loop command lives and where the player must stand to pass
 * it) and AreaDataOfsLoopback (the level-data offset to rewind to when a
 * loop is failed), indexed by the loop data entry the interpreter finds.
 *
 * Collaborators: core/enemies reads all four from ProcLoopCommand and
 * ExecGameLoopback.
 */
#ifndef SMBC_DATA_ENEMIES_H
#define SMBC_DATA_ENEMIES_H

#include <stdint.h>

/* smb.asm: LoopCmdWorldNumber — eleven entries, worlds three through
 * seven (world numbers are zero-based, World7 = 6) */
extern const uint8_t SMBC_LOOP_CMD_WORLD_NUMBER[11];

/* smb.asm: LoopCmdPageNumber — the page each loop command sits on */
extern const uint8_t SMBC_LOOP_CMD_PAGE_NUMBER[11];

/* smb.asm: LoopCmdYPosition — the vertical coordinate the player must
 * stand at, on solid ground, to pass the loop */
extern const uint8_t SMBC_LOOP_CMD_Y_POSITION[11];

/* smb.asm: AreaDataOfsLoopback — the level object offset to rewind the
 * parser to when a loop is failed */
extern const uint8_t SMBC_AREA_DATA_OFS_LOOPBACK[11];

#endif /* SMBC_DATA_ENEMIES_H */
