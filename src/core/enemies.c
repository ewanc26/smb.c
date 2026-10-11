/* core/enemies.c — see enemies.h */
#include "enemies.h"

#include "../data/enemies.h"
#include "enemyinit.h"

/* smb.asm: World7 — world numbers are zero-based */
#define SMBC_WORLD_7 0x06u

/* smb.asm: CheckpointEnemyID — identifiers below $15 get eight pixels
 * added to their vertical coordinate (the cmp leaves the carry clear
 * on this path, so the adc adds exactly eight) and the offscreen
 * masked bit set, then the init jump engine runs */
void checkpoint_enemy_id(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_ENEMY_ID + x] < 0x15u) {
    st->ram[SMBC_ENEMY_Y_POSITION + x] += 0x08u;
    st->ram[SMBC_ENEMY_OFFSCR_BITS_MASKED + x] = 0x01u;
  }
  run_init_enemy_routines(st, x);
}

/* smb.asm: InitEnemyObject — initialize the state and run the
 * identifier checkpoint */
static void init_enemy_object(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_STATE + x] = 0x00u;
  checkpoint_enemy_id(st, x);
}

/* smb.asm: EraseEnemyObject — clear all of a slot's variables.
 * Exported for the runner layer's OffscreenBoundsCheck */
void erase_enemy_object(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_FLAG + x] = 0x00u;
  st->ram[SMBC_ENEMY_ID + x] = 0x00u;
  st->ram[SMBC_ENEMY_STATE + x] = 0x00u;
  st->ram[SMBC_FLOATEY_NUM_CONTROL + x] = 0x00u;
  st->ram[SMBC_ENEMY_INTERVAL_TIMER + x] = 0x00u;
  st->ram[SMBC_SHELL_CHAIN_COUNTER + x] = 0x00u;
  st->ram[SMBC_ENEMY_SPR_ATTRIB + x] = 0x00u;
  st->ram[SMBC_ENEMY_FRAME_TIMER + x] = 0x00u;
}

/* smb.asm: KillAllEnemies — erase the first five slots and the frenzy
 * buffer (A holds zero after the loop; the asm also reloads X from
 * ObjectOffset, a no-op for the port's value parameter) */
static void kill_all_enemies(CoreState *st) {
  for (int x = 0x04; x >= 0; x--) {
    erase_enemy_object(st, (uint8_t)x);
  }
  st->ram[SMBC_ENEMY_FRENZY_BUFFER] = 0x00u;
}

/* smb.asm: Inc2B — advance the enemy data offset two bytes and reset
 * the page select (the asm's ldx ObjectOffset is a no-op here) */
static void inc2b(CoreState *st) {
  st->ram[SMBC_ENEMY_DATA_OFFSET] += 0x02u;
  st->ram[SMBC_ENEMY_OBJECT_PAGE_SEL] = 0x00u;
}

/* smb.asm: ParseRow0e — a three-byte area jump object: if the third
 * byte's high three bits name the current world, the second byte
 * becomes the area pointer and the low five the entrance page */
static void parse_row_0e(CoreState *st) {
  uint8_t y = (uint8_t)(st->ram[SMBC_ENEMY_DATA_OFFSET] + 0x02u);
  uint8_t b3 = st->enemy_data[y];
  if ((uint8_t)(b3 >> 5) == st->ram[SMBC_WORLD_NUMBER]) {
    st->ram[SMBC_AREA_POINTER] = st->enemy_data[y - 1u];
    st->ram[SMBC_ENTRANCE_PAGE] = (uint8_t)(b3 & 0x1fu);
  }
  /* NotUse: Inc3B */
  st->ram[SMBC_ENEMY_DATA_OFFSET] += 0x03u;
  st->ram[SMBC_ENEMY_OBJECT_PAGE_SEL] = 0x00u;
}

/* smb.asm: CheckFrenzyBuffer — store the frenzy buffer's object (or a
 * vine, when the vine flag offset is one) into the slot and run its
 * init; otherwise leave */
static void check_frenzy_buffer(CoreState *st, uint8_t x) {
  uint8_t id = st->ram[SMBC_ENEMY_FRENZY_BUFFER];
  if (id == 0x00u) {
    if (st->ram[SMBC_VINE_FLAG_OFFSET] != 0x01u) return; /* ExEPar */
    id = SMBC_VINE_OBJECT;
  }
  st->ram[SMBC_ENEMY_ID + x] = id; /* StrFre */
  init_enemy_object(st, x);
}

