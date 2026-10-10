/* data/sprites.c — see data/sprites.h for the contract. */
#include "sprites.h"

const uint8_t SMBC_DEFAULT_SPR_OFFSETS[SMBC_DEFAULT_SPR_OFFSETS_SIZE] = {
    /* smb.asm: DefaultSprOffsets */
    0x04u, 0x30u, 0x48u, 0x60u, 0x78u, 0x90u, 0xa8u, 0xc0u,
    0xd8u, 0xe8u, 0x24u, 0xf8u, 0xfcu, 0x28u, 0x2cu,
};

const uint8_t SMBC_SPRITE0_DATA[SMBC_SPRITE0_DATA_SIZE] = {
    /* smb.asm: Sprite0Data */
    0x18u, 0xffu, 0x23u, 0x58u,
};
