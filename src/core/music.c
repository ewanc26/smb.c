/* core/music.c — the music sequencer.
 *
 * Owns: smb.asm: MusicHandler and everything it calls that the sfx atoms do
 * not — header loading, the square 2 / square 1 / triangle / noise music
 * routines, length and envelope processing, and end-of-data handling.
 * Called once per frame from core/sound (RunSoundSubroutines).
 *
 * Song data lives in data/music as one ROM-order blob assembled at $f9b8;
 * the two-byte MusicData pointer in RAM indexes it directly. The header
 * blocks keep the disassembly's addressing: offsets are relative to the
 * start of the offset table, which the headers follow immediately.
 */
#include "frame.h"
#include "internal.h"
#include "state.h"
#include "../data/music.h"
#include "../hw/apu.h"

/* smb.asm: SND_MASTERCTRL_REG/SND_SQUARE1_REG/SND_SQUARE2_REG/
 * SND_TRIANGLE_REG/SND_NOISE_REG */
#define APU_MASTERCTRL 0x4015u
#define APU_SQUARE1    0x4000u
#define APU_SQUARE2    0x4004u
#define APU_TRIANGLE   0x4008u
#define APU_NOISE      0x400cu

/* smb.asm: lda (MusicData),y — the pointer in $f5/$f6 addresses the song
 * blob at its assembled base. */
static uint8_t music_data_read(const Smbc *smbc, uint8_t y) {
  const CoreState *st = &smbc->state;
  uint16_t ptr = (uint16_t)(st->ram[SMBC_MUSIC_DATA_LOW] |
                            ((uint16_t)st->ram[SMBC_MUSIC_DATA_HIGH] << 8));
  return SMBC_MUSIC_ROM[ptr - SMBC_MUSIC_ROM_BASE + y];
}

/* smb.asm: MusicHeaderData — the offset table and the headers form one
 * addressing region; offsets index from the region's start. */
static uint8_t mhd_read(uint8_t index) {
  if (index < SMBC_MUSIC_HEADER_OFFSET_DATA_SIZE) {
    return SMBC_MUSIC_HEADER_OFFSET_DATA[index];
  }
  return SMBC_MUSIC_HEADER_DATA[index - SMBC_MUSIC_HEADER_OFFSET_DATA_SIZE];
}

/* smb.asm: the envelope tables sit contiguously in ROM (EndOfCastle, Area,
 * WaterEvent, BowserFlame, BrickShatter) and reads spill across their
 * boundaries, so envelope lookup is over the combined region. */
static uint8_t env_read(uint8_t base, uint8_t y) {
  uint8_t i = (uint8_t)(base + y);
  if (i < SMBC_END_OF_CASTLE_MUSIC_ENV_DATA_SIZE) {
    return SMBC_END_OF_CASTLE_MUSIC_ENV_DATA[i];
  }
  i = (uint8_t)(i - SMBC_END_OF_CASTLE_MUSIC_ENV_DATA_SIZE);
  if (i < SMBC_AREA_MUSIC_ENV_DATA_SIZE) {
    return SMBC_AREA_MUSIC_ENV_DATA[i];
  }
  i = (uint8_t)(i - SMBC_AREA_MUSIC_ENV_DATA_SIZE);
  if (i < SMBC_WATER_EVENT_MUS_ENV_DATA_SIZE) {
    return SMBC_WATER_EVENT_MUS_ENV_DATA[i];
  }
  i = (uint8_t)(i - SMBC_WATER_EVENT_MUS_ENV_DATA_SIZE);
  if (i < SMBC_BOWSER_FLAME_ENV_DATA_SIZE) {
    return SMBC_BOWSER_FLAME_ENV_DATA[i];
  }
  i = (uint8_t)(i - SMBC_BOWSER_FLAME_ENV_DATA_SIZE);
  return SMBC_BRICK_SHATTER_ENV_DATA[i];
}

