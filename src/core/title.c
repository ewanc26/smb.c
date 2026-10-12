/* core/title.c — see core/title.h for the contract. */
#include "title.h"

#include "../data/title.h"
#include "area.h"
#include "frame.h"
#include "screen.h"
#include "setup.h"
#include "state.h"

/* smb.asm: ResetTitle */
static void reset_title(Smbc *smbc) {
  CoreState *st = &smbc->state;
  st->ram[SMBC_OPER_MODE] = SMBC_MODE_TITLE;
  st->ram[SMBC_OPER_MODE_TASK] = 0x00;
  st->ram[SMBC_SPRITE0_HIT_FLAG] = 0x00;
  st->ram[SMBC_DISABLE_SCREEN_FLAG] += 1u; /* inc — disable screen output */
}

/* smb.asm: GoContinue */
static void go_continue(Smbc *smbc, uint8_t a) {
  CoreState *st = &smbc->state;
  st->ram[SMBC_WORLD_NUMBER] = a;     /* start both players at the first */
  st->ram[SMBC_OFFSCR_WORLD_NUMBER] = a; /* area of the saved world */
  st->ram[SMBC_AREA_NUMBER] = 0x00;
  st->ram[SMBC_OFFSCR_AREA_NUMBER] = 0x00;
}

/* smb.asm: NullJoypad + RunDemo */
static void run_demo(Smbc *smbc) {
  CoreState *st = &smbc->state;
  /* TODO(port): smb.asm: GameCoreRoutine (GameMode tree) */
  if (st->ram[SMBC_GAME_ENGINE_SUBROUTINE] != 0x06) return; /* ExitMenu */
  reset_title(smbc); /* lose life routine during demo: reset modes */
}

static void null_joypad_run_demo(Smbc *smbc) {
  smbc->state.ram[SMBC_SAVED_JOYPAD1_BITS] = 0x00;
  run_demo(smbc);
}

/* smb.asm: ChkContinue + StartWorld1 */
static void chk_continue(Smbc *smbc, uint8_t a) {
  CoreState *st = &smbc->state;
  if (st->ram[SMBC_DEMO_TIMER] == 0x00) {
    reset_title(smbc);
    return;
  }
  /* asl — carry set iff the A button was also pushed */
  if ((a & SMBC_PAD_A) != 0) {
    go_continue(smbc, st->ram[SMBC_CONTINUE_WORLD]);
  }
  /* StartWorld1 */
  load_area_pointer(smbc);
  st->ram[SMBC_HIDDEN_1UP_FLAG] += 1u;        /* inc — 1-up box flag for */
  st->ram[SMBC_OFFSCR_HIDDEN_1UP_FLAG] += 1u; /* both players */
  st->ram[SMBC_FETCH_NEW_GAME_TIMER_FLAG] += 1u;
  st->ram[SMBC_OPER_MODE] += 1u; /* inc — set next game mode */
  /* if world select is on, primary hard mode must be on as well */
  st->ram[SMBC_PRIMARY_HARD_MODE] = st->ram[SMBC_WORLD_SELECT_ENABLE_FLAG];
  st->ram[SMBC_OPER_MODE_TASK] = 0x00;
  st->ram[SMBC_DEMO_TIMER] = 0x00;
  /* InitScores — clear player scores and coin displays */
  for (int x = 0x17; x >= 0; x--) {
    st->ram[SMBC_SCORE_AND_COIN_DISPLAY + x] = 0x00;
  }
}

