/* src/core/physics.c — smb.asm: PlayerMovementSubs, PlayerPhysicsSub,
 * MovePlayerHorizontally / MovePlayerVertically, PlayerBGCollision and
 * the head-bump block routines.
 *
 * Scratch usage carried over from the asm:
 *   $00 - side-check counter doubling as the impede direction flag;
 *         foot metatiles; friction data offset; block contents
 *   $01 - bottom left foot metatile
 *   $02 - block buffer vertical row offset (BB scratch)
 *   $04 - low nybble of the checked coordinate (BB scratch)
 *   $05 - the original head-bumped metatile
 *   $06/$07 - block buffer pointer (BB scratch)
 *   $eb - BlockBufferAdderData offset for the player's size */

#include "physics.h"

#include "../data/area.h"
#include "../data/gametext.h"
#include "blocks.h"
#include "collide.h"
#include "enemies.h"
#include "enemyinit.h"
#include "move.h"
#include "parser.h"
#include "screen.h"
#include "sound.h"

/* smb.asm: ClimbAdderLow / ClimbAdderHigh — horizontal step taken when
 * changing sides on a climbable, indexed by press direction and
 * facing */
static const uint8_t kClimbAdderLow[4] = {0x0eu, 0x04u, 0xfcu, 0xf2u};
static const uint8_t kClimbAdderHigh[4] = {0x00u, 0x00u, 0xffu, 0xffu};

/* smb.asm: the vertical physics tables, indexed 0-4 by horizontal
 * speed for jumps, 5-6 for swims (6 with a whirlpool) */
static const uint8_t kJumpMForceData[7] = {0x20u, 0x20u, 0x1eu, 0x28u,
                                           0x28u, 0x0du, 0x04u};
static const uint8_t kFallMForceData[7] = {0x70u, 0x70u, 0x60u, 0x90u,
                                           0x90u, 0x0au, 0x09u};
static const uint8_t kPlayerYSpdData[7] = {0xfcu, 0xfcu, 0xfcu, 0xfbu,
                                           0xfbu, 0xfeu, 0xffu};
static const uint8_t kInitMForceData[7] = {0x00u, 0x00u, 0x00u, 0x00u,
                                           0x00u, 0x80u, 0x00u};

/* smb.asm: MaxLeftXSpdData / MaxRightXSpdData / FrictionData — the
 * horizontal caps and friction picked by the X_Physics index; the
 * fourth right cap is for pipe intros */
static const uint8_t kMaxLeftXSpdData[3] = {0xd8u, 0xe8u, 0xf0u};
static const uint8_t kMaxRightXSpdData[4] = {0x28u, 0x18u, 0x10u, 0x0cu};
static const uint8_t kFrictionData[3] = {0xe4u, 0x98u, 0xd0u};

/* smb.asm: Climb_Y_SpeedData / Climb_Y_MForceData — indexed by the
 * up/down check: 0 nothing, 1 up, 2 down */
static const uint8_t kClimbYSpeedData[3] = {0x00u, 0xffu, 0x01u};
static const uint8_t kClimbYMForceData[3] = {0x00u, 0x20u, 0xffu};

/* smb.asm: PlayerAnimTmrData — animation timing by speed bracket */
static const uint8_t kPlayerAnimTmrData[3] = {0x02u, 0x04u, 0x07u};

/* smb.asm: PlayerBGUpperExtent — the highest vertical coordinate the
 * head check runs at, by size (big / small-or-crouching) */
static const uint8_t kPlayerBGUpperExtent[2] = {0x20u, 0x10u};

/* smb.asm: BlockBufferAdderData — the block buffer adder offset by
 * size bracket (big not swimming / small / crouching or swimming) */
static const uint8_t kBlockBufferAdderData[3] = {0x00u, 0x07u, 0x0eu};

/* smb.asm: BlockYPosAdderData — the bumped block's vertical snap
 * adder (big not crouching / small or crouching) */
static const uint8_t kBlockYPosAdderData[2] = {0x04u, 0x12u};

/* smb.asm: BrickQBlockMetatiles — question blocks, then the two
 * functionally identical brick sets */
static const uint8_t kBrickQBlockMetatiles[14] = {
    0xc1u, 0xc0u, 0x5fu, 0x60u, 0x55u, 0x56u, 0x57u,
    0x58u, 0x59u, 0x5au, 0x5bu, 0x5cu, 0x5du, 0x5eu};

/* smb.asm: SolidMTileUpperExt / ClimbMTileUpperExt — per-quadrant
 * upper extents for the solid and climbable checks */
static const uint8_t kSolidMTileUpperExt[4] = {0x10u, 0x61u, 0x88u, 0xc4u};
static const uint8_t kClimbMTileUpperExt[4] = {0x24u, 0x6du, 0x8au, 0xc6u};

/* smb.asm: AreaChangeTimerData — sideways pipe entry timer by left
 * page */
static const uint8_t kAreaChangeTimerData[2] = {0xa0u, 0x34u};

/* smb.asm: ClimbXPosAdder / ClimbPLocAdder — where the player is
 * placed on the vine, indexed facing - 1 */
static const uint8_t kClimbXPosAdder[2] = {0xf9u, 0x07u};
static const uint8_t kClimbPLocAdder[2] = {0xffu, 0x00u};

/* smb.asm: FlagpoleYPosData — the score brackets down the flagpole */
static const uint8_t kFlagpoleYPosData[5] = {0x18u, 0x22u, 0x50u, 0x68u,
                                             0x90u};

/* smb.asm: CoinTallyOffsets — the status bar coin tally digit offsets
 * by player */
static const uint8_t kCoinTallyOffsets[2] = {0x17u, 0x1du};

/* smb.asm: GetPlayerAnimSpeed — animation timing and the skid check;
 * the run flag doubles as RunningSpeed */
static void get_player_anim_speed(CoreState *st) {
  uint8_t y = 0x00u;
  uint8_t spd = st->ram[SMBC_PLAYER_XSPEED_ABSOLUTE];
  if (spd >= 0x1cu) {
    st->ram[SMBC_RUNNING_SPEED] = spd;
  } else {
    y = 0x01u;
    if (spd < 0x0eu) y = 0x02u;
    uint8_t a = st->ram[SMBC_SAVED_JOYPAD1_BITS] & 0x7fu;
    if (a != 0x00u) {
      if ((a & 0x03u) != st->ram[SMBC_PLAYER_MOVING_DIR]) {
        /* smb.asm: ProcSkid — below the skid threshold the moving
         * direction snaps to the facing direction and the speed is
         * nullified */
        if (spd < 0x0bu) {
          st->ram[SMBC_PLAYER_MOVING_DIR] = st->ram[SMBC_PLAYER_FACING_DIR];
          st->ram[SMBC_PLAYER_X_SPEED] = 0x00u;
          st->ram[SMBC_PLAYER_X_MOVE_FORCE] = 0x00u;
        }
      } else {
        st->ram[SMBC_RUNNING_SPEED] = 0x00u;
      }
    }
  }
  st->ram[SMBC_PLAYER_ANIM_TIMER_SET] = kPlayerAnimTmrData[y];
}

