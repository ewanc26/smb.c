/* tests/reference/compare.c — frame-by-frame reference comparison
 * (#36).
 *
 * Each scenario is a replay recording plus a baseline frame captured
 * at a fixed frame number. Strict mode (no arguments, what `make
 * test` runs) replays the recording from a fresh reset — hash-checked
 * every frame, so simulation regressions fail before pixels are even
 * compared — renders the checkpoint frame and compares it against the
 * baseline pixel by pixel at the palette-index level.
 *
 * The baselines are SELF-CAPTURED from this port (see
 * tests/reference/README.md): they pin current behaviour so
 * regressions are measurable, but they are not verified against NES
 * hardware or a reference emulator. The scenario format is
 * replay-driven so real reference captures can drop in later.
 *
 * `./build/tests/reference/compare --update` regenerates every
 * replay and baseline from the compiled-in input scripts. Updating
 * is loud and manual by design — never silently re-baseline to hide
 * a regression (the diff outputs exist to review first). */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/core/frame.h"
#include "../../src/core/replay.h"
#include "../../src/hw/joypad.h"
#include "../../src/hw/render.h"

/* the canonical 2C02 palette (same table as host/palette.c — the
 * harness maps indices to RGB only so baselines and diffs are
 * viewable as images; comparison itself is on indices) */
static const uint8_t PALETTE[64][3] = {
    {84, 84, 84},    {0, 30, 116},    {8, 16, 144},    {48, 0, 136},
    {68, 0, 100},    {92, 0, 48},     {84, 4, 0},      {60, 24, 0},
    {32, 42, 0},     {8, 58, 0},      {0, 64, 0},      {0, 60, 0},
    {0, 50, 60},     {0, 0, 0},       {0, 0, 0},       {0, 0, 0},
    {152, 150, 152}, {8, 76, 196},    {48, 50, 236},   {92, 30, 228},
    {136, 20, 176},  {160, 20, 100},  {152, 34, 32},   {120, 60, 0},
    {84, 90, 0},     {40, 114, 0},    {8, 124, 0},     {0, 118, 40},
    {0, 102, 120},   {0, 0, 0},       {0, 0, 0},       {0, 0, 0},
    {236, 238, 236}, {76, 154, 236},  {120, 124, 236}, {176, 98, 236},
    {228, 84, 236},  {236, 88, 180},  {236, 106, 100}, {212, 136, 32},
    {160, 170, 0},   {116, 196, 0},   {76, 208, 32},   {56, 204, 108},
    {56, 180, 204},  {60, 60, 60},    {0, 0, 0},       {0, 0, 0},
    {236, 238, 236}, {168, 204, 236}, {188, 188, 236}, {212, 178, 236},
    {236, 174, 236}, {236, 174, 212}, {236, 180, 176}, {228, 196, 144},
    {204, 210, 120}, {180, 222, 120}, {168, 226, 144}, {152, 226, 180},
    {160, 214, 228}, {160, 162, 160}, {0, 0, 0},       {0, 0, 0},
};

/* ------------------------------------------------------- scenarios */

static uint8_t idle_input(unsigned frame) {
  (void)frame;
  return 0x00u;
}

static uint8_t start_input(unsigned frame) {
  return frame == 200 ? 0x10u : 0x00u; /* Start */
}

static uint8_t walk_input(unsigned frame) {
  if (frame == 200) return 0x10u;
  if (frame >= 700) return 0x01u; /* hold Right */
  return 0x00u;
}

static uint8_t jump_input(unsigned frame) {
  if (frame == 200) return 0x10u;
  if (frame >= 700) return (frame % 90) < 12 ? 0x81u : 0x01u; /* A taps */
  return 0x00u;
}

typedef struct {
  const char *name;
  uint8_t (*script)(unsigned frame);
  unsigned frames;     /* recording length */
  unsigned checkpoint; /* frame whose pixels are compared (0-indexed) */
} Scenario;

static const Scenario SCENARIOS[] = {
    /* checkpoint is always the recording's final frame: strict mode
     * renders after the full replay, so the two must coincide */
    {"title", idle_input, 90, 89},
    {"demo", idle_input, 700, 699},
    {"start", start_input, 450, 449},
    {"walk", walk_input, 950, 949},
    {"jump", jump_input, 950, 949},
};
#define SCENARIO_COUNT (sizeof(SCENARIOS) / sizeof(SCENARIOS[0]))

/* ------------------------------------------------------------ ppm */

static void indices_to_rgb(const uint8_t *indices, uint8_t *rgb) {
  for (unsigned i = 0; i < SMBC_RENDER_PIXELS; i++) {
    memcpy(rgb + (size_t)i * 3, PALETTE[indices[i] & 0x3f], 3);
  }
}

static bool ppm_write(const char *path, const uint8_t *rgb) {
  FILE *f = fopen(path, "wb");
  if (f == NULL) return false;
  fprintf(f, "P6\n%u %u\n255\n", SMBC_RENDER_WIDTH, SMBC_RENDER_HEIGHT);
  bool ok = fwrite(rgb, 1, (size_t)SMBC_RENDER_PIXELS * 3, f) ==
            (size_t)SMBC_RENDER_PIXELS * 3;
  ok = fclose(f) == 0 && ok;
  return ok;
}

static bool ppm_read(const char *path, uint8_t *rgb) {
  FILE *f = fopen(path, "rb");
  if (f == NULL) return false;
  unsigned w = 0, h = 0, max = 0;
  bool ok = fscanf(f, "P6 %u %u %u", &w, &h, &max) == 3 && w == 256 &&
            h == 240 && max == 255 && fgetc(f) == '\n' &&
            fread(rgb, 1, (size_t)SMBC_RENDER_PIXELS * 3, f) ==
                (size_t)SMBC_RENDER_PIXELS * 3;
  fclose(f);
  return ok;
}

