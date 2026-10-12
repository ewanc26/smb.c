/* tests/hw/apu_synth.c — APU synthesis: timers, envelope, length
 * counters, sweep, triangle linear counter, noise LFSR, $4015
 * semantics, the mixer, and determinism. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../../src/hw/apu.h"
#include "../../src/hw/apu_synth.h"

static float buf[SMBC_APU_MAX_FRAME_SAMPLES];

static size_t frame(ApuSynth *synth, Apu *apu) {
  return apu_synth_frame(synth, apu, buf);
}

static void write(Apu *apu, uint16_t addr, uint8_t value) {
  apu_write(apu, addr, value);
}

static double rms(size_t samples) {
  double acc = 0.0;
  for (size_t i = 0; i < samples; i++) {
    acc += (double)buf[i] * buf[i];
  }
  return sqrt(acc / (double)samples);
}

int main(void) {
  /* sample cadence: one frame emits ~734 samples, always <= cap */
  {
    Apu apu;
    ApuSynth synth;
    apu_init(&apu);
    apu_synth_init(&synth);
    size_t total = 0;
    for (int f = 0; f < 60; f++) {
      size_t n = frame(&synth, &apu);
      assert(n > 700 && n <= SMBC_APU_MAX_FRAME_SAMPLES);
      total += n;
    }
    /* 60 frames at 60.0988 fps ≈ 44056 samples at 44100 Hz */
    assert(total > 43800 && total < 44300);
  }

  /* pulse 1: enable + constant volume + valid period → audible,
   * disable via $4015 → silent and length cleared */
  {
    Apu apu;
    ApuSynth synth;
    apu_init(&apu);
    apu_synth_init(&synth);
    write(&apu, 0x4015u, 0x01u);  /* enable pulse 1 */
    write(&apu, 0x4000u, 0x9fu);  /* duty 50%, halt, constant, vol 15 */
    write(&apu, 0x4001u, 0x00u);  /* no sweep */
    write(&apu, 0x4002u, 0x80u);  /* period low */
    write(&apu, 0x4003u, 0x08u);  /* period high 0, length idx 1 (254) */
    size_t n = frame(&synth, &apu);
    assert(rms(n) > 0.001);
    assert(synth.pulse[0].length == 254 || synth.pulse[0].length == 253);
    assert(apu.status4015 & 0x01u);
    write(&apu, 0x4015u, 0x00u); /* disable all */
    n = frame(&synth, &apu);
    assert(synth.pulse[0].length == 0);
    assert((apu.status4015 & 0x01u) == 0);
    assert(rms(n) == 0.0);
  }

  /* envelope: non-constant volume decays from 15 toward 0 */
  {
    Apu apu;
    ApuSynth synth;
    apu_init(&apu);
    apu_synth_init(&synth);
    write(&apu, 0x4015u, 0x01u);
    write(&apu, 0x4000u, 0x80u); /* duty 50%, no loop, decay rate 0 (fast) */
    write(&apu, 0x4001u, 0x00u);
    write(&apu, 0x4002u, 0x40u);
    write(&apu, 0x4003u, 0x08u);
    frame(&synth, &apu); /* first quarter frame starts the envelope */
    uint8_t early = synth.pulse[0].env_decay;
    for (int f = 0; f < 8; f++) {
      frame(&synth, &apu);
    }
    uint8_t late = synth.pulse[0].env_decay;
    assert(early >= 11 && early <= 15); /* started at 15, ~4 clocks
                                         * into the frame */
    assert(late < early); /* and runs down to 0 within 8 frames */
  }

  /* length counter: expires and silences the channel; halt flag
   * (control d5) keeps it alive */
  {
    Apu apu;
    ApuSynth synth;
    apu_init(&apu);
    apu_synth_init(&synth);
    write(&apu, 0x4015u, 0x01u);
    write(&apu, 0x4000u, 0xbfu);  /* halt SET (d5) — length frozen */
    write(&apu, 0x4001u, 0x00u);
    write(&apu, 0x4002u, 0x40u);
    write(&apu, 0x4003u, 0x18u);  /* length idx 3 = 2 */
    for (int f = 0; f < 10; f++) {
      frame(&synth, &apu);
    }
    assert(synth.pulse[0].length == 2); /* halted: never clocks down */

    write(&apu, 0x4000u, 0x1fu);  /* halt CLEAR, constant vol 15 */
    write(&apu, 0x4003u, 0x18u);  /* reload length 2 — drains within
                                   * a frame or two now */
    for (int f = 0; f < 4; f++) {
      frame(&synth, &apu);
    }
    assert(synth.pulse[0].length == 0);
    assert((apu.status4015 & 0x01u) == 0);
    size_t n = frame(&synth, &apu);
    assert(rms(n) == 0.0);
  }

  /* sweep: enabled upward sweep raises the period over time */
  {
    Apu apu;
    ApuSynth synth;
    apu_init(&apu);
    apu_synth_init(&synth);
    write(&apu, 0x4015u, 0x01u);
    write(&apu, 0x4000u, 0x9fu);
    write(&apu, 0x4002u, 0x00u);
    write(&apu, 0x4003u, 0x0au);  /* period 0x200, length idx 1 */
    write(&apu, 0x4001u, 0xa1u);  /* sweep up, shift 1, period 2 */
    for (int f = 0; f < 10; f++) {
      frame(&synth, &apu);
    }
    assert(synth.pulse[0].period > 0x200u); /* swept upward */
  }

  /* triangle: linear counter counts down; control flag freezes it */
  {
    Apu apu;
    ApuSynth synth;
    apu_init(&apu);
    apu_synth_init(&synth);
    write(&apu, 0x4015u, 0x04u);
    write(&apu, 0x4008u, 0x42u);  /* control clear, linear reload 0x42 */
    write(&apu, 0x400au, 0x40u);
    write(&apu, 0x400bu, 0x08u);  /* length idx 1, sets linear_reload */
    frame(&synth, &apu);
    assert(synth.tri.linear <= 0x42u && synth.tri.linear > 0);
    assert(!synth.tri.linear_reload); /* cleared: control flag clear */
    uint8_t l1 = synth.tri.linear;
    for (int f = 0; f < 4; f++) {
      frame(&synth, &apu);
    }
    assert(synth.tri.linear < l1); /* counting down */

    write(&apu, 0x4008u, 0xffu);  /* control SET */
    write(&apu, 0x400bu, 0x08u);  /* reload again */
    frame(&synth, &apu);
    uint8_t l2 = synth.tri.linear;
    frame(&synth, &apu);
    /* control set: reload flag holds, counter keeps refilling */
    assert(synth.tri.linear == l2 || synth.tri.linear == 0x7fu);
  }

  /* noise: LFSR advances, and the mode bit changes the feedback tap */
  {
    Apu apu;
    ApuSynth synth;
    apu_init(&apu);
    apu_synth_init(&synth);
    write(&apu, 0x4015u, 0x08u);
    write(&apu, 0x400cu, 0x3fu);  /* halt, constant, vol 15 */
    write(&apu, 0x400eu, 0x00u);  /* mode 0, fastest period */
    write(&apu, 0x400fu, 0x08u);  /* length idx 1 */
    frame(&synth, &apu);
    uint16_t seq_a = synth.noise.lfsr;
    frame(&synth, &apu);
    assert(synth.noise.lfsr != seq_a);
    assert(synth.noise.lfsr != 0); /* LFSR never reaches 0 */

    ApuSynth synth2;
    apu_synth_init(&synth2);
    Apu apu2;
    apu_init(&apu2);
    write(&apu2, 0x4015u, 0x08u);
    write(&apu2, 0x400cu, 0x3fu);
    write(&apu2, 0x400eu, 0x80u);  /* mode 1: different tap */
    write(&apu2, 0x400fu, 0x08u);
    frame(&synth2, &apu2);
    assert(synth2.noise.lfsr != seq_a); /* different tap, different
                                         * sequence */
  }

  /* mixer: enabled channels produce bounded non-zero output;
   * silence is exact */
  {
    Apu apu;
    ApuSynth synth;
    apu_init(&apu);
    apu_synth_init(&synth);
    size_t n = frame(&synth, &apu);
    assert(rms(n) == 0.0); /* nothing enabled */
    write(&apu, 0x4015u, 0x05u); /* pulse 1 + triangle */
    write(&apu, 0x4000u, 0x9fu);
    write(&apu, 0x4001u, 0x00u);
    write(&apu, 0x4002u, 0x80u);
    write(&apu, 0x4003u, 0x08u);
    write(&apu, 0x4008u, 0xffu);
    write(&apu, 0x400au, 0x80u);
    write(&apu, 0x400bu, 0x08u);
    n = frame(&synth, &apu);
    assert(rms(n) > 0.001);
    for (size_t i = 0; i < n; i++) {
      assert(buf[i] >= 0.0f && buf[i] <= 0.65f); /* NES mixer peaks at
        * ~0.258 (pulse) + ~0.373 (tnd) */
    }
  }

  /* determinism: identical write streams produce identical samples */
  {
    float ref[SMBC_APU_MAX_FRAME_SAMPLES];
    Apu apu;
    ApuSynth synth;
    apu_init(&apu);
    apu_synth_init(&synth);
    write(&apu, 0x4015u, 0x0fu);
    write(&apu, 0x4000u, 0x9fu);
    write(&apu, 0x4001u, 0x08u);
    write(&apu, 0x4002u, 0x66u);
    write(&apu, 0x4003u, 0x09u);
    write(&apu, 0x4004u, 0x5fu);
    write(&apu, 0x4006u, 0xa0u);
    write(&apu, 0x4007u, 0x08u);
    size_t n = frame(&synth, &apu);
    memcpy(ref, buf, n * sizeof(float));

    Apu apu2;
    ApuSynth synth2;
    apu_init(&apu2);
    apu_synth_init(&synth2);
    write(&apu2, 0x4015u, 0x0fu);
    write(&apu2, 0x4000u, 0x9fu);
    write(&apu2, 0x4001u, 0x08u);
    write(&apu2, 0x4002u, 0x66u);
    write(&apu2, 0x4003u, 0x09u);
    write(&apu2, 0x4004u, 0x5fu);
    write(&apu2, 0x4006u, 0xa0u);
    write(&apu2, 0x4007u, 0x08u);
    size_t n2 = frame(&synth2, &apu2);
    assert(n2 == n);
    assert(memcmp(ref, buf, n * sizeof(float)) == 0);
  }

  /* $4003 while disabled does not load the length counter (hardware:
   * length writes to a disabled channel are ignored) */
  {
    Apu apu;
    ApuSynth synth;
    apu_init(&apu);
    apu_synth_init(&synth);
    write(&apu, 0x4003u, 0xf8u); /* channel disabled */
    frame(&synth, &apu);
    assert(synth.pulse[0].length == 0);
    assert(apu.status4015 == 0);
  }

  printf("tests/hw/apu_synth: ok\n");
  return 0;
}
