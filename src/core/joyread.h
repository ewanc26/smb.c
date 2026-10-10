/* core/joyread.h — joypad reading in the game's own format.
 *
 * Owns: the strobe-and-serial read of both controllers into
 * SavedJoypad1Bits / SavedJoypad2Bits, including the select/start masking
 * against JoypadBitMask. The saved layout is the game's (A in d7), not the
 * host's (A in d0) — see the SMBC_PAD_* defines in state.h.
 */
#ifndef SMBC_CORE_JOYREAD_H
#define SMBC_CORE_JOYREAD_H

struct Smbc;

/* smb.asm: ReadJoypads — strobe both ports, read eight bits from each. */
void read_joypads(struct Smbc *smbc);

#endif /* SMBC_CORE_JOYREAD_H */