#define ENV_END_OF_CASTLE_BASE 0x00u
#define ENV_AREA_BASE          SMBC_END_OF_CASTLE_MUSIC_ENV_DATA_SIZE
#define ENV_WATER_EVENT_BASE   (ENV_AREA_BASE + SMBC_AREA_MUSIC_ENV_DATA_SIZE)

/* smb.asm: ProcessLengthData — the three low bits plus the header's length
 * offset (and the time-running-out adder) index MusicLengthLookupTbl. */
static uint8_t process_length_data_idx(Smbc *smbc, uint8_t a, uint8_t *idx) {
  CoreState *st = &smbc->state;
  uint8_t y = (uint8_t)((a & 0x07u) + st->ram[SMBC_NOTE_LEN_LOOKUP_TBL_OFS] +
                        st->ram[SMBC_NOTE_LENGTH_TBL_ADDER]);
  if (idx != 0) {
    *idx = y;
  }
  return SMBC_MUSIC_LENGTH_LOOKUP_TBL[y];
}

static uint8_t process_length_data(Smbc *smbc, uint8_t a) {
  return process_length_data_idx(smbc, a, 0);
}

/* smb.asm: AlternateLengthHandler — length bits arrive as d0,d7,d6; rotate
 * them into d2,d1,d0, then process as length data. Returns the length; the
 * original byte and the length-table index are handed back through orig and
 * idx (the asm leaves them in X and Y — the silent beat writes both). */
static uint8_t alternate_length_handler(Smbc *smbc, uint8_t a, uint8_t *orig,
                                        uint8_t *idx) {
  *orig = a;
  uint8_t length_bits = (uint8_t)(((a & 0x01u) << 2) | ((a & 0x80u) >> 6) |
                                  ((a & 0x40u) >> 6));
  return process_length_data_idx(smbc, length_bits, idx);
}

/* smb.asm: LoadControlRegs — returns the envelope start in a and the control
 * reg contents in x/y (0x82/0x7f). */
static uint8_t load_control_regs(Smbc *smbc, uint8_t *x, uint8_t *y) {
  CoreState *st = &smbc->state;
  uint8_t a;
  if ((st->ram[SMBC_EVENT_MUSIC_BUFFER] & SMBC_EVENT_MUSIC_END_OF_CASTLE) != 0) {
    a = 0x04u;
  } else if ((st->ram[SMBC_AREA_MUSIC_BUFFER] & 0x7du) == 0) {
    a = 0x28u; /* water music and all other event music */
  } else {
    a = 0x08u;
  }
  *x = 0x82u;
  *y = 0x7fu;
  return a;
}

/* smb.asm: LoadEnvelopeData */
static uint8_t load_envelope_data(Smbc *smbc, uint8_t y) {
  CoreState *st = &smbc->state;
  if ((st->ram[SMBC_EVENT_MUSIC_BUFFER] & SMBC_EVENT_MUSIC_END_OF_CASTLE) != 0) {
    return env_read(ENV_END_OF_CASTLE_BASE, y);
  }
  if ((st->ram[SMBC_AREA_MUSIC_BUFFER] & 0x7du) != 0) {
    return env_read(ENV_AREA_BASE, y);
  }
  return env_read(ENV_WATER_EVENT_BASE, y);
}

static void handle_square1_music(Smbc *smbc);
static void handle_triangle_music(Smbc *smbc);
static void handle_noise_music(Smbc *smbc);
static void handle_square2_music(Smbc *smbc);

/* smb.asm: LoadEventMusic */
static void load_event_music(Smbc *smbc, uint8_t a);

