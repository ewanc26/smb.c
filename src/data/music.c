/* data/music.c — see data/music.h for the contract. */
#include "music.h"

const uint8_t SMBC_MUSIC_SELECT_DATA[SMBC_MUSIC_SELECT_DATA_SIZE] = {
    /* smb.asm: MusicSelectData */
    SMBC_MUSIC_WATER, SMBC_MUSIC_GROUND, SMBC_MUSIC_UNDERGROUND,
    SMBC_MUSIC_CASTLE, SMBC_MUSIC_CLOUD, SMBC_MUSIC_PIPE_INTRO,
};
