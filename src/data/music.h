/* data/music.h — the sound engine's translated tables and constants.
 *
 * Owns: every byte the sound engine reads. The music bit flags written to
 * the area/event music queues, the per-area select table, the song blob
 * ("MUSIC DATA"), the header/offset blocks, the frequency and length
 * lookup tables, and the SFX/envelope tables including the residual bytes
 * that the disassembly relies on.
 *
 * The song blob keeps its ROM order so cross-song and past-the-end reads
 * (MusicLengthLookupTbl follows FreqRegLookupTbl; the envelope tables sit
 * contiguously) stay byte-accurate. The consumer is core/sound; see #1.
 */
#ifndef SMBC_DATA_MUSIC_H
#define SMBC_DATA_MUSIC_H

#include <stdint.h>

/* smb.asm: music constants written to the area music queue */
#define SMBC_MUSIC_SILENCE      0x80u /* smb.asm: Silence */
#define SMBC_MUSIC_STAR_POWER   0x40u /* smb.asm: StarPowerMusic */
#define SMBC_MUSIC_PIPE_INTRO   0x20u /* smb.asm: PipeIntroMusic */
#define SMBC_MUSIC_CLOUD        0x10u /* smb.asm: CloudMusic */
#define SMBC_MUSIC_CASTLE       0x08u /* smb.asm: CastleMusic */
#define SMBC_MUSIC_UNDERGROUND  0x04u /* smb.asm: UndergroundMusic */
#define SMBC_MUSIC_WATER        0x02u /* smb.asm: WaterMusic */
#define SMBC_MUSIC_GROUND       0x01u /* smb.asm: GroundMusic */

/* smb.asm: music constants written to the event music queue */
#define SMBC_EVENT_MUSIC_DEATH            0x01u /* smb.asm: DeathMusic */
#define SMBC_EVENT_MUSIC_GAME_OVER        0x02u /* smb.asm: GameOverMusic */
#define SMBC_EVENT_MUSIC_VICTORY          0x04u /* smb.asm: VictoryMusic */
#define SMBC_EVENT_MUSIC_END_OF_CASTLE    0x08u /* smb.asm: EndOfCastleMusic */
#define SMBC_EVENT_MUSIC_ALT_GAME_OVER    0x10u /* smb.asm: AltGameOverMusic */
#define SMBC_EVENT_MUSIC_END_OF_LEVEL     0x20u /* smb.asm: EndOfLevelMusic */
#define SMBC_EVENT_MUSIC_TIME_RUNNING_OUT 0x40u /* smb.asm: TimeRunningOutMusic */

#define SMBC_MUSIC_SELECT_DATA_SIZE 6u

/* smb.asm: MusicSelectData — indexed by AreaType, then cloud, then pipe */
extern const uint8_t SMBC_MUSIC_SELECT_DATA[SMBC_MUSIC_SELECT_DATA_SIZE];

/* ---- song blob -----------------------------------------------------------
 * smb.asm: the "MUSIC DATA" region, from Star_CloudMData through the three
 * trailing $ff bytes. Verified byte-for-byte against the donor ROM, where it
 * assembles at $f9b8; a header pointer P reads
 * SMBC_MUSIC_ROM[P - SMBC_MUSIC_ROM_BASE]. FreqRegLookupTbl follows the blob
 * immediately ($ff00). */
#define SMBC_MUSIC_ROM_BASE 0xf9b8u
#define SMBC_MUSIC_ROM_SIZE 1352u
extern const uint8_t SMBC_MUSIC_ROM[SMBC_MUSIC_ROM_SIZE];

/* ---- lookup tables -------------------------------------------------------
 * FreqRegLookupTbl is followed in ROM by MusicLengthLookupTbl; reads past the
 * 102-byte frequency table spill into the length table. */
#define SMBC_FREQ_REG_LOOKUP_TBL_SIZE 102u
extern const uint8_t SMBC_FREQ_REG_LOOKUP_TBL[SMBC_FREQ_REG_LOOKUP_TBL_SIZE];

#define SMBC_MUSIC_LENGTH_LOOKUP_TBL_SIZE 48u
extern const uint8_t SMBC_MUSIC_LENGTH_LOOKUP_TBL[SMBC_MUSIC_LENGTH_LOOKUP_TBL_SIZE];