/* smb.asm: LoadHeader — Y selects the entry through the offset table. */
static void load_header(Smbc *smbc, uint8_t y) {
  CoreState *st = &smbc->state;
  uint8_t ofs = mhd_read((uint8_t)(y - 1u)); /* MusicHeaderOffsetData = MHD-1 */
  st->ram[SMBC_NOTE_LEN_LOOKUP_TBL_OFS] = mhd_read(ofs);
  st->ram[SMBC_MUSIC_DATA_LOW] = mhd_read((uint8_t)(ofs + 1u));
  st->ram[SMBC_MUSIC_DATA_HIGH] = mhd_read((uint8_t)(ofs + 2u));
  st->ram[SMBC_MUSIC_OFFSET_TRIANGLE] = mhd_read((uint8_t)(ofs + 3u));
  st->ram[SMBC_MUSIC_OFFSET_SQUARE1] = mhd_read((uint8_t)(ofs + 4u));
  st->ram[SMBC_MUSIC_OFFSET_NOISE] = mhd_read((uint8_t)(ofs + 5u));
  st->ram[SMBC_NOISE_DATA_LOOPBACK_OFS] = st->ram[SMBC_MUSIC_OFFSET_NOISE];
  st->ram[SMBC_SQU2_NOTE_LEN_COUNTER] = 0x01u;
  st->ram[SMBC_SQU1_NOTE_LEN_COUNTER] = 0x01u;
  st->ram[SMBC_TRI_NOTE_LEN_COUNTER] = 0x01u;
  st->ram[SMBC_NOISE_BEAT_LEN_COUNTER] = 0x01u;
  st->ram[SMBC_MUSIC_OFFSET_SQUARE2] = 0x00u;
  st->ram[SMBC_ALT_REG_CONTENT_FLAG] = 0x00u;
  apu_write(&smbc->apu, APU_MASTERCTRL, 0x0bu);
  apu_write(&smbc->apu, APU_MASTERCTRL, 0x0fu);
}

/* smb.asm: FindAreaMusicHeader/FindEventMusicHeader — count down the music
 * constant's set bit into a header index, starting from the given base. */
static uint8_t find_music_header(uint8_t y, uint8_t a) {
  uint8_t carry;
  do {
    y++;
    carry = (uint8_t)(a & 0x01u);
    a >>= 1;
  } while (carry == 0);
  return y;
}

/* smb.asm: HandleAreaMusicLoopB — (re)start area music; also the loopback
 * path when a song's data runs out. */
static void handle_area_music_loop_b(Smbc *smbc, uint8_t a) {
  CoreState *st = &smbc->state;
  st->ram[SMBC_EVENT_MUSIC_BUFFER] = 0x00u;
  st->ram[SMBC_AREA_MUSIC_BUFFER] = a;
  if (a != SMBC_MUSIC_GROUND) {
    /* smb.asm: FindAreaMusicHeader — Y starts at 8 for area music (the
     * residual store of 8 to MusicOffset_Square2 is overwritten by
     * LoadHeader before it can be read, so it is not modelled) */
    load_header(smbc, find_music_header(0x08u, a));
    return;
  }
  /* ground level music walks its layout through GroundMusicHeaderOfs; when
   * the walk reaches $32 the offset is re-armed at $11 and the walk resumes
   * at $12, so the lead-in entry only plays once (smb.asm: GMLoopB) */
  uint8_t y = (uint8_t)(st->ram[SMBC_GROUND_MUSIC_HEADER_OFS] + 1u);
  if (y == 0x32u) {
    st->ram[SMBC_GROUND_MUSIC_HEADER_OFS] = 0x11u;
    y = 0x12u;
  }
  st->ram[SMBC_GROUND_MUSIC_HEADER_OFS] = y;
  load_header(smbc, y);
}

/* smb.asm: LoadAreaMusic */
static void load_area_music(Smbc *smbc, uint8_t a) {
  CoreState *st = &smbc->state;
  if (a == SMBC_MUSIC_UNDERGROUND) {
    sound_stop_square1_sfx(smbc);
  }
  st->ram[SMBC_GROUND_MUSIC_HEADER_OFS] = 0x10u;
  handle_area_music_loop_b(smbc, a);
}