/* smb.asm: HandleGroupEnemies — spawn two or three copies of the group
 * enemy at the right edge of the screen, 24 pixels apart, into the
 * first free slots below the fifth. Scratch $00-$03 become locals */
static void handle_group_enemies(CoreState *st, uint8_t id) {
  uint8_t rel = (uint8_t)(id - 0x37u);
  uint8_t enemy_id = SMBC_GREEN_KOOPA;         /* SnglID path ($3b-$3e) */
  if (rel < 0x04u) {
    enemy_id = SMBC_GOOMBA;
    if (st->ram[SMBC_PRIMARY_HARD_MODE] != 0x00u) {
      enemy_id = SMBC_BUZZY_BEETLE; /* hard mode mutates the goomba */
    }
  }
  uint8_t ypos = ((rel & 0x02u) != 0x00u) ? 0x70u : 0xb0u; /* $00 */
  uint8_t page = st->ram[SMBC_SCREEN_RIGHT_PAGE_LOC];      /* $02 */
  uint8_t xpos = st->ram[SMBC_SCREEN_RIGHT_X_POS];         /* $03 */
  st->ram[SMBC_NUMBER_OF_GROUP_ENEMIES] = (uint8_t)(0x02u + (rel & 0x01u));
  while (st->ram[SMBC_NUMBER_OF_GROUP_ENEMIES] != 0x00u) { /* GrLoop */
    int x = -1; /* GSltLp: find an empty slot below the fifth */
    for (int s = 0; s < 0x05; s++) {
      if (st->ram[SMBC_ENEMY_FLAG + s] == 0x00u) {
        x = s;
        break;
      }
    }
    if (x < 0) break; /* NextED */
    st->ram[SMBC_ENEMY_ID + x] = enemy_id;
    st->ram[SMBC_ENEMY_PAGE_LOC + x] = page;
    st->ram[SMBC_ENEMY_X_POSITION + x] = xpos;
    uint16_t next = (uint16_t)xpos + 0x18u;
    xpos = (uint8_t)next;
    page = (uint8_t)(page + (next >> 8));
    st->ram[SMBC_ENEMY_Y_POSITION + x] = ypos;
    st->ram[SMBC_ENEMY_Y_HIGH_POS + x] = 0x01u;
    st->ram[SMBC_ENEMY_FLAG + x] = 0x01u;
    checkpoint_enemy_id(st, (uint8_t)x);
    st->ram[SMBC_NUMBER_OF_GROUP_ENEMIES]--;
  }
  inc2b(st); /* NextED */
}

/* smb.asm: ExecGameLoopback — a failed loop command sends the player,
 * the renderer and both object parsers back four pages, and rewinds
 * the level data offset by the loop entry's table value */
static void exec_game_loopback(CoreState *st, uint8_t y) {
  st->ram[SMBC_PLAYER_PAGE_LOC] -= 0x04u;
  st->ram[SMBC_CURRENT_PAGE_LOC] -= 0x04u;
  st->ram[SMBC_SCREEN_LEFT_PAGE_LOC] -= 0x04u;
  st->ram[SMBC_SCREEN_RIGHT_PAGE_LOC] -= 0x04u;
  st->ram[SMBC_AREA_OBJECT_PAGE_LOC] -= 0x04u;
  st->ram[SMBC_ENEMY_OBJECT_PAGE_SEL] = 0x00u;
  st->ram[SMBC_AREA_OBJECT_PAGE_SEL] = 0x00u;
  st->ram[SMBC_ENEMY_DATA_OFFSET] = 0x00u;
  st->ram[SMBC_ENEMY_OBJECT_PAGE_LOC] = 0x00u;
  st->ram[SMBC_AREA_DATA_OFFSET] = SMBC_AREA_DATA_OFS_LOOPBACK[y];
}

/* smb.asm: ProcessEnemyData — walk the enemy object stream. Objects
 * spawn between the right screen boundary and the boundary extended by
 * 48 pixels; further out they wait, behind it they are skipped */
