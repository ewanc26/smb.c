/* host/settings.c — see host/settings.h for the contract. */
#include "settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void settings_defaults(SmbcSettings *settings) {
  settings->scale = 3;
  settings->rewind_seconds = 10;
  settings->overlay = false;
}

size_t settings_serialize(const SmbcSettings *settings, char *out, size_t cap) {
  int n = snprintf(out, cap,
                   "smbc-settings %u\nscale %u\nrewind_seconds %u\noverlay %u\n",
                   SMBC_SETTINGS_VERSION, settings->scale,
                   settings->rewind_seconds, settings->overlay ? 1u : 0u);
  if (n < 0 || (size_t)n >= cap) {
    return 0;
  }
  return (size_t)n;
}

static void set_err(char *err, size_t err_len, const char *msg, unsigned line) {
  if (err != NULL && err_len > 0) {
    snprintf(err, err_len, "line %u: %s", line, msg);
  }
}

/* Reads an integer in [lo, hi]. False on anything else. */
static bool parse_in_range(const char *text, unsigned long lo, unsigned long hi,
                           unsigned *out) {
  char *end = NULL;
  unsigned long v = strtoul(text, &end, 10);
  if (end == text || *end != '\0' || v < lo || v > hi) {
    return false;
  }
  *out = (unsigned)v;
  return true;
}

bool settings_parse(SmbcSettings *settings, const char *text, char *err,
                    size_t err_len) {
  SmbcSettings work = *settings;
  bool header_seen = false;
  bool seen[4];
  memset(seen, 0, sizeof(seen));

  unsigned lineno = 0;
  const char *p = text;
  while (*p != '\0') {
    size_t len = strcspn(p, "\n");
    char line[128];
    if (len >= sizeof(line)) {
      set_err(err, err_len, "line too long", lineno + 1);
      return false;
    }
    memcpy(line, p, len);
    line[len] = '\0';
    p += len + (p[len] == '\n' ? 1 : 0);
    lineno++;

    if (line[0] == '\0' || line[0] == '#') {
      continue;
    }
    if (strncmp(line, "smbc-settings", 13) == 0 &&
        (line[13] == ' ' || line[13] == '\0')) {
      unsigned version = 0;
      if (header_seen || lineno != 1) {
        set_err(err, err_len, "misplaced header", lineno);
        return false;
      }
      header_seen = true;
      if (sscanf(line + 13, "%u", &version) != 1) {
        set_err(err, err_len, "bad version", lineno);
        return false;
      }
      if (version > SMBC_SETTINGS_VERSION) {
        set_err(err, err_len, "newer settings version than this build", lineno);
        return false;
      }
      continue;
    }

    char name[32], value[32];
    int consumed = 0;
    if (sscanf(line, "%31s %31s%n", name, value, &consumed) != 2 ||
        line[consumed] != '\0') {
      set_err(err, err_len, "expected: <name> <value>", lineno);
      return false;
    }

    unsigned v = 0;
    if (strcmp(name, "scale") == 0) {
      if (seen[0]) { set_err(err, err_len, "scale given twice", lineno); return false; }
      if (!parse_in_range(value, SMBC_SETTINGS_SCALE_MIN, SMBC_SETTINGS_SCALE_MAX, &v)) {
        set_err(err, err_len, "scale must be 1-8", lineno);
        return false;
      }
      work.scale = v;
      seen[0] = true;
    } else if (strcmp(name, "rewind_seconds") == 0) {
      if (seen[1]) { set_err(err, err_len, "rewind_seconds given twice", lineno); return false; }
      if (!parse_in_range(value, SMBC_SETTINGS_REWIND_MIN, SMBC_SETTINGS_REWIND_MAX, &v)) {
        set_err(err, err_len, "rewind_seconds must be 1-60", lineno);
        return false;
      }
      work.rewind_seconds = v;
      seen[1] = true;
    } else if (strcmp(name, "overlay") == 0) {
      if (seen[2]) { set_err(err, err_len, "overlay given twice", lineno); return false; }
      if (!parse_in_range(value, 0, 1, &v)) {
        set_err(err, err_len, "overlay must be 0 or 1", lineno);
        return false;
      }
      work.overlay = v == 1u;
      seen[2] = true;
    } else {
      set_err(err, err_len, "unknown setting", lineno);
      return false;
    }
  }

  *settings = work;
  return true;
}