static void load_event_music(Smbc *smbc, uint8_t a) {
  CoreState *st = &smbc->state;
  st->ram[SMBC_EVENT_MUSIC_BUFFER] = a;
  if (a == SMBC_EVENT_MUSIC_DEATH) {
    sound_stop_square1_sfx(smbc);
    sound_stop_square2_sfx(smbc);
  }
  st->ram[SMBC_AREA_MUSIC_BUFFER_ALT] = st->ram[SMBC_AREA_MUSIC_BUFFER];
  st->ram[SMBC_NOTE_LENGTH_TBL_ADDER] = 0x00u;
  st->ram[SMBC_AREA_MUSIC_BUFFER] = 0x00u;
  if (a == SMBC_EVENT_MUSIC_TIME_RUNNING_OUT) {
    st->ram[SMBC_NOTE_LENGTH_TBL_ADDER] = 0x08u;
  }
  load_header(smbc, find_music_header(0x00u, a));
}

/* smb.asm: EndOfMusicData — returns whether a new song was loaded (the
 * loopback paths fall from LoadHeader into HandleSquare2Music) or the
 * engine was silenced. */
static bool end_of_music_data(Smbc *smbc) {
  CoreState *st = &smbc->state;
  uint8_t a = st->ram[SMBC_EVENT_MUSIC_BUFFER];
  if (a == SMBC_EVENT_MUSIC_TIME_RUNNING_OUT) {
    a = st->ram[SMBC_AREA_MUSIC_BUFFER_ALT];
    if (a != 0) {
      handle_area_music_loop_b(smbc, a);
      return true;
    }
    a = 0x00u; /* alt was empty: the victory check below sees zero */
  } else {
    a = (uint8_t)(a & SMBC_EVENT_MUSIC_VICTORY);
  }
  if (a != 0) { /* smb.asm: VictoryMLoopBack */
    load_event_music(smbc, a);
    return true;
  }
  a = (uint8_t)(st->ram[SMBC_AREA_MUSIC_BUFFER] & 0x5fu);
  if (a != 0) { /* smb.asm: MusicLoopBack — any area music but pipe intro */
    handle_area_music_loop_b(smbc, a);
    return true;
  }
  st->ram[SMBC_AREA_MUSIC_BUFFER] = 0x00u;
  st->ram[SMBC_EVENT_MUSIC_BUFFER] = 0x00u;
  apu_write(&smbc->apu, APU_TRIANGLE, 0x00u);
  apu_write(&smbc->apu, APU_SQUARE1, 0x90u);
  apu_write(&smbc->apu, APU_SQUARE2, 0x90u);
  return false;
}

/* smb.asm: HandleSquare2Music */
static void handle_square2_music(Smbc *smbc) {
  CoreState *st = &smbc->state;
  for (;;) {
    if (--st->ram[SMBC_SQU2_NOTE_LEN_COUNTER] != 0) {
      break;
    }
    uint8_t y = st->ram[SMBC_MUSIC_OFFSET_SQUARE2]++;
    uint8_t a = music_data_read(smbc, y);
    if (a == 0) {
      if (!end_of_music_data(smbc)) {
        return;
      }
      continue; /* a new song was loaded; its header run starts here */
    }
    if ((a & 0x80u) != 0) { /* smb.asm: Squ2LengthHandler */
      st->ram[SMBC_SQU2_NOTE_LEN_BUFFER] = process_length_data(smbc, a);
      y = st->ram[SMBC_MUSIC_OFFSET_SQUARE2]++;
      a = music_data_read(smbc, y);
    }
    /* smb.asm: Squ2NoteHandler */
    if (st->ram[SMBC_SQUARE2_SOUND_BUFFER] == 0) {
      uint8_t x = 0x04u, yregs = a; /* rest: SetFreq leaves x=4, y=note */
      if (sound_set_freq_squ2(smbc, a)) {
        a = load_control_regs(smbc, &x, &yregs);
      } else {
        a = 0x00u;
      }
      st->ram[SMBC_SQU2_ENVELOPE_DATA_CTRL] = a;
      sound_dump_sq2_regs(smbc, x, yregs);
    }
    st->ram[SMBC_SQU2_NOTE_LEN_COUNTER] = st->ram[SMBC_SQU2_NOTE_LEN_BUFFER];
    break;
  }

  /* smb.asm: MiscSqu2MusicTasks */
  if (st->ram[SMBC_SQUARE2_SOUND_BUFFER] != 0) {
    handle_square1_music(smbc);
    return;
  }
  if ((st->ram[SMBC_EVENT_MUSIC_BUFFER] & 0x91u) != 0) {
    handle_square1_music(smbc);
    return;
  }
  uint8_t y = st->ram[SMBC_SQU2_ENVELOPE_DATA_CTRL];
  if (y != 0) {
    st->ram[SMBC_SQU2_ENVELOPE_DATA_CTRL]--;
  }
  apu_write(&smbc->apu, APU_SQUARE2, load_envelope_data(smbc, y));
  apu_write(&smbc->apu, APU_SQUARE2 + 1u, 0x7fu);
  handle_square1_music(smbc);
}