/* smb.asm: ImposeFriction — lr is the left/right controller bits the
 * caller had in A */
static void impose_friction(CoreState *st, uint8_t lr) {
  uint8_t collided = lr & st->ram[SMBC_PLAYER_COLLISION_BITS];
  int8_t spd = (int8_t)st->ram[SMBC_PLAYER_X_SPEED];
  if (collided != 0x00u) {
    /* smb.asm: JoypFrict — the right controller bit in carry; clear
     * carry means the left button was pressed */
    if ((collided & 0x01u) != 0x00u) goto left_frict;
    goto rght_frict;
  }
  if (spd == 0) goto set_abs_spd;
  if (spd > 0) goto rght_frict;
left_frict: {
  uint16_t sum = (uint16_t)st->ram[SMBC_PLAYER_X_MOVE_FORCE] +
                 st->ram[SMBC_FRICTION_ADDER_LOW];
  st->ram[SMBC_PLAYER_X_MOVE_FORCE] = (uint8_t)sum;
  int8_t ns = (int8_t)(st->ram[SMBC_PLAYER_X_SPEED] +
                       st->ram[SMBC_FRICTION_ADDER_HIGH] + (sum >> 8u));
  if (ns >= (int8_t)st->ram[SMBC_MAXIMUM_RIGHT_SPEED])
    ns = (int8_t)st->ram[SMBC_MAXIMUM_RIGHT_SPEED];
  st->ram[SMBC_PLAYER_X_SPEED] = (uint8_t)ns;
  spd = ns;
  goto x_spd_sign;
}
rght_frict: {
  uint16_t diff = (uint16_t)st->ram[SMBC_PLAYER_X_MOVE_FORCE] -
                  st->ram[SMBC_FRICTION_ADDER_LOW];
  st->ram[SMBC_PLAYER_X_MOVE_FORCE] = (uint8_t)diff;
  int8_t ns =
      (int8_t)(st->ram[SMBC_PLAYER_X_SPEED] -
               st->ram[SMBC_FRICTION_ADDER_HIGH] -
               ((diff & 0x0100u) != 0x00u ? 0x01u : 0x00u));
  if (ns < (int8_t)st->ram[SMBC_MAXIMUM_LEFT_SPEED])
    ns = (int8_t)st->ram[SMBC_MAXIMUM_LEFT_SPEED];
  st->ram[SMBC_PLAYER_X_SPEED] = (uint8_t)ns;
  spd = ns;
}
x_spd_sign:
  if (spd < 0) spd = (int8_t)-spd;
set_abs_spd:
  st->ram[SMBC_PLAYER_XSPEED_ABSOLUTE] = (uint8_t)spd;
}

/* smb.asm: MovePlayerHorizontally — the player's entry into
 * MoveObjectHorizontally; the returned value feeds X_Scroll */
static uint8_t move_player_horizontally(CoreState *st) {
  if (st->ram[SMBC_JUMPSPRING_ANIM_CTRL] != 0x00u)
    return st->ram[SMBC_JUMPSPRING_ANIM_CTRL];
  return move_object_horizontally(st, 0x00u);
}

/* smb.asm: MovePlayerVertically — the player's entry into
 * ImposeGravitySprObj */
static void move_player_vertically(CoreState *st) {
  if (st->ram[SMBC_TIMER_CONTROL] == 0x00u &&
      st->ram[SMBC_JUMPSPRING_ANIM_CTRL] != 0x00u)
    return;
  impose_gravity(st, 0x00u, st->ram[SMBC_VERTICAL_FORCE], 0x00u, 0x04u,
                 0x00u);
}

/* smb.asm: LRAir / JSMove / ExitMov1 — the shared tail of the three
 * airborne state subs */
static void lr_air(CoreState *st) {
  uint8_t lr = st->ram[SMBC_LEFT_RIGHT_BUTTONS];
  if (lr != 0x00u) impose_friction(st, lr);
  st->ram[SMBC_PLAYER_X_SCROLL] = move_player_horizontally(st);
  if (st->ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x0bu)
    st->ram[SMBC_VERTICAL_FORCE] = 0x28u;
  move_player_vertically(st);
}

/* smb.asm: OnGroundStateSub */
static void on_ground_state_sub(CoreState *st) {
  get_player_anim_speed(st);
  uint8_t lr = st->ram[SMBC_LEFT_RIGHT_BUTTONS];
  if (lr != 0x00u) st->ram[SMBC_PLAYER_FACING_DIR] = lr;
  impose_friction(st, lr);
  st->ram[SMBC_PLAYER_X_SCROLL] = move_player_horizontally(st);
}

/* smb.asm: FallingSub */
static void falling_sub(CoreState *st) {
  st->ram[SMBC_VERTICAL_FORCE] = st->ram[SMBC_VERTICAL_FORCE_DOWN];
  lr_air(st);
}

/* smb.asm: JumpSwimSub */
static void jump_swim_sub(CoreState *st) {
  if ((int8_t)st->ram[SMBC_PLAYER_Y_SPEED] >= 0) goto dump_fall;
  if ((st->ram[SMBC_A_B_BUTTONS] & SMBC_A_BUTTON &
       st->ram[SMBC_PREVIOUS_A_B_BUTTONS]) != 0x00u)
    goto proc_swim;
  if ((uint8_t)(st->ram[SMBC_JUMP_ORIGIN_Y_POSITION] -
                st->ram[SMBC_PLAYER_Y_POSITION]) <
      st->ram[SMBC_DIFF_TO_HALT_JUMP])
    goto proc_swim;
dump_fall:
  st->ram[SMBC_VERTICAL_FORCE] = st->ram[SMBC_VERTICAL_FORCE_DOWN];
proc_swim:
  if (st->ram[SMBC_SWIMMING_FLAG] == 0x00u) {
    lr_air(st);
    return;
  }
  get_player_anim_speed(st);
  if (st->ram[SMBC_PLAYER_Y_POSITION] < 0x14u)
    st->ram[SMBC_VERTICAL_FORCE] = 0x18u;
  /* smb.asm: LRWater */
  uint8_t lr = st->ram[SMBC_LEFT_RIGHT_BUTTONS];
  if (lr != 0x00u) st->ram[SMBC_PLAYER_FACING_DIR] = lr;
  lr_air(st);
}

