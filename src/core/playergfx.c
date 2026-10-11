/* core/playergfx.c — see core/playergfx.h for the contract. */
#include "playergfx.h"
#include "../data/sprites.h"

/* smb.asm: DumpTwoSpr — the same byte into a sprite and the one after */
static void dump_two_spr(CoreState *st, uint8_t y, uint8_t a) {
  st->ram[SMBC_SPRITE_DATA + 4u + y] = a;
  st->ram[SMBC_SPRITE_DATA + y] = a;
}

/* smb.asm: DrawOneSpriteRow → DrawSpriteObject — one row of two
 * side-by-side sprites; the flip bit swaps the tiles and sets the
 * horizontal-flip attribute. Returns the OAM offset and Y position
 * walked to the next row. */
static void draw_one_sprite_row(CoreState *st, uint8_t *y, uint8_t *ypos,
                                uint8_t tile1, uint8_t tile2, uint8_t flip,
                                uint8_t attr, uint8_t xpos) {
  uint8_t a;
  if (flip & 0x02u) { /* d1 after the two lsr: horizontal flip */
    st->ram[SMBC_SPRITE_TILENUMBER + 4u + *y] = tile1;
    st->ram[SMBC_SPRITE_TILENUMBER + *y] = tile2;
    a = 0x40u;
  } else {
    st->ram[SMBC_SPRITE_TILENUMBER + *y] = tile1;
    st->ram[SMBC_SPRITE_TILENUMBER + 4u + *y] = tile2;
    a = 0x00u;
  }
  a |= attr;
  st->ram[SMBC_SPRITE_ATTRIBUTES + *y] = a;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 4u + *y] = a;
  st->ram[SMBC_SPRITE_Y_POSITION + *y] = *ypos;
  st->ram[SMBC_SPRITE_Y_POSITION + 4u + *y] = *ypos;
  st->ram[SMBC_SPRITE_X_POSITION + *y] = xpos;
  st->ram[SMBC_SPRITE_X_POSITION + 4u + *y] = (uint8_t)(xpos + 0x08u);
  *ypos += 0x08u;
  *y += 0x08u;
}

/* smb.asm: DrawPlayerLoop — rows of two tiles from the graphics
 * table, top to bottom */
static void draw_player_loop(CoreState *st, uint8_t x, uint8_t y,
                             uint8_t ypos, uint8_t flip, uint8_t attr,
                             uint8_t xpos, uint8_t rows) {
  do {
    draw_one_sprite_row(st, &y, &ypos, SMBC_PLAYER_GRAPHICS_TABLE[x],
                        SMBC_PLAYER_GRAPHICS_TABLE[x + 1u], flip, attr,
                        xpos);
    x += 0x02u;
    rows--;
  } while (rows != 0x00u);
}

/* smb.asm: RenderPlayerSub — the player's relative coordinates,
 * facing and attributes into the row renderer; the relative X also
 * feeds Player_Pos_ForScroll */
static void render_player_sub(CoreState *st, uint8_t rows) {
  uint8_t xpos = st->ram[SMBC_PLAYER_REL_XPOS];
  st->ram[SMBC_PLAYER_POS_FORSCROLL] = xpos;
  draw_player_loop(st, st->ram[SMBC_PLAYER_GFX_OFFSET],
                   st->ram[SMBC_PLAYER_SPR_DATA_OFFSET],
                   st->ram[SMBC_PLAYER_REL_YPOS],
                   st->ram[SMBC_PLAYER_FACING_DIR],
                   st->ram[SMBC_PLAYER_SPR_ATTRIB], xpos, rows);
}

/* smb.asm: ChkForPlayerAttrib — the killed pose clears the flip bits
 * on rows three and four; the crouch, standing and grow poses on row
 * four only, the second sprite of each row horizontally flipped */