/* smb.asm: HandleSquare1Music */
static void handle_square1_music(Smbc *smbc) {
  CoreState *st = &smbc->state;
  if (st->ram[SMBC_MUSIC_OFFSET_SQUARE1] != 0) {
    if (--st->ram[SMBC_SQU1_NOTE_LEN_COUNTER] == 0) {
      uint8_t a;
      for (;;) { /* smb.asm: FetchSqu1MusicData */
        uint8_t y = st->ram[SMBC_MUSIC_OFFSET_SQUARE1]++;
        a = music_data_read(smbc, y);
        if (a != 0) {
          break;
        }
        apu_write(&smbc->apu, APU_SQUARE1, 0x83u);
        apu_write(&smbc->apu, APU_SQUARE1 + 1u, 0x94u);
        st->ram[SMBC_ALT_REG_CONTENT_FLAG] = 0x94u;
      }
      /* smb.asm: Squ1NoteHandler */
      uint8_t note, idx;
      st->ram[SMBC_SQU1_NOTE_LEN_COUNTER] =
          alternate_length_handler(smbc, a, &note, &idx);
      if (st->ram[SMBC_SQUARE1_SOUND_BUFFER] == 0) {
        uint8_t x = 0x00u, yregs = 0x00u;
        a = (uint8_t)(note & 0x3eu);
        if (sound_set_freq_squ1(smbc, a)) {
          a = load_control_regs(smbc, &x, &yregs);
        } else {
          a = 0x00u;
          x = 0x00u;
          yregs = (uint8_t)(note & 0x3eu); /* SetFreq leaves y=note */
        }
        st->ram[SMBC_SQU1_ENVELOPE_DATA_CTRL] = a;
        sound_dump_squ1_regs(smbc, x, yregs);
      }
    }

    /* smb.asm: MiscSqu1MusicTasks */
    if (st->ram[SMBC_SQUARE1_SOUND_BUFFER] == 0) {
      if ((st->ram[SMBC_EVENT_MUSIC_BUFFER] & 0x91u) == 0) {
        uint8_t y = st->ram[SMBC_SQU1_ENVELOPE_DATA_CTRL];
        if (y != 0) {
          st->ram[SMBC_SQU1_ENVELOPE_DATA_CTRL]--;
        }
        apu_write(&smbc->apu, APU_SQUARE1, load_envelope_data(smbc, y));
      }
      /* smb.asm: DeathMAltReg */
      uint8_t a = st->ram[SMBC_ALT_REG_CONTENT_FLAG];
      apu_write(&smbc->apu, APU_SQUARE1 + 1u, a != 0 ? a : 0x7fu);
    }
  }
  handle_triangle_music(smbc);
}

