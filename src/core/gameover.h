/* core/gameover.h — the game over mode.
 *
 * Owns: the GameOverMode task tree (SetupGameOver, ScreenRoutines,
 * RunGameOver) and TransposePlayers, the onscreen/offscreen player swap
 * shared with the game mode's player-switch paths. ScreenRoutines is a
 * separate atom and still pending translation.
 */
#ifndef SMBC_CORE_GAMEOVER_H
#define SMBC_CORE_GAMEOVER_H

#include <stdbool.h>

struct Smbc;

/* smb.asm: GameOverMode — jump table order:
 *   0 SetupGameOver, 1 ScreenRoutines, 2 RunGameOver */
void game_over_mode(struct Smbc *smbc);

/* smb.asm: TransposePlayers — returns the carry flag: true when the game
 * ends (1-player game, or the offscreen player is out of lives), false
 * when the players were swapped and the game continues. */
bool transpose_players(struct Smbc *smbc);

#endif /* SMBC_CORE_GAMEOVER_H */
