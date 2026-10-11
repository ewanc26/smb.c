/* core/routines.c — see core/routines.h for the contract. */
#include "routines.h"

#include <stdbool.h>

#include "../data/music.h"
#include "area.h"
#include "blocks.h"
#include "collide.h"
#include "enemyinit.h"
#include "fball.h"
#include "frame.h"
#include "gameover.h"
#include "offscreen.h"
#include "playergfx.h"
#include "screen.h"
#include "state.h"

/* smb.asm: PlayerStarting_X_Pos */
static const uint8_t kPlayerStartingXPos[] = {0x28u, 0x18u, 0x38u, 0x28u};

/* smb.asm: AltYPosOffset */
static const uint8_t kAltYPosOffset[] = {0x08u, 0x00u};

/* smb.asm: PlayerStarting_Y_Pos */
static const uint8_t kPlayerStartingYPos[] = {
    0x00u, 0x20u, 0xb0u, 0x50u, 0x00u, 0x00u, 0xb0u, 0xb0u, 0xf0u,
};

/* smb.asm: PlayerBGPriorityData — the ninth byte is GameTimerData's
 * dummy, read when the vine-entrance offset overflows the table */
static const uint8_t kPlayerBGPriorityData[] = {
    0x00u, 0x20u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x20u,
};

/* smb.asm: GameTimerData — the first byte is the bg-priority dummy */
static const uint8_t kGameTimerData[] = {0x20u, 0x04u, 0x03u, 0x02u};

/* smb.asm: HalfwayPageNybbles — page numbers for areas -1 to -4 */
static const uint8_t kHalfwayPageNybbles[] = {
    0x56u, 0x40u, 0x65u, 0x70u, 0x66u, 0x40u, 0x66u, 0x40u,
    0x66u, 0x40u, 0x66u, 0x60u, 0x65u, 0x70u, 0x00u, 0x00u,
};

/* smb.asm: Hidden1UpCoinAmts */
static const uint8_t kHidden1UpCoinAmts[] = {
    0x15u, 0x23u, 0x16u, 0x1bu, 0x17u, 0x18u, 0x23u, 0x63u,
};

/* smb.asm: X_SubtracterData */
static const uint8_t kXSubtracterData[] = {0x00u, 0x10u};

/* smb.asm: OffscrJoypadBitsData */
static const uint8_t kOffscrJoypadBitsData[] = {0x01u, 0x02u};

/* smb.asm: ChgAreaMode — disable the screen and sprite 0, back to the
 * first mode task */
static void chg_area_mode(CoreState *st) {
  st->ram[SMBC_DISABLE_SCREEN_FLAG] += 1u;
  st->ram[SMBC_OPER_MODE_TASK] = 0x00u;
  st->ram[SMBC_SPRITE0_HIT_FLAG] = 0x00u;
}

/* smb.asm: SetEntr — mode of entry 2 (from the top of the area) */
static void set_entr(CoreState *st) {
  st->ram[SMBC_ALT_ENTRANCE_CONTROL] = 0x02u;
  chg_area_mode(st);
}

/* smb.asm: NextArea — advance to the next area and reset for it */
static void next_area(Smbc *smbc) {
  CoreState *st = &smbc->state;
  st->ram[SMBC_AREA_NUMBER] += 1u;
  load_area_pointer(smbc);
  st->ram[SMBC_FETCH_NEW_GAME_TIMER_FLAG] += 1u;
  chg_area_mode(st);
  st->ram[SMBC_HALFWAY_PAGE] = 0x00u; /* A is zero after ChgAreaMode */
  st->ram[SMBC_EVENT_MUSIC_QUEUE] = SMBC_MUSIC_SILENCE;
}

