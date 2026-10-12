/* host/audio.h — audio device seam.
 *
 * Owns: the host audio device (SDL), the APU synth instance, and the
 * lock-free SPSC ring that decouples them: the fixed-step frontend
 * loop produces one video frame of samples per push (audio_push_frame
 * after each smbc_frame), the SDL callback consumes at its own rate.
 * Simulation timing never blocks on the device, and the device never
 * touches core state directly — the synth reads the Apu write log.
 * The core stays headless: no SDL types cross into it.
 *
 * Not owned: channel/mixer emulation (hw/apu_synth.c), APU register
 * behaviour (hw/apu.c). */
#ifndef SMBC_HOST_AUDIO_H
#define SMBC_HOST_AUDIO_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

#include <SDL.h>

#include "../hw/apu_synth.h"

typedef struct Smbc Smbc; /* opaque here; defined in core/frame.h */

#define SMBC_AUDIO_RATE 44100 /* matches SMBC_APU_RATE */
#define SMBC_AUDIO_CHANNELS 2
#define SMBC_AUDIO_RING 16384 /* mono samples, ~370 ms */

typedef struct {
  SDL_AudioDeviceID device;
  ApuSynth synth;
  float ring[SMBC_AUDIO_RING];
  _Atomic unsigned head; /* written by the frontend thread */
  _Atomic unsigned tail; /* written by the audio callback */
} SmbcAudio;

/* Open the device and start the stream. Failure is not fatal to the
 * frontend — the game runs silent; returns false on SDL failure. */
bool audio_open(SmbcAudio *audio);

/* Frontend side: after each smbc_frame, synthesise the frame's audio
 * from the Apu write log and queue it for the device. With no device
 * open the log is still drained (it is capped; losing it would cost
 * edge semantics when the device appears). */
void audio_push_frame(SmbcAudio *audio, Smbc *smbc);

/* Device-callback side: pull n stereo float frames from the ring;
 * underrun pads with silence. */
void smbc_audio_render(SmbcAudio *audio, float *out, size_t frames);

void audio_close(SmbcAudio *audio);

#endif /* SMBC_HOST_AUDIO_H */