static void chk_for_player_attrib(CoreState *st) {
  uint8_t y = st->ram[SMBC_PLAYER_SPR_DATA_OFFSET];
  uint8_t o = st->ram[SMBC_PLAYER_GFX_OFFSET];
  bool killed = st->ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x0bu;
  if (!killed && o != 0x50u && o != 0xb8u && o != 0xc0u && o != 0xc8u) {
    return;
  }
  if (killed) {
    st->ram[SMBC_SPRITE_ATTRIBUTES + 16u + y] &= 0x3fu;
    st->ram[SMBC_SPRITE_ATTRIBUTES + 20u + y] =
        (st->ram[SMBC_SPRITE_ATTRIBUTES + 20u + y] & 0x3fu) | 0x40u;
  }
  st->ram[SMBC_SPRITE_ATTRIBUTES + 24u + y] &= 0x3fu;
  st->ram[SMBC_SPRITE_ATTRIBUTES + 28u + y] =
      (st->ram[SMBC_SPRITE_ATTRIBUTES + 28u + y] & 0x3fu) | 0x40u;
}

/* smb.asm: GetGfxOffsetAdder — the small player's poses sit eight
 * entries down the offsets table */
static uint8_t get_gfx_offset_adder(CoreState *st, uint8_t y) {
  if (st->ram[SMBC_PLAYER_SIZE] != 0x00u) y += 0x08u;
  return y;
}

/* smb.asm: GetOffsetFromAnimCtrl — frame control times eight plus
 * the pose's table offset */
static uint8_t get_offset_from_anim_ctrl(uint8_t ctrl, uint8_t y) {
  return (uint8_t)((ctrl << 3u) + SMBC_PLAYER_GFX_TBL_OFFSETS[y]);
}

/* smb.asm: GetCurrentAnimOffset */
static uint8_t get_current_anim_offset(CoreState *st, uint8_t y) {
  return get_offset_from_anim_ctrl(st->ram[SMBC_PLAYER_ANIM_CTRL], y);
}

/* smb.asm: AnimationControl — the current frame's offset, and when
 * the animation timer expires the frame control walks the extent */
static uint8_t animation_control(CoreState *st, uint8_t extent, uint8_t y) {
  uint8_t offset = get_current_anim_offset(st, y);
  if (st->ram[SMBC_PLAYER_ANIM_TIMER] == 0x00u) {
    st->ram[SMBC_PLAYER_ANIM_TIMER] = st->ram[SMBC_PLAYER_ANIM_TIMER_SET];
    uint8_t ctrl = (uint8_t)(st->ram[SMBC_PLAYER_ANIM_CTRL] + 0x01u);
    st->ram[SMBC_PLAYER_ANIM_CTRL] = (ctrl < extent) ? ctrl : 0x00u;
  }
  return offset;
}

/* smb.asm: NonAnimatedActs — static poses reset the frame control and
 * take the pose's base offset */
static uint8_t non_animated_acts(CoreState *st, uint8_t y) {
  y = get_gfx_offset_adder(st, y);
  st->ram[SMBC_PLAYER_ANIM_CTRL] = 0x00u;
  return SMBC_PLAYER_GFX_TBL_OFFSETS[y];
}

/* smb.asm: ProcessPlayerAction — the pose from the player's state:
 * climbing and falling animate their frames, jumping crouches or
 * uses the jump frame, on the ground the walk cycle runs while
 * moving, with a skid frame when the direction disagrees */
