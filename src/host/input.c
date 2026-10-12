/* host/input.c — controller mapping. */
#include "input.h"

/* hw/joypad.h bitmap bits */
#define SMBC_BTN_RIGHT 0x01u
#define SMBC_BTN_LEFT 0x02u
#define SMBC_BTN_DOWN 0x04u
#define SMBC_BTN_UP 0x08u
#define SMBC_BTN_START 0x10u
#define SMBC_BTN_SELECT 0x20u
#define SMBC_BTN_B 0x40u
#define SMBC_BTN_A 0x80u

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
