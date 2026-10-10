/* core/victory.c — see core/victory.h for the contract. */
#include "victory.h"

#include "../data/music.h"
#include "area.h"
#include "frame.h"
#include "gameover.h"
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
static void victory_mode_subroutines(Smbc *smbc) {
  switch (smbc->state.ram[SMBC_OPER_MODE_TASK]) {
    case 0: /* TODO(port): smb.asm: BridgeCollapse (player/bridge atoms) */ break;
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
    /* TODO(port): smb.asm: EnemiesAndLoopsCore (core/enemies) */
  }
  /* AutoPlayer — TODO(port): smb.asm: RelativePlayerPosition and
   * PlayerGfxHandler (core/player) */
}