/* smb.asm: HandleTriangleMusic */
static void handle_triangle_music(Smbc *smbc) {
  CoreState *st = &smbc->state;
  if (--st->ram[SMBC_TRI_NOTE_LEN_COUNTER] == 0) {
    uint8_t y = st->ram[SMBC_MUSIC_OFFSET_TRIANGLE]++;
    uint8_t a = music_data_read(smbc, y);
    if (a != 0) {
      if ((a & 0x80u) != 0) { /* length data */
        st->ram[SMBC_TRI_NOTE_LEN_BUFFER] = process_length_data(smbc, a);
        apu_write(&smbc->apu, APU_TRIANGLE, 0x1fu);
        y = st->ram[SMBC_MUSIC_OFFSET_TRIANGLE]++;
        a = music_data_read(smbc, y);
      }
      if (a != 0) { /* smb.asm: TriNoteHandler */
        sound_set_freq_tri(smbc, a);
        uint8_t x = st->ram[SMBC_TRI_NOTE_LEN_BUFFER];
        st->ram[SMBC_TRI_NOTE_LEN_COUNTER] = x;
        if ((st->ram[SMBC_EVENT_MUSIC_BUFFER] & 0x6eu) != 0 ||
            (st->ram[SMBC_AREA_MUSIC_BUFFER] & 0x0au) != 0) {
          if (x >= 0x12u) {
            a = 0xffu;
          } else if ((st->ram[SMBC_EVENT_MUSIC_BUFFER] &
                      SMBC_EVENT_MUSIC_END_OF_CASTLE) != 0) {
            a = 0x0fu;
          } else {
            a = 0x1fu;
          }
          apu_write(&smbc->apu, APU_TRIANGLE, a);
        }
      } else {
        apu_write(&smbc->apu, APU_TRIANGLE, 0x00u);
      }
    } else {
      apu_write(&smbc->apu, APU_TRIANGLE, 0x00u);
    }
  }
  handle_noise_music(smbc);
}

/* smb.asm: HandleNoiseMusic */
static void handle_noise_music(Smbc *smbc) {
  CoreState *st = &smbc->state;
  if ((st->ram[SMBC_AREA_MUSIC_BUFFER] & 0xf3u) == 0) {
    return; /* smb.asm: ExitMusicHandler */
  }
  if (--st->ram[SMBC_NOISE_BEAT_LEN_COUNTER] != 0) {
    return;
  }
  uint8_t a;
  for (;;) { /* smb.asm: FetchNoiseBeatData */
    uint8_t y = st->ram[SMBC_MUSIC_OFFSET_NOISE]++;
    a = music_data_read(smbc, y);
    if (a != 0) {
      break;
    }
    st->ram[SMBC_MUSIC_OFFSET_NOISE] = st->ram[SMBC_NOISE_DATA_LOOPBACK_OFS];
  }
  /* smb.asm: NoiseBeatHandler */
  uint8_t beat, idx;
  st->ram[SMBC_NOISE_BEAT_LEN_COUNTER] =
      alternate_length_handler(smbc, a, &beat, &idx);
  a = (uint8_t)(beat & 0x3eu);
  uint8_t x, y;
  if (a == 0x30u) { /* long beat */
    a = 0x1cu; x = 0x03u; y = 0x58u;
  } else if (a == 0x20u) { /* strong beat */
    a = 0x1cu; x = 0x0cu; y = 0x18u;
  } else if ((a & 0x10u) != 0) { /* short beat */
    a = 0x1cu; x = 0x03u; y = 0x18u;
  } else { /* smb.asm: SilentBeat — X/Y keep the beat byte and length index */
    a = 0x10u; x = beat; y = idx;
  }
  /* smb.asm: PlayBeat */
  apu_write(&smbc->apu, APU_NOISE, a);
  apu_write(&smbc->apu, APU_NOISE + 2u, x);
  apu_write(&smbc->apu, APU_NOISE + 3u, y);
}

/* smb.asm: MusicHandler */
void sound_music_handler(Smbc *smbc) {
  CoreState *st = &smbc->state;
  uint8_t a = st->ram[SMBC_EVENT_MUSIC_QUEUE];
  if (a != 0) {
    load_event_music(smbc, a);
  } else {
    a = st->ram[SMBC_AREA_MUSIC_QUEUE];
    if (a != 0) {
      load_area_music(smbc, a);
    } else if ((st->ram[SMBC_EVENT_MUSIC_BUFFER] |
                st->ram[SMBC_AREA_MUSIC_BUFFER]) == 0) {
      return; /* no music, then leave */
    }
  }
  /* smb.asm: ContinueMusic */
  handle_square2_music(smbc);
}
