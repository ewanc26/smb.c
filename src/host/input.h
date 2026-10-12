/* host/input.h — controller mapping.
 *
 * Owns: translating SDL keyboard and game-controller state into the
 * NES joypad bitmap (hw/joypad.h: d7 A, d6 B, d5 Select, d4 Start,
 * d3 Up, d2 Down, d1 Left, d0 Right). Keyboard: arrows for the D-pad,
 * X = A, Z = B, Return = Start, Right Shift = Select. A connected
 * SDL_GameController mirrors the same buttons; keyboard and controller
 * OR together into player 1.
 *
 * Not owned: the joypad shift-register behaviour itself — the bitmap
 * goes to joypad_set_buttons and the core does the rest. */
#ifndef SMBC_HOST_INPUT_H
#define SMBC_HOST_INPUT_H

#include <stdint.h>

#include <SDL.h>

/* Open the first available game controller, or leave it NULL — the
 * keyboard always works. */
SDL_GameController *input_open_controller(void);

/* The current player-1 bitmap from keyboard + controller state. */
uint8_t input_poll(SDL_GameController *controller);

void input_close_controller(SDL_GameController *controller);

#endif /* SMBC_HOST_INPUT_H */
