/* core/victory.h — the victory mode.
 *
 * Owns: the VictoryMode wrapper and VictoryModeSubroutines task tree.
 * SetupVictoryMode, PrintVictoryMessages and PlayerEndWorld are
 * translated; BridgeCollapse and PlayerVictoryWalk depend on the player
 * and scroll atoms, and the wrapper's EnemiesAndLoopsCore /
 * RelativePlayerPosition / PlayerGfxHandler calls on the enemy and
 * player atoms — all still pending, stubbed where they would run.
 */
#ifndef SMBC_CORE_VICTORY_H
#define SMBC_CORE_VICTORY_H

struct Smbc;

/* smb.asm: VictoryMode */
void victory_mode(struct Smbc *smbc);

#endif /* SMBC_CORE_VICTORY_H */
