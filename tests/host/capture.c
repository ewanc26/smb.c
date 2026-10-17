/* tests/host/capture.c — PPM encoding and save-state files (#25, #28). */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../../src/core/replay.h"
#include "../../src/hw/render.h"
#include "../../src/host/capture.h"
#include "../../src/host/palette.h"

#define STATE_PATH "build/tests/host/capture-test.sav"
#define PPM_PATH "build/tests/host/capture-test.ppm"

int main(void) {
  char err[128];

  /* PPM: header, size and palette mapping */
  {
    size_t size = capture_ppm_size();
    uint8_t *out = malloc(size);
    assert(out != NULL);
    uint8_t fb[SMBC_RENDER_PIXELS];
    memset(fb, 0x0f, sizeof(fb));
    fb[SMBC_RENDER_PIXELS - 1] = 0x30;

    assert(capture_encode_ppm(fb, out, size - 1) == 0);
    size_t n = capture_encode_ppm(fb, out, size);
    assert(n == size);
    assert(memcmp(out, "P6\n256 240\n255\n", 15) == 0);

    size_t head = 15;
    assert(out[head] == SMBC_2C02_RGB[0x0f][0]);
    assert(out[head + 1] == SMBC_2C02_RGB[0x0f][1]);
    assert(out[head + 2] == SMBC_2C02_RGB[0x0f][2]);
    size_t last = head + (size_t)(SMBC_RENDER_PIXELS - 1) * 3u;
    assert(out[last] == SMBC_2C02_RGB[0x30][0]);
    assert(out[last + 2] == SMBC_2C02_RGB[0x30][2]);
    free(out);
  }

  /* PPM file: written in full to disk */
  {
    uint8_t fb[SMBC_RENDER_PIXELS];
    memset(fb, 0x21, sizeof(fb));
    assert(capture_write_ppm(PPM_PATH, fb, err, sizeof(err)));
    FILE *f = fopen(PPM_PATH, "rb");
    assert(f != NULL);
    fseek(f, 0, SEEK_END);
    assert((size_t)ftell(f) == capture_ppm_size());
    fclose(f);
    remove(PPM_PATH);
    assert(!capture_write_ppm("build/no-such-dir/x.ppm", fb, err, sizeof(err)));
    assert(strlen(err) > 0);
  }

  /* save state: write, then load into a machine that started elsewhere */
  {
    Smbc a, b;
    smbc_init(&a);
    for (unsigned i = 0; i < 150; i++) {
      joypad_set_buttons(&a.joypad1, i >= 40 && i < 90 ? 0x81u : 0x00u);
      smbc_frame(&a);
    }
    assert(capture_write_state(&a, STATE_PATH, err, sizeof(err)));

    smbc_init(&b);
    for (unsigned i = 0; i < 20; i++) {
      smbc_frame(&b);
    }
    assert(capture_read_state(&b, STATE_PATH, err, sizeof(err)));
    assert(smbc_state_hash(&a) == smbc_state_hash(&b));
  }

  /* a corrupt or missing file is refused and leaves the machine untouched */
  {
    Smbc victim;
    smbc_init(&victim);
    for (unsigned i = 0; i < 30; i++) {
      smbc_frame(&victim);
    }
    uint64_t before = smbc_state_hash(&victim);

    FILE *f = fopen(STATE_PATH, "wb");
    assert(f != NULL);
    fputs("not a save state", f);
    fclose(f);
    assert(!capture_read_state(&victim, STATE_PATH, err, sizeof(err)));
    assert(strcmp(err, "not a valid save state") == 0);
    assert(smbc_state_hash(&victim) == before);

    assert(!capture_read_state(&victim, "build/tests/host/missing.sav", err,
                               sizeof(err)));
    assert(strcmp(err, "cannot open for reading") == 0);
    assert(smbc_state_hash(&victim) == before);
  }
  remove(STATE_PATH);

  printf("capture: ok\n");
  return 0;
}
