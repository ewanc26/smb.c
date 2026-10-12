/* host/audio.h — audio device seam.
 *
 * Owns: the host audio device (SDL) and the callback contract the
 * core's APU synthesis (#35) will feed. Synthesis itself is NOT here:
 * smbc_audio_render currently emits silence, and when #35 lands it
 * becomes the single place the device callback pulls samples from.
 * The core stays headless — no SDL types cross into it.
 *
 * Not owned: APU register behaviour, mixing, or emulation timing. */
#ifndef SMBC_HOST_AUDIO_H
#define SMBC_HOST_AUDIO_H

#include <stdbool.h>
#include <stddef.h>

#include <SDL.h>

typedef struct Smbc Smbc; /* opaque here; defined in core/frame.h */

typedef struct {
  SDL_AudioDeviceID device;
  Smbc *smbc; /* borrowed; the synthesis seam reads core state */
} SmbcAudio;

/* Open the device and start the stream. Failure is not fatal to the
 * frontend — the game runs silent; returns false on SDL failure. */
bool audio_open(SmbcAudio *audio, Smbc *smbc);

/* The #35 seam: render n stereo float frames from core state. Emits
 * silence until APU synthesis exists. */
void smbc_audio_render(Smbc *smbc, float *out, size_t frames);

void audio_close(SmbcAudio *audio);

#endif /* SMBC_HOST_AUDIO_H */
