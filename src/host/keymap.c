/* host/keymap.c — see host/keymap.h for the contract. */
#include "keymap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../hw/joypad.h"

/* SDL scancodes: X, Z, Return, Right Shift, arrows */
#define KEY_X 27u
#define KEY_Z 29u
#define KEY_RETURN 40u
#define KEY_RSHIFT 229u
#define KEY_RIGHT 79u
#define KEY_LEFT 80u
#define KEY_DOWN 81u
#define KEY_UP 82u

/* SDL_GameControllerButton: A, B, Back, Start, D-pad */
#define PAD_A 0u
#define PAD_B 1u
#define PAD_BACK 4u
#define PAD_START 6u
#define PAD_DPAD_UP 11u
#define PAD_DPAD_DOWN 12u
#define PAD_DPAD_LEFT 13u
#define PAD_DPAD_RIGHT 14u

static const char *const ACTION_NAMES[SMBC_ACTION_COUNT] = {
    "a", "b", "select", "start", "up", "down", "left", "right"};

static const uint8_t ACTION_BUTTON[SMBC_ACTION_COUNT] = {
    SMBC_BUTTON_A,  SMBC_BUTTON_B,    SMBC_BUTTON_SELECT, SMBC_BUTTON_START,
    SMBC_BUTTON_UP, SMBC_BUTTON_DOWN, SMBC_BUTTON_LEFT,   SMBC_BUTTON_RIGHT};

void keymap_defaults(SmbcKeymap *map) {
  static const uint16_t keys[SMBC_ACTION_COUNT] = {
      KEY_X, KEY_Z, KEY_RSHIFT, KEY_RETURN, KEY_UP, KEY_DOWN, KEY_LEFT,
      KEY_RIGHT};
  static const uint16_t pads[SMBC_ACTION_COUNT] = {
      PAD_A,      PAD_B,         PAD_BACK,       PAD_START,
      PAD_DPAD_UP, PAD_DPAD_DOWN, PAD_DPAD_LEFT, PAD_DPAD_RIGHT};
  for (unsigned a = 0; a < SMBC_ACTION_COUNT; a++) {
    map->slot[a][SMBC_SLOT_KEY] = (SmbcBinding){.bound = true, .code = keys[a]};
    map->slot[a][SMBC_SLOT_PAD] = (SmbcBinding){.bound = true, .code = pads[a]};
  }
}

/* Which other action, if any, already holds this physical input in the same
 * slot kind (keyboard against keyboard, pad against pad). */
static bool find_owner(const SmbcKeymap *map, SmbcAction action,
                       SmbcSlot slot, SmbcBinding binding,
                       SmbcAction *owner) {
  for (unsigned a = 0; a < SMBC_ACTION_COUNT; a++) {
    if (a == action) {
      continue;
    }
    const SmbcBinding *held = &map->slot[a][slot];
    if (held->bound && binding.bound && held->code == binding.code) {
      *owner = (SmbcAction)a;
      return true;
    }
  }
  return false;
}

bool keymap_bind(SmbcKeymap *map, SmbcAction action, SmbcSlot slot,
                 SmbcBinding binding, SmbcAction *conflict) {
  if (action >= SMBC_ACTION_COUNT || slot >= SMBC_SLOT_COUNT) {
    return false;
  }
  if (binding.bound) {
    unsigned limit = slot == SMBC_SLOT_KEY ? SMBC_KEYMAP_KEYS
                                           : SMBC_KEYMAP_PAD_BUTTONS;
    if (binding.code >= limit) {
      return false;
    }
    SmbcAction owner;
    if (find_owner(map, action, slot, binding, &owner)) {
      if (conflict != NULL) {
        *conflict = owner;
      }
      return false;
    }
  }
  map->slot[action][slot] = binding;
  return true;
}

uint8_t keymap_resolve(const SmbcKeymap *map, const SmbcInputState *input) {
  uint8_t bitmap = 0;
  for (unsigned a = 0; a < SMBC_ACTION_COUNT; a++) {
    const SmbcBinding *key = &map->slot[a][SMBC_SLOT_KEY];
    const SmbcBinding *pad = &map->slot[a][SMBC_SLOT_PAD];
    bool held = (key->bound && input->keys[key->code] != 0) ||
                (pad->bound && pad->code < 32u &&
                 (input->pad_buttons >> pad->code) & 1u);
    if (held) {
      bitmap |= ACTION_BUTTON[a];
    }
  }
  return bitmap;
}

/* ------------------------------------------------------------ text */

