/* core/gameover.c — see core/gameover.h for the contract. */
#include "gameover.h"

#include "../data/music.h"
#include "area.h"
#include "frame.h"
#include "state.h"

/* smb.asm: SetupGameOver */
static void setup_game_over(Smbc *smbc) {
  CoreState *st = &smbc->state;
  st->ram[SMBC_SCREEN_ROUTINE_TASK] = 0x00;  /* reset screen routine task */
  st->ram[SMBC_SPRITE0_HIT_FLAG] = 0x00;     /* disable sprite 0 check */
  st->ram[SMBC_EVENT_MUSIC_QUEUE] = SMBC_EVENT_MUSIC_GAME_OVER;
  st->ram[SMBC_DISABLE_SCREEN_FLAG] += 1u;   /* inc — disable screen output */
  st->ram[SMBC_OPER_MODE_TASK] += 1u;        /* inc — set secondary mode */
}

/* smb.asm: RunGameOver (with TerminateGame and ContinueGame) */
static void run_game_over(Smbc *smbc) {
  CoreState *st = &smbc->state;
  st->ram[SMBC_DISABLE_SCREEN_FLAG] = 0x00; /* reenable screen */
  if ((st->ram[SMBC_SAVED_JOYPAD1_BITS] & SMBC_PAD_START) == 0 &&
      st->ram[SMBC_SCREEN_TIMER] != 0) {
    return; /* GameIsOn — wait for start or the screen timer */
  }
  /* TerminateGame */
  st->ram[SMBC_EVENT_MUSIC_QUEUE] = SMBC_MUSIC_SILENCE;
  if (!transpose_players(smbc)) {
    /* ContinueGame — the other player keeps going */
    load_area_pointer(smbc);
    st->ram[SMBC_PLAYER_SIZE] = 0x01;
    st->ram[SMBC_FETCH_NEW_GAME_TIMER_FLAG] += 1u; /* inc */
    st->ram[SMBC_TIMER_CONTROL] = 0x00;
    st->ram[SMBC_PLAYER_STATUS] = 0x00;
    st->ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x00;
    st->ram[SMBC_OPER_MODE_TASK] = 0x00;
    st->ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
    return;
  }
  st->ram[SMBC_CONTINUE_WORLD] = st->ram[SMBC_WORLD_NUMBER];
  /* lda #$00 / asl — residual ASL instruction, result still zero */
  st->ram[SMBC_OPER_MODE_TASK] = 0x00;
  st->ram[SMBC_SCREEN_TIMER] = 0x00;
  st->ram[SMBC_OPER_MODE] = SMBC_MODE_TITLE;
}

void game_over_mode(Smbc *smbc) {
  /* smb.asm: GameOverMode — jump table order:
   *   0 SetupGameOver, 1 ScreenRoutines, 2 RunGameOver */
  switch (smbc->state.ram[SMBC_OPER_MODE_TASK]) {
    case 0: setup_game_over(smbc); break;
    case 1: /* TODO(port): smb.asm: ScreenRoutines (core/screen) */ break;
    case 2: run_game_over(smbc); break;
    default: break;
  }
}

bool transpose_players(Smbc *smbc) {
  CoreState *st = &smbc->state;
  /* smb.asm: TransposePlayers — carry set by default to end the game */
  if (st->ram[SMBC_NUMBER_OF_PLAYERS] == 0x00) return true;  /* 1-player */
  if ((st->ram[SMBC_OFFSCR_NUMBEROF_LIVES] & 0x80u) != 0) return true; /* bmi */
  st->ram[SMBC_CURRENT_PLAYER] ^= 0x01u;
  /* TransLoop — swap the 7 bytes of onscreen/offscreen player info */
  for (int x = 6; x >= 0; x--) {
    uint8_t a = st->ram[SMBC_ONSCREEN_PLAYER_INFO + x];
    st->ram[SMBC_ONSCREEN_PLAYER_INFO + x] =
        st->ram[SMBC_OFFSCREEN_PLAYER_INFO + x];
    st->ram[SMBC_OFFSCREEN_PLAYER_INFO + x] = a;
  }
  return false; /* clc — get the game going */
}
