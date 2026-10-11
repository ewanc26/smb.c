/* core/cannon.c — see core/cannon.h for the contract. */
#include "cannon.h"

#include "../data/cannon.h"
#include "collide.h"
#include "enemies.h"
#include "move.h"
#include "offscreen.h"
#include "runners.h"

/* smb.asm: ProcessCannons */
void process_cannons(CoreState *st) {
  if (st->ram[SMBC_AREA_TYPE] == 0x00u) return; /* water area: no cannons */
  for (int slot = 0x02; slot >= 0; slot--) {
    uint8_t x = (uint8_t)slot;
    st->ram[SMBC_OBJECT_OFFSET] = x;
    bool check = true;
    if (st->ram[SMBC_ENEMY_FLAG + x] == 0x00u) {
      /* the LSFR bits, masked by the secondary hard mode flag, pick a
       * cannon; values of six or more are ignored */
      uint8_t y = (uint8_t)(st->ram[SMBC_PSEUDO_RANDOM_BIT_REG + 1u + x] &
                            SMBC_CANNON_BITMASKS[st->ram[SMBC_SECONDARY_HARD_MODE]]);
      if (y < 0x06u && st->ram[SMBC_CANNON_PAGE_LOC + y] != 0x00u) {
        if (st->ram[SMBC_CANNON_TIMER + y] != 0x00u) {
          /* smb.asm: the sbc #$00 always borrows — the carry is clear
           * from the earlier masking compare */
          st->ram[SMBC_CANNON_TIMER + y]--;
        } else if (st->ram[SMBC_TIMER_CONTROL] == 0x00u) {
          /* smb.asm: FireCannon — spawn the bill eight pixels above
           * the cannon and skip the handler for the spawn frame */
          st->ram[SMBC_CANNON_TIMER + y] = 0x0eu;
          st->ram[SMBC_ENEMY_PAGE_LOC + x] = st->ram[SMBC_CANNON_PAGE_LOC + y];
          st->ram[SMBC_ENEMY_X_POSITION + x] = st->ram[SMBC_CANNON_X_POSITION + y];
          st->ram[SMBC_ENEMY_Y_POSITION + x] =
              (uint8_t)(st->ram[SMBC_CANNON_Y_POSITION + y] - 0x08u);
          st->ram[SMBC_ENEMY_Y_HIGH_POS + x] = 0x01u;
          st->ram[SMBC_ENEMY_FLAG + x] = 0x01u;
          st->ram[SMBC_ENEMY_STATE + x] = 0x00u; /* lsr of #$01 */
          st->ram[SMBC_ENEMY_BOUND_BOX_CTRL + x] = 0x09u;
          st->ram[SMBC_ENEMY_ID + x] = SMBC_BULLET_BILL_CANNON;
          check = false;
        }
      }
    }
    if (!check) continue;
    /* smb.asm: Chk_BB — hand off the cannon-fired bullet bills */
    if (st->ram[SMBC_ENEMY_ID + x] != SMBC_BULLET_BILL_CANNON) continue;
    offscreen_bounds_check(st, x);
    if (st->ram[SMBC_ENEMY_FLAG + x] == 0x00u) continue;
    get_enemy_offscreen_bits(st, x);
    bullet_bill_handler(st, x);
  }
}

/* smb.asm: BulletBillHandler */
void bullet_bill_handler(CoreState *st, uint8_t x) {
  bool killed = false;
  if (st->ram[SMBC_TIMER_CONTROL] == 0x00u) {
    if (st->ram[SMBC_ENEMY_STATE + x] == 0x00u) {
      if ((st->ram[SMBC_ENEMY_OFFSCREEN_BITS] & 0x0cu) == 0x0cu) {
        killed = true; /* already both off the sides of the screen */
      } else {
        /* move toward the player: right when the bill is left of him */
        uint8_t dir = 0x01u;
        int8_t diff = player_enemy_diff(st, x);
        if (diff >= 0) dir++;
        st->ram[SMBC_ENEMY_MOVING_DIR + x] = dir;
        st->ram[SMBC_ENEMY_X_SPEED + x] = SMBC_BULLET_BILL_X_SPD_DATA[dir - 1u];
        /* smb.asm: the $00 low byte plus 40, plus the carry the page
         * subtraction left (set when the bill is not left of the
         * player), must reach 80 or the player is too close */
        uint8_t low = (uint8_t)(st->ram[SMBC_ENEMY_X_POSITION + x] -
                                st->ram[SMBC_PLAYER_X_POSITION]);
        uint8_t reach = (uint8_t)(low + 0x28u + (diff >= 0 ? 1u : 0u));
        if (reach < 0x50u) {
          killed = true;
        } else {
          st->ram[SMBC_ENEMY_STATE + x] = 0x01u;
          st->ram[SMBC_ENEMY_FRAME_TIMER + x] = 0x0au;
          st->ram[SMBC_SQUARE2_SOUND_QUEUE] = SMBC_SFX_BLAST;
        }
      }
    }
    if (!killed) {
      /* smb.asm: ChkDSte — a defeated bill (state d5 set) falls */
      if ((st->ram[SMBC_ENEMY_STATE + x] & 0x20u) != 0x00u) {
        move_d_enemy_vertically(st, x);
      }
      /* smb.asm: BBFly */
      move_enemy_horizontally(st, x);
    }
  }
  if (killed) {
    /* smb.asm: KillBB */
    erase_enemy_object(st, x);
    return;
  }
  /* smb.asm: RunBBSubs */
  get_enemy_offscreen_bits(st, x);
  relative_enemy_position(st, x);
  get_enemy_bound_box(st, x);
  player_enemy_collision(st, x);
  enemy_gfx_handler(st, x);
}
