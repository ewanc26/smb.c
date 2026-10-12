/* core/victory.c — see core/victory.h for the contract. */
#include "victory.h"

#include "../data/enemies.h"
#include "../data/metatiles.h"
#include "../data/music.h"
#include "area.h"
#include "enemies.h"
#include "frame.h"
#include "gameover.h"
#include "move.h"
#include "offscreen.h"
#include "playergfx.h"
#include "runners.h"
#include "state.h"

/* smb.asm: SetupVictoryMode */
static void setup_victory_mode(Smbc *smbc) {
  CoreState *st = &smbc->state;
  /* inx — destination is the page after the right side of the screen */
  st->ram[SMBC_DESTINATION_PAGE_LOC] = st->ram[SMBC_SCREEN_RIGHT_PAGE_LOC] + 1u;
  st->ram[SMBC_EVENT_MUSIC_QUEUE] = SMBC_EVENT_MUSIC_END_OF_CASTLE;
  st->ram[SMBC_OPER_MODE_TASK] += 1u; /* IncModeTask_B */
}

/* smb.asm: SetEndTimer + IncModeTask_A */
static void set_end_timer(Smbc *smbc) {
  CoreState *st = &smbc->state;
  st->ram[SMBC_WORLD_END_TIMER] = 0x06;
  st->ram[SMBC_OPER_MODE_TASK] += 1u; /* move onto next task in mode */
}

/* smb.asm: PrintVictoryMessages — the primary counter advances once per
 * overflow of the secondary counter (+4 per frame); which message (if
 * any) prints on that frame depends on the counter, the world and the
 * player. The $09 comparison is residual code — the counter never
 * reaches 9. */
static void print_victory_messages(Smbc *smbc) {
  CoreState *st = &smbc->state;
  if (st->ram[SMBC_SECONDARY_MSG_COUNTER] == 0x00) {
    uint8_t p = st->ram[SMBC_PRIMARY_MSG_COUNTER];
    if (p < 0x09) {
      bool world8 = st->ram[SMBC_WORLD_NUMBER] == SMBC_WORLD_8;
      int8_t msg_y = -1; /* no message this frame */
      if (p == 0x00) {
        /* ThankPlayer — first message; luigi's is one index up */
        msg_y = (st->ram[SMBC_CURRENT_PLAYER] != 0x00) ? 1 : 0;
      } else if (world8 ? p >= 0x03 : p >= 0x02) {
        uint8_t y = world8 ? (uint8_t)(p - 1u) : p; /* world 8: sbc #$01 */
        y += 1u; /* SecondPartMsg: iny */
        if (!world8) {
          y -= 1u; /* dey for worlds 1-7 */
          if (y >= 0x04) {
            set_end_timer(smbc);
            return; /* SetEndTimer — skip the counters entirely */
          }
          if (y >= 0x03) goto inc_msg_counter; /* keep counting */
        }
        msg_y = (int8_t)y;
      }
      if (msg_y >= 0) {
        /* EvalForMusic — only world 8 can reach this with y == 3 */
        if (msg_y == 3) {
          st->ram[SMBC_EVENT_MUSIC_QUEUE] = SMBC_EVENT_MUSIC_VICTORY;
        }
        /* PrintMsg — $0c-$0d first, $0e world 1-7, $0f-$12 world 8 */
        st->ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = (uint8_t)(msg_y + 0x0c);
      }
    }
  }
inc_msg_counter:
  /* IncMsgCounter — secondary takes +4, its carry increments primary */
  {
    uint8_t sec = st->ram[SMBC_SECONDARY_MSG_COUNTER];
    st->ram[SMBC_SECONDARY_MSG_COUNTER] = (uint8_t)(sec + 0x04u);
    if (sec + 0x04u > 0xffu) {
      st->ram[SMBC_PRIMARY_MSG_COUNTER] += 1u;
    }
  }
  if (st->ram[SMBC_PRIMARY_MSG_COUNTER] >= 0x07) {
    set_end_timer(smbc); /* carry clear from cmp falls here at $07+ */
  }
}

