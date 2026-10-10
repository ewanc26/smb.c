/* data/statusbar.c — see data/statusbar.h for the contract. */
#include "statusbar.h"

const uint8_t SMBC_STATUS_BAR_DATA[SMBC_STATUS_BAR_DATA_SIZE] = {
    /* smb.asm: StatusBarData */
    0xf0u, 0x06u, /* top score display on title screen */
    0x62u, 0x06u, /* player score */
    0x62u, 0x06u,
    0x6du, 0x02u, /* coin tally */
    0x6du, 0x02u,
    0x7au, 0x03u, /* game timer */
};

const uint8_t SMBC_STATUS_BAR_OFFSET[SMBC_STATUS_BAR_OFFSET_SIZE] = {
    /* smb.asm: StatusBarOffset */
    0x06u, 0x0cu, 0x12u, 0x18u, 0x1eu, 0x24u,
};

const uint8_t SMBC_STATUS_BAR_NYBBLES[SMBC_STATUS_BAR_NYBBLES_SIZE] = {
    /* smb.asm: StatusBarNybbles */
    0x02u, 0x13u,
};
