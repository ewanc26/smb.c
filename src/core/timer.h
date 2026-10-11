/* core/timer.h — the game timer countdown.
 *
 * Owns: RunGameTimer, the per-frame decrement of the on-screen game
 * timer and the four-hundred-unit threshold at which the "time running
 * out" music starts; when the timer reaches 000 it forces the player's
 * death and latches the timer-expired flag.
 *
 * Collaborators: the mode/engine counters and timer digits in
 * core/state; the status-bar writers and digit maths (core/screen); the
 * player-injury path (core/collide); and the event music queue with its
 * TimeRunningOutMusic constant (data/music). The game engine calls
 * run_game_timer directly, between ProcessWhirlpools/FlagpoleRoutine
 * and ColorRotation.
 */
#ifndef SMBC_CORE_TIMER_H
#define SMBC_CORE_TIMER_H

#include "state.h"

/* smb.asm: RunGameTimer — count down the game timer once per control
 * period, or force the player's death once it reads 000 */
void run_game_timer(CoreState *state);

#endif /* SMBC_CORE_TIMER_H */