/* smb.asm: PlayerEndWorld */
static void player_end_world(Smbc *smbc) {
  CoreState *st = &smbc->state;
  if (st->ram[SMBC_WORLD_END_TIMER] != 0x00) return; /* EndExitOne */
  if (st->ram[SMBC_WORLD_NUMBER] >= SMBC_WORLD_8) {
    /* EndChkBButton — B on either controller after world 8 */
    uint8_t a = (uint8_t)(st->ram[SMBC_SAVED_JOYPAD1_BITS] |
                          st->ram[SMBC_SAVED_JOYPAD2_BITS]) & SMBC_PAD_B;
    if (a == 0x00) return; /* EndExitTwo */
    st->ram[SMBC_WORLD_SELECT_ENABLE_FLAG] = 0x01;
    st->ram[SMBC_NUMBEROF_LIVES] = 0xffu; /* remove onscreen player's lives */
    terminate_game(smbc);
    return;
  }
  /* on to the next world */
  st->ram[SMBC_AREA_NUMBER] = 0x00;
  st->ram[SMBC_LEVEL_NUMBER] = 0x00;
  st->ram[SMBC_OPER_MODE_TASK] = 0x00;
  st->ram[SMBC_WORLD_NUMBER] += 1u; /* inc */
  load_area_pointer(smbc);
  st->ram[SMBC_FETCH_NEW_GAME_TIMER_FLAG] += 1u; /* inc */
  st->ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
}

/* smb.asm: VictoryModeSubroutines — jump table order:
 *   0 BridgeCollapse, 1 SetupVictoryMode, 2 PlayerVictoryWalk,
 *   3 PrintVictoryMessages, 4 PlayerEndWorld */
/* smb.asm: RemBridge — write one blank metatile (the BlockGfxData quad
 * at offset $0c) into vram buffer 1 at the offset plus one; the name
 * table address is the $04/$05 pair the caller set (always $22xx here,
 * the low byte from BridgeCollapseData) */
static void rem_bridge(CoreState *st, uint8_t lo, uint8_t y) {
  st->ram[SMBC_VRAM_BUFFER1 + 2u + y] = SMBC_BLOCK_GFX_DATA[12u];
  st->ram[SMBC_VRAM_BUFFER1 + 3u + y] = SMBC_BLOCK_GFX_DATA[13u];
  st->ram[SMBC_VRAM_BUFFER1 + 7u + y] = SMBC_BLOCK_GFX_DATA[14u];
  st->ram[SMBC_VRAM_BUFFER1 + 8u + y] = SMBC_BLOCK_GFX_DATA[15u];
  st->ram[SMBC_VRAM_BUFFER1 + y] = lo;
  st->ram[SMBC_VRAM_BUFFER1 + 5u + y] = (uint8_t)(lo + 0x20u);
  st->ram[SMBC_VRAM_BUFFER1 - 1u + y] = 0x22u;
  st->ram[SMBC_VRAM_BUFFER1 + 4u + y] = 0x22u;
  st->ram[SMBC_VRAM_BUFFER1 + 1u + y] = 0x02u;
  st->ram[SMBC_VRAM_BUFFER1 + 6u + y] = 0x02u;
  st->ram[SMBC_VRAM_BUFFER1 + 9u + y] = 0x00u;
}

/* smb.asm: BridgeCollapse — bowser hangs on the bridge while the axe,
 * chain and bridge metatiles blank out behind him, then he falls
 * through the gap; once he is low enough the mode moves on */
