/* host/input.h — SDL input: reading the keyboard and game controllers.
 *
 * Owns: filling a keymap.h input snapshot from SDL each tick, and opening or
 * closing controllers. Which binding means which button lives in keymap.h;
 * which pad is active lives in padset.h. This module only talks to SDL.
 *
 * Keyboard and gamepad together feed player 1. Controllers arrive and leave
 * while the game runs: the frontend listens for SDL_CONTROLLERDEVICEADDED and
 * SDL_CONTROLLERDEVICEREMOVED and uses the helpers below. */
#ifndef SMBC_HOST_INPUT_H
#define SMBC_HOST_INPUT_H

#include <stdint.h>

#include <SDL.h>

#include "keymap.h"

/* Open the controller at SDL device index, or NULL if it cannot be opened. */
SDL_GameController *input_open_pad(int device_index);

/* Instance id of an open controller (the id hot-plug events carry), or -1. */
int32_t input_pad_instance_id(SDL_GameController *controller);

/* The controller for an instance id, or NULL if it is not open. */
SDL_GameController *input_pad_from_instance(int32_t instance_id);

/* The joypad bitmap for this tick. controller may be NULL (keyboard only). */
uint8_t input_poll(const SmbcKeymap *map, SDL_GameController *controller);

#endif /* SMBC_HOST_INPUT_H */
