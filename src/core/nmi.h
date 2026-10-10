/* core/nmi.h — the non-maskable interrupt handler.
 *
 * Owns: one frame of the game's interrupt-driven work: screen update,
 * sound, input, pause, timers, the pseudorandom register, sprite shuffle
 * and the scroll/mode dispatch. Called by the frame loop once per frame.
 */
#ifndef SMBC_CORE_NMI_H
#define SMBC_CORE_NMI_H

struct Smbc;

/* smb.asm: NonMaskableInterrupt */
void nmi_run(struct Smbc *smbc);

#endif /* SMBC_CORE_NMI_H */