static uint8_t process_player_action(CoreState *st) {
  uint8_t state = st->ram[SMBC_PLAYER_STATE];
  if (state == 0x03u) { /* ActionClimbing */
    if (st->ram[SMBC_PLAYER_Y_SPEED] == 0x00u) {
      return non_animated_acts(st, 0x05u);
    }
    return animation_control(st, 0x02u, get_gfx_offset_adder(st, 0x05u));
  }
  if (state == 0x02u) { /* ActionFalling */
    return get_current_anim_offset(st, get_gfx_offset_adder(st, 0x04u));
  }
  if (state == 0x01u) { /* jumping or swimming */
    if (st->ram[SMBC_SWIMMING_FLAG] != 0x00u) { /* ActionSwimming */
      uint8_t y = get_gfx_offset_adder(st, 0x01u);
      if (st->ram[SMBC_JUMP_SWIM_TIMER] != 0x00u ||
          st->ram[SMBC_PLAYER_ANIM_CTRL] != 0x00u ||
          (st->ram[SMBC_A_B_BUTTONS] & 0x80u) != 0x00u) {
        return animation_control(st, 0x03u, y);
      }
      return get_current_anim_offset(st, y);
    }
    if (st->ram[SMBC_CROUCHING_FLAG] != 0x00u) {
      return non_animated_acts(st, 0x06u);
    }
    return non_animated_acts(st, 0x00u);
  }
  /* ProcOnGroundActs */
  if (st->ram[SMBC_CROUCHING_FLAG] != 0x00u) {
    return non_animated_acts(st, 0x06u);
  }
  if ((st->ram[SMBC_PLAYER_X_SPEED] |
       st->ram[SMBC_LEFT_RIGHT_BUTTONS]) == 0x00u) {
    return non_animated_acts(st, 0x02u); /* standing */
  }
  if (st->ram[SMBC_PLAYER_XSPEED_ABS] >= 0x09u &&
      (st->ram[SMBC_PLAYER_MOVING_DIR] & st->ram[SMBC_PLAYER_FACING_DIR]) ==
          0x00u) {
    return non_animated_acts(st, 0x03u); /* skidding */
  }
  /* ActionWalkRun */
  return animation_control(st, 0x03u, get_gfx_offset_adder(st, 0x04u));
}

/* smb.asm: HandleChangeSize — every fourth frame the grow/shrink
 * frame control advances; at ten the change is done. The grow pose
 * animates through the adder, the shrink borrows the swimming rows */
static uint8_t handle_change_size(CoreState *st) {
  uint8_t ctrl = st->ram[SMBC_PLAYER_ANIM_CTRL];
  if ((st->ram[SMBC_FRAME_COUNTER] & 0x03u) == 0x00u) {
    ctrl++;
    if (ctrl >= 0x0au) {
      ctrl = 0x00u;
      st->ram[SMBC_PLAYER_CHANGE_SIZE_FLAG] = 0x00u;
    }
    st->ram[SMBC_PLAYER_ANIM_CTRL] = ctrl;
  }
  if (st->ram[SMBC_PLAYER_SIZE] != 0x00u) { /* ShrinkPlayer */
    uint8_t adder = SMBC_CHANGE_SIZE_OFFSET_ADDER[ctrl + 0x0au];
    return SMBC_PLAYER_GFX_TBL_OFFSETS[adder != 0x00u ? 0x09u : 0x01u];
  }
  return get_offset_from_anim_ctrl(SMBC_CHANGE_SIZE_OFFSET_ADDER[ctrl],
                                   0x0fu);
}

/* smb.asm: PlayerOffscreenChk — the vertical offscreen bits park
 * whole rows of player sprites offscreen, bottom row first */
static void player_offscreen_chk(CoreState *st) {
  uint8_t bits = (uint8_t)(st->ram[SMBC_PLAYER_OFFSCREEN_BITS] >> 4u);
  uint8_t y = (uint8_t)(st->ram[SMBC_PLAYER_SPR_DATA_OFFSET] + 0x18u);
  for (uint8_t row = 0x00u; row < 0x04u; row++) {
    if (bits & 0x01u) dump_two_spr(st, y, 0xf8u);
    bits >>= 1u;
    y -= 0x08u;
  }
}

/* smb.asm: PlayerGfxProcessing — draw the pose, fix the flip bits,
 * then overlay the fireball-throw frame while its timer runs */
