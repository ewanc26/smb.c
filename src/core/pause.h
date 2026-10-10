/* core/pause.h — the pause routine.
 *
 * Owns: pause state toggling from the start button, the pause timer and
 * the pause sound queue write.
 */
#ifndef SMBC_CORE_PAUSE_H
#define SMBC_CORE_PAUSE_H

struct Smbc;

/* smb.asm: PauseRoutine */
void pause_routine(struct Smbc *smbc);

#endif /* SMBC_CORE_PAUSE_H */
