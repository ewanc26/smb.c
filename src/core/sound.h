/* core/sound.h — the sound engine.
 *
 * Owns: music and sound effect processing against the APU register image.
 * The engine itself is pending translation; the hw/apu register capture it
 * will write to is already in place.
 */
#ifndef SMBC_CORE_SOUND_H
#define SMBC_CORE_SOUND_H

struct Smbc;

/* smb.asm: SoundEngine — TODO(port): translate the sound engine (music
 * data under "MUSIC DATA" in the disassembly comes with it). */
void sound_engine(struct Smbc *smbc);

#endif /* SMBC_CORE_SOUND_H */