void entrance_game_timer_setup(Smbc *smbc) {
  CoreState *st = &smbc->state;
  st->ram[SMBC_PLAYER_PAGE_LOC] = st->ram[SMBC_SCREEN_LEFT_PAGE_LOC];
  st->ram[SMBC_VERTICAL_FORCE_DOWN] = 0x28u;
  st->ram[SMBC_PLAYER_FACING_DIR] = 0x01u; /* facing right */
  st->ram[SMBC_PLAYER_Y_HIGH_POS] = 0x01u;
  st->ram[SMBC_PLAYER_STATE] = 0x00u; /* on the ground */
  st->ram[SMBC_PLAYER_COLLISION_BITS] -= 1u; /* dec from zero: $ff */
  /* water areas get the swimming flag; others do not */
  st->ram[SMBC_SWIMMING_FLAG] = st->ram[SMBC_AREA_TYPE] == 0x00u ? 0x01u : 0x00u;
  uint8_t x = st->ram[SMBC_PLAYER_ENTRANCE_CTRL];
  uint8_t y = st->ram[SMBC_ALT_ENTRANCE_CONTROL];
  if (y >= 0x02u) x = kAltYPosOffset[y - 0x02u];
  st->ram[SMBC_PLAYER_X_POSITION] = kPlayerStartingXPos[y];
  st->ram[SMBC_PLAYER_Y_POSITION] = kPlayerStartingYPos[x];
  st->ram[SMBC_PLAYER_SPR_ATTRIB] = kPlayerBGPriorityData[x];
  get_player_colors_core(st);
  y = st->ram[SMBC_GAME_TIMER_SETTING];
  if (y != 0x00u && st->ram[SMBC_FETCH_NEW_GAME_TIMER_FLAG] != 0x00u) {
    st->ram[SMBC_GAME_TIMER_DISPLAY] = kGameTimerData[y];
    st->ram[SMBC_GAME_TIMER_DISPLAY + 2u] = 0x01u;
    st->ram[SMBC_GAME_TIMER_DISPLAY + 1u] = 0x00u;
    st->ram[SMBC_FETCH_NEW_GAME_TIMER_FLAG] = 0x00u;
    st->ram[SMBC_STAR_INVINCIBLE_TIMER] = 0x00u;
  }
  if (st->ram[SMBC_JOYPAD_OVERRIDE] != 0x00u) { /* ChkOverR — vine */
    st->ram[SMBC_PLAYER_STATE] = 0x03u; /* climbing */
    init_block_xy_pos(st, 0x00u);
    st->ram[SMBC_BLOCK_Y_POSITION] = 0xf0u;
    setup_vine(st, 0x05u, 0x00u);
  }
  if (st->ram[SMBC_AREA_TYPE] == 0x00u) { /* ChkSwimE — water area */
    /* the asm passes $07 to SetupBubble; at this point it still holds
     * PlayerHole's limit from before the area change (zero on a fresh
     * start) */
    setup_bubble(st, x, st->ram[SMBC_PLAYER_HOLE_LIMIT]);
  }
  st->ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x07u; /* SetPESub */
}

void player_lose_life(Smbc *smbc) {
  CoreState *st = &smbc->state;
  st->ram[SMBC_DISABLE_SCREEN_FLAG] += 1u;
  st->ram[SMBC_SPRITE0_HIT_FLAG] = 0x00u;
  st->ram[SMBC_EVENT_MUSIC_QUEUE] = SMBC_MUSIC_SILENCE;
  st->ram[SMBC_NUMBEROF_LIVES] -= 1u;
  if ((st->ram[SMBC_NUMBEROF_LIVES] & 0x80u) != 0x00u) { /* bpl fell */
    st->ram[SMBC_OPER_MODE_TASK] = 0x00u;
    st->ram[SMBC_OPER_MODE] = SMBC_MODE_GAME_OVER;
    return;
  }
  /* StillInGame — the halfway page for the world and level */
  uint8_t x = (uint8_t)(st->ram[SMBC_WORLD_NUMBER] << 1u);
  if ((st->ram[SMBC_LEVEL_NUMBER] & 0x02u) != 0x00u) x++;
  uint8_t a = kHalfwayPageNybbles[x];
  if ((st->ram[SMBC_LEVEL_NUMBER] & 0x01u) == 0x00u) a >>= 4u;
  a &= 0x0fu;
  /* past the halfway page the player starts at the beginning */
  if (a > st->ram[SMBC_SCREEN_LEFT_PAGE_LOC]) a = 0x00u;
  st->ram[SMBC_HALFWAY_PAGE] = a;
  (void)transpose_players(smbc); /* the carry is ignored here */
  continue_game(smbc);
}

