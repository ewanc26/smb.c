/* data/messages.h — message VRAM buffer sets.
 *
 * Owns: the ROM-resident message update sets from the disassembly's VRAM
 * BUFFER DATA section, in WriteBufferToScreen format. Tile values use the
 * game's text encoding ($0a-$23 = A-Z, $24 = space, $2b = !, $af = .).
 */
#ifndef SMBC_DATA_MESSAGES_H
#define SMBC_DATA_MESSAGES_H

#include <stdint.h>

/* smb.asm: MarioThanksMessage .. WorldSelectMessage2 */
extern const uint8_t SMBC_MARIO_THANKS_MESSAGE[20];
extern const uint8_t SMBC_LUIGI_THANKS_MESSAGE[20];
extern const uint8_t SMBC_MUSHROOM_RETAINER_SAVED[44];
extern const uint8_t SMBC_PRINCESS_SAVED1[23];
extern const uint8_t SMBC_PRINCESS_SAVED2[31];
extern const uint8_t SMBC_WORLD_SELECT_MESSAGE1[17];
extern const uint8_t SMBC_WORLD_SELECT_MESSAGE2[21];

#endif /* SMBC_DATA_MESSAGES_H */
