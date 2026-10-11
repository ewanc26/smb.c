/* core/whirlpool.c — see core/whirlpool.h for the contract. */
#include "whirlpool.h"

#include "move.h"

/* smb.asm: ProcessWhirlpools. The whirlpool data shares the cannon
 * slots, so the left extent, length and page location are the same
 * memory as Cannon_X_Position, Cannon_Y_Position and Cannon_PageLoc.
 * The routine walks the five slots from the last; the first the player
 * falls within is activated and wins */
void process_whirlpools(CoreState *st) {
  if (st->ram[SMBC_AREA_TYPE] != 0x00u) return;     /* not a water level */
  st->ram[SMBC_WHIRLPOOL_FLAG] = 0x00u;             /* sta: A is zero here */
  if (st->ram[SMBC_TIMER_CONTROL] != 0x00u) return; /* master timer running */
  for (int slot = 0x04; slot >= 0; slot--) {
    uint8_t y = (uint8_t)slot;
    /* smb.asm: WhLoop — right extent = left extent + length, with the
     * carry folded into the page location */
    uint16_t right = (uint16_t)st->ram[SMBC_WHIRLPOOL_LEFT_EXTENT + y] +
                     st->ram[SMBC_WHIRLPOOL_LENGTH + y];
    uint8_t right_low = (uint8_t)right;
    uint8_t right_carry = (uint8_t)(right >> 8);
    uint8_t page = st->ram[SMBC_WHIRLPOOL_PAGE_LOC + y];
    if (page == 0x00u) continue; /* beq NextWh: empty slot */
    uint8_t right_page = (uint8_t)(page + right_carry);
    /* smb.asm: player minus left extent; a negative high byte means the
     * player is too far left */
    uint8_t pl_x = st->ram[SMBC_PLAYER_X_POSITION];
    uint8_t left = st->ram[SMBC_WHIRLPOOL_LEFT_EXTENT + y];
    uint8_t left_borrow = (pl_x < left) ? 1u : 0u;
    uint8_t left_high = (uint8_t)(st->ram[SMBC_PLAYER_PAGE_LOC] -
                                  st->ram[SMBC_WHIRLPOOL_PAGE_LOC + y] -
                                  left_borrow);
    if ((left_high & 0x80u) != 0x00u) continue; /* bmi NextWh */
    /* smb.asm: right extent minus player; a negative high byte means the
     * player is past the right edge, so move on */
    uint8_t pl_page = st->ram[SMBC_PLAYER_PAGE_LOC];
    uint8_t right_borrow = (right_low < pl_x) ? 1u : 0u;
    uint8_t right_high =
        (uint8_t)(right_page - pl_page - right_borrow);
    if ((right_high & 0x80u) != 0x00u) continue; /* bpl not taken */
    /* smb.asm: WhirlpoolActivate — the player is inside; centre is left
     * extent plus half the length */
    uint8_t half = (uint8_t)(st->ram[SMBC_WHIRLPOOL_LENGTH + y] >> 1);
    uint16_t centre = (uint16_t)st->ram[SMBC_WHIRLPOOL_LEFT_EXTENT + y] + half;
    uint8_t centre_low = (uint8_t)centre;
    uint8_t centre_page =
        (uint8_t)(st->ram[SMBC_WHIRLPOOL_PAGE_LOC + y] + (uint8_t)(centre >> 8));
    /* smb.asm: the pull runs only on frames with FrameCounter bit 0 set */
    if ((st->ram[SMBC_FRAME_COUNTER] & 0x01u) != 0x00u) {
      uint8_t pl_now = st->ram[SMBC_PLAYER_X_POSITION];
      uint8_t centre_borrow = (centre_low < pl_now) ? 1u : 0u;
      uint8_t centre_high =
          (uint8_t)(centre_page - pl_page - centre_borrow);
      if ((centre_high & 0x80u) == 0x00u) {
        /* smb.asm: LeftWh — player left of (or at) centre: pull right one
         * pixel, but only when collision bit 0 is set */
        if ((st->ram[SMBC_PLAYER_COLLISION_BITS] & 0x01u) != 0x00u) {
          uint16_t sum = (uint16_t)pl_now + 1u;
          st->ram[SMBC_PLAYER_X_POSITION] = (uint8_t)sum;
          st->ram[SMBC_PLAYER_PAGE_LOC] =
              (uint8_t)(pl_page + (uint8_t)(sum >> 8));
        }
      } else {
        /* player right of centre: pull left one pixel */
        uint8_t borrow = (pl_now < 0x01u) ? 1u : 0u;
        st->ram[SMBC_PLAYER_X_POSITION] = (uint8_t)(pl_now - 0x01u);
        st->ram[SMBC_PLAYER_PAGE_LOC] = (uint8_t)(pl_page - borrow);
      }
    }
    /* smb.asm: WhPull — set the flag and impose the whirlpool gravity on
     * the player (SprObject 0): downward amount $10, maximum speed $01,
     * no upward part. The asm jumps into ImposeGravity and leaves */
    st->ram[SMBC_WHIRLPOOL_FLAG] = 0x01u;
    impose_gravity(st, 0x00u, 0x10u, 0x00u, 0x01u, 0x00u);
    return;
  }
}
