/* data/screen.c — see data/screen.h for the contract. */
#include "screen.h"

const uint8_t SMBC_AREA_PALETTE[4] = {
    /* smb.asm: AreaPalette */
    0x01u, 0x02u, 0x03u, 0x04u,
};

const uint8_t SMBC_BG_COLOR_CTRL_ADDR[4] = {
    /* smb.asm: BGColorCtrl_Addr */
    0x00u, 0x09u, 0x0au, 0x04u,
};

const uint8_t SMBC_BACKGROUND_COLORS[8] = {
    /* smb.asm: BackgroundColors */
    0x22u, 0x22u, 0x0fu, 0x0fu, /* by area type */
    0x0fu, 0x22u, 0x0fu, 0x0fu, /* by background color control */
};

const uint8_t SMBC_PLAYER_COLORS[12] = {
    /* smb.asm: PlayerColors */
    0x22u, 0x16u, 0x27u, 0x18u, /* mario */
    0x22u, 0x30u, 0x27u, 0x19u, /* luigi */
    0x22u, 0x37u, 0x27u, 0x16u, /* fiery (either player) */
};
