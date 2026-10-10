/* data/music.h — music select constants and table.
 *
 * Owns: the music bit flags written to the area/event music queues, and
 * the per-area-type select table. The sound engine (pending) consumes the
 * queues; see issue #1.
 */
#ifndef SMBC_DATA_MUSIC_H
#define SMBC_DATA_MUSIC_H

#include <stdint.h>

/* smb.asm: music constants */
#define SMBC_MUSIC_SILENCE      0x80u /* smb.asm: Silence */
#define SMBC_MUSIC_STAR_POWER   0x40u /* smb.asm: StarPowerMusic */
#define SMBC_MUSIC_PIPE_INTRO   0x20u /* smb.asm: PipeIntroMusic */
#define SMBC_MUSIC_CLOUD        0x10u /* smb.asm: CloudMusic */
#define SMBC_MUSIC_CASTLE       0x08u /* smb.asm: CastleMusic */
#define SMBC_MUSIC_UNDERGROUND  0x04u /* smb.asm: UndergroundMusic */
#define SMBC_MUSIC_WATER        0x02u /* smb.asm: WaterMusic */
#define SMBC_MUSIC_GROUND       0x01u /* smb.asm: GroundMusic */

#define SMBC_MUSIC_SELECT_DATA_SIZE 6u

/* smb.asm: MusicSelectData — indexed by AreaType, then cloud, then pipe */
extern const uint8_t SMBC_MUSIC_SELECT_DATA[SMBC_MUSIC_SELECT_DATA_SIZE];

#endif /* SMBC_DATA_MUSIC_H */
