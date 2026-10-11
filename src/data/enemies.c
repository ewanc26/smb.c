/* data/enemies.c — see enemies.h */
#include "enemies.h"

const uint8_t SMBC_LOOP_CMD_WORLD_NUMBER[11] = {
    0x03u, 0x03u, 0x06u, 0x06u, 0x06u, 0x06u,
    0x06u, 0x06u, 0x07u, 0x07u, 0x07u,
};

const uint8_t SMBC_LOOP_CMD_PAGE_NUMBER[11] = {
    0x05u, 0x09u, 0x04u, 0x05u, 0x06u, 0x08u,
    0x09u, 0x0au, 0x06u, 0x0bu, 0x10u,
};

const uint8_t SMBC_LOOP_CMD_Y_POSITION[11] = {
    0x40u, 0xb0u, 0xb0u, 0x80u, 0x40u, 0x40u,
    0x80u, 0x40u, 0xf0u, 0xf0u, 0xf0u,
};

const uint8_t SMBC_AREA_DATA_OFS_LOOPBACK[11] = {
    0x12u, 0x36u, 0x0eu, 0x0eu, 0x0eu, 0x32u,
    0x32u, 0x32u, 0x0au, 0x26u, 0x40u,
};

const uint8_t SMBC_NORMAL_X_SPD_DATA[2] = {0xf8u, 0xf4u};

const uint8_t SMBC_HBRO_WALKING_TIMER_DATA[2] = {0x80u, 0x50u};

const uint8_t SMBC_FIREBAR_SPIN_SPD_DATA[5] = {
    0x28u, 0x38u, 0x28u, 0x38u, 0x28u,
};

const uint8_t SMBC_FIREBAR_SPIN_DIR_DATA[5] = {
    0x00u, 0x00u, 0x10u, 0x10u, 0x00u,
};

const uint8_t SMBC_PLAT_POS_DATA_LOW[3] = {0x08u, 0x0cu, 0xf8u};

const uint8_t SMBC_PLAT_POS_DATA_HIGH[3] = {0x00u, 0x00u, 0xffu};

/* smb.asm: LakituDiffAdj */
const uint8_t SMBC_LAKITU_DIFF_ADJ_DATA[3] = {0x15u, 0x30u, 0x40u};

/* smb.asm: PRDiffAdjustData */
const uint8_t SMBC_PR_DIFF_ADJUST_DATA[12] = {0x26u, 0x2cu, 0x32u, 0x38u,
                                              0x20u, 0x22u, 0x24u, 0x26u,
                                              0x13u, 0x14u, 0x15u, 0x16u};

/* smb.asm: FlyCCXPositionData */
const uint8_t SMBC_FLYCC_X_POSITION_DATA[16] = {0x80u, 0x30u, 0x40u, 0x80u,
                                                0x30u, 0x50u, 0x50u, 0x70u,
                                                0x20u, 0x40u, 0x80u, 0xa0u,
                                                0x70u, 0x40u, 0x90u, 0x68u};

/* smb.asm: FlyCCXSpeedData */
const uint8_t SMBC_FLYCC_X_SPEED_DATA[12] = {0x0eu, 0x05u, 0x06u, 0x0eu,
                                             0x1cu, 0x20u, 0x10u, 0x0cu,
                                             0x1eu, 0x22u, 0x18u, 0x14u};

/* smb.asm: FlyCCTimerData */
const uint8_t SMBC_FLYCC_TIMER_DATA[4] = {0x10u, 0x60u, 0x20u, 0x48u};

/* smb.asm: FlameYPosData */
const uint8_t SMBC_FLAME_Y_POS_DATA[4] = {0x90u, 0x80u, 0x70u, 0x90u};

/* smb.asm: FlameYMFAdderData */
const uint8_t SMBC_FLAME_YMF_ADDER_DATA[2] = {0xffu, 0x01u};

/* smb.asm: FlameTimerData */
const uint8_t SMBC_FLAME_TIMER_DATA[8] = {0xbfu, 0x40u, 0xbfu, 0xbfu,
                                          0xbfu, 0x40u, 0x40u, 0xbfu};

/* smb.asm: FireworksXPosData */
const uint8_t SMBC_FIREWORKS_X_POS_DATA[6] = {0x00u, 0x30u, 0x60u,
                                              0x60u, 0x00u, 0x20u};

/* smb.asm: FireworksYPosData */
const uint8_t SMBC_FIREWORKS_Y_POS_DATA[6] = {0x60u, 0x40u, 0x70u,
                                              0x40u, 0x60u, 0x30u};

/* smb.asm: Enemy17YPosData */
const uint8_t SMBC_ENEMY17_Y_POS_DATA[8] = {0x40u, 0x30u, 0x90u, 0x50u,
                                            0x20u, 0x60u, 0xa0u, 0x70u};

/* smb.asm: SwimCC_IDData */
const uint8_t SMBC_SWIMCC_ID_DATA[2] = {0x0au, 0x0bu};
