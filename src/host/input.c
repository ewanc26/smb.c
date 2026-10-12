/* host/input.c — controller mapping. */
#include "input.h"

#include "../hw/joypad.h"

/* the bitmap is in the hw serial-port layout (hw/joypad.h), which is what
 * joypad_set_buttons consumes — NOT the in-game SavedJoypadBits order */
#define SMBC_BTN_A      SMBC_BUTTON_A
#define SMBC_BTN_B      SMBC_BUTTON_B
#define SMBC_BTN_SELECT SMBC_BUTTON_SELECT
#define SMBC_BTN_START  SMBC_BUTTON_START
#define SMBC_BTN_UP     SMBC_BUTTON_UP
#define SMBC_BTN_DOWN   SMBC_BUTTON_DOWN
#define SMBC_BTN_LEFT   SMBC_BUTTON_LEFT
#define SMBC_BTN_RIGHT  SMBC_BUTTON_RIGHT

SDL_GameController *input_open_controller(void) {
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
    if (SDL_IsGameController(i)) {
      return SDL_GameControllerOpen(i);
    }
  }
  return NULL;
}

uint8_t input_poll(SDL_GameController *controller) {
  uint8_t bitmap = 0;

  const uint8_t *keys = SDL_GetKeyboardState(NULL);
  if (keys[SDL_SCANCODE_X]) bitmap |= SMBC_BTN_A;
  if (keys[SDL_SCANCODE_Z]) bitmap |= SMBC_BTN_B;
  if (keys[SDL_SCANCODE_RSHIFT]) bitmap |= SMBC_BTN_SELECT;
  if (keys[SDL_SCANCODE_RETURN]) bitmap |= SMBC_BTN_START;
  if (keys[SDL_SCANCODE_UP]) bitmap |= SMBC_BTN_UP;
  if (keys[SDL_SCANCODE_DOWN]) bitmap |= SMBC_BTN_DOWN;
  if (keys[SDL_SCANCODE_LEFT]) bitmap |= SMBC_BTN_LEFT;
  if (keys[SDL_SCANCODE_RIGHT]) bitmap |= SMBC_BTN_RIGHT;

  if (controller != NULL) {
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A))
      bitmap |= SMBC_BTN_A;
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B))
      bitmap |= SMBC_BTN_B;
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK))
      bitmap |= SMBC_BTN_SELECT;
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START))
      bitmap |= SMBC_BTN_START;
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP))
      bitmap |= SMBC_BTN_UP;
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
      bitmap |= SMBC_BTN_DOWN;
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
      bitmap |= SMBC_BTN_LEFT;
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
      bitmap |= SMBC_BTN_RIGHT;
  }
  return bitmap;
}

void input_close_controller(SDL_GameController *controller) {
  if (controller != NULL) {
    SDL_GameControllerClose(controller);
  }
}
