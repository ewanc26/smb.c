/* core/mode.c — see core/mode.h for the contract. */
#include "mode.h"

#include "frame.h"
#include "gameover.h"
#include "state.h"
#include "title.h"
#include "victory.h"

void oper_mode_execution_tree(Smbc *smbc) {
  /* smb.asm: OperModeExecutionTree — jump table order:
   *   0 TitleScreenMode, 1 GameMode, 2 VictoryMode, 3 GameOverMode */
  switch (smbc->state.ram[SMBC_OPER_MODE]) {
    case SMBC_MODE_TITLE:    title_screen_mode(smbc); break;
    case SMBC_MODE_GAME:     /* TODO(port): smb.asm: GameMode */ break;
    case SMBC_MODE_VICTORY:  victory_mode(smbc); break;
    case SMBC_MODE_GAME_OVER: game_over_mode(smbc); break;
    default: break;
  }
}