void player_ctrl_routine(Smbc *smbc) {
  CoreState *st = &smbc->state;
  if (st->ram[SMBC_GAME_ENGINE_SUBROUTINE] != 0x0bu) {
    if (st->ram[SMBC_AREA_TYPE] == 0x00u) {
      /* water areas: outside the band between the status bar and the
       * bottom of the screen the controller is dead */
      if (st->ram[SMBC_PLAYER_Y_HIGH_POS] != 0x01u ||
          st->ram[SMBC_PLAYER_Y_POSITION] >= 0xd0u) {
        st->ram[SMBC_SAVED_JOYPAD1_BITS] = 0x00u; /* DisJoyp */
      }
    }
    uint8_t joy = st->ram[SMBC_SAVED_JOYPAD1_BITS];
    st->ram[SMBC_A_B_BUTTONS] = joy & 0xc0u;
    st->ram[SMBC_LEFT_RIGHT_BUTTONS] = joy & 0x03u;
    st->ram[SMBC_UP_DOWN_BUTTONS] = joy & 0x0cu;
    if ((joy & 0x04u) != 0x00u && st->ram[SMBC_PLAYER_STATE] == 0x00u &&
        st->ram[SMBC_LEFT_RIGHT_BUTTONS] != 0x00u) {
      /* down on the ground with a direction: crouch, not move */
      st->ram[SMBC_LEFT_RIGHT_BUTTONS] = 0x00u;
      st->ram[SMBC_UP_DOWN_BUTTONS] = 0x00u;
    }
  }
  /* TODO(port): smb.asm: PlayerMovementSubs (core/player physics) */
  uint8_t y;
  if (st->ram[SMBC_PLAYER_SIZE] != 0x00u) {
    y = 0x01u; /* small */
  } else if (st->ram[SMBC_CROUCHING_FLAG] == 0x00u) {
    y = 0x00u;
  } else {
    y = 0x02u; /* big and crouching */
  }
  /* Player_BoundBoxCtrl shares the address with SprObj_BoundBoxCtrl */
  st->ram[SMBC_SPROBJ_BOUND_BOX_CTRL] = y;
  int8_t xspeed = (int8_t)st->ram[SMBC_PLAYER_X_SPEED];
  if (xspeed != 0) {
    /* zero speed keeps the previous moving direction */
    st->ram[SMBC_PLAYER_MOVING_DIR] = xspeed < 0 ? 0x02u : 0x01u;
  }
  scroll_handler(smbc);
  get_player_offscreen_bits(st);
  relative_player_position(st);
  get_player_bound_box(st);
  /* TODO(port): smb.asm: PlayerBGCollision (core/collide) */
  uint8_t sub = st->ram[SMBC_GAME_ENGINE_SUBROUTINE];
  if (st->ram[SMBC_PLAYER_Y_POSITION] >= 0x40u && sub != 0x05u &&
      sub != 0x07u && sub >= 0x04u) {
    /* below the 64th pixel during normal play: lose the background
     * priority */
    st->ram[SMBC_PLAYER_SPR_ATTRIB] &= 0xdfu;
  }
  /* PlayerHole — the fall-down-a-hole handling */
  uint8_t yhigh = st->ram[SMBC_PLAYER_Y_HIGH_POS];
  if ((uint8_t)(yhigh - 0x02u) & 0x80u) return; /* bmi ExitCtrl */
  st->ram[SMBC_SCROLL_LOCK] = 0x01u;
  uint8_t limit = 0x04u; /* $07 default */
  bool die = false;
  if (st->ram[SMBC_GAME_TIMER_EXPIRED_FLAG] != 0x00u ||
      st->ram[SMBC_CLOUD_TYPE_OVERRIDE] == 0x00u) {
    die = true; /* HoleDie */
    if (sub != 0x0bu) {
      if (st->ram[SMBC_DEATH_MUSIC_LOADED] == 0x00u) {
        st->ram[SMBC_EVENT_MUSIC_QUEUE] = 0x01u; /* the death music */
        st->ram[SMBC_DEATH_MUSIC_LOADED] = 0x01u;
      }
      limit = 0x06u; /* HoleBottom */
    }
  }
  if ((uint8_t)(yhigh - limit) & 0x80u) return; /* cmp $07, bmi */
  if (!die) { /* dex, bmi CloudExit — fall in a cloud bonus area */
    st->ram[SMBC_JOYPAD_OVERRIDE] = 0x00u;
    set_entr(st);
    st->ram[SMBC_ALT_ENTRANCE_CONTROL] += 1u; /* mode of entry 3 */
    return;
  }
  if (st->ram[SMBC_EVENT_MUSIC_BUFFER] != 0x00u) return;
  st->ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x06u; /* PlayerLoseLife */
}

