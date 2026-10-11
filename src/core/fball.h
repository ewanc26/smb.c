/* core/fball.h — the fireball and air bubble objects.
 *
 * Owns: ProcFireball_Bubble with the two fireball cores and the three
 * air bubbles, and the draw routines for all of them. The movement,
 * offscreen and collision leaves live in move.c, offscreen.c and
 * collide.c. */
#ifndef SMBC_CORE_FBALL_H
#define SMBC_CORE_FBALL_H

#include "state.h"

/* smb.asm: ProcFireball_Bubble — the GameEngine entry: spawn a
 * fireball for fiery mario on a fresh B press, run both fireball
 * cores, and on water levels the three air bubbles. */
void proc_fireball_bubble(CoreState *state);

/* smb.asm: FireballObjCore — one fireball: init from the player,
 * gravity and movement, collisions, explosion and erasure. */
void fireball_obj_core(CoreState *state, uint8_t x);

/* smb.asm: BubbleCheck — create an air bubble when the timer expires
 * and move it slowly upwards, offscreen above the status bar. */
void bubble_check(CoreState *state, uint8_t x);

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
