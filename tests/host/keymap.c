/* tests/host/keymap.c — mapping resolution, rebinding conflicts and the
 * versioned configuration (#22). Runs with a mocked input snapshot. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/hw/joypad.h"
#include "../../src/host/keymap.h"

#define KEY_X 27u
#define KEY_Z 29u
#define KEY_RETURN 40u
#define KEY_UP 82u

static void press_key(SmbcInputState *in, unsigned code) { in->keys[code] = 1; }

int main(void) {
  char buf[1024], err[128];

  /* defaults: keyboard and pad each drive the NES buttons, and together
   * they OR into one bitmap */
  {
    SmbcKeymap map;
    keymap_defaults(&map);
    SmbcInputState in;
    memset(&in, 0, sizeof(in));
    assert(keymap_resolve(&map, &in) == 0);

    press_key(&in, KEY_X);
    assert(keymap_resolve(&map, &in) == SMBC_BUTTON_A);
    in.pad_buttons = 1u << 0; /* pad A as well: still just A */
    assert(keymap_resolve(&map, &in) == SMBC_BUTTON_A);

    memset(&in, 0, sizeof(in));
    press_key(&in, KEY_UP);
    in.pad_buttons = 1u << 13; /* pad D-pad left */
    assert(keymap_resolve(&map, &in) == (SMBC_BUTTON_UP | SMBC_BUTTON_LEFT));
  }

  /* rebinding: a key already held by another action is refused and the
   * map is unchanged; rebinding an action to its own key is fine */
  {
    SmbcKeymap map;
    keymap_defaults(&map);
    SmbcBinding z_key = {.bound = true, .code = KEY_Z};
    SmbcAction owner = SMBC_ACTION_COUNT;
    assert(!keymap_bind(&map, SMBC_ACT_A, SMBC_SLOT_KEY, z_key, &owner));
    assert(owner == SMBC_ACT_B);
    assert(map.slot[SMBC_ACT_A][SMBC_SLOT_KEY].code == KEY_X);

    SmbcBinding x_key = {.bound = true, .code = KEY_X};
    assert(keymap_bind(&map, SMBC_ACT_A, SMBC_SLOT_KEY, x_key, NULL));

    /* a keyboard key may equal a pad button number: different devices */
    SmbcBinding pad_zero = {.bound = true, .code = 0};
    assert(keymap_bind(&map, SMBC_ACT_B, SMBC_SLOT_KEY, pad_zero, NULL));

    /* out-of-range codes are refused */
    SmbcBinding huge = {.bound = true, .code = 600};
    assert(!keymap_bind(&map, SMBC_ACT_B, SMBC_SLOT_KEY, huge, NULL));
  }

  /* clearing a slot releases it: the input no longer counts */
  {
    SmbcKeymap map;
    keymap_defaults(&map);
    SmbcInputState in;
    memset(&in, 0, sizeof(in));
    press_key(&in, KEY_RETURN);
    assert(keymap_resolve(&map, &in) == SMBC_BUTTON_START);
    SmbcBinding none = {.bound = false, .code = 0};
    assert(keymap_bind(&map, SMBC_ACT_START, SMBC_SLOT_KEY, none, NULL));
    assert(keymap_resolve(&map, &in) == 0);
  }

  /* serialise then parse reproduces the map exactly */
  {
    SmbcKeymap map, back;
    keymap_defaults(&map);
    size_t n = keymap_serialize(&map, buf, sizeof(buf));
    assert(n > 0 && n == strlen(buf));
    assert(strncmp(buf, "smbc-keymap 1\n", 14) == 0);
    char tiny[20];
    assert(keymap_serialize(&map, tiny, sizeof(tiny)) == 0); /* too small */

    keymap_defaults(&back);
    SmbcBinding none = {.bound = false, .code = 0};
    keymap_bind(&back, SMBC_ACT_UP, SMBC_SLOT_KEY, none, NULL);
    assert(keymap_parse(&back, buf, err, sizeof(err)));
    assert(memcmp(&map, &back, sizeof(map)) == 0);
  }

  /* migration: a file with no header is version 0 and reads with the same
   * body rules; slots it leaves out keep their defaults */
  {
    SmbcKeymap map;
    keymap_defaults(&map);
    assert(keymap_parse(&map, "start key none\nb pad 2\n", err, sizeof(err)));
    assert(!map.slot[SMBC_ACT_START][SMBC_SLOT_KEY].bound);
    assert(map.slot[SMBC_ACT_START][SMBC_SLOT_PAD].code == 6);
    assert(map.slot[SMBC_ACT_B][SMBC_SLOT_PAD].code == 2);
    assert(map.slot[SMBC_ACT_A][SMBC_SLOT_KEY].code == KEY_X);
  }

  /* a newer format is refused, not guessed at */
  {
    SmbcKeymap map;
    keymap_defaults(&map);
    SmbcKeymap before = map;
    assert(!keymap_parse(&map, "smbc-keymap 2\n", err, sizeof(err)));
    assert(strstr(err, "newer keymap version") != NULL);
    assert(memcmp(&map, &before, sizeof(map)) == 0);
  }

  /* malformed files are refused with the line named, and leave the map
   * untouched */
  {
    const char *bad[] = {
        "smbc-keymap 1\nwalk key 27\n",              /* unknown action */
        "smbc-keymap 1\na joy 27\n",                 /* unknown slot */
        "smbc-keymap 1\na key 27 extra\n",           /* too many fields */
        "smbc-keymap 1\na key 999\n",                /* key out of range */
        "smbc-keymap 1\na pad 40\n",                 /* pad out of range */
        "smbc-keymap 1\na key 1\na key 2\n",         /* slot twice */
        "smbc-keymap 1\na key ten\n",                /* not a number */
        "smbc-keymap 1\nsmbc-keymap 1\n",            /* misplaced header */
        "a key 27\nsmbc-keymap 1\n",                 /* header not first */
    };
    for (unsigned i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
      SmbcKeymap map;
      keymap_defaults(&map);
      SmbcKeymap before = map;
      assert(!keymap_parse(&map, bad[i], err, sizeof(err)));
      assert(strlen(err) > 0);
      assert(memcmp(&map, &before, sizeof(map)) == 0);
    }
  }

  /* a file that puts two actions on one key is refused as a whole, even
   * though each line alone is valid */
  {
    SmbcKeymap map;
    keymap_defaults(&map);
    SmbcKeymap before = map;
    assert(!keymap_parse(&map, "b key 27\n", err, sizeof(err)));
    assert(strstr(err, "both bound") != NULL);
    assert(memcmp(&map, &before, sizeof(map)) == 0);
  }

  /* swapping two keys in one file is valid: only the final map matters */
  {
    SmbcKeymap map;
    keymap_defaults(&map);
    assert(keymap_parse(&map, "a key 29\nb key 27\n", err, sizeof(err)));
    assert(map.slot[SMBC_ACT_A][SMBC_SLOT_KEY].code == 29);
    assert(map.slot[SMBC_ACT_B][SMBC_SLOT_KEY].code == 27);
  }

  printf("keymap: ok\n");
  return 0;
}
