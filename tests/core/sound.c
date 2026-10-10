/* tests/core/sound.c — exercises src/core/sound.c, sfx.c and music.c: the
 * pause jingle, square 1/2 and noise sfx dispatch, music header loads and
 * the first frame of the ground-level lead-in, silence, and the DAC
 * counter. Register expectations are derived from the ROM tables in
 * data/music. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/sound.h"
#include "../../src/core/state.h"
#include "../../src/data/music.h"

static void pause_jingle(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;

  /* fresh pause: the jingle starts, buffers are silenced, sound re-enabled */
  smbc.state.ram[SMBC_PAUSE_SOUND_QUEUE] = 0x01u;
  sound_engine(&smbc);
  assert(smbc.state.ram[SMBC_PAUSE_SOUND_BUFFER] == 0x01u);
  assert(smbc.state.ram[SMBC_PAUSE_MODE_FLAG] == 0x01u);
  assert(smbc.state.ram[SMBC_SQU1_SFX_LEN_COUNTER] == 0x29u);
  assert(smbc.apu.regs[0x15] == 0x0fu);
  assert(smbc.apu.regs[0x00] == 0x84u);
  assert(smbc.apu.regs[0x01] == 0x7fu);
  assert(smbc.apu.regs[0x02] == 0x54u);
  assert(smbc.apu.regs[0x03] == 0x08u);
  assert(smbc.state.ram[SMBC_PAUSE_SOUND_QUEUE] == 0x00u);

  /* at length $24 the second tone plays */
  smbc.state.ram[SMBC_SQU1_SFX_LEN_COUNTER] = 0x24u;
  sound_engine(&smbc);
  assert(smbc.apu.regs[0x02] == 0x6au);
  assert(smbc.state.ram[SMBC_SQU1_SFX_LEN_COUNTER] == 0x23u);

  /* when the counter expires the buffer clears and sound cuts off */
  smbc.state.ram[SMBC_SQU1_SFX_LEN_COUNTER] = 0x01u;
  sound_engine(&smbc);
  assert(smbc.state.ram[SMBC_PAUSE_SOUND_BUFFER] == 0x00u);
  assert(smbc.apu.regs[0x15] == 0x00u);

  /* unpausing (queue 2) replays the jingle and clears pause mode at the end */
  smbc.state.ram[SMBC_PAUSE_SOUND_QUEUE] = 0x02u;
  sound_engine(&smbc);
  assert(smbc.state.ram[SMBC_PAUSE_SOUND_BUFFER] == 0x02u);
  smbc.state.ram[SMBC_SQU1_SFX_LEN_COUNTER] = 0x01u;
  sound_engine(&smbc);
  assert(smbc.state.ram[SMBC_PAUSE_MODE_FLAG] == 0x00u);
  assert(smbc.state.ram[SMBC_PAUSE_SOUND_BUFFER] == 0x00u);
}

static void square1_sfx(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;

  /* small jump: regs dump, frequency from the table, length counter set */
  smbc.state.ram[SMBC_SQUARE1_SOUND_QUEUE] = SMBC_SFX_SMALL_JUMP;
  sound_engine(&smbc);
  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_BUFFER] == SMBC_SFX_SMALL_JUMP);
  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_QUEUE] == 0x00u);
  assert(smbc.state.ram[SMBC_SQU1_SFX_LEN_COUNTER] == 0x27u);
  assert(smbc.apu.regs[0x00] == 0x82u);
  assert(smbc.apu.regs[0x01] == 0xa7u);
  assert(smbc.apu.regs[0x02] == 0xfeu);
  assert(smbc.apu.regs[0x03] == 0x08u);

  /* at length $25 the second jump part dumps into the control regs */
  smbc.state.ram[SMBC_SQU1_SFX_LEN_COUNTER] = 0x25u;
  sound_engine(&smbc);
  assert(smbc.apu.regs[0x00] == 0x5fu);
  assert(smbc.apu.regs[0x01] == 0xf6u);
  assert(smbc.state.ram[SMBC_SQU1_SFX_LEN_COUNTER] == 0x24u);

  /* at length $20 the third part */
  smbc.state.ram[SMBC_SQU1_SFX_LEN_COUNTER] = 0x20u;
  sound_engine(&smbc);
  assert(smbc.apu.regs[0x00] == 0x48u);
  assert(smbc.apu.regs[0x01] == 0xbcu);

  /* length running out stops the channel and clears the buffer */
  smbc.state.ram[SMBC_SQU1_SFX_LEN_COUNTER] = 0x01u;
  sound_engine(&smbc);
  assert(smbc.state.ram[SMBC_SQUARE1_SOUND_BUFFER] == 0x00u);
  assert(smbc.apu.regs[0x15] == 0x0fu);
}

