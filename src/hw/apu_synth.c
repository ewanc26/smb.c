/* hw/apu_synth.c — NES APU synthesis. See hw/apu_synth.h. */
#include "apu_synth.h"

/* NTSC: CPU 1789773 Hz, video frame 29780.5 cycles (60.0988 Hz),
 * 44100 Hz output → 40.5844 cycles per sample */
#define SMBC_CPU_HZ 1789773.0
#define SMBC_CYCLES_PER_FRAME 29780.5
#define SMBC_CYCLES_PER_SAMPLE (SMBC_CPU_HZ / (double)SMBC_APU_RATE)

/* frame sequencer step boundaries, in APU cycles from sequence start */
static const double SEQ4[4] = {7457.0, 14913.0, 22371.0, 29829.0};
static const double SEQ5[4] = {7457.0, 14913.0, 22371.0, 29881.0};

static const uint8_t LENGTH_TABLE[32] = {
    10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
    12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

static const uint8_t DUTY[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0}, /* 12.5% */
    {0, 1, 1, 0, 0, 0, 0, 0}, /* 25% */
    {0, 1, 1, 1, 1, 0, 0, 0}, /* 50% */
    {1, 0, 0, 1, 1, 1, 1, 1}, /* 25% negated */
};

static const uint8_t TRI_SEQ[32] = {15, 14, 13, 12, 11, 10, 9, 8,
                                    7,  6,  5,  4,  3,  2,  1, 0,
                                    0,  1,  2,  3,  4,  5,  6, 7,
                                    8,  9,  10, 11, 12, 13, 14, 15};

void apu_synth_clock_qh(ApuSynth *synth, bool with_half); /* fwd */

static const uint16_t NOISE_PERIODS[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034,
    4068,
};

void apu_synth_init(ApuSynth *synth) {
  *synth = (ApuSynth){.noise = {.lfsr = 1}};
}

/* ---------------------------------------------------- write events */

static void env_write(ApuPulse *p, uint8_t v) {
  p->env_loop = (v & 0x20u) != 0; /* d5 — also the length halt flag */
  p->env_const = (v & 0x10u) != 0;
  p->env_v = v & 0x0fu;
}

static void apply_write(ApuSynth *synth, uint16_t addr, uint8_t v) {
  switch (addr) {
  case 0x00: /* $4000 pulse 1 control */
    synth->pulse[0].duty = v >> 6;
    env_write(&synth->pulse[0], v);
    break;
  case 0x01: /* $4001 pulse 1 sweep */
    synth->pulse[0].sweep_en = (v & 0x80u) != 0;
    synth->pulse[0].sweep_p = (v >> 4) & 0x07u;
    synth->pulse[0].sweep_neg = (v & 0x08u) != 0;
    synth->pulse[0].sweep_shift = v & 0x07u;
    synth->pulse[0].sweep_reload = true;
    break;
  case 0x02: /* $4002 pulse 1 timer low */
    synth->pulse[0].period = (synth->pulse[0].period & 0x700u) | v;
    break;
  case 0x03: /* $4003 pulse 1 timer high + length */
    synth->pulse[0].period =
        (synth->pulse[0].period & 0xffu) | ((uint16_t)(v & 0x07u) << 8);
    if (synth->pulse[0].enabled) {
      synth->pulse[0].length = LENGTH_TABLE[v >> 3];
    }
    synth->pulse[0].duty_step = 0;
    synth->pulse[0].env_start = true;
    break;
  case 0x04: /* $4004 pulse 2 control */
    synth->pulse[1].duty = v >> 6;
    env_write(&synth->pulse[1], v);
    break;
  case 0x05: /* $4005 pulse 2 sweep */
    synth->pulse[1].sweep_en = (v & 0x80u) != 0;
    synth->pulse[1].sweep_p = (v >> 4) & 0x07u;
    synth->pulse[1].sweep_neg = (v & 0x08u) != 0;
    synth->pulse[1].sweep_shift = v & 0x07u;
    synth->pulse[1].sweep_reload = true;
    break;
  case 0x06: /* $4006 pulse 2 timer low */
    synth->pulse[1].period = (synth->pulse[1].period & 0x700u) | v;
    break;
  case 0x07: /* $4007 pulse 2 timer high + length */
    synth->pulse[1].period =
        (synth->pulse[1].period & 0xffu) | ((uint16_t)(v & 0x07u) << 8);
    if (synth->pulse[1].enabled) {
      synth->pulse[1].length = LENGTH_TABLE[v >> 3];
    }
    synth->pulse[1].duty_step = 0;
    synth->pulse[1].env_start = true;
    break;
  case 0x08: /* $4008 triangle control + linear reload value */
    synth->tri.control = (v & 0x80u) != 0;
    synth->tri.linear_r = v & 0x7fu;
    break;
  case 0x0a: /* $400A triangle timer low */
    synth->tri.period = (synth->tri.period & 0x700u) | v;
    break;
  case 0x0b: /* $400B triangle timer high + length */
    synth->tri.period =
        (synth->tri.period & 0xffu) | ((uint16_t)(v & 0x07u) << 8);
    if (synth->tri.enabled) {
      synth->tri.length = LENGTH_TABLE[v >> 3];
    }
    synth->tri.linear_reload = true;
    break;
  case 0x0c: /* $400C noise control */
    synth->noise.env_loop = (v & 0x20u) != 0;
    synth->noise.env_const = (v & 0x10u) != 0;
    synth->noise.env_v = v & 0x0fu;
    break;
  case 0x0e: /* $400E noise mode + period */
    synth->noise.mode = (v & 0x80u) != 0;
    synth->noise.period = NOISE_PERIODS[v & 0x0fu];
    break;
  case 0x0f: /* $400F noise length */
    if (synth->noise.enabled) {
      synth->noise.length = LENGTH_TABLE[v >> 3];
    }
    synth->noise.env_start = true;
    break;
  case 0x15: /* $4015 enables; disabling clears the length counter */
    synth->pulse[0].enabled = (v & 0x01u) != 0;
    synth->pulse[1].enabled = (v & 0x02u) != 0;
    synth->tri.enabled = (v & 0x04u) != 0;
    synth->noise.enabled = (v & 0x08u) != 0;
    if (!synth->pulse[0].enabled) synth->pulse[0].length = 0;
    if (!synth->pulse[1].enabled) synth->pulse[1].length = 0;
    if (!synth->tri.enabled) synth->tri.length = 0;
    if (!synth->noise.enabled) synth->noise.length = 0;
    break;
  case 0x17: /* $4017 frame counter */
    synth->seq_5step = (v & 0x80u) != 0;
    synth->seq_cycle = 0.0;
    synth->seq_step = 0;
    if (synth->seq_5step) {
      apu_synth_clock_qh(synth, true); /* 5-step clocks immediately */
    }
    break;
  default:
    break; /* $4010-$4013 DMC and $4014 OAM DMA: not APU synthesis */
  }
}