/* smb.asm: ClimbingSub */
static void climbing_sub(CoreState *st) {
  uint16_t sum = (uint16_t)st->ram[SMBC_PLAYER_YMF_DUMMY] +
                 st->ram[SMBC_PLAYER_Y_MOVE_FORCE];
  st->ram[SMBC_PLAYER_YMF_DUMMY] = (uint8_t)sum;
  uint8_t adder = (int8_t)st->ram[SMBC_PLAYER_Y_SPEED] < 0 ? 0xffu : 0x00u;
  /* smb.asm: MoveOnVine — the sign adder parks in $00 */
  uint16_t pos = (uint16_t)st->ram[SMBC_PLAYER_Y_POSITION] +
                 st->ram[SMBC_PLAYER_Y_SPEED] + (sum >> 8u);
  st->ram[SMBC_PLAYER_Y_POSITION] = (uint8_t)pos;
  st->ram[SMBC_PLAYER_Y_HIGH_POS] =
      (uint8_t)(st->ram[SMBC_PLAYER_Y_HIGH_POS] + adder + (pos >> 8u));
  uint8_t pressed =
      st->ram[SMBC_LEFT_RIGHT_BUTTONS] & st->ram[SMBC_PLAYER_COLLISION_BITS];
  if (pressed == 0x00u) {
    /* smb.asm: InitCSTimer — A holds the and result here, which is
     * zero */
    st->ram[SMBC_CLIMB_SIDE_TIMER] = 0x00u;
    return;
  }
  if (st->ram[SMBC_CLIMB_SIDE_TIMER] != 0x00u) return;
  st->ram[SMBC_CLIMB_SIDE_TIMER] = 0x18u;
  /* the press direction in carry from lsr, the facing direction in Y */
  uint8_t x = (pressed & 0x01u) != 0x00u ? 0x00u : 0x02u;
  if (st->ram[SMBC_PLAYER_FACING_DIR] - 1u != 0x00u) x++;
  uint16_t xsum = (uint16_t)st->ram[SMBC_PLAYER_X_POSITION] +
                  kClimbAdderLow[x];
  st->ram[SMBC_PLAYER_X_POSITION] = (uint8_t)xsum;
  st->ram[SMBC_PLAYER_PAGE_LOC] =
      (uint8_t)(st->ram[SMBC_PLAYER_PAGE_LOC] + kClimbAdderHigh[x] +
                (xsum >> 8u));
  st->ram[SMBC_PLAYER_FACING_DIR] =
      st->ram[SMBC_LEFT_RIGHT_BUTTONS] ^ 0x03u;
}

/* smb.asm: PlayerPhysicsSub */
void player_physics_sub(CoreState *st) {
  if (st->ram[SMBC_PLAYER_STATE] == 0x03u) {
    /* climbing on a climbable tile: 0 nothing, 1 up, 2 down */
    uint8_t y = 0x00u;
    uint8_t a = st->ram[SMBC_UP_DOWN_BUTTONS] &
                st->ram[SMBC_PLAYER_COLLISION_BITS];
    if (a != 0x00u) {
      y = 0x01u;
      if ((a & 0x08u) == 0x00u) y = 0x02u;
    }
    st->ram[SMBC_PLAYER_Y_MOVE_FORCE] = kClimbYMForceData[y];
    uint8_t anim = 0x08u;
    st->ram[SMBC_PLAYER_Y_SPEED] = kClimbYSpeedData[y];
    /* smb.asm: bpl skips the halving, so a negative speed animates
     * faster */
    if ((int8_t)kClimbYSpeedData[y] < 0) anim >>= 1u;
    st->ram[SMBC_PLAYER_ANIM_TIMER_SET] = anim;
    return;
  }
  /* smb.asm: CheckForJumping */
  if (st->ram[SMBC_JUMPSPRING_ANIM_CTRL] == 0x00u) {
    uint8_t a = st->ram[SMBC_A_B_BUTTONS] & SMBC_A_BUTTON;
    if (a != 0x00u && (a & st->ram[SMBC_PREVIOUS_A_B_BUTTONS]) == 0x00u) {
      /* smb.asm: ProcJumping */
      if (st->ram[SMBC_PLAYER_STATE] == 0x00u) goto init_js;
      if (st->ram[SMBC_SWIMMING_FLAG] != 0x00u) {
        if (st->ram[SMBC_JUMP_SWIM_TIMER] != 0x00u) goto init_js;
        if ((int8_t)st->ram[SMBC_PLAYER_Y_SPEED] < 0) goto init_js;
      }
    }
  }
  goto x_physics;
init_js:
  st->ram[SMBC_JUMP_SWIM_TIMER] = 0x20u;
  st->ram[SMBC_PLAYER_YMF_DUMMY] = 0x00u;
  st->ram[SMBC_PLAYER_Y_MOVE_FORCE] = 0x00u;
  st->ram[SMBC_JUMP_ORIGIN_Y_HIGHPOS] = st->ram[SMBC_PLAYER_Y_HIGH_POS];
  st->ram[SMBC_JUMP_ORIGIN_Y_POSITION] = st->ram[SMBC_PLAYER_Y_POSITION];
  st->ram[SMBC_PLAYER_STATE] = 0x01u;
  uint8_t y = 0x00u;
  uint8_t spd = st->ram[SMBC_PLAYER_XSPEED_ABSOLUTE];
  if (spd >= 0x09u) {
    y = 0x01u;
    if (spd >= 0x10u) {
      y = 0x02u;
      if (spd >= 0x19u) {
        y = 0x03u;
        if (spd >= 0x1cu) y = 0x04u;
      }
    }
  }
  st->ram[SMBC_DIFF_TO_HALT_JUMP] = 0x01u;
  if (st->ram[SMBC_SWIMMING_FLAG] != 0x00u) {
    y = 0x05u;
    if (st->ram[SMBC_WHIRLPOOL_FLAG] != 0x00u) y = 0x06u;
  }
  st->ram[SMBC_VERTICAL_FORCE] = kJumpMForceData[y];
  st->ram[SMBC_VERTICAL_FORCE_DOWN] = kFallMForceData[y];
  st->ram[SMBC_PLAYER_Y_MOVE_FORCE] = kInitMForceData[y];
  st->ram[SMBC_PLAYER_Y_SPEED] = kPlayerYSpdData[y];
  if (st->ram[SMBC_SWIMMING_FLAG] != 0x00u) {
    st->ram[SMBC_SQUARE1_SOUND_QUEUE] = SMBC_SFX_ENEMY_STOMP;
    if (st->ram[SMBC_PLAYER_Y_POSITION] < 0x14u)
      st->ram[SMBC_PLAYER_Y_SPEED] = 0x00u;
  } else {
    /* smb.asm: PJumpSnd — big by default, small when PlayerSize set */
    st->ram[SMBC_SQUARE1_SOUND_QUEUE] =
        st->ram[SMBC_PLAYER_SIZE] != 0x00u ? SMBC_SFX_SMALL_JUMP
                                           : SMBC_SFX_BIG_JUMP;
  }
x_physics: {
  /* smb.asm: X_Physics — $00 is the friction data offset */
  uint8_t yi = 0x00u, z = 0x00u;
  if (st->ram[SMBC_PLAYER_STATE] == 0x00u) {
    yi = 0x01u;
    if (st->ram[SMBC_AREA_TYPE] != 0x00u) {
      yi = 0x00u;
      if (st->ram[SMBC_LEFT_RIGHT_BUTTONS] ==
          st->ram[SMBC_PLAYER_MOVING_DIR]) {
        if ((st->ram[SMBC_A_B_BUTTONS] & SMBC_B_BUTTON) != 0x00u) {
          st->ram[SMBC_RUNNING_TIMER] = 0x0au;
          goto get_x_phy;
        }
        if (st->ram[SMBC_RUNNING_TIMER] != 0x00u) goto get_x_phy;
      }
    }
  } else if (st->ram[SMBC_PLAYER_XSPEED_ABSOLUTE] >= 0x19u) {
    goto get_x_phy;
  }
  /* smb.asm: ChkRFast */
  yi++;
  z++;
  if (st->ram[SMBC_RUNNING_SPEED] != 0x00u ||
      st->ram[SMBC_PLAYER_XSPEED_ABSOLUTE] >= 0x21u)
    z++;
get_x_phy:
  st->ram[SMBC_MAXIMUM_LEFT_SPEED] = kMaxLeftXSpdData[yi];
  if (st->ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x07u) yi = 0x03u;
  st->ram[SMBC_MAXIMUM_RIGHT_SPEED] = kMaxRightXSpdData[yi];
  st->ram[SMBC_FRICTION_ADDER_LOW] = kFrictionData[z];
  st->ram[SMBC_FRICTION_ADDER_HIGH] = 0x00u;
  if (st->ram[SMBC_PLAYER_FACING_DIR] != st->ram[SMBC_PLAYER_MOVING_DIR]) {
    uint8_t c = st->ram[SMBC_FRICTION_ADDER_LOW] >> 7u;
    st->ram[SMBC_FRICTION_ADDER_LOW] <<= 1u;
    st->ram[SMBC_FRICTION_ADDER_HIGH] = c;
  }
}
}