/* -------------------------------------------------------- update */

/* regenerate a scenario's recording and baseline from its script */
static bool update_scenario(const Scenario *sc) {
  char path[128];
  Smbc smbc;
  smbc_init(&smbc);
  SmbcReplay rec;
  replay_init(&rec);
  uint8_t fb[SMBC_RENDER_PIXELS];
  uint8_t rgb[SMBC_RENDER_PIXELS * 3];
  bool ok = true;
  for (unsigned i = 0; ok && i < sc->frames; i++) {
    uint8_t in = sc->script(i);
    joypad_set_buttons(&smbc.joypad1, in);
    smbc_frame(&smbc);
    ok = replay_append(&rec, in, 0x00u, smbc_state_hash(&smbc));
    if (i == sc->checkpoint) {
      ppu_render_frame(&smbc.ppu, fb, NULL);
      indices_to_rgb(fb, rgb);
    }
  }
  char err[128];
  snprintf(path, sizeof(path), "tests/reference/%s.smbcr", sc->name);
  ok = ok && replay_save(&rec, path, err, sizeof(err));
  if (ok) {
    snprintf(path, sizeof(path), "tests/reference/%s.ppm", sc->name);
    ok = ppm_write(path, rgb);
  }
  replay_free(&rec);
  return ok;
}

/* ------------------------------------------------------- compare */

static bool compare_scenario(const Scenario *sc, unsigned *fail_pixels) {
  char path[128];
  snprintf(path, sizeof(path), "tests/reference/%s.smbcr", sc->name);
  SmbcReplay rec;
  char err[128];
  if (!replay_load(&rec, path, err, sizeof(err))) {
    fprintf(stderr, "  %s: %s\n", path, err);
    return false;
  }
  if (rec.count < sc->frames) {
    fprintf(stderr, "  %s: recording has %u frames, scenario wants %u\n",
            sc->name, rec.count, sc->frames);
    replay_free(&rec);
    return false;
  }
  Smbc smbc;
  smbc_init(&smbc);
  uint32_t diverge = SMBC_REPLAY_NO_DIVERGENCE;
  uint32_t ran = replay_run(&smbc, &rec, sc->frames, &diverge, NULL, NULL);
  replay_free(&rec);
  if (diverge != SMBC_REPLAY_NO_DIVERGENCE || ran != sc->frames) {
    fprintf(stderr, "  %s: state divergence at frame %u\n", sc->name,
            diverge);
    return false;
  }

  /* the checkpoint frame is the last one replay_run ran — render the
   * current PPU state. (checkpoints are the final frame of each
   * recording; assert the table keeps it that way) */
  assert(sc->checkpoint == sc->frames - 1);
  uint8_t fb[SMBC_RENDER_PIXELS];
  uint8_t actual[SMBC_RENDER_PIXELS * 3];
  uint8_t baseline[SMBC_RENDER_PIXELS * 3];
  ppu_render_frame(&smbc.ppu, fb, NULL);
  indices_to_rgb(fb, actual);

  snprintf(path, sizeof(path), "tests/reference/%s.ppm", sc->name);
  if (!ppm_read(path, baseline)) {
    fprintf(stderr, "  %s: cannot read baseline %s\n", sc->name, path);
    return false;
  }
  unsigned diffs = 0;
  size_t first = 0;
  uint8_t diff_img[SMBC_RENDER_PIXELS * 3];
  memcpy(diff_img, actual, sizeof(diff_img));
  for (size_t i = 0; i < (size_t)SMBC_RENDER_PIXELS * 3; i += 3) {
    if (memcmp(actual + i, baseline + i, 3) != 0) {
      if (diffs == 0) first = i / 3;
      diffs++;
      diff_img[i] = 255; /* red where the frames disagree */
      diff_img[i + 1] = 0;
      diff_img[i + 2] = 0;
    }
  }
  if (diffs != 0) {
    char out[160];
    system("mkdir -p build/reference");
    snprintf(out, sizeof(out), "build/reference/%s-actual.ppm", sc->name);
    ppm_write(out, actual);
    snprintf(out, sizeof(out), "build/reference/%s-diff.ppm", sc->name);
    ppm_write(out, diff_img);
    fprintf(stderr,
            "  %s: %u pixels differ (first at %zu,%zu) — see "
            "build/reference/%s-{actual,diff}.ppm\n",
            sc->name, diffs, first % 256, first / 256, sc->name);
    *fail_pixels += diffs;
    return false;
  }
  return true;
}

int main(int argc, char **argv) {
  if (argc == 2 && strcmp(argv[1], "--update") == 0) {
    fprintf(stderr,
            "WARNING: regenerating reference baselines from current port "
            "output.\nBaselines pin behaviour — review the git diff before "
            "committing.\n");
    for (unsigned i = 0; i < SCENARIO_COUNT; i++) {
      if (!update_scenario(&SCENARIOS[i])) {
        fprintf(stderr, "update failed for %s\n", SCENARIOS[i].name);
        return 1;
      }
      printf("updated %s\n", SCENARIOS[i].name);
    }
    return 0;
  }
  if (argc != 1) {
    fprintf(stderr, "usage: compare [--update]\n");
    return 1;
  }
  unsigned fail_pixels = 0;
  unsigned failed = 0;
  for (unsigned i = 0; i < SCENARIO_COUNT; i++) {
    if (!compare_scenario(&SCENARIOS[i], &fail_pixels)) {
      failed++;
    }
  }
  if (failed != 0) {
    fprintf(stderr, "tests/reference/compare: %u scenario(s) FAILED\n",
            failed);
    return 1;
  }
  printf("tests/reference/compare: ok\n");
  return 0;
}
