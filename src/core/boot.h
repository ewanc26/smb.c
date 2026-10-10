/* core/boot.h — the reset sequence.
 *
 * Owns: power-on/reset initialisation, translated from the disassembly's
 * reset entry. Runs once per Smbc instance before the first frame.
 */
#ifndef SMBC_CORE_BOOT_H
#define SMBC_CORE_BOOT_H

struct Smbc;

/* smb.asm: Start — PPU warm-up waits, warm/cold boot check, memory clear,
 * sound and PPU enable, sprites offscreen, NMIs on. The 6502's stack
 * pointer and status flags have no model here; EndlessLoop is the host
 * frame loop. */
void boot_run(struct Smbc *smbc);

#endif /* SMBC_CORE_BOOT_H */
