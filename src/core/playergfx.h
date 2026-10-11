/* core/playergfx.h — the player's sprite rendering.
 *
 * Owns: PlayerGfxHandler — pose selection from the player's state and
 * animation timers, the grow/shrink interleave, the fireball-throw
 * overlay, the swim-kick tile patch, the offscreen row parking — plus
 * DrawPlayer_Intermediate for the world/lives display and the star /
 * fire-flower palette cycling helpers.
 */
#ifndef SMBC_CORE_PLAYERGFX_H
#define SMBC_CORE_PLAYERGFX_H

#include "state.h"

/* smb.asm: PlayerGfxHandler — run from GameEngine after
 * RelativePlayerPosition. */
void player_gfx_handler(CoreState *state);

/* smb.asm: DrawPlayer_Intermediate — the standing player on the
 * world/lives intermediate display. */
void draw_player_intermediate(CoreState *state);

/* smb.asm: CyclePlayerPalette — the star/fire-flower palette cycle;
 * the input is the shifted frame counter computed by the caller. */
void cycle_player_palette(CoreState *state, uint8_t a);

/* smb.asm: ResetPalStar — force the player's palette bits to 0. */
void reset_pal_star(CoreState *state);

#endif /* SMBC_CORE_PLAYERGFX_H */
