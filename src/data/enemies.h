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

/* smb.asm: LakituDiffAdj — the difference adjusters for lakitu's own
 * movement */
extern const uint8_t SMBC_LAKITU_DIFF_ADJ_DATA[3];

/* smb.asm: PRDiffAdjustData — pseudorandom difference adjusters for
 * thrown spinys, four groups of four indexed by two LSFR bits */
extern const uint8_t SMBC_PR_DIFF_ADJUST_DATA[12];

/* smb.asm: FlyCCXPositionData / FlyCCXSpeedData / FlyCCTimerData —
 * the flying cheep-cheep's position offsets, horizontal speeds and
 * respawn timers */
extern const uint8_t SMBC_FLYCC_X_POSITION_DATA[16];
extern const uint8_t SMBC_FLYCC_X_SPEED_DATA[12];
extern const uint8_t SMBC_FLYCC_TIMER_DATA[4];

/* smb.asm: FlameYPosData / FlameYMFAdderData / FlameTimerData —
 * bowser's flame heights, movement forces and respawn timers */
extern const uint8_t SMBC_FLAME_Y_POS_DATA[4];
extern const uint8_t SMBC_FLAME_YMF_ADDER_DATA[2];
extern const uint8_t SMBC_FLAME_TIMER_DATA[8];

/* smb.asm: FireworksXPosData / FireworksYPosData */
extern const uint8_t SMBC_FIREWORKS_X_POS_DATA[6];
extern const uint8_t SMBC_FIREWORKS_Y_POS_DATA[6];

/* smb.asm: Enemy17YPosData / SwimCC_IDData — the bullet bill /
 * cheep-cheep frenzy generator's vertical position filter. The
 * Bitmasks table itself lives in data/scenery */
extern const uint8_t SMBC_ENEMY17_Y_POS_DATA[8];
extern const uint8_t SMBC_SWIMCC_ID_DATA[2];
