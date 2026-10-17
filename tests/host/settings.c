/* tests/host/settings.c — defaults, ranges, versioning and the file round trip
 * for player settings (#24). */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/host/settings.h"

int main(void) {
  char buf[256], err[128];

  /* defaults and a round trip */
  {
    SmbcSettings a, b;
    settings_defaults(&a);
    assert(a.scale == 3 && a.rewind_seconds == 10 && !a.overlay);
    size_t n = settings_serialize(&a, buf, sizeof(buf));
    assert(n > 0 && n == strlen(buf));
    assert(strncmp(buf, "smbc-settings 1\n", 16) == 0);
    char tiny[8];
    assert(settings_serialize(&a, tiny, sizeof(tiny)) == 0);

    settings_defaults(&b);
    b.scale = 6; b.rewind_seconds = 1; b.overlay = true;
    assert(settings_parse(&b, buf, err, sizeof(err)));
    assert(b.scale == 3 && b.rewind_seconds == 10 && !b.overlay);
  }

  /* values are read and bounds are inclusive */
  {
    SmbcSettings s;
    settings_defaults(&s);
    assert(settings_parse(&s, "scale 8\nrewind_seconds 60\noverlay 1\n", err, sizeof(err)));
    assert(s.scale == 8 && s.rewind_seconds == 60 && s.overlay);
    assert(settings_parse(&s, "scale 1\nrewind_seconds 1\noverlay 0\n", err, sizeof(err)));
    assert(s.scale == 1 && s.rewind_seconds == 1 && !s.overlay);
  }

  /* a partial file keeps the other values */
  {
    SmbcSettings s;
    settings_defaults(&s);
    assert(settings_parse(&s, "overlay 1\n", err, sizeof(err)));
    assert(s.overlay && s.scale == 3 && s.rewind_seconds == 10);
  }

  /* out-of-range, unknown, malformed and duplicate values are refused whole */
  {
    const char *bad[] = {
        "scale 0\n", "scale 9\n", "scale three\n", "scale -1\n",
        "rewind_seconds 0\n", "rewind_seconds 61\n",
        "overlay 2\n", "overlay yes\n",
        "volume 5\n", "scale 2 extra\n",
        "scale 2\nscale 3\n", "scale 2\n\nrewind_seconds 99\n",
    };
    for (unsigned i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
      SmbcSettings s;
      settings_defaults(&s);
      SmbcSettings before = s;
      assert(!settings_parse(&s, bad[i], err, sizeof(err)));
      assert(strlen(err) > 0);
      assert(memcmp(&s, &before, sizeof(s)) == 0);
    }
  }

  /* versioning: no header is version 0, a newer version is refused */
  {
    SmbcSettings s;
    settings_defaults(&s);
    assert(settings_parse(&s, "scale 5\n", err, sizeof(err)));
    assert(s.scale == 5);
    SmbcSettings t;
    settings_defaults(&t);
    assert(!settings_parse(&t, "smbc-settings 2\nscale 5\n", err, sizeof(err)));
    assert(strstr(err, "newer settings version") != NULL);
    assert(t.scale == 3);
    assert(!settings_parse(&t, "scale 5\nsmbc-settings 1\n", err, sizeof(err)));
  }

  printf("settings: ok\n");
  return 0;
}