static void process_enemy_data(CoreState *st, uint8_t x) {
  uint8_t y = st->ram[SMBC_ENEMY_DATA_OFFSET];
  if (st->enemy_data[y] == 0xffu) { /* EOD terminator */
    check_frenzy_buffer(st, x);
    return;
  }
  /* CheckEndofBuffer — the residual code keeping enemies out of the
   * sixth slot (asm: not sure what this was intended for, exactly) */
  if ((st->enemy_data[y] & 0x0fu) != 0x0eu && x >= 0x05u &&
      (st->enemy_data[y + 1u] & 0x3fu) != 0x2eu) {
    return;
  }
  /* CheckRightBounds — the extended right boundary, $06/$07 in the
   * asm. The +48 carry feeds the page add even through the and */
  uint16_t ext = (uint16_t)st->ram[SMBC_SCREEN_RIGHT_X_POS] + 0x30u;
  uint8_t ext_col = (uint8_t)(ext & 0xf0u);
  uint8_t ext_page = (uint8_t)(st->ram[SMBC_SCREEN_RIGHT_PAGE_LOC] + (ext >> 8));
  /* the page select bit is d7 of the second byte */
  if ((st->enemy_data[y + 1u] & 0x80u) != 0x00u &&
      st->ram[SMBC_ENEMY_OBJECT_PAGE_SEL] == 0x00u) {
    st->ram[SMBC_ENEMY_OBJECT_PAGE_SEL]++;
    st->ram[SMBC_ENEMY_OBJECT_PAGE_LOC]++;
  }
  /* CheckPageCtrlRow — a row $0f object with the page select clear is
   * page control: store the new page and loop back to the top */
  if ((st->enemy_data[y] & 0x0fu) == 0x0fu &&
      st->ram[SMBC_ENEMY_OBJECT_PAGE_SEL] == 0x00u) {
    st->ram[SMBC_ENEMY_OBJECT_PAGE_LOC] =
        (uint8_t)(st->enemy_data[y + 1u] & 0x3fu);
    st->ram[SMBC_ENEMY_DATA_OFFSET] += 0x02u;
    st->ram[SMBC_ENEMY_OBJECT_PAGE_SEL]++;
    return; /* the asm jumps back to ProcLoopCommand, which the caller
             * reaches on the next frame */
  }
  /* PositionEnemyObj — compare the object's column against the right
   * boundary as a sixteen-bit page:column value */
  st->ram[SMBC_ENEMY_PAGE_LOC + x] = st->ram[SMBC_ENEMY_OBJECT_PAGE_LOC];
  uint8_t b1 = st->enemy_data[y];
  st->ram[SMBC_ENEMY_X_POSITION + x] = (uint8_t)(b1 & 0xf0u);
  uint16_t obj = ((uint16_t)st->ram[SMBC_ENEMY_PAGE_LOC + x] << 8) |
                 st->ram[SMBC_ENEMY_X_POSITION + x];
  uint16_t edge = ((uint16_t)st->ram[SMBC_SCREEN_RIGHT_PAGE_LOC] << 8) |
                  st->ram[SMBC_SCREEN_RIGHT_X_POS];
  if (obj < edge) { /* behind the boundary: skip it */
    if ((b1 & 0x0fu) == 0x0eu) parse_row_0e(st);
    else inc2b(st); /* CheckThreeBytes */
    return;
  }
  /* CheckRightExtBounds — beyond the extended boundary it waits */
  if (obj > (((uint16_t)ext_page << 8) | ext_col)) {
    check_frenzy_buffer(st, x);
    return;
  }
  st->ram[SMBC_ENEMY_Y_HIGH_POS + x] = 0x01u;
  uint8_t ypos = (uint8_t)(b1 << 4); /* low nybble as a vertical pixel */
  st->ram[SMBC_ENEMY_Y_POSITION + x] = ypos;
  if (ypos == 0xe0u) { /* row $0e reached through the spawn path */
    parse_row_0e(st);
    return;
  }
  uint8_t b2 = st->enemy_data[y + 1u];
  if ((b2 & 0x40u) != 0x00u && st->ram[SMBC_SECONDARY_HARD_MODE] == 0x00u) {
    inc2b(st); /* a hard mode object skipped before hard mode */
    return;
  }
  /* CheckForEnemyGroup */
  uint8_t id = (uint8_t)(b2 & 0x3fu);
  if (id >= 0x37u && id < 0x3fu) {
    handle_group_enemies(st, id); /* DoGroup */
    return;
  }
  /* BuzzyBeetleMutate — hard mode turns goombas into buzzy beetles */
  if (id == SMBC_GOOMBA && st->ram[SMBC_PRIMARY_HARD_MODE] != 0x00u) {
    id = SMBC_BUZZY_BEETLE;
  }
  st->ram[SMBC_ENEMY_ID + x] = id; /* StrID */
  st->ram[SMBC_ENEMY_FLAG + x] = 0x01u;
  init_enemy_object(st, x);
  if (st->ram[SMBC_ENEMY_FLAG + x] != 0x00u) inc2b(st);
}