void game_menu_routine(Smbc *smbc) {
  CoreState *st = &smbc->state;
  uint8_t y = 0x00;
  uint8_t a = st->ram[SMBC_SAVED_JOYPAD1_BITS] | st->ram[SMBC_SAVED_JOYPAD2_BITS];
  if (a == SMBC_PAD_START || a == (SMBC_PAD_A | SMBC_PAD_START)) {
    chk_continue(smbc, a); /* StartGame */
    return;
  }
  if (a != SMBC_PAD_SELECT) {
    if (st->ram[SMBC_DEMO_TIMER] == 0x00) {
      /* demo timer expired: run the demo (controller bits stored into
       * SelectTimer are residual) */
      st->ram[SMBC_SELECT_TIMER] = a;
      if (demo_engine(smbc)) {
        reset_title(smbc); /* carry set — demo over */
        return;
      }
      run_demo(smbc);
      return;
    }
    /* ChkWorldSel */
    if (st->ram[SMBC_WORLD_SELECT_ENABLE_FLAG] == 0x00 ||
        a != SMBC_PAD_B) {
      null_joypad_run_demo(smbc);
      return;
    }
    y = 0x01; /* B pressed with world select on: same code as select */
  }
  /* SelectBLogic */
  if (st->ram[SMBC_DEMO_TIMER] == 0x00) {
    reset_title(smbc);
    return;
  }
  st->ram[SMBC_DEMO_TIMER] = 0x18; /* reset demo timer */
  if (st->ram[SMBC_SELECT_TIMER] != 0x00) {
    null_joypad_run_demo(smbc);
    return;
  }
  st->ram[SMBC_SELECT_TIMER] = 0x10; /* reset select button timer */
  if (y == 0x01) {
    /* IncWorldSel */
    uint8_t x = (uint8_t)((st->ram[SMBC_WORLD_SELECT_NUMBER] + 1u) & 0x07u);
    st->ram[SMBC_WORLD_SELECT_NUMBER] = x;
    go_continue(smbc, x);
    /* UpdateShroom — GoContinue leaves X at zero */
    for (x = 0x00; x < SMBC_WSELECT_BUFFER_TEMPLATE_SIZE; x++) {
      st->ram[SMBC_VRAM_BUFFER1_OFFSET + x] = SMBC_WSELECT_BUFFER_TEMPLATE[x];
    }
    st->ram[SMBC_VRAM_BUFFER1 + 3] = st->ram[SMBC_WORLD_NUMBER] + 1u;
  } else {
    st->ram[SMBC_NUMBER_OF_PLAYERS] ^= 0x01u;
    draw_mushroom_icon(smbc);
  }
  null_joypad_run_demo(smbc);
}

void draw_mushroom_icon(Smbc *smbc) {
  CoreState *st = &smbc->state;
  /* smb.asm: DrawMushroomIcon — default position is the 1-player game */
  for (int y = 7; y >= 0; y--) {
    st->ram[SMBC_VRAM_BUFFER1_OFFSET + y] = SMBC_MUSHROOM_ICON_DATA[y];
  }
  if (st->ram[SMBC_NUMBER_OF_PLAYERS] == 0x00) return;
  /* 2-player game: blank the 1-player position, icon on the 2-player */
  st->ram[SMBC_VRAM_BUFFER1 + 3] = 0x24;
  st->ram[SMBC_VRAM_BUFFER1 + 5] = 0xce;
}

bool demo_engine(Smbc *smbc) {
  CoreState *st = &smbc->state;
  uint8_t x = st->ram[SMBC_DEMO_ACTION];
  if (st->ram[SMBC_DEMO_ACTION_TIMER] == 0x00) {
    x += 1u;
    st->ram[SMBC_DEMO_ACTION] += 1u; /* inc */
    /* carry set by default for demo over */
    uint8_t t = SMBC_DEMO_TIMING_DATA[x - 1u]; /* DemoTimingData-1,x */
    st->ram[SMBC_DEMO_ACTION_TIMER] = t;
    if (t == 0x00) return true; /* DemoOver */
  }
  /* DoAction */
  st->ram[SMBC_SAVED_JOYPAD1_BITS] = SMBC_DEMO_ACTION_DATA[x - 1u];
  st->ram[SMBC_DEMO_ACTION_TIMER] -= 1u; /* dec */
  return false; /* clc — demo still going */
}

void title_screen_mode(Smbc *smbc) {
  /* smb.asm: TitleScreenMode — jump table order:
   *   0 InitializeGame, 1 ScreenRoutines, 2 PrimaryGameSetup,
   *   3 GameMenuRoutine */
  switch (smbc->state.ram[SMBC_OPER_MODE_TASK]) {
    case 0: /* TODO(port): smb.asm: InitializeGame */ break;
    case 1: screen_routines(smbc); break;
    case 2: primary_game_setup(smbc); break;
    case 3: game_menu_routine(smbc); break;
    default: break;
  }
}