static void bridge_collapse(Smbc *smbc) {
  CoreState *st = &smbc->state;
  uint8_t x = st->ram[SMBC_BOWSER_FRONT_OFFSET];
  if (st->ram[SMBC_ENEMY_ID + x] != SMBC_BOWSER) goto set_m2;
  st->ram[SMBC_OBJECT_OFFSET] = x;
  uint8_t state = st->ram[SMBC_ENEMY_STATE + x];
  if (state == 0x00u) {
    /* RemoveBridge — the feet counter paces the collapse, the body
     * controls bit flips bowser's feet each step */
    st->ram[SMBC_BOWSER_FEET_COUNTER] -= 1u;
    if (st->ram[SMBC_BOWSER_FEET_COUNTER] != 0x00u) goto draw_bowser;
    st->ram[SMBC_BOWSER_FEET_COUNTER] = 0x04u;
    st->ram[SMBC_BOWSER_BODY_CONTROLS] ^= 0x01u;
    uint8_t ofs = st->ram[SMBC_BRIDGE_COLLAPSE_OFFSET];
    rem_bridge(st, SMBC_BRIDGE_COLLAPSE_DATA[ofs],
               (uint8_t)(st->ram[SMBC_VRAM_BUFFER1_OFFSET] + 1u));
    /* MoveVOffset — the write used offset plus one; put the real offset
     * back and advance it past the ten bytes just queued */
    st->ram[SMBC_VRAM_BUFFER1_OFFSET] += 10u;
    st->ram[SMBC_SQUARE2_SOUND_QUEUE] = SMBC_SFX_BLAST;
    st->ram[SMBC_NOISE_SOUND_QUEUE] = SMBC_SFX_BRICK_SHATTER;
    st->ram[SMBC_BRIDGE_COLLAPSE_OFFSET] = (uint8_t)(ofs + 1u);
    if (ofs + 1u != 0x0fu) goto draw_bowser;
    /* the whole bridge is gone: bowser drops, defeated */
    st->ram[SMBC_ENEMY_Y_SPEED + x] = 0x00u;     /* InitVStf */
    st->ram[SMBC_ENEMY_Y_MOVE_FORCE + x] = 0x00u;
    st->ram[SMBC_ENEMY_STATE + x] = 0x40u;
    st->ram[SMBC_SQUARE2_SOUND_QUEUE] = SMBC_SFX_BOWSER_FALL;
    goto draw_bowser;
  }
  if ((state & 0x40u) != 0x00u &&
      st->ram[SMBC_ENEMY_Y_POSITION + x] < 0xe0u) {
    /* MoveD_Bowser — slide the defeated bowser down through the gap */
    set_x_move_amt(st, x, 0x0fu, 0x02u); /* MoveEnemySlowVert */
    goto draw_bowser;
  }
set_m2:
  st->ram[SMBC_EVENT_MUSIC_QUEUE] = SMBC_MUSIC_SILENCE; /* SetM2 */
  st->ram[SMBC_OPER_MODE_TASK] += 1u;
  kill_all_enemies(st);
  return;
draw_bowser:
  bowser_gfx_handler(st, x);
}

static void victory_mode_subroutines(Smbc *smbc) {
  switch (smbc->state.ram[SMBC_OPER_MODE_TASK]) {
    case 0: bridge_collapse(smbc); break;
    case 1: setup_victory_mode(smbc); break;
    case 2: /* TODO(port): smb.asm: PlayerVictoryWalk (player/scroll atoms) */ break;
    case 3: print_victory_messages(smbc); break;
    case 4: player_end_world(smbc); break;
    default: break;
  }
}

void victory_mode(Smbc *smbc) {
  CoreState *st = &smbc->state;
  victory_mode_subroutines(smbc);
  if (st->ram[SMBC_OPER_MODE_TASK] != 0x00) {
    st->ram[SMBC_OBJECT_OFFSET] = 0x00; /* reset enemy object offset */
    enemies_and_loops_core(st, 0x00); /* not on bridge collapse */
  }
  /* AutoPlayer */
  relative_player_position(st);
  player_gfx_handler(st);
}