/* smb.asm: AutoControlPlayer — override the controller bits and run
 * the player control routine with them */
static void auto_control_player(Smbc *smbc, uint8_t a) {
  smbc->state.ram[SMBC_SAVED_JOYPAD1_BITS] = a;
  player_ctrl_routine(smbc);
}

/* smb.asm: MovePlayerYAxis */
static void move_player_y_axis(CoreState *st, uint8_t a) {
  st->ram[SMBC_PLAYER_Y_POSITION] += a;
}

/* smb.asm: EnterSidePipe — walk the player out of the side pipe, one
 * pixel column at a time */
static void enter_side_pipe(Smbc *smbc) {
  CoreState *st = &smbc->state;
  uint8_t y = 0x01u; /* controller right by default */
  st->ram[SMBC_PLAYER_X_SPEED] = 0x08u;
  if ((st->ram[SMBC_PLAYER_X_POSITION] & 0x0fu) == 0x00u) {
    st->ram[SMBC_PLAYER_X_SPEED] = 0x00u;
    y = 0x00u; /* aligned: stop */
  }
  auto_control_player(smbc, y);
}

void player_entrance(Smbc *smbc) {
  CoreState *st = &smbc->state;
  if (st->ram[SMBC_ALT_ENTRANCE_CONTROL] == 0x02u) { /* EntrMode2 */
    if (st->ram[SMBC_JOYPAD_OVERRIDE] != 0x00u) { /* VineEntr */
      if (st->ram[SMBC_VINE_HEIGHT] != 0x60u) return;
      uint8_t y = 0x00u;
      uint8_t a = 0x01u; /* walk right off the vine */
      if (st->ram[SMBC_PLAYER_Y_POSITION] >= 0x99u) {
        st->ram[SMBC_PLAYER_STATE] = 0x03u; /* climbing */
        y = 0x01u;
        a = 0x08u;
        /* cover the hole in the block buffer, then force the climb */
        st->ram[SMBC_BLOCK_BUFFER_1 + 0xb4u] = 0x08u;
      }
      st->ram[SMBC_DISABLE_COLLISION_DET] = y;
      auto_control_player(smbc, a);
      if (st->ram[SMBC_PLAYER_X_POSITION] < 0x48u) return;
    } else {
      move_player_y_axis(st, 0xffu); /* rise out of the pipe */
      if (st->ram[SMBC_PLAYER_Y_POSITION] >= 0x91u) return;
    }
  } else if (st->ram[SMBC_PLAYER_Y_POSITION] < 0x30u) {
    auto_control_player(smbc, 0x00u); /* still dropping in */
    return;
  } else {
    uint8_t ctrl = st->ram[SMBC_PLAYER_ENTRANCE_CTRL];
    if (ctrl == 0x06u || ctrl == 0x07u) {
      if (st->ram[SMBC_PLAYER_SPR_ATTRIB] == 0x00u) {
        auto_control_player(smbc, 0x01u); /* walk right */
        return;
      }
      /* IntroEntr — the pipe intro */
      enter_side_pipe(smbc);
      st->ram[SMBC_CHANGE_AREA_TIMER] -= 1u;
      if (st->ram[SMBC_CHANGE_AREA_TIMER] != 0x00u) return;
      st->ram[SMBC_DISABLE_INTERMEDIATE] += 1u;
      next_area(smbc);
      return;
    }
  }
  /* PlayerRdy — hand control to the player */
  st->ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x08u;
  st->ram[SMBC_PLAYER_FACING_DIR] = 0x01u;
  st->ram[SMBC_ALT_ENTRANCE_CONTROL] = 0x00u;
  st->ram[SMBC_DISABLE_COLLISION_DET] = 0x00u;
  st->ram[SMBC_JOYPAD_OVERRIDE] = 0x00u;
}

