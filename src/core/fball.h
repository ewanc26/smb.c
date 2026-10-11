/* core/fball.h — the fireball and air bubble objects.
 *
 * Owns: the fireball and bubble draw routines. FireballObjCore,
 * BubbleCheck and ProcFireball_Bubble land here as they are ported.
 */
#ifndef SMBC_CORE_FBALL_H
#define SMBC_CORE_FBALL_H

#include "state.h"

/* smb.asm: DrawFireball — the fireball sprite at its relative
 * position; DrawFirebar is the shared body, the tile alternating
 * every four frames and flipping both ways every eight. */
void draw_fireball(CoreState *state, uint8_t x);

/* smb.asm: DrawExplosion_Fireball — the four-sprite explosion run
 * off the state's low bits, killing the fireball when they expire. */
void draw_explosion_fireball(CoreState *state, uint8_t x);

/* smb.asm: DrawBubble — the air bubble sprite while the player is
 * within the screen vertically and the bubble's offscreen d3 is
 * clear. */
void draw_bubble(CoreState *state, uint8_t x);

#endif /* SMBC_CORE_FBALL_H */