size_t keymap_serialize(const SmbcKeymap *map, char *out, size_t cap) {
  size_t used = 0;
  int n = snprintf(out, cap, "smbc-keymap %u\n", SMBC_KEYMAP_VERSION);
  if (n < 0 || (size_t)n >= cap) {
    return 0;
  }
  used = (size_t)n;
  for (unsigned a = 0; a < SMBC_ACTION_COUNT; a++) {
    for (unsigned s = 0; s < SMBC_SLOT_COUNT; s++) {
      const SmbcBinding *b = &map->slot[a][s];
      const char *kind = s == SMBC_SLOT_KEY ? "key" : "pad";
      if (b->bound) {
        n = snprintf(out + used, cap - used, "%s %s %u\n", ACTION_NAMES[a],
                     kind, (unsigned)b->code);
      } else {
        n = snprintf(out + used, cap - used, "%s %s none\n", ACTION_NAMES[a],
                     kind);
      }
      if (n < 0 || (size_t)n >= cap - used) {
        return 0;
      }
      used += (size_t)n;
    }
  }
  return used;
}

static bool parse_action(const char *word, SmbcAction *action) {
  for (unsigned a = 0; a < SMBC_ACTION_COUNT; a++) {
    if (strcmp(word, ACTION_NAMES[a]) == 0) {
      *action = (SmbcAction)a;
      return true;
    }
  }
  return false;
}

static void set_err(char *err, size_t err_len, const char *msg,
                    unsigned line) {
  if (err != NULL && err_len > 0) {
    snprintf(err, err_len, "line %u: %s", line, msg);
  }
}

/* Two different actions may not share one physical input in the same slot
 * kind. Checked on the whole map after parsing, so a file that swaps keys
 * is judged by its final result. */
static bool check_conflicts(const SmbcKeymap *map, char *err, size_t err_len) {
  for (unsigned a = 0; a < SMBC_ACTION_COUNT; a++) {
    for (unsigned s = 0; s < SMBC_SLOT_COUNT; s++) {
      const SmbcBinding *x = &map->slot[a][s];
      if (!x->bound) {
        continue;
      }
      for (unsigned b = a + 1; b < SMBC_ACTION_COUNT; b++) {
        const SmbcBinding *y = &map->slot[b][s];
        if (y->bound && y->code == x->code) {
          if (err != NULL && err_len > 0) {
            snprintf(err, err_len, "%s and %s both bound to the same %s",
                     ACTION_NAMES[a], ACTION_NAMES[b],
                     s == SMBC_SLOT_KEY ? "key" : "pad button");
          }
          return false;
        }
      }
    }
  }
  return true;
}

bool keymap_parse(SmbcKeymap *map, const char *text, char *err,
                  size_t err_len) {
  SmbcKeymap work = *map;
  bool header_seen = false;
  bool seen[SMBC_ACTION_COUNT][SMBC_SLOT_COUNT];
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
    if (strncmp(line, "smbc-keymap", 11) == 0 &&
        (line[11] == ' ' || line[11] == '\0')) {
      unsigned version = 0;
      if (header_seen || lineno != 1) {
        set_err(err, err_len, "misplaced header", lineno);
        return false;
      }
      header_seen = true;
      if (sscanf(line + 11, "%u", &version) != 1) {
        set_err(err, err_len, "bad version", lineno);
        return false;
      }
      if (version > SMBC_KEYMAP_VERSION) {
        set_err(err, err_len, "newer keymap version than this build", lineno);
        return false;
      }
      continue;
    }

    char word1[16], word2[16], word3[32];
    int consumed = 0;
    if (sscanf(line, "%15s %15s %31s%n", word1, word2, word3, &consumed) != 3 ||
        line[consumed] != '\0') {
      set_err(err, err_len, "expected: <action> key|pad <code|none>", lineno);
      return false;
    }
    SmbcAction action;
    if (!parse_action(word1, &action)) {
      set_err(err, err_len, "unknown action", lineno);
      return false;
    }
    SmbcSlot slot;
    if (strcmp(word2, "key") == 0) {
      slot = SMBC_SLOT_KEY;
    } else if (strcmp(word2, "pad") == 0) {
      slot = SMBC_SLOT_PAD;
    } else {
      set_err(err, err_len, "slot must be key or pad", lineno);
      return false;
    }
    if (seen[action][slot]) {
      set_err(err, err_len, "slot given twice", lineno);
      return false;
    }
    seen[action][slot] = true;

    SmbcBinding binding = {.bound = false, .code = 0};
    if (strcmp(word3, "none") != 0) {
      char *end = NULL;
      unsigned long code = strtoul(word3, &end, 10);
      unsigned limit = slot == SMBC_SLOT_KEY ? SMBC_KEYMAP_KEYS
                                             : SMBC_KEYMAP_PAD_BUTTONS;
      if (end == word3 || *end != '\0' || code >= limit) {
        set_err(err, err_len, "code out of range or not a number", lineno);
        return false;
      }
      binding = (SmbcBinding){.bound = true, .code = (uint16_t)code};
    }
    work.slot[action][slot] = binding;
  }

  if (!check_conflicts(&work, err, err_len)) {
    return false;
  }
  *map = work;
  return true;
}