/* ----------------------------------------------------- unit clocks */

static void env_clock(uint8_t *div, uint8_t *decay, uint8_t v, bool loop,
                      bool *start) {
  if (*start) {
    *start = false;
    *decay = 15;
    *div = v;
  } else if (*div == 0) {
    *div = v;
    if (*decay > 0) {
      (*decay)--;
    } else if (loop) {
      *decay = 15;
    }
  } else {
    (*div)--;
  }
}

static void sweep_clock(ApuPulse *p, bool second_channel) {
  uint16_t change = (uint16_t)(p->period >> p->sweep_shift);
  /* the negate quirk: pulse 1 subtracts the change, pulse 2 subtracts
   * change + 1 (ones' complement on the second unit) */
  uint16_t target =
      p->sweep_neg ? (uint16_t)(p->period - change - (second_channel ? 1u : 0u))
                   : (uint16_t)(p->period + change);
  if (p->sweep_div == 0 && p->sweep_en && p->sweep_shift > 0 &&
      p->period >= 8 && target <= 0x7ffu) {
    p->period = target;
  }
  if (p->sweep_div == 0 || p->sweep_reload) {
    p->sweep_div = p->sweep_p;
    p->sweep_reload = false;
  } else {
    p->sweep_div--;
  }
}

static void length_clock(uint8_t *length, bool halt) {
  if (!halt && *length > 0) {
    (*length)--;
  }
}

/* quarter-frame clock; with_half also clocks length counters and
 * sweeps (the half-frame units). Non-static so the $4017 five-step
 * immediate clock can call it. */
void apu_synth_clock_qh(ApuSynth *synth, bool with_half) {
  env_clock(&synth->pulse[0].env_div, &synth->pulse[0].env_decay,
            synth->pulse[0].env_v, synth->pulse[0].env_loop,
            &synth->pulse[0].env_start);
  env_clock(&synth->pulse[1].env_div, &synth->pulse[1].env_decay,
            synth->pulse[1].env_v, synth->pulse[1].env_loop,
            &synth->pulse[1].env_start);
  env_clock(&synth->noise.env_div, &synth->noise.env_decay,
            synth->noise.env_v, synth->noise.env_loop,
            &synth->noise.env_start);
  /* triangle linear counter */
  if (synth->tri.linear_reload) {
    synth->tri.linear = synth->tri.linear_r;
  } else if (synth->tri.linear > 0) {
    synth->tri.linear--;
  }
  if (!synth->tri.control) {
    synth->tri.linear_reload = false;
  }
  if (with_half) {
    sweep_clock(&synth->pulse[0], false);
    sweep_clock(&synth->pulse[1], true);
    length_clock(&synth->pulse[0].length, synth->pulse[0].env_loop);
    length_clock(&synth->pulse[1].length, synth->pulse[1].env_loop);
    length_clock(&synth->tri.length, synth->tri.control);
    length_clock(&synth->noise.length, synth->noise.env_loop);
  }
}

/* ------------------------------------------------------ generators */

