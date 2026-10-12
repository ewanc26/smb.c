/* host/input.h — controller mapping.
 *
 * Owns: translating SDL keyboard and game-controller state into the
 * NES joypad bitmap (hw/joypad.h serial-port order: d0 A, d1 B,
 * d2 Select, d3 Start, d4 Up, d5 Down, d6 Left, d7 Right). Keyboard: arrows for the D-pad,
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
