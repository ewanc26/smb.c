/* core/gamemode.c — see core/gamemode.h for the contract. */
#include "gamemode.h"

#include "blocks.h"
#include "frame.h"
#include "offscreen.h"
#include "screen.h"
#include "setup.h"
#include "state.h"

void upd_scroll_var(Smbc *smbc) {
  CoreState *st = &smbc->state;
  /* smb.asm: UpdScrollVar — the vram address controller at 6 (one of two
   * $0341s) means the buffer is mid-write; leave */
  if (st->ram[SMBC_VRAM_BUFFER_ADDR_CTRL] == 0x06) return; /* ExitEng */
  if (st->ram[SMBC_AREA_PARSER_TASK_NUM] == 0x00) {
    if (st->ram[SMBC_SCROLL_THIRTY_TWO] < 0x20) return; /* bmi ExitEng */
    /* wrap the 0-31 scroll range and reset the level graphics buffer */
    st->ram[SMBC_SCROLL_THIRTY_TWO] -= 0x20u;
    st->ram[SMBC_VRAM_BUFFER2_OFFSET] = 0x00;
  }
  /* RunParser — TODO(port): smb.asm: AreaParserTaskHandler
   * (core/areaparser) */
}

/* smb.asm: GameRoutines — jump table order:
 *   0 Entrance_GameTimerSetup, 1 Vine_AutoClimb, 2 SideExitPipeEntry,
 *   3 VerticalPipeEntry, 4 FlagpoleSlide, 5 PlayerEndLevel,
 *   6 PlayerLoseLife, 7 PlayerEntrance, 8 PlayerCtrlRoutine,
 *   9 PlayerChangeSize, 10 PlayerInjuryBlink, 11 PlayerDeath,
 *   12 PlayerFireFlower */
static void game_routines(Smbc *smbc) {
  /* TODO(port): all thirteen routines belong to the player atom */
  switch (smbc->state.ram[SMBC_GAME_ENGINE_SUBROUTINE]) {
    default: break;
  }
}

/* smb.asm: GameEngine — the per-frame object and music processing. The
 * translation, offscreen and block calls run, as do the music check,
 * palette-cycle control computation and the SaveAB tail; the
 * fireball/bubble, enemy, player-graphics, misc-object, cannon,
 * whirlpool, flagpole, timer and colour calls still wait on their
 * atoms. */
static void game_engine(Smbc *smbc) {
  CoreState *st = &smbc->state;
  /* TODO(port): smb.asm: ProcFireball_Bubble, the ProcELoop enemy and
   * floatey-number passes, PlayerGfxHandler, MiscObjectsCore,
   * ProcessCannons, ProcessWhirlpools, FlagpoleRoutine, RunGameTimer,
   * ColorRotation */
  /* smb.asm: GetPlayerOffscreenBits, then RelativePlayerPosition — the
   * player's offscreen bits and screen-relative coordinates, ahead of
   * PlayerGfxHandler */
  get_player_offscreen_bits(st);
  relative_player_position(st);
  /* smb.asm: BlockObjMT_Updater, then BlockObjectsCore for the second
   * block object and the first */
  block_obj_mt_updater(st);
  st->ram[SMBC_OBJECT_OFFSET] = 0x01u;
  block_objects_core(st, 0x01u);
  st->ram[SMBC_OBJECT_OFFSET] = 0x00u;
  block_objects_core(st, 0x00u);
  bool cycle_palette = true;
  /* cmp #$02 / bpl — the sign of (Player_Y_HighPos - 2) decides whether
   * the player is far enough below the screen to skip the music check */
  if ((st->ram[SMBC_PLAYER_Y_HIGH_POS] - 0x02u & 0x80u) != 0) {
    uint8_t star = st->ram[SMBC_STAR_INVINCIBLE_TIMER];
    if (star == 0x00) {
      /* ClrPlrPal — TODO(port): smb.asm: ResetPalStar */
      cycle_palette = false;
    } else if (star == 0x04u && st->ram[SMBC_INTERVAL_TIMER_CONTROL] == 0x00u) {
      get_area_music(st); /* re-attain appropriate level music */
    }
  }
  if (cycle_palette) {
    /* NoChgMus — the star timer picks the palette cycle rate; the
     * computed value is CyclePlayerPalette's input */
    uint8_t a = st->ram[SMBC_FRAME_COUNTER];
    if (st->ram[SMBC_STAR_INVINCIBLE_TIMER] < 0x08u) a >>= 2;
    a >>= 1;
    (void)a; /* TODO(port): smb.asm: CyclePlayerPalette */
  }
  /* SaveAB */
  st->ram[SMBC_PREV_A_B_BUTTONS] = st->ram[SMBC_A_B_BUTTONS];
  st->ram[SMBC_LEFT_RIGHT_BUTTONS] = 0x00; /* nullify left/right buttons */
  upd_scroll_var(smbc);
}

void game_core_routine(Smbc *smbc) {
  CoreState *st = &smbc->state;
  /* smb.asm: GameCoreRoutine — the current player's controller bits
   * become the master controller bits */
  st->ram[SMBC_SAVED_JOYPAD1_BITS] =
      st->ram[SMBC_SAVED_JOYPAD1_BITS + st->ram[SMBC_CURRENT_PLAYER]];
  game_routines(smbc);
  if (st->ram[SMBC_OPER_MODE_TASK] >= 0x03) {
    game_engine(smbc); /* only if we are supposed to be here */
  }
}

void game_mode(Smbc *smbc) {
  /* smb.asm: GameMode — jump table order:
   *   0 InitializeArea, 1 ScreenRoutines, 2 SecondaryGameSetup,
   *   3 GameCoreRoutine */
  switch (smbc->state.ram[SMBC_OPER_MODE_TASK]) {
    case 0: /* TODO(port): smb.asm: InitializeArea (core/area) */ break;
    case 1: screen_routines(smbc); break;
    case 2: secondary_game_setup(smbc); break;
    case 3: game_core_routine(smbc); break;
    default: break;
  }
}
