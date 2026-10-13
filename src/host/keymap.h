/* host/keymap.h — rebindable controller mapping (#22).
 *
 * Owns: the logical NES actions, the binding for each action on a keyboard
 * and on a gamepad, conflict-checked rebinding, reset to defaults, turning
 * an input snapshot into the joypad bitmap (hw/joypad.h order), and a
 * versioned text configuration.
 *
 * Independent of SDL: keys are SDL scancode numbers and pad buttons are
 * SDL_GameControllerButton numbers, passed as plain integers, so the whole
 * module is testable with a mocked input snapshot. host/input.c fills the
 * snapshot from SDL.
 *
 * Configuration format, version 1 (text, one entry per line):
 *   smbc-keymap 1
 *   <action> key <scancode>      keyboard binding
 *   <action> pad <button>        gamepad binding
 *   <action> key none            clear a slot
 * Actions: a b select start up down left right. Lines with a key or pad slot
 * omitted keep the default for that slot. A file with no header is version 0
 * and is read with the same body rules. A newer version is refused.
 */
#ifndef SMBC_HOST_KEYMAP_H
#define SMBC_HOST_KEYMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SMBC_KEYMAP_VERSION 1u
#define SMBC_KEYMAP_KEYS 512u   /* SDL_NUM_SCANCODES */
#define SMBC_KEYMAP_PAD_BUTTONS 32u

typedef enum {
  SMBC_ACT_A,
  SMBC_ACT_B,
  SMBC_ACT_SELECT,
  SMBC_ACT_START,
  SMBC_ACT_UP,
  SMBC_ACT_DOWN,
  SMBC_ACT_LEFT,
  SMBC_ACT_RIGHT,
  SMBC_ACTION_COUNT
} SmbcAction;

typedef enum { SMBC_SLOT_KEY, SMBC_SLOT_PAD, SMBC_SLOT_COUNT } SmbcSlot;

typedef struct {
  bool bound;   /* false = cleared */
  uint16_t code; /* scancode for SMBC_SLOT_KEY, button for SMBC_SLOT_PAD */
} SmbcBinding;

typedef struct {
  SmbcBinding slot[SMBC_ACTION_COUNT][SMBC_SLOT_COUNT];
} SmbcKeymap;

/* A snapshot of the input devices for one tick. */
typedef struct {
  uint8_t keys[SMBC_KEYMAP_KEYS]; /* non-zero = held */
  uint32_t pad_buttons;           /* bit n = button n held */
} SmbcInputState;

void keymap_defaults(SmbcKeymap *map);

/* Bind one slot. Refuses (returns false, map unchanged) when the same key
 * or pad button is already bound to a different action, and reports that
 * action through conflict. Rebinding a slot to what it already holds is
 * allowed. */
bool keymap_bind(SmbcKeymap *map, SmbcAction action, SmbcSlot slot,
                 SmbcBinding binding, SmbcAction *conflict);

/* Joypad bitmap for one tick, in the hw serial-port order: A, B, Select,
 * Start, Up, Down, Left, Right. Keyboard and pad slots OR together. */
uint8_t keymap_resolve(const SmbcKeymap *map, const SmbcInputState *input);

/* Serialise to out (cap bytes, NUL-terminated). Returns the length, or 0
 * when cap is too small. */
size_t keymap_serialize(const SmbcKeymap *map, char *out, size_t cap);

/* Parse text into map. The map is only changed when the whole text is
 * valid; on error it is untouched and err explains the first problem. */
bool keymap_parse(SmbcKeymap *map, const char *text, char *err,
                  size_t err_len);

#endif /* SMBC_HOST_KEYMAP_H */
