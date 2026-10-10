/* data/gametext.h — the game text blocks.
 *
 * Owns: the GameText blob and its offset table, the luigi name patch and
 * the warp zone numbers. Tile values use the game's text encoding
 * ($0a-$23 = A-Z, $24 = space, $28 = -, $2b = !, $9f = crown). Each block
 * is a sequence of (vram address, length, data) update sets terminated by
 * $ff; the two-player name rows share their block with the one-player
 * message that follows them.
 */
#ifndef SMBC_DATA_GAMETEXT_H
#define SMBC_DATA_GAMETEXT_H

#include <stdint.h>

#define SMBC_GAME_TEXT_SIZE 155u
#define SMBC_GAME_TEXT_OFFSETS_SIZE 10u
#define SMBC_LUIGI_NAME_SIZE 5u
#define SMBC_WARP_ZONE_NUMBERS_SIZE 12u

/* smb.asm: GameText — TopStatusBarLine, WorldLivesDisplay,
 * Two/OnePlayerTimeUp, Two/OnePlayerGameOver, WarpZoneWelcome */
extern const uint8_t SMBC_GAME_TEXT[SMBC_GAME_TEXT_SIZE];

/* smb.asm: GameTextOffsets — indexed by (message number * 2), adjusted
 * for player count */
extern const uint8_t SMBC_GAME_TEXT_OFFSETS[SMBC_GAME_TEXT_OFFSETS_SIZE];

/* smb.asm: LuigiName — no address or length, patched over "MARIO" */
extern const uint8_t SMBC_LUIGI_NAME[SMBC_LUIGI_NAME_SIZE];

/* smb.asm: WarpZoneNumbers — one world number per warp zone pipe */
extern const uint8_t SMBC_WARP_ZONE_NUMBERS[SMBC_WARP_ZONE_NUMBERS_SIZE];

#endif /* SMBC_DATA_GAMETEXT_H */
