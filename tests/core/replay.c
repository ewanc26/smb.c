/* tests/core/replay.c — recording, loading, replay boundaries and
 * malformed files (#37). */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/core/replay.h"
#include "../../src/hw/joypad.h"

/* a fixed input script: press Start, then hold Right with an A press */
static uint8_t script_input(unsigned frame) {
  if (frame == 30) return 0x10u; /* Start */
  if (frame == 31) return 0x00u;
  if (frame >= 60) return (frame % 90 < 45) ? 0x81u : 0x01u; /* Right ± A */
  return 0x00u;
}

static void record_script(SmbcReplay *replay, unsigned frames) {
  Smbc smbc;
  smbc_init(&smbc);
  replay_init(replay);
  for (unsigned i = 0; i < frames; i++) {
    uint8_t in = script_input(i);
    joypad_set_buttons(&smbc.joypad1, in);
    smbc_frame(&smbc);
    assert(replay_append(replay, in, 0x00u, smbc_state_hash(&smbc)));
  }
}

int main(void) {
  char err[128];

  /* hash stability: identical input sequences hash identically */
  {
    Smbc a, b;
    smbc_init(&a);
    smbc_init(&b);
    for (unsigned i = 0; i < 120; i++) {
      joypad_set_buttons(&a.joypad1, script_input(i));
      joypad_set_buttons(&b.joypad1, script_input(i));
      smbc_frame(&a);
      smbc_frame(&b);
    }
    assert(smbc_state_hash(&a) == smbc_state_hash(&b));
    /* a different sequence diverges — the differing input is still
     * held at the end, so the joypad bitmap alone distinguishes them */
    Smbc c;
    smbc_init(&c);
    for (unsigned i = 0; i < 120; i++) {
      joypad_set_buttons(&c.joypad1, i >= 110 ? 0x02u : script_input(i));
      smbc_frame(&c);
    }
    assert(smbc_state_hash(&a) != smbc_state_hash(&c));
  }

  /* replay from a fresh reset reproduces the recording exactly */
  {
    SmbcReplay rec;
    record_script(&rec, 300);
    Smbc smbc;
    smbc_init(&smbc);
    uint32_t diverge = 0;
    uint64_t got = 0, want = 0;
    uint32_t ran = replay_run(&smbc, &rec, rec.count, &diverge, &got, &want);
    assert(ran == 300);
    assert(diverge == SMBC_REPLAY_NO_DIVERGENCE);
    replay_free(&rec);
  }

  /* save/load round trip, then a clean replay of the loaded file.
   * tests run from the repo root with build/ present, so a fixed
   * path there avoids the POSIX-visibility mess around mkstemp */
  const char *path = "build/test-replay.bin";
  {
    SmbcReplay rec;
    record_script(&rec, 300);
    assert(replay_save(&rec, path, err, sizeof(err)));

    SmbcReplay loaded;
    assert(replay_load(&loaded, path, err, sizeof(err)));
    assert(loaded.count == rec.count);
    for (uint32_t i = 0; i < rec.count; i++) {
      assert(loaded.frames[i].p1 == rec.frames[i].p1);
      assert(loaded.frames[i].hash == rec.frames[i].hash);
    }
    Smbc smbc;
    smbc_init(&smbc);
    uint32_t diverge = 0;
    assert(replay_run(&smbc, &loaded, loaded.count, &diverge, NULL, NULL) ==
           300);
    assert(diverge == SMBC_REPLAY_NO_DIVERGENCE);
    replay_free(&loaded);
    replay_free(&rec);
  }

  /* stop_frame boundary: an early stop runs exactly that many frames */
  {
    SmbcReplay rec;
    record_script(&rec, 300);
    Smbc smbc;
    smbc_init(&smbc);
    uint32_t diverge = 0;
    assert(replay_run(&smbc, &rec, 100, &diverge, NULL, NULL) == 100);
    assert(diverge == SMBC_REPLAY_NO_DIVERGENCE);
    assert(smbc.state.ram[SMBC_FRAME_COUNTER] != 0);
    replay_free(&rec);
  }

  /* divergence: a corrupted expected hash is reported at its frame */
  {
    SmbcReplay rec;
    record_script(&rec, 300);
    rec.frames[150].hash ^= 0xffu;
    Smbc smbc;
    smbc_init(&smbc);
    uint32_t diverge = 0;
    uint64_t got = 0, want = 0;
    assert(replay_run(&smbc, &rec, rec.count, &diverge, &got, &want) == 150);
    assert(diverge == 150);
    assert(got == (want ^ 0xffu));
    replay_free(&rec);
  }

  /* malformed files fail safely with a message */
  {
    SmbcReplay rec;
    FILE *f = fopen(path, "wb");
    fputs("NOTAREPLAY......", f); /* bad magic */
    fclose(f);
    assert(!replay_load(&rec, path, err, sizeof(err)));
    assert(strstr(err, "magic") != NULL);

    f = fopen(path, "wb");
    fwrite("SMBCRPLY", 1, 8, f); /* valid magic, version 99 */
    fputc(99, f);
    fputc(0, f);
    fputc(0, f);
    fputc(0, f);
    for (int i = 0; i < 4; i++) fputc(0, f); /* count = 0 */
    fclose(f);
    assert(!replay_load(&rec, path, err, sizeof(err)));
    assert(strstr(err, "version") != NULL);

    f = fopen(path, "wb");
    fwrite("SMBCRPLY", 1, 8, f); /* truncated: header says nothing */
    fclose(f);
    assert(!replay_load(&rec, path, err, sizeof(err)));
    assert(strstr(err, "truncated") != NULL);

    assert(!replay_load(&rec, "/nonexistent/replay.bin", err, sizeof(err)));
    remove(path);
  }

  printf("tests/core/replay: ok\n");
  return 0;
}