/* smb.asm: PlayerMovementSubs */
void player_movement_subs(CoreState *st) {
  if (st->ram[SMBC_PLAYER_SIZE] != 0x00u) {
    st->ram[SMBC_CROUCHING_FLAG] = 0x00u;
  } else if (st->ram[SMBC_PLAYER_STATE] == 0x00u) {
    st->ram[SMBC_CROUCHING_FLAG] = st->ram[SMBC_UP_DOWN_BUTTONS] & 0x04u;
  }
  /* a big player in the air skips the store entirely */
  player_physics_sub(st);
  if (st->ram[SMBC_PLAYER_CHANGE_SIZE_FLAG] != 0x00u) return;
  if (st->ram[SMBC_PLAYER_STATE] != 0x03u)
    st->ram[SMBC_CLIMB_SIDE_TIMER] = 0x18u;
  switch (st->ram[SMBC_PLAYER_STATE]) {
    case 0x00u: on_ground_state_sub(st); break;
    case 0x01u: jump_swim_sub(st); break;
    case 0x02u: falling_sub(st); break;
    default: climbing_sub(st); break;
  }
}

/* smb.asm: GetMTileAttrib — the metatile quadrant (2 MSB) as an
 * index; the metatile itself stays in A */
static uint8_t get_m_tile_attrib(uint8_t mt) {
  return (uint8_t)((mt >> 6u) & 0x03u);
}

/* smb.asm: CheckForSolidMTiles */
static int check_for_solid_m_tiles(uint8_t mt) {
  return mt >= kSolidMTileUpperExt[get_m_tile_attrib(mt)];
}

/* smb.asm: CheckForClimbMTiles */
static int check_for_climb_m_tiles(uint8_t mt) {
  return mt >= kClimbMTileUpperExt[get_m_tile_attrib(mt)];
}

/* smb.asm: CheckForCoinMTiles — the coin grab sound queues here too */
static int check_for_coin_m_tiles(CoreState *st, uint8_t mt) {
  if (mt != 0xc2u && mt != 0xc3u) return 0;
  st->ram[SMBC_SQUARE2_SOUND_QUEUE] = SMBC_SFX_COIN_GRAB;
  return 1;
}

/* smb.asm: ChkJumpspringMetatiles */
static int chk_jumpspring_metatiles(uint8_t mt) {
  return mt == 0x67u || mt == 0x68u;
}

/* smb.asm: ChkInvisibleMTiles — zero flag set on the hidden blocks */
static int chk_invisible_m_tiles(uint8_t mt) {
  return mt == 0x5fu || mt == 0x60u;
}

/* smb.asm: ErACM — blank the coin or axe in the block buffer, then
 * erase it from the screen */
static void erase_coin_axe(CoreState *st) {
  uint16_t ptr =
      (uint16_t)(st->ram[SMBC_BB_PTR_LO] | (st->ram[SMBC_BB_PTR_HI] << 8u));
  st->ram[ptr + st->ram[SMBC_BB_Y_COORD]] = 0x00u;
  remove_coin_axe(st);
}

/* smb.asm: GiveOneCoin — one coin on the tally, one hundred coins
 * earns a life; then the 200 points */
static void give_one_coin(CoreState *st) {
  st->ram[SMBC_DIGIT_MODIFIER + 5u] = 0x01u;
  digits_math_routine(st, kCoinTallyOffsets[st->ram[SMBC_CURRENT_PLAYER]]);
  st->ram[SMBC_COIN_TALLY]++;
  if (st->ram[SMBC_COIN_TALLY] == 100u) {
    st->ram[SMBC_COIN_TALLY] = 0x00u;
    st->ram[SMBC_NUMBEROF_LIVES]++;
    st->ram[SMBC_SQUARE2_SOUND_QUEUE] = SMBC_SFX_EXTRA_LIFE;
  }
  /* smb.asm: CoinPoints */
  st->ram[SMBC_DIGIT_MODIFIER + 4u] = 0x02u;
  add_to_score(st);
}

/* smb.asm: HandleCoinMetatile */
static void handle_coin_metatile(CoreState *st) {
  erase_coin_axe(st);
  st->ram[SMBC_COIN_TALLY_FOR_1UPS]++;
  give_one_coin(st);
}

/* smb.asm: HandleAxeMetatile — mode change into the auto-control
 * mode, then erase like a coin */
static void handle_axe_metatile(CoreState *st) {
  st->ram[SMBC_OPER_MODE_TASK] = 0x00u;
  st->ram[SMBC_OPER_MODE] = 0x02u;
  st->ram[SMBC_PLAYER_X_SPEED] = 0x18u;
  erase_coin_axe(st);
}

/* smb.asm: ChkForLandJumpSpring */
static void chk_for_land_jump_spring(CoreState *st, uint8_t mt) {
  if (!chk_jumpspring_metatiles(mt)) return;
  st->ram[SMBC_VERTICAL_FORCE] = 0x70u;
  st->ram[SMBC_JUMPSPRING_FORCE] = 0xf9u;
  st->ram[SMBC_JUMPSPRING_TIMER] = 0x03u;
  st->ram[SMBC_JUMPSPRING_ANIM_CTRL] = 0x01u;
}

/* smb.asm: HandlePipeEntry — right_mt is $00 (bottom right foot
 * metatile), left_mt is $01 (bottom left) */