void vine_auto_climb(Smbc *smbc) {
  CoreState *st = &smbc->state;
  if (st->ram[SMBC_PLAYER_Y_HIGH_POS] == 0x00u &&
      st->ram[SMBC_PLAYER_Y_POSITION] < 0xe4u) {
    set_entr(st); /* above the status bar: switch modes */
    return;
  }
  st->ram[SMBC_JOYPAD_OVERRIDE] = 0x08u; /* hold up */
  st->ram[SMBC_PLAYER_STATE] = 0x03u;    /* climbing */
  auto_control_player(smbc, 0x08u);
}

/* smb.asm: ChgAreaPipe — when the change-area timer expires, set the
 * mode of entry and change modes */
static void chg_area_pipe(CoreState *st, uint8_t y) {
  st->ram[SMBC_CHANGE_AREA_TIMER] -= 1u;
  if (st->ram[SMBC_CHANGE_AREA_TIMER] != 0x00u) return;
  st->ram[SMBC_ALT_ENTRANCE_CONTROL] = y;
  chg_area_mode(st);
}

void vertical_pipe_entry(Smbc *smbc) {
  CoreState *st = &smbc->state;
  move_player_y_axis(st, 0x01u); /* sink into the pipe */
  scroll_handler(smbc);
  uint8_t y = 0x00u;
  if (st->ram[SMBC_WARP_ZONE_CONTROL] == 0x00u) {
    y = 0x01u;
    if (st->ram[SMBC_AREA_TYPE] == 0x03u) y = 0x02u; /* castle */
  }
  chg_area_pipe(st, y);
}

void side_exit_pipe_entry(Smbc *smbc) {
  enter_side_pipe(smbc);
  chg_area_pipe(&smbc->state, 0x02u);
}

/* smb.asm: InitChangeSize — start the grow/shrink sequence unless one
 * is already running */
static void init_change_size(CoreState *st) {
  if (st->ram[SMBC_PLAYER_CHANGE_SIZE_FLAG] != 0x00u) return;
  st->ram[SMBC_PLAYER_ANIM_CTRL] = 0x00u;
  st->ram[SMBC_PLAYER_CHANGE_SIZE_FLAG] += 1u;
  st->ram[SMBC_PLAYER_SIZE] ^= 0x01u;
}

/* smb.asm: DonePlayerTask — timers run again, back to the player
 * control routine */
static void done_player_task(CoreState *st) {
  st->ram[SMBC_TIMER_CONTROL] = 0x00u;
  st->ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x08u;
}