static void square2_sfx(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;

  /* coin grab */
  smbc.state.ram[SMBC_SQUARE2_SOUND_QUEUE] = SMBC_SFX_COIN_GRAB;
  sound_engine(&smbc);
  assert(smbc.state.ram[SMBC_SQUARE2_SOUND_BUFFER] == SMBC_SFX_COIN_GRAB);
  assert(smbc.state.ram[SMBC_SQU2_SFX_LEN_COUNTER] == 0x34u);
  assert(smbc.apu.regs[0x04] == 0x8du);
  assert(smbc.apu.regs[0x05] == 0x7fu);
  assert(smbc.apu.regs[0x06] == 0x71u);
  assert(smbc.apu.regs[0x07] == 0x08u);

  /* at length $30 the coin's second tone touches the low frequency reg */
  smbc.state.ram[SMBC_SQU2_SFX_LEN_COUNTER] = 0x30u;
  sound_engine(&smbc);
  assert(smbc.apu.regs[0x06] == 0x54u);
  assert(smbc.state.ram[SMBC_SQU2_SFX_LEN_COUNTER] == 0x2fu);
}

static void noise_sfx(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;

  /* brick shatter: even length counters skip the tone write */
  smbc.state.ram[SMBC_NOISE_SOUND_QUEUE] = SMBC_SFX_BRICK_SHATTER;
  sound_engine(&smbc);
  assert(smbc.state.ram[SMBC_NOISE_SOUND_BUFFER] == SMBC_SFX_BRICK_SHATTER);
  assert(smbc.state.ram[SMBC_NOISE_SFX_LEN_COUNTER] == 0x1fu);

  /* odd counter: envelope and period from the shatter tables */
  sound_engine(&smbc);
  assert(smbc.apu.regs[0x0c] == 0x1fu);
  assert(smbc.apu.regs[0x0e] == 0x0cu);
  assert(smbc.apu.regs[0x0f] == 0x18u);
  assert(smbc.state.ram[SMBC_NOISE_SFX_LEN_COUNTER] == 0x1eu);
}