void handle_pipe_entry(CoreState *st, uint8_t right_mt, uint8_t left_mt) {
  if ((st->ram[SMBC_UP_DOWN_BUTTONS] & 0x04u) == 0x00u) return;
  if (right_mt != 0x11u || left_mt != 0x10u) return;
  st->ram[SMBC_CHANGE_AREA_TIMER] = 0x30u;
  st->ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x03u;
  st->ram[SMBC_SQUARE1_SOUND_QUEUE] = SMBC_SFX_PIPE_DOWN_INJURY;
  st->ram[SMBC_PLAYER_SPR_ATTRIB] = 0x20u;
  uint8_t wzc = st->ram[SMBC_WARP_ZONE_CONTROL];
  if (wzc == 0x00u) return;
  uint8_t x = (uint8_t)((wzc & 0x03u) << 2u);
  uint8_t xpos = st->ram[SMBC_PLAYER_X_POSITION];
  if (xpos >= 0x60u) {
    x++;
    if (xpos >= 0xa0u) x++;
  }
  /* smb.asm: GetWNum */
  uint8_t w = (uint8_t)(SMBC_WARP_ZONE_NUMBERS[x] - 1u);
  st->ram[SMBC_WORLD_NUMBER] = w;
  uint8_t offs = SMBC_WORLD_ADDR_OFFSETS[w];
  st->ram[SMBC_AREA_POINTER] = SMBC_AREA_ADDR_OFFSETS[offs];
  st->ram[SMBC_EVENT_MUSIC_QUEUE] = SMBC_MUSIC_SILENCE;
  st->ram[SMBC_ENTRANCE_PAGE] = 0x00u;
  st->ram[SMBC_AREA_NUMBER] = 0x00u;
  st->ram[SMBC_LEVEL_NUMBER] = 0x00u;
  st->ram[SMBC_ALT_ENTRANCE_CONTROL] = 0x00u;
  st->ram[SMBC_HIDDEN_1UP_FLAG]++;
  st->ram[SMBC_FETCH_NEW_GAME_TIMER_FLAG]++;
}

/* smb.asm: ImpedePlayerMove — $00 (the side argument) is 1 for a
 * left side collision, 2 for a right side one */
static void impede_player_move(CoreState *st, uint8_t side) {
  uint8_t x = side;
  int8_t spd = (int8_t)st->ram[SMBC_PLAYER_X_SPEED];
  uint8_t a = 0x00u;
  x--;
  if (x != 0x00u) {
    /* smb.asm: RImpd — right side collision */
    x = 0x02u;
    if (spd >= 0x01) goto ex_ipm;
    a = 0x01u;
  } else {
    x = 0x01u;
    if (spd < 0x00) goto ex_ipm;
    a = 0xffu;
  }
  /* smb.asm: NXSpd */
  st->ram[SMBC_SIDE_COLLISION_TIMER] = 0x10u;
  st->ram[SMBC_PLAYER_X_SPEED] = 0x00u;
  uint8_t hi = (int8_t)a < 0 ? 0xffu : 0x00u;
  /* smb.asm: PlatF — the high bits park in $00 */
  uint16_t pos = (uint16_t)a + st->ram[SMBC_PLAYER_X_POSITION];
  st->ram[SMBC_PLAYER_X_POSITION] = (uint8_t)pos;
  st->ram[SMBC_PLAYER_PAGE_LOC] =
      (uint8_t)(st->ram[SMBC_PLAYER_PAGE_LOC] + hi + (pos >> 8u));
ex_ipm:
  st->ram[SMBC_PLAYER_COLLISION_BITS] =
      (uint8_t)((x ^ 0xffu) & st->ram[SMBC_PLAYER_COLLISION_BITS]);
}

/* smb.asm: StopPlayerMove — impede with the current side counter */
static void stop_player_move(CoreState *st, uint8_t side) {
  impede_player_move(st, side);
}

/* smb.asm: HandleClimbing — mt is the climbable metatile */
static void handle_climbing(CoreState *st, uint8_t mt) {
  uint8_t nyb = st->ram[SMBC_BB_LOW_NYBBLE];
  if (nyb < 0x06u || nyb >= 0x0au) return;
  /* smb.asm: ChkForFlagpole */
  if (mt == 0x24u || mt == 0x25u) {
    /* smb.asm: FlagpoleCollision */
    if (st->ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x05u)
      goto put_player_on_vine;
    st->ram[SMBC_PLAYER_FACING_DIR] = 0x01u;
    st->ram[SMBC_SCROLL_LOCK]++;
    if (st->ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x04u) goto run_fr;
    kill_enemies(st, SMBC_BULLET_BILL_CANNON);
    st->ram[SMBC_EVENT_MUSIC_QUEUE] = SMBC_MUSIC_SILENCE;
    st->ram[SMBC_FLAGPOLE_SOUND_QUEUE] = SMBC_MUSIC_SILENCE >> 1u;
    uint8_t x = 0x04u;
    st->ram[SMBC_FLAGPOLE_COLLISION_Y_POS] =
        st->ram[SMBC_PLAYER_Y_POSITION];
    /* smb.asm: ChkFlagpoleYPosLoop */
    while (st->ram[SMBC_PLAYER_Y_POSITION] < kFlagpoleYPosData[x]) {
      x--;
      if (x == 0x00u) break;
    }
    st->ram[SMBC_FLAGPOLE_SCORE] = x;
  run_fr:
    st->ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x04u;
    goto put_player_on_vine;
  }
  /* smb.asm: VineCollision */
  if (mt == 0x26u && st->ram[SMBC_PLAYER_Y_POSITION] < 0x20u)
    st->ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x01u;
put_player_on_vine:
  st->ram[SMBC_PLAYER_STATE] = 0x03u;
  st->ram[SMBC_PLAYER_X_SPEED] = 0x00u;
  st->ram[SMBC_PLAYER_X_MOVE_FORCE] = 0x00u;
  if ((uint8_t)(st->ram[SMBC_PLAYER_X_POSITION] -
                st->ram[SMBC_SCREEN_LEFT_X_POS]) < 0x10u)
    st->ram[SMBC_PLAYER_FACING_DIR] = 0x02u;
  uint8_t y = st->ram[SMBC_PLAYER_FACING_DIR];
  st->ram[SMBC_PLAYER_X_POSITION] = (uint8_t)(
      (uint8_t)(st->ram[SMBC_BB_PTR_LO] << 4u) + kClimbXPosAdder[y - 1u]);
  if (st->ram[SMBC_BB_PTR_LO] == 0x00u)
    st->ram[SMBC_PLAYER_PAGE_LOC] =
        (uint8_t)(st->ram[SMBC_SCREEN_RIGHT_PAGE_LOC] +
                  kClimbPLocAdder[y - 1u]);
}

/* smb.asm: CheckSideMTiles — returns nonzero when the caller should
 * leave PlayerBGCollision, which is every path; side is the $00
 * counter for StopPlayerMove */
