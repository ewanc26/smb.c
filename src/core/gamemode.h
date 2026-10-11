/* core/gamemode.h — the game mode.
 *
 * Owns: the GameMode task tree, GameCoreRoutine and the GameEngine
 * skeleton. Tasks 0-1 (InitializeArea, ScreenRoutines) and every
 * GameRoutines entry belong to the area, screen and player atoms; the
 * GameEngine's remaining object-processing calls (fireball/bubble,
 * enemies, player graphics, misc objects, whirlpools, flagpole) likewise
 * wait on their atoms. What runs now: the master joypad copy, the
 * subroutine dispatch, the task gate, the player offscreen bits and
 * relative position, the block pass, the cannon pass, the game timer,
 * colour rotation, the star-music check, the SaveAB tail and
 * UpdScrollVar.
 */
#ifndef SMBC_CORE_GAMEMODE_H
#define SMBC_CORE_GAMEMODE_H

struct Smbc;

/* smb.asm: GameMode — jump table order:
 *   0 InitializeArea, 1 ScreenRoutines, 2 SecondaryGameSetup,
 *   3 GameCoreRoutine */
void game_mode(struct Smbc *smbc);

/* smb.asm: GameCoreRoutine */
void game_core_routine(struct Smbc *smbc);

/* smb.asm: UpdScrollVar — the GameEngine's tail, also called from
 * ScrollScreen and PlayerVictoryWalk once those atoms land. */
void upd_scroll_var(struct Smbc *smbc);

#endif /* SMBC_CORE_GAMEMODE_H */