/* smb.asm: ProcLoopCommand — handle a loop command on column zero,
 * then the frenzy queue, then the data stream */
static void proc_loop_command(CoreState *st, uint8_t x) {
  if (st->ram[SMBC_LOOP_COMMAND] != 0x00u &&
      st->ram[SMBC_CURRENT_COLUMN_POS] == 0x00u) {
    for (int y = 0x0a; y >= 0; y--) { /* FindLoop */
      if (st->ram[SMBC_WORLD_NUMBER] != SMBC_LOOP_CMD_WORLD_NUMBER[y] ||
          st->ram[SMBC_CURRENT_PAGE_LOC] != SMBC_LOOP_CMD_PAGE_NUMBER[y]) {
        continue;
      }
      bool correct =
          st->ram[SMBC_PLAYER_Y_POSITION] == SMBC_LOOP_CMD_Y_POSITION[y] &&
          st->ram[SMBC_PLAYER_STATE] == 0x00u;
      uint8_t world = st->ram[SMBC_WORLD_NUMBER];
      bool clear_counters = true;
      if (correct && world != SMBC_WORLD_7) {
        /* InitMLp */
      } else if (world == SMBC_WORLD_7) {
        if (correct) st->ram[SMBC_MULTI_LOOP_CORRECT_CNTR]++;
        st->ram[SMBC_MULTI_LOOP_PASS_CNTR]++; /* IncMLoop */
        if (st->ram[SMBC_MULTI_LOOP_PASS_CNTR] != 0x03u) {
          clear_counters = false; /* InitLCmd straight away */
        } else if (st->ram[SMBC_MULTI_LOOP_CORRECT_CNTR] != 0x03u) {
          exec_game_loopback(st, (uint8_t)y); /* DoLpBack */
          kill_all_enemies(st);
        }
      } else {
        exec_game_loopback(st, (uint8_t)y); /* WrongChk → DoLpBack */
        kill_all_enemies(st);
      }
      if (clear_counters) { /* InitMLp */
        st->ram[SMBC_MULTI_LOOP_PASS_CNTR] = 0x00u;
        st->ram[SMBC_MULTI_LOOP_CORRECT_CNTR] = 0x00u;
      }
      st->ram[SMBC_LOOP_COMMAND] = 0x00u; /* InitLCmd */
      break;
    }
  }
  /* ChkEnemyFrenzy */
  if (st->ram[SMBC_ENEMY_FRENZY_QUEUE] != 0x00u) {
    st->ram[SMBC_ENEMY_ID + x] = st->ram[SMBC_ENEMY_FRENZY_QUEUE];
    st->ram[SMBC_ENEMY_FLAG + x] = 0x01u;
    st->ram[SMBC_ENEMY_STATE + x] = 0x00u;
    st->ram[SMBC_ENEMY_FRENZY_QUEUE] = 0x00u;
    init_enemy_object(st, x);
    return;
  }
  process_enemy_data(st, x);
}

void enemies_and_loops_core(CoreState *st, uint8_t x) {
  uint8_t flag = st->ram[SMBC_ENEMY_FLAG + x];
  if ((flag & 0x80u) != 0x00u) { /* ChkBowserF */
    uint8_t y = (uint8_t)(flag & 0x0fu);
    if (st->ram[SMBC_ENEMY_FLAG + y] != 0x00u) return;
    st->ram[SMBC_ENEMY_FLAG + x] = 0x00u; /* clear the paired flag too */
    return;
  }
  if (flag != 0x00u) {
    /* RunEnemyObjectsCore — the per-frame enemy runners, pending their
     * own atom */
    return;
  }
  /* ChkAreaTsk — leave on the parser's seventh task */
  if ((st->ram[SMBC_AREA_PARSER_TASK_NUM] & 0x07u) == 0x07u) return;
  proc_loop_command(st, x);
}
