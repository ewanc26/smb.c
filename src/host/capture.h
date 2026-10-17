/* host/capture.h — screenshots (#28) and save-state files (#25).
 *
 * Owns: encoding the core's 2C02 index frame as a binary PPM (P6) using the
 * host palette, and reading or writing the core's snapshot image to a file.
 * The image format itself is core/snapshot.h's; this module only moves
 * bytes to and from disk.
 *
 * Not owned: which key takes a capture, or where the file names come from
 * (frontend.c). */
#ifndef SMBC_HOST_CAPTURE_H
#define SMBC_HOST_CAPTURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../core/frame.h"

/* Size of the PPM for one native frame: header plus 256×240 RGB bytes. */
size_t capture_ppm_size(void);

/* Encode fb (SMBC_RENDER_PIXELS palette indices) into out. Returns the
 * number of bytes written, or 0 if cap is too small. Pure: no I/O. */
size_t capture_encode_ppm(const uint8_t *fb, uint8_t *out, size_t cap);

/* Write the frame to path as a PPM. False with a message in err on failure. */
bool capture_write_ppm(const char *path, const uint8_t *fb, char *err,
                       size_t err_len);

/* Save and load the simulation state. Loading validates the file through
 * smbc_snapshot_read and leaves smbc untouched on failure. */
bool capture_write_state(const Smbc *smbc, const char *path, char *err,
                         size_t err_len);
bool capture_read_state(Smbc *smbc, const char *path, char *err,
                        size_t err_len);

#endif /* SMBC_HOST_CAPTURE_H */
