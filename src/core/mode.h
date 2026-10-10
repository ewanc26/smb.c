/* core/mode.h — the mode execution tree.
 *
 * Owns: dispatch on OperMode (title / game / victory / game over). The
 * modes themselves are pending translation; the dispatch shape mirrors the
 * disassembly's JumpEngine table so translated modes slot in by index.
 */
#ifndef SMBC_CORE_MODE_H
#define SMBC_CORE_MODE_H

struct Smbc;

/* smb.asm: OperModeExecutionTree — all four modes dispatch. GameOverMode
 * (core/gameover) and TitleScreenMode (core/title) are complete;
 * VictoryMode (core/victory) and GameMode (core/gamemode) run their
 * skeletons with the player/enemy/scroll/parser-dependent pieces
 * stubbed for their own atoms. */
void oper_mode_execution_tree(struct Smbc *smbc);

#endif /* SMBC_CORE_MODE_H */