/* ---- header blocks -------------------------------------------------------
 * smb.asm: MusicHeaderOffsetData is MusicHeaderData - 1, and both header
 * tables are indexed from MusicHeaderData itself: index Y reads
 * SMBC_MUSIC_HEADER_OFFSET_DATA[Y - 1], and the resulting offset O addresses
 * from the start of the offset table, so header byte I of that entry is
 * SMBC_MUSIC_HEADER_DATA[O - SMBC_MUSIC_HEADER_OFFSET_DATA_SIZE + I] (the
 * smallest offset in the table is the offset table's own length, so O never
 * points back into the offset table when reading a header). Both tables are
 * verbatim from the donor ROM ($f90d and $f93e). */
#define SMBC_MUSIC_HEADER_OFFSET_DATA_SIZE 49u
extern const uint8_t SMBC_MUSIC_HEADER_OFFSET_DATA[SMBC_MUSIC_HEADER_OFFSET_DATA_SIZE];

#define SMBC_MUSIC_HEADER_DATA_SIZE 122u
extern const uint8_t SMBC_MUSIC_HEADER_DATA[SMBC_MUSIC_HEADER_DATA_SIZE];

/* ---- envelope tables -----------------------------------------------------
 * These five sit contiguously in ROM (EndOfCastle..BrickShatter); the engine
 * reads across their boundaries, so consumers combine them rather than
 * indexing any single table. */
#define SMBC_END_OF_CASTLE_MUSIC_ENV_DATA_SIZE 4u
extern const uint8_t SMBC_END_OF_CASTLE_MUSIC_ENV_DATA[SMBC_END_OF_CASTLE_MUSIC_ENV_DATA_SIZE];

#define SMBC_AREA_MUSIC_ENV_DATA_SIZE 8u
extern const uint8_t SMBC_AREA_MUSIC_ENV_DATA[SMBC_AREA_MUSIC_ENV_DATA_SIZE];

#define SMBC_WATER_EVENT_MUS_ENV_DATA_SIZE 40u
extern const uint8_t SMBC_WATER_EVENT_MUS_ENV_DATA[SMBC_WATER_EVENT_MUS_ENV_DATA_SIZE];

#define SMBC_BOWSER_FLAME_ENV_DATA_SIZE 32u
extern const uint8_t SMBC_BOWSER_FLAME_ENV_DATA[SMBC_BOWSER_FLAME_ENV_DATA_SIZE];

#define SMBC_BRICK_SHATTER_ENV_DATA_SIZE 16u
extern const uint8_t SMBC_BRICK_SHATTER_ENV_DATA[SMBC_BRICK_SHATTER_ENV_DATA_SIZE];

/* ---- sound effect tables ------------------------------------------------- */
#define SMBC_SWIM_STOMP_ENVELOPE_DATA_SIZE 14u
extern const uint8_t SMBC_SWIM_STOMP_ENVELOPE_DATA[SMBC_SWIM_STOMP_ENVELOPE_DATA_SIZE];

#define SMBC_EXTRA_LIFE_FREQ_DATA_SIZE 6u
extern const uint8_t SMBC_EXTRA_LIFE_FREQ_DATA[SMBC_EXTRA_LIFE_FREQ_DATA_SIZE];

/* 27 bytes of PowerUpGrabFreqData + 3 residual bytes read via -1 indexing */
#define SMBC_POWER_UP_GRAB_FREQ_DATA_SIZE 30u
extern const uint8_t SMBC_POWER_UP_GRAB_FREQ_DATA[SMBC_POWER_UP_GRAB_FREQ_DATA_SIZE];

#define SMBC_PUP_VGROW_FREQ_DATA_SIZE 32u
extern const uint8_t SMBC_PUP_VGROW_FREQ_DATA[SMBC_PUP_VGROW_FREQ_DATA_SIZE];

#define SMBC_BRICK_SHATTER_FREQ_DATA_SIZE 16u
extern const uint8_t SMBC_BRICK_SHATTER_FREQ_DATA[SMBC_BRICK_SHATTER_FREQ_DATA_SIZE];

#endif /* SMBC_DATA_MUSIC_H */
