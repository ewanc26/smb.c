/* host/capture.c — see host/capture.h for the contract. */
#include "capture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core/snapshot.h"
#include "../hw/render.h"
#include "palette.h"

#define SMBC_PPM_HEADER_MAX 32u

static void set_err(char *err, size_t err_len, const char *msg) {
  if (err != NULL && err_len > 0) {
    snprintf(err, err_len, "%s", msg);
  }
}

size_t capture_ppm_size(void) {
  char header[SMBC_PPM_HEADER_MAX];
  int n = snprintf(header, sizeof(header), "P6\n%u %u\n255\n",
                   SMBC_RENDER_WIDTH, SMBC_RENDER_HEIGHT);
  return (size_t)n + (size_t)SMBC_RENDER_PIXELS * 3u;
}

size_t capture_encode_ppm(const uint8_t *fb, uint8_t *out, size_t cap) {
  if (fb == NULL || out == NULL || cap < capture_ppm_size()) {
    return 0;
  }
  int n = snprintf((char *)out, SMBC_PPM_HEADER_MAX, "P6\n%u %u\n255\n",
                   SMBC_RENDER_WIDTH, SMBC_RENDER_HEIGHT);
  uint8_t *px = out + n;
  for (unsigned i = 0; i < SMBC_RENDER_PIXELS; i++) {
    /* the renderer only produces 6-bit indices; mask defensively */
    const uint8_t *rgb = SMBC_2C02_RGB[fb[i] & 0x3fu];
    px[i * 3u] = rgb[0];
    px[i * 3u + 1u] = rgb[1];
    px[i * 3u + 2u] = rgb[2];
  }
  return (size_t)n + (size_t)SMBC_RENDER_PIXELS * 3u;
}

bool capture_write_ppm(const char *path, const uint8_t *fb, char *err,
                       size_t err_len) {
  size_t size = capture_ppm_size();
  uint8_t *buf = malloc(size);
  if (buf == NULL) {
    set_err(err, err_len, "out of memory");
    return false;
  }
  size_t n = capture_encode_ppm(fb, buf, size);
  bool ok = false;
  FILE *f = fopen(path, "wb");
  if (f == NULL) {
    set_err(err, err_len, "cannot open for writing");
  } else {
    ok = fwrite(buf, 1, n, f) == n;
    if (fclose(f) != 0) {
      ok = false;
    }
    if (!ok) {
      set_err(err, err_len, "write failed");
    }
  }
  free(buf);
  return ok;
}

bool capture_write_state(const Smbc *smbc, const char *path, char *err,
                         size_t err_len) {
  size_t size = smbc_snapshot_size();
  uint8_t *buf = malloc(size);
  if (buf == NULL) {
    set_err(err, err_len, "out of memory");
    return false;
  }
  bool ok = smbc_snapshot_write(smbc, buf, size);
  FILE *f = ok ? fopen(path, "wb") : NULL;
  if (f == NULL) {
    ok = false;
    set_err(err, err_len, "cannot open for writing");
  } else {
    ok = fwrite(buf, 1, size, f) == size;
    if (fclose(f) != 0) {
      ok = false;
    }
    if (!ok) {
      set_err(err, err_len, "write failed");
    }
  }
  free(buf);
  return ok;
}

bool capture_read_state(Smbc *smbc, const char *path, char *err,
                        size_t err_len) {
  size_t size = smbc_snapshot_size();
  FILE *f = fopen(path, "rb");
  if (f == NULL) {
    set_err(err, err_len, "cannot open for reading");
    return false;
  }
  uint8_t *buf = malloc(size + 1); /* one extra byte detects oversize files */
  if (buf == NULL) {
    fclose(f);
    set_err(err, err_len, "out of memory");
    return false;
  }
  size_t n = fread(buf, 1, size + 1, f);
  fclose(f);
  bool ok = smbc_snapshot_read(smbc, buf, n);
  if (!ok) {
    set_err(err, err_len, "not a valid save state");
  }
  free(buf);
  return ok;
}