static void check_side_m_tiles(CoreState *st, uint8_t mt, uint8_t side) {
  if (chk_invisible_m_tiles(mt)) return;
  if (check_for_climb_m_tiles(mt)) {
    handle_climbing(st, mt);
    return;
  }
  if (check_for_coin_m_tiles(st, mt)) {
    handle_coin_metatile(st);
    return;
  }
  if (chk_jumpspring_metatiles(mt)) {
    if (st->ram[SMBC_JUMPSPRING_ANIM_CTRL] != 0x00u) return;
    stop_player_move(st, side);
    return;
  }
  /* smb.asm: ChkPBtm — only a grounded player facing right can enter
   * a sideways pipe */
  if (st->ram[SMBC_PLAYER_STATE] != 0x00u) {
    stop_player_move(st, side);
    return;
  }
  if (st->ram[SMBC_PLAYER_FACING_DIR] - 1u != 0x00u) {
    stop_player_move(st, side);
    return;
  }
  if (mt != 0x6cu && mt != 0x1fu) {
    stop_player_move(st, side);
    return;
  }
  /* smb.asm: PipeDwnS */
  if (st->ram[SMBC_PLAYER_SPR_ATTRIB] == 0x00u)
    st->ram[SMBC_SQUARE1_SOUND_QUEUE] = SMBC_SFX_PIPE_DOWN_INJURY;
  st->ram[SMBC_PLAYER_SPR_ATTRIB] |= 0x20u;
  if ((st->ram[SMBC_PLAYER_X_POSITION] & 0x0fu) != 0x00u) {
    uint8_t y = st->ram[SMBC_SCREEN_LEFT_PAGE_LOC] != 0x00u ? 0x01u : 0x00u;
    st->ram[SMBC_CHANGE_AREA_TIMER] = kAreaChangeTimerData[y];
  }
  /* smb.asm: ChkGERtn — sideways pipe entry only from routine 8;
   * 7 (the player entrance) and everything else leave */
  if (st->ram[SMBC_GAME_ENGINE_SUBROUTINE] == 0x08u)
    st->ram[SMBC_GAME_ENGINE_SUBROUTINE] = 0x02u;
}

/* smb.asm: BlockBumpedChk — the matched index or 0xff when the
 * metatile is not bumpable */
static uint8_t block_bumped_chk(uint8_t mt) {
  for (int8_t y = 0x0d; y >= 0; y--) {
    if (mt == kBrickQBlockMetatiles[(uint8_t)y]) return (uint8_t)y;
  }
  return 0xffu;
}

/* smb.asm: FindEmptyMiscSlot — the first free slot of 8, 7, 6,
 * falling back to 8 */
static uint8_t find_empty_misc_slot(CoreState *st) {
  uint8_t y = 0x08u;
  while (st->ram[SMBC_MISC_STATE + y] != 0x00u) {
    y--;
    if (y == 0x05u) {
      y = 0x08u;
      break;
    }
  }
  st->ram[SMBC_JUMP_COIN_MISC_OFFSET] = y;
  return y;
}

/* smb.asm: GiveOneCoin's helpers split out — SetupJumpCoin is also
 * the jumping-coin part of CheckTopOfBlock; x is the block slot */
static void setup_jump_coin_tail(CoreState *st, uint8_t x, uint8_t y) {
  /* smb.asm: JCoinC */
  st->ram[SMBC_MISC_Y_SPEED + y] = 0xfbu;
  st->ram[SMBC_MISC_Y_HIGH_POS + y] = 0x01u;
  st->ram[SMBC_MISC_STATE + y] = 0x01u;
  st->ram[SMBC_SQUARE2_SOUND_QUEUE] = 0x01u;
  st->ram[SMBC_OBJECT_OFFSET] = x;
  give_one_coin(st);
  st->ram[SMBC_COIN_TALLY_FOR_1UPS]++;
}

/* smb.asm: SetupJumpCoin — the jumping coin off a bumped block, from
 * the block buffer scratch rather than the block object */
static void setup_jump_coin(CoreState *st, uint8_t x) {
  uint8_t y = find_empty_misc_slot(st);
  st->ram[SMBC_MISC_PAGE_LOC + y] = st->ram[SMBC_BLOCK_PAGE_LOC2 + x];
  st->ram[SMBC_MISC_X_POSITION + y] =
      (uint8_t)((st->ram[SMBC_BB_PTR_LO] << 4u) | 0x05u);
  st->ram[SMBC_MISC_Y_POSITION + y] =
      (uint8_t)(st->ram[SMBC_BB_Y_COORD] + 0x20u);
  setup_jump_coin_tail(st, x, y);
}

/* smb.asm: CoinBlock — the jumping coin off a coin block, from the
 * block object's coordinates */
static void coin_block(CoreState *st, uint8_t x) {
  uint8_t y = find_empty_misc_slot(st);
  st->ram[SMBC_MISC_PAGE_LOC + y] = st->ram[SMBC_BLOCK_PAGE_LOC + x];
  st->ram[SMBC_MISC_X_POSITION + y] =
      st->ram[SMBC_BLOCK_X_POSITION + x] | 0x05u;
  st->ram[SMBC_MISC_Y_POSITION + y] =
      (uint8_t)(st->ram[SMBC_BLOCK_Y_POSITION + x] - 0x10u);
  setup_jump_coin_tail(st, x, y);
}

/* smb.asm: SetupPowerUp — the power-up rises out of the bumped block
 * in the enemy buffer's special slot */
static void setup_power_up(CoreState *st, uint8_t x) {
  st->ram[SMBC_ENEMY_ID + 5u] = SMBC_POWER_UP_OBJECT;
  st->ram[SMBC_ENEMY_PAGE_LOC + 5u] = st->ram[SMBC_BLOCK_PAGE_LOC + x];
  st->ram[SMBC_ENEMY_X_POSITION + 5u] = st->ram[SMBC_BLOCK_X_POSITION + x];
  st->ram[SMBC_ENEMY_Y_HIGH_POS + 5u] = 0x01u;
  st->ram[SMBC_ENEMY_Y_POSITION + 5u] =
      (uint8_t)(st->ram[SMBC_BLOCK_Y_POSITION + x] - 0x08u);
  /* smb.asm: PwrUpJmp */
  st->ram[SMBC_ENEMY_STATE + 5u] = 0x01u;
  st->ram[SMBC_ENEMY_FLAG + 5u] = 0x01u;
  st->ram[SMBC_ENEMY_BOUND_BOX_CTRL + 5u] = 0x03u;
  if (st->ram[SMBC_POWER_UP_TYPE] < 0x02u) {
    uint8_t status = st->ram[SMBC_PLAYER_STATUS];
    st->ram[SMBC_POWER_UP_TYPE] =
        status < 0x02u ? status : (uint8_t)(status >> 1u);
  }
  /* smb.asm: PutBehind */
  st->ram[SMBC_ENEMY_SPR_ATTRIB + 5u] = 0x20u;
  st->ram[SMBC_SQUARE2_SOUND_QUEUE] = SMBC_SFX_GROW_POWER_UP;
}

/* smb.asm: CheckTopOfBlock — a coin sitting on the bumped block is
 * collected as a jumping coin; uses and updates the $02/$06 scratch */
