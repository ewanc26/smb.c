/* core/title.h — the title screen mode.
 *
 * Owns: the TitleScreenMode task tree and its menu task (GameMenuRoutine
 * with the select/B world-select logic, DrawMushroomIcon, DemoEngine,
 * GoContinue). Tasks 0-2 (InitializeGame, ScreenRoutines,
 * PrimaryGameSetup) belong to other atoms and are still pending; the
 * demo path's GameCoreRoutine call is a stub until GameMode lands.
 */
#ifndef SMBC_CORE_TITLE_H
#define SMBC_CORE_TITLE_H

#include <stdbool.h>

struct Smbc;

/* smb.asm: TitleScreenMode — jump table order:
 *   0 InitializeGame, 1 ScreenRoutines, 2 PrimaryGameSetup,
 *   3 GameMenuRoutine */
void title_screen_mode(struct Smbc *smbc);

/* smb.asm: GameMenuRoutine */
void game_menu_routine(struct Smbc *smbc);

/* smb.asm: DrawMushroomIcon */
void draw_mushroom_icon(struct Smbc *smbc);

/* smb.asm: DemoEngine — returns the carry flag: true when the demo is
 * over. */
bool demo_engine(struct Smbc *smbc);

#endif /* SMBC_CORE_TITLE_H */