static void player_gfx_processing(CoreState *st, uint8_t gfx_offset) {
  st->ram[SMBC_PLAYER_GFX_OFFSET] = gfx_offset;
  render_player_sub(st, 0x04u);
  chk_for_player_attrib(st);
  uint8_t throw = st->ram[SMBC_FIREBALL_THROWING_TIMER];
  if (throw == 0x00u) {
    player_offscreen_chk(st);
    return;
  }
  uint8_t anim = st->ram[SMBC_PLAYER_ANIM_TIMER];
  st->ram[SMBC_FIREBALL_THROWING_TIMER] = 0x00u;
  if (anim >= throw) {
    player_offscreen_chk(st);
    return;
  }
  st->ram[SMBC_FIREBALL_THROWING_TIMER] = anim;
  st->ram[SMBC_PLAYER_GFX_OFFSET] = SMBC_PLAYER_GFX_TBL_OFFSETS[0x07u];
  uint8_t rows = 0x04u;
  if ((st->ram[SMBC_PLAYER_X_SPEED] |
       st->ram[SMBC_LEFT_RIGHT_BUTTONS]) != 0x00u) {
    rows = 0x03u;
  }
  render_player_sub(st, rows);
  player_offscreen_chk(st);
}

void player_gfx_handler(CoreState *st) {
  if (st->ram[SMBC_INJURY_TIMER] != 0x00u &&
      (st->ram[SMBC_FRAME_COUNTER] & 0x01u) != 0x00u) {
    return; /* injured: only drawn every other frame (the blink) */
  }
  if (st->ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x0bu) {
    player_gfx_processing(st, SMBC_PLAYER_GFX_TBL_OFFSETS[0x0eu]);
    return;
  }
  if (st->ram[SMBC_PLAYER_CHANGE_SIZE_FLAG] != 0x00u) {
    player_gfx_processing(st, handle_change_size(st));
    return;
  }
  player_gfx_processing(st, process_player_action(st));
  /* smb.asm: the swimming kick — with the swim flag set and a moving
   * player, sprite 7/8's tile is patched every eighth frame */
  if (st->ram[SMBC_SWIMMING_FLAG] == 0x00u ||
      st->ram[SMBC_PLAYER_STATE] == 0x00u) {
    return;
  }
  if ((st->ram[SMBC_FRAME_COUNTER] & 0x04u) != 0x00u) return;
  uint8_t y = st->ram[SMBC_PLAYER_SPR_DATA_OFFSET];
  if ((st->ram[SMBC_PLAYER_FACING_DIR] & 0x01u) == 0x00u) y += 0x04u;
  uint8_t kick = SMBC_SWIM_KICK_TILE_NUM[0];
  if (st->ram[SMBC_PLAYER_SIZE] != 0x00u) {
    if (st->ram[SMBC_SPRITE_TILENUMBER + 24u + y] ==
        SMBC_PLAYER_GRAPHICS_TABLE[0x9eu]) {
      return;
    }
    kick = SMBC_SWIM_KICK_TILE_NUM[1];
  }
  st->ram[SMBC_SPRITE_TILENUMBER + 24u + y] = kick;
}

void draw_player_intermediate(CoreState *st) {
  /* smb.asm: DrawPlayer_Intermediate — the small standing pose at a
   * fixed spot, from IntermediatePlayerData */
  draw_player_loop(st, 0xb8u, 0x04u, SMBC_INTERMEDIATE_PLAYER_DATA[0],
                   SMBC_INTERMEDIATE_PLAYER_DATA[1],
                   SMBC_INTERMEDIATE_PLAYER_DATA[2],
                   SMBC_INTERMEDIATE_PLAYER_DATA[3],
                   SMBC_INTERMEDIATE_PLAYER_DATA[5]);
  st->ram[SMBC_SPRITE_ATTRIBUTES + 32u] =
      st->ram[SMBC_SPRITE_ATTRIBUTES + 36u] | 0x40u;
}

void cycle_player_palette(CoreState *st, uint8_t a) {
  st->ram[SMBC_PLAYER_SPR_ATTRIB] =
      (st->ram[SMBC_PLAYER_SPR_ATTRIB] & 0xfcu) | (a & 0x03u);
}

void reset_pal_star(CoreState *st) {
  st->ram[SMBC_PLAYER_SPR_ATTRIB] &= 0xfcu;
}