static bool sweep_muting(const ApuPulse *p, bool second_channel) {
  uint16_t change = (uint16_t)(p->period >> p->sweep_shift);
  uint16_t target =
      p->sweep_neg ? (uint16_t)(p->period - change - (second_channel ? 1u : 0u))
                   : (uint16_t)(p->period + change);
  return p->period < 8u || target > 0x7ffu;
}

static uint8_t env_volume(uint8_t v, uint8_t decay, bool constant) {
  return constant ? v : decay;
}

static void clock_channels(ApuSynth *synth) {
  for (unsigned i = 0; i < 2; i++) {
    ApuPulse *p = &synth->pulse[i];
    if (p->timer == 0) {
      p->timer = p->period;
      p->duty_step = (uint8_t)((p->duty_step + 1) & 7);
    } else {
      p->timer--;
    }
  }
  ApuTriangle *t = &synth->tri;
  if (t->timer == 0) {
    t->timer = t->period;
    if (t->length > 0 && t->linear > 0) {
      t->seq_step = (uint8_t)((t->seq_step + 1) & 31);
    }
  } else {
    t->timer--;
  }
  ApuNoise *n = &synth->noise;
  if (n->timer == 0) {
    n->timer = n->period;
    uint16_t tap = n->mode ? 6u : 1u;
    uint16_t feedback =
        (uint16_t)((n->lfsr & 1u) ^ ((n->lfsr >> tap) & 1u));
    n->lfsr = (uint16_t)((n->lfsr >> 1) | (feedback << 14));
  } else {
    n->timer--;
  }
}

static uint8_t pulse_output(const ApuPulse *p, bool second_channel) {
  if (!p->enabled || p->length == 0 || sweep_muting(p, second_channel)) {
    return 0;
  }
  return (uint8_t)(DUTY[p->duty][p->duty_step] *
                   env_volume(p->env_v, p->env_decay, p->env_const));
}

static uint8_t tri_output(const ApuTriangle *t) {
  /* disabled channels are pulled to 0; while enabled, the sequence
   * position holds when halted (length or linear at 0) and the last
   * value keeps driving the DAC */
  if (!t->enabled) {
    return 0;
  }
  return TRI_SEQ[t->seq_step];
}

static uint8_t noise_output(const ApuNoise *n) {
  if (!n->enabled || n->length == 0 || (n->lfsr & 1u) != 0) {
    return 0;
  }
  return env_volume(n->env_v, n->env_decay, n->env_const);
}

static float mix(unsigned p1, unsigned p2, unsigned t, unsigned n) {
  /* NESdev non-linear approximation; DMC is 0 (unused by SMB) */
  float pulse_out = 0.0f;
  if (p1 + p2 > 0) {
    pulse_out = 95.88f / ((8128.0f / (float)(p1 + p2)) + 100.0f);
  }
  float tnd_out = 0.0f;
  double tnd = (double)t / 8227.0 + (double)n / 12241.0;
  if (tnd > 0.0) {
    tnd_out = (float)(159.79 / ((1.0 / tnd) + 100.0));
  }
  return pulse_out + tnd_out; /* 0..~0.3 */
}

/* --------------------------------------------------------- driver */

size_t apu_synth_frame(ApuSynth *synth, Apu *apu, float *out) {
  for (uint8_t i = 0; i < apu->log_len; i++) {
    apply_write(synth, apu->log[i].addr, apu->log[i].value);
  }
  apu->log_len = 0;

  double target = synth->frame_carry + SMBC_CYCLES_PER_FRAME;
  unsigned cycles = (unsigned)target;
  synth->frame_carry = target - (double)cycles;

  const double *steps = synth->seq_5step ? SEQ5 : SEQ4;
  size_t samples = 0;
  for (unsigned cycle = 0; cycle < cycles; cycle++) {
    /* frame sequencer */
    synth->seq_cycle += 1.0;
    if (synth->seq_step < 4 && synth->seq_cycle >= steps[synth->seq_step]) {
      bool half = synth->seq_5step
                      ? (synth->seq_step == 0 || synth->seq_step == 2)
                      : (synth->seq_step == 1 || synth->seq_step == 3);
      apu_synth_clock_qh(synth, half);
      synth->seq_step++;
      if (synth->seq_step == 4) {
        synth->seq_step = 0;
        synth->seq_cycle = 0.0;
      }
    }

    clock_channels(synth);

    synth->sample_cycle += 1.0;
    if (synth->sample_cycle >= SMBC_CYCLES_PER_SAMPLE) {
      synth->sample_cycle -= SMBC_CYCLES_PER_SAMPLE;
      out[samples++] = mix(pulse_output(&synth->pulse[0], false),
                           pulse_output(&synth->pulse[1], true),
                           tri_output(&synth->tri),
                           noise_output(&synth->noise));
    }
  }

  apu->status4015 = (uint8_t)((synth->pulse[0].length > 0 ? 0x01u : 0u) |
                              (synth->pulse[1].length > 0 ? 0x02u : 0u) |
                              (synth->tri.length > 0 ? 0x04u : 0u) |
                              (synth->noise.length > 0 ? 0x08u : 0u));
  return samples;
}