void player_change_size(Smbc *smbc) {
  CoreState *st = &smbc->state;
  uint8_t tc = st->ram[SMBC_TIMER_CONTROL];
  if (tc == 0xf8u) {
    init_change_size(st);
    return;
  }
  if (tc == 0xc4u) done_player_task(st);
}

void player_injury_blink(Smbc *smbc) {
  CoreState *st = &smbc->state;
  uint8_t tc = st->ram[SMBC_TIMER_CONTROL];
  if (tc >= 0xf0u) {
    /* ExitBlink — at exactly $f0 the branch is not taken and the asm
     * falls through into InitChangeSize (the injury shrink) */
    if (tc != 0xf0u) return;
    init_change_size(st);
    return;
  }
  if (tc == 0xc8u) {
    done_player_task(st);
    return;
  }
  player_ctrl_routine(smbc);
}

void player_death(Smbc *smbc) {
  CoreState *st = &smbc->state;
  if (st->ram[SMBC_TIMER_CONTROL] >= 0xf0u) return; /* ExitDeath */
  player_ctrl_routine(smbc);
}

void player_fire_flower(Smbc *smbc) {
  CoreState *st = &smbc->state;
  if (st->ram[SMBC_TIMER_CONTROL] == 0xc0u) {
    done_player_task(st);   /* ResetPalFireFlower */
    reset_pal_star(st);
    return;
  }
  /* the palette cycles every four frames */
  cycle_player_palette(st, st->ram[SMBC_FRAME_COUNTER] >> 2u);
}

void flagpole_slide(Smbc *smbc) {
  CoreState *st = &smbc->state;
  if (st->ram[SMBC_ENEMY_ID + 5u] != 0x30u) { /* FlagpoleFlagObject */
    st->ram[SMBC_GAME_ENGINE_SUBROUTINE] += 1u; /* NoFPObj — residual */
    return;
  }
  st->ram[SMBC_SQUARE1_SOUND_QUEUE] = st->ram[SMBC_FLAGPOLE_SOUND_QUEUE];
  st->ram[SMBC_FLAGPOLE_SOUND_QUEUE] = 0x00u;
  /* slide down the pole until the bottom, then stand */
  uint8_t a = st->ram[SMBC_PLAYER_Y_POSITION] >= 0x9eu ? 0x00u : 0x04u;
  auto_control_player(smbc, a);
}

void player_end_level(Smbc *smbc) {
  CoreState *st = &smbc->state;
  auto_control_player(smbc, 0x01u); /* walk right */
  if (st->ram[SMBC_PLAYER_Y_POSITION] >= 0xaeu &&
      st->ram[SMBC_SCROLL_LOCK] != 0x00u) {
    /* off the flagpole: the end-of-level music, once */
    st->ram[SMBC_EVENT_MUSIC_QUEUE] = SMBC_EVENT_MUSIC_END_OF_LEVEL;
    st->ram[SMBC_SCROLL_LOCK] = 0x00u;
  }
  if ((st->ram[SMBC_PLAYER_COLLISION_BITS] & 0x01u) == 0x00u) {
    if (st->ram[SMBC_STAR_FLAG_TASK_CONTROL] == 0x00u) {
      st->ram[SMBC_STAR_FLAG_TASK_CONTROL] += 1u;
    }
    /* InCastle — the background priority gives the illusion of being
     * inside the castle */
    st->ram[SMBC_PLAYER_SPR_ATTRIB] = 0x20u;
  }
  if (st->ram[SMBC_STAR_FLAG_TASK_CONTROL] != 0x05u) return;
  st->ram[SMBC_LEVEL_NUMBER] += 1u;
  if (st->ram[SMBC_LEVEL_NUMBER] == 0x03u) {
    /* the hidden 1-up for enough coins in the third area */
    if (st->ram[SMBC_COIN_TALLY_FOR_1UPS] >=
        kHidden1UpCoinAmts[st->ram[SMBC_WORLD_NUMBER]]) {
      st->ram[SMBC_HIDDEN_1UP_FLAG] += 1u;
    }
  }
  next_area(smbc);
}

