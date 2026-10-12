/* hw/apu_synth.h — NES APU synthesis (#35).
 *
 * Owns: the pulse, triangle and noise channels (SMB uses no DMC),
 * their timers, envelopes, length counters, pulse sweeps, the frame
 * sequencer, $4015 enable/status semantics and the non-linear mixer.
 * Sample generation is cycle-stepped at the NTSC CPU rate and
 * resampled to 44.1 kHz by accumulator — no SDL, no threads, so the
 * unit suite drives it directly.
 *
 * Driven in video-frame chunks: apu_synth_frame consumes the Apu
 * write log (hw/apu.h — write events carry the envelope/length
 * restart semantics a value snapshot cannot), applies the writes,
 * advances 29780.5 APU cycles and emits the frame's samples. SMB's
 * sound engine writes registers during NMI, so "all writes land at
 * frame start" matches the original's timing to within a scanline.
 *
 * State lives OUTSIDE the Smbc bundle and the replay hash: audio
 * never feeds back into gameplay.
 *
 * Not owned: DMC (unused by SMB), frame IRQs (the game inhibits
 * them), host buffering/threading (host/audio.c). */
#ifndef SMBC_HW_APU_SYNTH_H
#define SMBC_HW_APU_SYNTH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "apu.h"

#define SMBC_APU_RATE 44100u
#define SMBC_APU_MAX_FRAME_SAMPLES 736u /* ~733.8, rounded up */

typedef struct {
  uint16_t timer;  /* divider, counts APU cycles */
  uint16_t period; /* t register (11 bits) */
  uint8_t duty;    /* $4000/4 d7-d6 */
  uint8_t duty_step;
  uint8_t length; /* length counter */
  uint8_t env_v, env_div, env_decay;
  bool env_loop, env_const, env_start;
  uint8_t sweep_p, sweep_div, sweep_shift;
  bool sweep_en, sweep_neg, sweep_reload;
  bool enabled; /* $4015 */
} ApuPulse;

typedef struct {
  uint16_t timer, period;
  uint8_t seq_step; /* 32-step sequence position */
  uint8_t length;
  uint8_t linear;    /* linear counter */
  uint8_t linear_r;  /* reload value, $4008 d6-d0 */
  bool linear_reload;
  bool control; /* $4008 d7 — also the length halt flag */
  bool enabled;
} ApuTriangle;

typedef struct {
  uint16_t timer, period;
  uint16_t lfsr; /* 15-bit */
  uint8_t length;
  uint8_t env_v, env_div, env_decay;
  bool env_loop, env_const, env_start;
  bool mode; /* $400E d7 — feedback tap */
  bool enabled;
} ApuNoise;

typedef struct {
  ApuPulse pulse[2];
  ApuTriangle tri;
  ApuNoise noise;
  bool seq_5step;   /* $4017 d7 */
  unsigned seq_step; /* next sequencer step */
  double seq_cycle;  /* APU cycles into the current step */
  double sample_cycle; /* APU cycles toward the next output sample */
  double frame_carry;  /* fractional 29780.5 accumulation */
} ApuSynth;

void apu_synth_init(ApuSynth *synth);

/* Consume apu's write log (apu->log_len resets to 0), apply the
 * writes, advance one video frame and write the mixed mono samples
 * to out (SMBC_APU_MAX_FRAME_SAMPLES capacity). Returns the sample
 * count and refreshes apu->status4015. */
size_t apu_synth_frame(ApuSynth *synth, Apu *apu, float *out);

#endif /* SMBC_HW_APU_SYNTH_H */
