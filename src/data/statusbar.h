/* data/statusbar.h — status bar layout tables.
 *
 * Owns: the VRAM low-address/length pairs and display-digit offsets the
 * status bar writer (core/screen) uses to print scores, coin tallies and
 * the game timer into VRAM buffer 1.
 */
#ifndef SMBC_DATA_STATUSBAR_H
#define SMBC_DATA_STATUSBAR_H

#include <stdint.h>

#define SMBC_STATUS_BAR_DATA_SIZE 12u
#define SMBC_STATUS_BAR_OFFSET_SIZE 6u
#define SMBC_STATUS_BAR_NYBBLES_SIZE 2u

/* smb.asm: StatusBarData — (vram low address, length) pairs */
extern const uint8_t SMBC_STATUS_BAR_DATA[SMBC_STATUS_BAR_DATA_SIZE];

/* smb.asm: StatusBarOffset */
extern const uint8_t SMBC_STATUS_BAR_OFFSET[SMBC_STATUS_BAR_OFFSET_SIZE];

/* smb.asm: StatusBarNybbles — per-player (score, coins) selector pair */
extern const uint8_t SMBC_STATUS_BAR_NYBBLES[SMBC_STATUS_BAR_NYBBLES_SIZE];

#endif /* SMBC_DATA_STATUSBAR_H */