void scroll_handler(Smbc *smbc) {
  CoreState *st = &smbc->state;
  st->ram[SMBC_PLAYER_X_SCROLL] += st->ram[SMBC_PLATFORM_X_SCROLL];
  uint8_t y = 0x00u; /* the scroll amount this frame */
  if (st->ram[SMBC_SCROLL_LOCK] == 0x00u &&
      st->ram[SMBC_PLAYER_POS_FORSCROLL] >= 0x50u &&
      st->ram[SMBC_SIDE_COLLISION_TIMER] == 0x00u) {
    int8_t scroll = (int8_t)st->ram[SMBC_PLAYER_X_SCROLL];
    if (scroll >= 0x01) {
      /* moving right: scroll with the player's speed, one less unless
       * it is one; past 112 pixels the full speed */
      y = (uint8_t)scroll;
      if (scroll > 0x01) y--;
      if (st->ram[SMBC_PLAYER_POS_FORSCROLL] >= 0x70u) y = (uint8_t)scroll;
    }
  }
  if (y != 0x00u) { /* ScrollScreen */
    st->ram[SMBC_SCROLL_AMOUNT] = y;
    st->ram[SMBC_SCROLL_THIRTY_TWO] += y;
    uint16_t x = (uint16_t)st->ram[SMBC_SCREEN_LEFT_X_POS] + y;
    st->ram[SMBC_SCREEN_LEFT_X_POS] = (uint8_t)x;
    st->ram[SMBC_HORIZONTAL_SCROLL] = (uint8_t)x;
    uint8_t page = (uint8_t)(st->ram[SMBC_SCREEN_LEFT_PAGE_LOC] +
                             (uint8_t)(x >> 8));
    st->ram[SMBC_SCREEN_LEFT_PAGE_LOC] = page;
    /* the page's LSB mirrors into the PPU control register's name
     * table bit */
    st->ram[SMBC_MIRROR_PPU_CTRL1] =
        (st->ram[SMBC_MIRROR_PPU_CTRL1] & 0xfeu) | (page & 0x01u);
    (void)get_screen_position(st);
    st->ram[SMBC_SCROLL_INTERVAL_TIMER] = 0x08u;
  } else {
    st->ram[SMBC_SCROLL_AMOUNT] = 0x00u; /* InitScrlAmt */
  }
  /* ChkPOffscr — clamp the player at the screen edges; the asm's
   * ScreenEdge_X_Pos/ScreenEdge_PageLoc alias ScreenLeft_* */
  uint8_t bits = get_x_offscreen_bits(st, 0x00u);
  uint8_t side = 0x00u;
  if ((bits & 0x80u) == 0x00u) {
    side = 0x01u; /* right side */
    if ((bits & 0x20u) == 0x00u) {
      st->ram[SMBC_PLATFORM_X_SCROLL] = 0x00u; /* InitPlatScrl */
      return;
    }
  }
  uint16_t pos = (uint16_t)st->ram[SMBC_SCREEN_LEFT_X_POS + side] -
                 kXSubtracterData[side];
  st->ram[SMBC_PLAYER_X_POSITION] = (uint8_t)pos;
  st->ram[SMBC_PLAYER_PAGE_LOC] =
      (uint8_t)(st->ram[SMBC_SCREEN_LEFT_PAGE_LOC + side] -
                (uint8_t)(pos >> 15)); /* the borrow: bit 15 of the
                                        * sign-extended difference */
  /* pressing away from the edge keeps the speed; anything else is
   * nullified (the asm's beq skips the store) */
  if (st->ram[SMBC_LEFT_RIGHT_BUTTONS] != kOffscrJoypadBitsData[side]) {
    st->ram[SMBC_PLAYER_X_SPEED] = 0x00u;
  }
  st->ram[SMBC_PLATFORM_X_SCROLL] = 0x00u;
}
