/* core/state.h — the NES RAM map.
 *
 * Owns: a single flat 2KB image of the console's internal RAM
 * ($0000-$07FF), plus the named offsets the game code uses. The original
 * treats this memory as shared, fixed-address state; the port keeps that
 * shape instead of inventing structures the disassembly does not have.
 *
 * Offsets below are smb.asm's game defines, cited per line. Add further
 * names as their routines are translated — never rename one to something
 * that does not appear in the disassembly.
 */
#ifndef SMBC_CORE_STATE_H
#define SMBC_CORE_STATE_H

#include <stdint.h>

#define SMBC_RAM_SIZE 0x0800u

typedef struct {
  uint8_t ram[SMBC_RAM_SIZE];
} CoreState;

void state_init(CoreState *state);

/* Zero page */
#define SMBC_OBJECT_OFFSET      0x0008u /* smb.asm: ObjectOffset */
#define SMBC_FRAME_COUNTER      0x0009u /* smb.asm: FrameCounter */
#define SMBC_A_B_BUTTONS        0x000au /* smb.asm: A_B_Buttons */
#define SMBC_UP_DOWN_BUTTONS    0x000bu /* smb.asm: Up_Down_Buttons */
#define SMBC_PREV_A_B_BUTTONS   0x000du /* smb.asm: PreviousA_B_Buttons */

/* $0700 page */
#define SMBC_SAVED_JOYPAD1_BITS 0x06fcu /* smb.asm: SavedJoypad1Bits */
#define SMBC_SAVED_JOYPAD2_BITS 0x06fdu /* smb.asm: SavedJoypad2Bits */
#define SMBC_JOYPAD_BIT_MASK    0x074au /* smb.asm: JoypadBitMask */
#define SMBC_JOYPAD_OVERRIDE    0x0758u /* smb.asm: JoypadOverride */

#endif /* SMBC_CORE_STATE_H */
