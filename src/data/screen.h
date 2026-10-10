/* data/screen.h — ScreenRoutines lookup tables.
 *
 * Owns: the four small tables the screen task tree's palette and colour
 * subroutines index: the per-area-type palette selector, the background
 * colour control's vram address offsets, the background colours and the
 * player colours.
 */
#ifndef SMBC_DATA_SCREEN_H
#define SMBC_DATA_SCREEN_H

#include <stdint.h>

/* smb.asm: AreaPalette */
extern const uint8_t SMBC_AREA_PALETTE[4];

/* smb.asm: BGColorCtrl_Addr — indexed by BackgroundColorCtrl - 4 */
extern const uint8_t SMBC_BG_COLOR_CTRL_ADDR[4];

/* smb.asm: BackgroundColors — first row indexed by AreaType, second by
 * BackgroundColorCtrl when set */
extern const uint8_t SMBC_BACKGROUND_COLORS[8];

/* smb.asm: PlayerColors — mario, luigi, fiery */
extern const uint8_t SMBC_PLAYER_COLORS[12];

#endif /* SMBC_DATA_SCREEN_H */
