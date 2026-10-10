/* core/mode.c — see core/mode.h for the contract. */
#include "mode.h"

#include "frame.h"
#include "state.h"

void oper_mode_execution_tree(Smbc *smbc) {
  /* smb.asm: OperModeExecutionTree — jump table order:
   *   0 TitleScreenMode, 1 GameMode, 2 VictoryMode, 3 GameOverMode */
  switch (smbc->state.ram[SMBC_OPER_MODE]) {
    case SMBC_MODE_TITLE:    /* TODO(port): smb.asm: TitleScreenMode */ break;
    case SMBC_MODE_GAME:     /* TODO(port): smb.asm: GameMode */ break;
    case SMBC_MODE_VICTORY:  /* TODO(port): smb.asm: VictoryMode */ break;
    case SMBC_MODE_GAME_OVER: /* TODO(port): smb.asm: GameOverMode */ break;
    default: break;
  }
}