static void check_top_of_block(CoreState *st) {
  uint8_t y = st->ram[SMBC_BB_Y_COORD];
  if (y == 0x00u) return;
  y = (uint8_t)(y - 0x10u);
  st->ram[SMBC_BB_Y_COORD] = y;
  uint16_t ptr =
      (uint16_t)(st->ram[SMBC_BB_PTR_LO] | (st->ram[SMBC_BB_PTR_HI] << 8u));
  if (st->ram[ptr + y] != 0xc2u) return;
  st->ram[ptr + y] = 0x00u;
  remove_coin_axe(st);
  setup_jump_coin(st, st->ram[SMBC_SPDATA_OFFSET_CTRL]);
}

/* smb.asm: SpawnBrickChunks — chunk B lives two bytes on in the
 * block arrays */
static void spawn_brick_chunks(CoreState *st, uint8_t x) {
  st->ram[SMBC_BLOCK_ORIG_X_POS + x] = st->ram[SMBC_BLOCK_X_POSITION + x];
  st->ram[SMBC_BLOCK_X_SPEED + x] = 0xf0u;
  st->ram[SMBC_BLOCK_X_SPEED + 2u + x] = 0xf0u;
  st->ram[SMBC_BLOCK_Y_SPEED + x] = 0xfau;
  st->ram[SMBC_BLOCK_Y_SPEED + 2u + x] = 0xfcu;
  st->ram[SMBC_BLOCK_Y_MOVE_FORCE + x] = 0x00u;
  st->ram[SMBC_BLOCK_Y_MOVE_FORCE + 2u + x] = 0x00u;
  st->ram[SMBC_BLOCK_PAGE_LOC + 2u + x] = st->ram[SMBC_BLOCK_PAGE_LOC + x];
  st->ram[SMBC_BLOCK_X_POSITION + 2u + x] = st->ram[SMBC_BLOCK_X_POSITION + x];
  st->ram[SMBC_BLOCK_Y_POSITION + 2u + x] =
      (uint8_t)(st->ram[SMBC_BLOCK_Y_POSITION + x] + 0x08u);
  st->ram[SMBC_BLOCK_Y_SPEED + x] = 0xfau;
}

/* smb.asm: BrickShatter */
static void brick_shatter(CoreState *st, uint8_t x) {
  check_top_of_block(st);
  st->ram[SMBC_BLOCK_REP_FLAG + x] = SMBC_SFX_BRICK_SHATTER;
  st->ram[SMBC_NOISE_SOUND_QUEUE] = SMBC_SFX_BRICK_SHATTER;
  spawn_brick_chunks(st, x);
  st->ram[SMBC_PLAYER_Y_SPEED] = 0xfeu;
  st->ram[SMBC_DIGIT_MODIFIER + 5u] = 0x05u;
  add_to_score(st);
}

/* smb.asm: BumpBlock */
static void bump_block(CoreState *st, uint8_t x, uint8_t mt) {
  check_top_of_block(st);
  st->ram[SMBC_SQUARE1_SOUND_QUEUE] = SMBC_SFX_BUMP;
  st->ram[SMBC_BLOCK_X_SPEED + x] = 0x00u;
  st->ram[SMBC_BLOCK_Y_MOVE_FORCE + x] = 0x00u;
  st->ram[SMBC_PLAYER_Y_SPEED] = 0x00u;
  st->ram[SMBC_BLOCK_Y_SPEED + x] = 0xfeu;
  uint8_t y = block_bumped_chk(mt);
  if (y == 0xffu) return;
  if (y >= 0x09u) y = (uint8_t)(y - 0x05u);
  switch (y) {
    case 0x00u:
    case 0x04u:
      /* smb.asm: MushFlowerBlock */
      st->ram[SMBC_POWER_UP_TYPE] = 0x00u;
      setup_power_up(st, x);
      break;
    case 0x01u:
    case 0x02u:
    case 0x07u:
      coin_block(st, x);
      break;
    case 0x03u:
    case 0x08u:
      /* smb.asm: ExtraLifeMushBlock */
      st->ram[SMBC_POWER_UP_TYPE] = 0x03u;
      setup_power_up(st, x);
      break;
    case 0x05u:
      /* smb.asm: VineBlock */
      setup_vine(st, 0x05u, st->ram[SMBC_SPDATA_OFFSET_CTRL]);
      break;
    default:
      /* smb.asm: StarBlock */
      st->ram[SMBC_POWER_UP_TYPE] = 0x02u;
      setup_power_up(st, x);
      break;
  }
}

/* smb.asm: PlayerHeadCollision — mt is the metatile the head check
 * returned; the block buffer scratch ($02/$06/$07) is still live */
static void player_head_collision(CoreState *st, uint8_t mt) {
  uint8_t orig_mt = mt; /* pushed in the asm, parked in $05 later */
  uint8_t x = st->ram[SMBC_SPDATA_OFFSET_CTRL];
  st->ram[SMBC_BLOCK_STATE + x] =
      st->ram[SMBC_PLAYER_SIZE] != 0x00u ? 0x11u : 0x12u;
  destroy_block_metatile(st);
  x = st->ram[SMBC_SPDATA_OFFSET_CTRL];
  st->ram[SMBC_BLOCK_ORIG_Y_POS + x] = st->ram[SMBC_BB_Y_COORD];
  st->ram[SMBC_BLOCK_BBUF_LOW + x] = st->ram[SMBC_BB_PTR_LO];
  uint8_t buf_mt = st->ram[(uint16_t)(
      (st->ram[SMBC_BB_PTR_LO] | (st->ram[SMBC_BB_PTR_HI] << 8u)) +
      st->ram[SMBC_BB_Y_COORD])];
  uint8_t idx = block_bumped_chk(buf_mt);
  if (idx == 0xffu) {
    /* no match: keep the metatile as the block's contents — small
     * players get the metatile itself, big players the tya'd zero */
    st->ram[SMBC_BLOCK_METATILE + x] =
        st->ram[SMBC_PLAYER_SIZE] != 0x00u ? buf_mt : 0x00u;
  } else {
    st->ram[SMBC_BLOCK_STATE + x] = 0x11u;
    uint8_t new_mt = 0xc4u;
    if (buf_mt == 0x58u || buf_mt == 0x5du) {
      /* smb.asm: StartBTmr — a coin brick starts (or continues) the
       * brick coin timer */
      if (st->ram[SMBC_BRICK_COIN_TIMER_FLAG] == 0x00u) {
        st->ram[SMBC_BRICK_COIN_TIMER] = 0x0bu;
        st->ram[SMBC_BRICK_COIN_TIMER_FLAG]++;
      }
      if (st->ram[SMBC_BRICK_COIN_TIMER] != 0x00u) new_mt = buf_mt;
    }
    st->ram[SMBC_BLOCK_METATILE + x] = new_mt;
  }
  init_block_xy_pos(st, x);
  uint16_t ptr =
      (uint16_t)(st->ram[SMBC_BB_PTR_LO] | (st->ram[SMBC_BB_PTR_HI] << 8u));
  st->ram[ptr + st->ram[SMBC_BB_Y_COORD]] = 0x23u;
  st->ram[SMBC_BLOCK_BOUNCE_TIMER] = 0x10u;
  /* smb.asm: the block's vertical snap by size bracket */
  uint8_t y = 0x00u;
  if (st->ram[SMBC_CROUCHING_FLAG] != 0x00u) {
    y = 0x01u;
  } else if (st->ram[SMBC_PLAYER_SIZE] != 0x00u) {
    y = 0x01u;
  }
  st->ram[SMBC_BLOCK_Y_POSITION + x] =
      (uint8_t)((st->ram[SMBC_PLAYER_Y_POSITION] + kBlockYPosAdderData[y]) &
                0xf0u);
  if (st->ram[SMBC_BLOCK_STATE + x] == 0x11u) {
    bump_block(st, x, orig_mt);
  } else {
    brick_shatter(st, x);
  }
  /* smb.asm: InvOBit */
  st->ram[SMBC_SPDATA_OFFSET_CTRL] ^= 0x01u;
}