static void ground_music_first_frame(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;

  /* ground music loads the lead-in header (GroundLevelLeadInHdr) */
  smbc.state.ram[SMBC_AREA_MUSIC_QUEUE] = SMBC_MUSIC_GROUND;
  sound_engine(&smbc);
  assert(smbc.state.ram[SMBC_GROUND_MUSIC_HEADER_OFS] == 0x11u);
  assert(smbc.state.ram[SMBC_NOTE_LEN_LOOKUP_TBL_OFS] == 0x18u);
  assert(smbc.state.ram[SMBC_MUSIC_DATA_LOW] == 0xf9u);
  assert(smbc.state.ram[SMBC_MUSIC_DATA_HIGH] == 0xfau);
  assert(smbc.state.ram[SMBC_AREA_MUSIC_BUFFER] == SMBC_MUSIC_GROUND);
  assert(smbc.state.ram[SMBC_AREA_MUSIC_QUEUE] == 0x00u);
  assert(smbc.apu.regs[0x15] == 0x0fu);

  /* square 2: $82 is a length byte, $34 the first note; the envelope write
   * spills past AreaMusicEnvData into WaterEventMusEnvData (ROM layout) */
  assert(smbc.state.ram[SMBC_MUSIC_OFFSET_SQUARE2] == 0x02u);
  assert(smbc.state.ram[SMBC_SQU2_NOTE_LEN_BUFFER] == 0x09u);
  assert(smbc.state.ram[SMBC_SQU2_NOTE_LEN_COUNTER] == 0x09u);
  assert(smbc.state.ram[SMBC_SQU2_ENVELOPE_DATA_CTRL] == 0x07u);
  assert(smbc.apu.regs[0x04] == 0x90u);
  assert(smbc.apu.regs[0x05] == 0x7fu);
  assert(smbc.apu.regs[0x06] == 0xa9u);
  assert(smbc.apu.regs[0x07] == 0x08u);

  /* square 1: first byte is a note; envelope and residual alt-reg follow */
  assert(smbc.state.ram[SMBC_MUSIC_OFFSET_SQUARE1] == 0x0eu);
  assert(smbc.state.ram[SMBC_SQU1_NOTE_LEN_COUNTER] == 0x09u);
  assert(smbc.state.ram[SMBC_SQU1_ENVELOPE_DATA_CTRL] == 0x07u);
  assert(smbc.state.ram[SMBC_ALT_REG_CONTENT_FLAG] == 0x00u);
  assert(smbc.apu.regs[0x00] == 0x90u);
  assert(smbc.apu.regs[0x01] == 0x7fu);
  assert(smbc.apu.regs[0x02] == 0x2eu);
  assert(smbc.apu.regs[0x03] == 0x09u);

  /* triangle: $82 length then $18 note; the ctrl reg keeps the length-path
   * value because ground music is not in the $0a mask */
  assert(smbc.state.ram[SMBC_MUSIC_OFFSET_TRIANGLE] == 0x17u);
  assert(smbc.state.ram[SMBC_TRI_NOTE_LEN_BUFFER] == 0x09u);
  assert(smbc.state.ram[SMBC_TRI_NOTE_LEN_COUNTER] == 0x09u);
  assert(smbc.apu.regs[0x08] == 0x1fu);

  /* noise: beat byte $31 is a long beat */
  assert(smbc.state.ram[SMBC_MUSIC_OFFSET_NOISE] == 0x22u);
  assert(smbc.state.ram[SMBC_NOISE_BEAT_LEN_COUNTER] == 0x12u);
  assert(smbc.apu.regs[0x0c] == 0x1cu);
  assert(smbc.apu.regs[0x0e] == 0x03u);
  assert(smbc.apu.regs[0x0f] == 0x58u);

  /* the DAC counter rises while ground music plays */
  assert(smbc.state.ram[SMBC_DAC_COUNTER] == 0x01u);
  assert(smbc.apu.regs[0x11] == 0x00u); /* $4011 lags: sty stores the pre-inc value */
}

static void silence_and_dac(void) {
  Smbc smbc;
  smbc_init(&smbc);
  smbc.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;

  /* silence music runs one empty song and shuts the tonal channels down */
  smbc.state.ram[SMBC_AREA_MUSIC_QUEUE] = SMBC_MUSIC_SILENCE;
  sound_engine(&smbc);
  assert(smbc.state.ram[SMBC_AREA_MUSIC_BUFFER] == 0x00u);
  assert(smbc.state.ram[SMBC_EVENT_MUSIC_BUFFER] == 0x00u);
  assert(smbc.apu.regs[0x00] == 0x90u);
  assert(smbc.apu.regs[0x04] == 0x90u);
  assert(smbc.apu.regs[0x08] == 0x00u);

  /* with no music the DAC counter winds down and mirrors into $4011 */
  smbc.state.ram[SMBC_DAC_COUNTER] = 0x05u;
  sound_engine(&smbc);
  assert(smbc.state.ram[SMBC_DAC_COUNTER] == 0x04u);
  assert(smbc.apu.regs[0x11] == 0x05u); /* $4011 lags one frame here too */

  /* at zero it stays at zero */
  smbc.state.ram[SMBC_DAC_COUNTER] = 0x00u;
  sound_engine(&smbc);
  assert(smbc.state.ram[SMBC_DAC_COUNTER] == 0x00u);
}

int main(void) {
  pause_jingle();
  square1_sfx();
  square2_sfx();
  noise_sfx();
  ground_music_first_frame();
  silence_and_dac();
  printf("tests/core/sound: ok\n");
  return 0;
}
