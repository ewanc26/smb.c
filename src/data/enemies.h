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

/* smb.asm: NormalXSpdData — horizontal speed for normal enemies,
 * indexed by primary hard mode */
extern const uint8_t SMBC_NORMAL_X_SPD_DATA[2];

/* smb.asm: HBroWalkingTimerData — hammer bro walking delay, indexed by
 * secondary hard mode */
extern const uint8_t SMBC_HBRO_WALKING_TIMER_DATA[2];

/* smb.asm: FirebarSpinSpdData / FirebarSpinDirData — firebar spin
 * speed and direction, indexed by identifier minus $1b */
extern const uint8_t SMBC_FIREBAR_SPIN_SPD_DATA[5];
extern const uint8_t SMBC_FIREBAR_SPIN_DIR_DATA[5];

/* smb.asm: PlatPosDataLow / PlatPosDataHigh — pixel offsets applied to
 * platform spawn positions */
extern const uint8_t SMBC_PLAT_POS_DATA_LOW[3];
extern const uint8_t SMBC_PLAT_POS_DATA_HIGH[3];
