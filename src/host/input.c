/* host/input.c — SDL side of input; see host/input.h for the contract. */
#include "input.h"

#include <string.h>

#define SMBC_PAD_BUTTONS 15 /* SDL_CONTROLLER_BUTTON_MAX */

SDL_GameController *input_open_pad(int device_index) {
  if (!SDL_IsGameController(device_index)) {
    return NULL;
  }
  return SDL_GameControllerOpen(device_index);
}

int32_t input_pad_instance_id(SDL_GameController *controller) {
  if (controller == NULL) {
    return -1;
  }
  return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
}

SDL_GameController *input_pad_from_instance(int32_t instance_id) {
  return SDL_GameControllerFromInstanceID((SDL_JoystickID)instance_id);
}

uint8_t input_poll(const SmbcKeymap *map, SDL_GameController *controller) {
  SmbcInputState in;
  memset(&in, 0, sizeof(in));

  const Uint8 *keys = SDL_GetKeyboardState(NULL);
  int n = SDL_NUM_SCANCODES < (int)SMBC_KEYMAP_KEYS ? SDL_NUM_SCANCODES
                                                    : (int)SMBC_KEYMAP_KEYS;
  for (int i = 0; i < n; i++) {
    in.keys[i] = keys[i];
  }

  if (controller != NULL) {
    for (int b = 0; b < SMBC_PAD_BUTTONS; b++) {
      if (SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)b)) {
        in.pad_buttons |= 1u << b;
      }
    }
  }
  return keymap_resolve(map, &in);
}
