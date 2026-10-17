/* host/settings.h — persistent player settings (#24).
 *
 * Owns: the options the frontend exposes (window scale, rewind length and
 * whether the diagnostics overlay starts on), their ranges and defaults, and
 * a versioned text file (smbc.settings) that stores them.
 *
 * Not owned: how the options are changed in the window (frontend.c) or the
 * controls themselves (keymap.c). Options the game does not have yet, such as
 * audio volume or gameplay tweaks, are not here: a setting only exists once
 * something honours it.
 *
 * File format, version 1 (text, one "name value" per line):
 *   smbc-settings 1
 *   scale 3
 *   rewind_seconds 10
 *   overlay 0
 * A file with no header is version 0 and is read with the same body rules. A
 * newer version is refused. Unknown names and out-of-range values are errors,
 * and a file with any error leaves the settings untouched.
 */
#ifndef SMBC_HOST_SETTINGS_H
#define SMBC_HOST_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SMBC_SETTINGS_VERSION 1u
#define SMBC_SETTINGS_SCALE_MIN 1u
#define SMBC_SETTINGS_SCALE_MAX 8u
#define SMBC_SETTINGS_REWIND_MIN 1u
#define SMBC_SETTINGS_REWIND_MAX 60u

typedef struct {
  unsigned scale;          /* integer window scale, 1-8 */
  unsigned rewind_seconds; /* history length, 1-60 seconds at 60 Hz */
  bool overlay;            /* diagnostics overlay starts visible */
} SmbcSettings;

void settings_defaults(SmbcSettings *settings);

/* Serialise to out (cap bytes, NUL-terminated). Returns the length, or 0 when
 * cap is too small. */
size_t settings_serialize(const SmbcSettings *settings, char *out, size_t cap);

/* Parse text into settings. Changes settings only when the whole text is
 * valid; on error err explains the first problem. */
bool settings_parse(SmbcSettings *settings, const char *text, char *err,
                    size_t err_len);

#endif /* SMBC_HOST_SETTINGS_H */