/* smb.asm: PlayerBGCollision */
void player_bg_collision(CoreState *st) {
  if (st->ram[SMBC_DISABLE_COLLISION_DET] != 0x00u) return;
  uint8_t sub = st->ram[SMBC_GAME_ENGINE_SUBROUTINE];
  if (sub == 0x0bu || sub < 0x04u) return;
  if (st->ram[SMBC_SWIMMING_FLAG] != 0x00u) {
    st->ram[SMBC_PLAYER_STATE] = 0x01u;
  } else {
    uint8_t ps = st->ram[SMBC_PLAYER_STATE];
    if (ps == 0x00u || ps == 0x03u) st->ram[SMBC_PLAYER_STATE] = 0x02u;
  }
  /* smb.asm: ChkOnScr */
  if (st->ram[SMBC_PLAYER_Y_HIGH_POS] != 0x01u) return;
  st->ram[SMBC_PLAYER_COLLISION_BITS] = 0xffu;
  if (st->ram[SMBC_PLAYER_Y_POSITION] >= 0xcfu) return;
  /* smb.asm: ChkCollSize — the $eb adder offset by size bracket */
  uint8_t y = 0x02u;
  if (st->ram[SMBC_CROUCHING_FLAG] == 0x00u &&
      st->ram[SMBC_PLAYER_SIZE] == 0x00u) {
    y = 0x01u;
    if (st->ram[SMBC_SWIMMING_FLAG] == 0x00u) y = 0x00u;
  }
  uint8_t eb = kBlockBufferAdderData[y];
  /* smb.asm: HeadChk — the extent index is the size, plus one when
   * crouching */
  uint8_t x = st->ram[SMBC_PLAYER_SIZE];
  if (st->ram[SMBC_CROUCHING_FLAG] != 0x00u) x++;
  if (st->ram[SMBC_PLAYER_Y_POSITION] >= kPlayerBGUpperExtent[x]) {
    uint8_t mt = block_buffer_colli_head(st, eb);
    if (mt != 0x00u) {
      if (check_for_coin_m_tiles(st, mt)) {
        /* smb.asm: AwardTouchedCoin */
        handle_coin_metatile(st);
        return;
      }
      if ((int8_t)st->ram[SMBC_PLAYER_Y_SPEED] < 0 &&
          st->ram[SMBC_BB_LOW_NYBBLE] >= 0x04u) {
        if (check_for_solid_m_tiles(mt)) {
          /* smb.asm: SolidOrClimb */
          if (mt != 0x26u)
            st->ram[SMBC_SQUARE1_SOUND_QUEUE] = SMBC_SFX_BUMP;
          st->ram[SMBC_PLAYER_Y_SPEED] = 0x01u;
        } else if (st->ram[SMBC_AREA_TYPE] == 0x00u ||
                   st->ram[SMBC_BLOCK_BOUNCE_TIMER] != 0x00u) {
          /* smb.asm: NYSpd */
          st->ram[SMBC_PLAYER_Y_SPEED] = 0x01u;
        } else {
          player_head_collision(st, mt);
        }
      }
    }
  }
  /* smb.asm: DoFootCheck */
  if (st->ram[SMBC_PLAYER_Y_POSITION] < 0xcfu) {
    uint8_t mt_left = block_buffer_colli_feet(st, eb);
    if (check_for_coin_m_tiles(st, mt_left)) {
      handle_coin_metatile(st);
      return;
    }
    uint8_t mt_right = block_buffer_colli_feet(st, eb);
    if (mt_left == 0x00u) {
      if (mt_right == 0x00u) goto do_side;
      if (check_for_coin_m_tiles(st, mt_right)) {
        handle_coin_metatile(st);
        return;
      }
    }
    /* smb.asm: ChkFootMTile — A is the nonzero foot's metatile */
    uint8_t mt = mt_left != 0x00u ? mt_left : mt_right;
    if (!check_for_climb_m_tiles(mt) &&
        (int8_t)st->ram[SMBC_PLAYER_Y_SPEED] >= 0) {
      if (mt == 0xc5u) {
        handle_axe_metatile(st);
        return;
      }
      if (!chk_invisible_m_tiles(mt)) {
        if (st->ram[SMBC_JUMPSPRING_ANIM_CTRL] != 0x00u) {
          /* smb.asm: InitSteP */
          st->ram[SMBC_PLAYER_STATE] = 0x00u;
        } else if (st->ram[SMBC_BB_LOW_NYBBLE] < 0x05u) {
          /* smb.asm: LandPlyr */
          chk_for_land_jump_spring(st, mt);
          st->ram[SMBC_PLAYER_Y_POSITION] &= 0xf0u;
          handle_pipe_entry(st, mt_right, mt_left);
          st->ram[SMBC_PLAYER_Y_SPEED] = 0x00u;
          st->ram[SMBC_PLAYER_Y_MOVE_FORCE] = 0x00u;
          st->ram[SMBC_STOMP_CHAIN_COUNTER] = 0x00u;
          st->ram[SMBC_PLAYER_STATE] = 0x00u;
        } else {
          impede_player_move(st, st->ram[SMBC_PLAYER_MOVING_DIR]);
        }
      }
    }
  }
do_side:
  /* smb.asm: DoPlayerSideCheck / SideCheckLoop */
  eb = (uint8_t)(eb + 0x02u);
  uint8_t counter = 0x02u; /* $00 — doubles as the impede side flag */
  for (;;) {
    eb++;
    if (st->ram[SMBC_PLAYER_Y_POSITION] >= 0x20u) {
      if (st->ram[SMBC_PLAYER_Y_POSITION] >= 0xe4u) return;
      uint8_t mt = block_buffer_colli_side(st, eb);
      if (mt != 0x00u && mt != 0x1cu && mt != 0x6bu &&
          !check_for_climb_m_tiles(mt)) {
        check_side_m_tiles(st, mt, counter);
        return;
      }
    }
    /* smb.asm: BHalf */
    eb++;
    if (st->ram[SMBC_PLAYER_Y_POSITION] < 0x08u ||
        st->ram[SMBC_PLAYER_Y_POSITION] >= 0xd0u)
      return;
    uint8_t mt = block_buffer_colli_side(st, eb);
    if (mt != 0x00u) {
      check_side_m_tiles(st, mt, counter);
      return;
    }
    counter--;
    if (counter == 0x00u) return;
  }
}
