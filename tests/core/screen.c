/* tests/core/screen.c — exercises src/core/screen.c: the buffer writer's
 * address, length, repeat and increment modes, the indirect advance and
 * the address table load. */
#include <assert.h>
#include <stdio.h>

#include "../../src/core/frame.h"
#include "../../src/core/screen.h"
#include "../../src/core/enemies.h"
#include "../../src/core/state.h"

int main(void) {
  Smbc smbc;
  smbc_init(&smbc);

  /* smb.asm: the pointer load — buffer 1, its offset, and buffer 2 */
  smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x00u;
  load_vram_addr_table(&smbc.state);
  assert(smbc.state.ram[SMBC_TEMP_VRAM_PTR] == 0x01u);
  assert(smbc.state.ram[SMBC_TEMP_VRAM_PTR + 1] == 0x03u);
  smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x05u;
  load_vram_addr_table(&smbc.state);
  assert(smbc.state.ram[SMBC_TEMP_VRAM_PTR] == 0x00u);
  assert(smbc.state.ram[SMBC_TEMP_VRAM_PTR + 1] == 0x03u);
  smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x07u;
  load_vram_addr_table(&smbc.state);
  assert(smbc.state.ram[SMBC_TEMP_VRAM_PTR] == 0x41u);
  assert(smbc.state.ram[SMBC_TEMP_VRAM_PTR + 1] == 0x03u);

  /* one set: three bytes to $2100, increment-by-1, no repeat */
  smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x00u;
  load_vram_addr_table(&smbc.state);
  smbc.state.ram[0x0301] = 0x21u;
  smbc.state.ram[0x0302] = 0x00u;
  smbc.state.ram[0x0303] = 0x03u;
  smbc.state.ram[0x0304] = 0xaau;
  smbc.state.ram[0x0305] = 0xbbu;
  smbc.state.ram[0x0306] = 0xccu;
  smbc.state.ram[0x0307] = 0x00u; /* terminator */
  update_screen(&smbc);
  assert(smbc.ppu.vram[0x2100] == 0xaau);
  assert(smbc.ppu.vram[0x2101] == 0xbbu);
  assert(smbc.ppu.vram[0x2102] == 0xccu);
  /* the indirect advanced onto the terminator */
  assert(smbc.state.ram[SMBC_TEMP_VRAM_PTR] == 0x07u);
  assert(smbc.state.ram[SMBC_TEMP_VRAM_PTR + 1] == 0x03u);
  /* InitScroll ran with A=0 */
  assert(smbc.ppu.scroll_x == 0x00u);
  assert(smbc.ppu.scroll_y == 0x00u);

  /* d6 set: repeat one byte; d7 set: increment-by-32 */
  load_vram_addr_table(&smbc.state);
  smbc.state.ram[0x0301] = 0x22u;
  smbc.state.ram[0x0302] = 0x40u;
  smbc.state.ram[0x0303] = 0xc2u; /* repeat + inc-32, length 2 */
  smbc.state.ram[0x0304] = 0xeeu;
  smbc.state.ram[0x0305] = 0x00u;
  update_screen(&smbc);
  assert(smbc.ppu.vram[0x2240] == 0xeeu);
  assert(smbc.ppu.vram[0x2260] == 0xeeu); /* +32, same byte */
  /* mirror took the increment-by-32 bit for the set */
  assert((smbc.state.ram[SMBC_MIRROR_PPU_CTRL1] & 0x04u) != 0);

  /* an empty buffer is just the fall-through to InitScroll */
  load_vram_addr_table(&smbc.state);
  smbc.state.ram[0x0301] = 0x00u;
  smbc.state.ram[SMBC_HORIZONTAL_SCROLL] = 0x99u; /* untouched here */
  update_screen(&smbc);
  assert(smbc.ppu.scroll_x == 0x00u);

  /* PrintStatusBarNumbers: nybbles $00 print the mario score twice */
  CoreState state;
  state_init(&state);
  state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  for (int i = 0; i < 6; i++) {
    state.ram[SMBC_DISPLAY_DIGITS + 6 + i] = (uint8_t)(i + 1); /* 123456 */
  }
  print_status_bar_numbers(&state, 0x00u);
  assert(state.ram[SMBC_VRAM_BUFFER1] == 0x20u);     /* high addr */
  assert(state.ram[SMBC_VRAM_BUFFER1 + 1] == 0x62u); /* low addr */
  assert(state.ram[SMBC_VRAM_BUFFER1 + 2] == 0x06u); /* length */
  for (int i = 0; i < 6; i++) {
    assert(state.ram[SMBC_VRAM_BUFFER1 + 3 + i] == (uint8_t)(i + 1));
  }
  /* the second set's header overwrote the first's terminator */
  assert(state.ram[SMBC_VRAM_BUFFER1 + 9] == 0x20u);
  assert(state.ram[SMBC_VRAM_BUFFER1 + 10] == 0x62u);
  assert(state.ram[SMBC_VRAM_BUFFER1 + 11] == 0x06u);
  for (int i = 0; i < 6; i++) {
    assert(state.ram[SMBC_VRAM_BUFFER1 + 12 + i] == (uint8_t)(i + 1));
  }
  assert(state.ram[SMBC_VRAM_BUFFER1 + 18] == 0x00u); /* buffer ends in zero */
  assert(state.ram[SMBC_VRAM_BUFFER1_OFFSET] == 0x12u); /* 2 sets x 9 */

  /* game timer set (selector 5) lives at $207a, 3 digits */
  state_init(&state);
  state.ram[SMBC_DISPLAY_DIGITS + 33] = 0x03u;
  state.ram[SMBC_DISPLAY_DIGITS + 34] = 0x00u;
  state.ram[SMBC_DISPLAY_DIGITS + 35] = 0x00u;
  print_status_bar_numbers(&state, 0x04u); /* low nybble 4 -> selector 5 */
  assert(state.ram[SMBC_VRAM_BUFFER1] == 0x20u);
  assert(state.ram[SMBC_VRAM_BUFFER1 + 1] == 0x7au);
  assert(state.ram[SMBC_VRAM_BUFFER1 + 2] == 0x03u);
  assert(state.ram[SMBC_VRAM_BUFFER1 + 3] == 0x03u);
  assert(state.ram[SMBC_VRAM_BUFFER1 + 6] == 0x20u); /* high nybble 0:
                                                       * mario score next */
  assert(state.ram[SMBC_VRAM_BUFFER1_OFFSET] == 0x0fu); /* 6 + 9 */
  /* selectors 6+ print nothing (timer caller's high nybble $a) */
  state_init(&state);
  print_status_bar_numbers(&state, 0xa4u);
  assert(state.ram[SMBC_VRAM_BUFFER1 + 1] == 0x7au); /* timer only */
  assert(state.ram[SMBC_VRAM_BUFFER1 + 6] == 0x00u); /* ends there */
  assert(state.ram[SMBC_VRAM_BUFFER1_OFFSET] == 0x06u);

  /* DigitsMathRoutine: 300 - 1 borrows twice down to 299 */
  state_init(&state);
  state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  const uint8_t y = 0x23u; /* smb.asm: game timer's last digit */
  state.ram[SMBC_DISPLAY_DIGITS + y - 2] = 0x03u;
  state.ram[SMBC_DIGIT_MODIFIER + 5] = 0xffu;
  digits_math_routine(&state, y);
  assert(state.ram[SMBC_DISPLAY_DIGITS + y - 2] == 0x02u);
  assert(state.ram[SMBC_DISPLAY_DIGITS + y - 1] == 0x09u);
  assert(state.ram[SMBC_DISPLAY_DIGITS + y] == 0x09u);
  for (int i = 0; i < 6; i++) {
    assert(state.ram[SMBC_DIGIT_MODIFIER + i] == 0x00u); /* erased */
  }
  /* 9 + 1 carries up */
  state_init(&state);
  state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  state.ram[SMBC_DISPLAY_DIGITS + y - 1] = 0x02u;
  state.ram[SMBC_DISPLAY_DIGITS + y] = 0x09u;
  state.ram[SMBC_DIGIT_MODIFIER + 5] = 0x01u;
  digits_math_routine(&state, y);
  assert(state.ram[SMBC_DISPLAY_DIGITS + y] == 0x00u);
  assert(state.ram[SMBC_DISPLAY_DIGITS + y - 1] == 0x03u);
  /* title screen: modifiers are erased but digits untouched */
  state_init(&state);
  state.ram[SMBC_DISPLAY_DIGITS + y] = 0x05u;
  state.ram[SMBC_DIGIT_MODIFIER + 5] = 0x01u;
  digits_math_routine(&state, y);
  assert(state.ram[SMBC_DISPLAY_DIGITS + y] == 0x05u);
  assert(state.ram[SMBC_DIGIT_MODIFIER + 5] == 0x00u);

  /* update_screen with a ROM table entry: the ground palette lands in
   * palette RAM at $3f00 */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x02u; /* GroundPaletteData */
  update_screen(&smbc);
  assert(smbc.ppu.vram[0x3f00] == 0x0fu);
  assert(smbc.ppu.vram[0x3f01] == 0x29u);
  assert(smbc.ppu.vram[0x3f02] == 0x1au);
  assert(smbc.ppu.vram[0x3f1f] == 0x17u); /* 32nd and last byte */
  /* the post-write $3F00/$00/$00/$00 sequence reinitialises the address
   * register; the pair toggle lands the final address at $0000 */
  assert(smbc.ppu.vram_addr == 0x0000u);

  /* a message entry: "THANK YOU MARIO!" at $2548 */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x0cu;
  update_screen(&smbc);
  const uint8_t mario[6] = {0x16u, 0x0au, 0x1bu, 0x12u, 0x18u, 0x2bu};
  assert(smbc.ppu.vram[0x2548] == 0x1du); /* T */
  assert(smbc.ppu.vram[0x254c] == 0x14u); /* K */
  for (int i = 0; i < 6; i++) {
    assert(smbc.ppu.vram[0x2552 + i] == mario[i]); /* MARIO! */
  }

  /* a chained entry: the mushroom retainer's two sentences in one go */
  smbc_init(&smbc);
  smbc.state.ram[SMBC_VRAM_BUFFER_ADDR_CTRL] = 0x0eu;
  update_screen(&smbc);
  assert(smbc.ppu.vram[0x25c5] == 0x0bu); /* B of BUT */
  assert(smbc.ppu.vram[0x2605] == 0x0au); /* A of ANOTHER */
  assert(smbc.ppu.vram[0x2613] == 0x2bu); /* ! */

  /* smb.asm: AddToScore — a +100 modifier lands on the score digits
   * through the current player's offset */
  state_init(&state);
  state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  state.ram[SMBC_CURRENT_PLAYER] = 0x00u;
  state.ram[SMBC_DIGIT_MODIFIER + 2] = 0x01u; /* hundreds digit */
  add_to_score(&state);
  assert(state.ram[SMBC_DISPLAY_DIGITS + 0x0bu - 3] == 0x01u);
  assert(state.ram[SMBC_DIGIT_MODIFIER + 2] == 0x00u);

  /* smb.asm: FloateyNumbersRoutine — control $00 leaves untouched */
  Smbc fl;
  smbc_init(&fl);
  fl.state.ram[SMBC_FLOATEY_NUM_CONTROL + 0] = 0x00u;
  fl.state.ram[SMBC_FLOATEY_NUM_TIMER + 0] = 0x30u;
  floatey_numbers_routine(&fl.state, 0x00u);
  assert(fl.state.ram[SMBC_FLOATEY_NUM_TIMER + 0] == 0x30u);

  /* control $0b clamped, an expired timer clears the control */
  smbc_init(&fl);
  fl.state.ram[SMBC_FLOATEY_NUM_CONTROL + 0] = 0x7fu;
  fl.state.ram[SMBC_FLOATEY_NUM_TIMER + 0] = 0x00u;
  floatey_numbers_routine(&fl.state, 0x00u);
  assert(fl.state.ram[SMBC_FLOATEY_NUM_CONTROL + 0] == 0x00u);

  /* timer at $2b with control $0b: a 1-up, its sound, and the score
   * update; the number rises a pixel and draws its sprite halves */
  smbc_init(&fl);
  fl.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  fl.state.ram[SMBC_FLOATEY_NUM_CONTROL + 0] = 0x0bu;
  fl.state.ram[SMBC_FLOATEY_NUM_TIMER + 0] = 0x2bu;
  fl.state.ram[SMBC_FLOATEY_NUM_Y_POS + 0] = 0x40u;
  fl.state.ram[SMBC_FLOATEY_NUM_X_POS + 0] = 0x50u;
  fl.state.ram[SMBC_ENEMY_ID + 0] = 0x06u; /* short enemy, alive */
  fl.state.ram[SMBC_ENEMY_SPR_DATA_OFFSET + 0] = 0x00u;
  fl.state.ram[SMBC_SPDATA_OFFSET_CTRL] = 0x01u;
  fl.state.ram[SMBC_ALT_SPR_DATA_OFFSET + 1] = 0x30u;
  floatey_numbers_routine(&fl.state, 0x00u);
  assert(fl.state.ram[SMBC_FLOATEY_NUM_TIMER + 0] == 0x2au);
  assert(fl.state.ram[SMBC_NUMBEROF_LIVES] == 0x01u);
  assert(fl.state.ram[SMBC_SQUARE2_SOUND_QUEUE] == SMBC_SFX_EXTRA_LIFE);
  assert(fl.state.ram[SMBC_FLOATEY_NUM_Y_POS + 0] == 0x3fu);
  /* the alternate OAM offset was chosen for an active short enemy */
  assert(fl.state.ram[SMBC_SPRITE_Y_POSITION + 0x30] == 0x37u);
  assert(fl.state.ram[SMBC_SPRITE_Y_POSITION + 0x34] == 0x37u);
  assert(fl.state.ram[SMBC_SPRITE_X_POSITION + 0x30] == 0x50u);
  assert(fl.state.ram[SMBC_SPRITE_X_POSITION + 0x34] == 0x58u);
  assert(fl.state.ram[SMBC_SPRITE_TILENUMBER + 0x30] == 0xfdu);
  assert(fl.state.ram[SMBC_SPRITE_TILENUMBER + 0x34] == 0xfeu);

  /* a spiny keeps its own OAM offset and a "200" rises in the bar */
  smbc_init(&fl);
  fl.state.ram[SMBC_OPER_MODE] = SMBC_MODE_GAME;
  fl.state.ram[SMBC_FLOATEY_NUM_CONTROL + 0] = 0x02u;
  fl.state.ram[SMBC_FLOATEY_NUM_TIMER + 0] = 0x2bu;
  fl.state.ram[SMBC_FLOATEY_NUM_Y_POS + 0] = 0x10u; /* status bar */
  fl.state.ram[SMBC_FLOATEY_NUM_X_POS + 0] = 0x20u;
  fl.state.ram[SMBC_ENEMY_ID + 0] = SMBC_SPINY;
  fl.state.ram[SMBC_ENEMY_SPR_DATA_OFFSET + 0] = 0x18u;
  fl.state.ram[SMBC_SPDATA_OFFSET_CTRL] = 0x00u;
  fl.state.ram[SMBC_ALT_SPR_DATA_OFFSET + 0] = 0x30u;
  floatey_numbers_routine(&fl.state, 0x00u);
  assert(fl.state.ram[SMBC_NUMBEROF_LIVES] == 0x00u);
  assert(fl.state.ram[SMBC_DISPLAY_DIGITS + 0x0bu - 1] == 0x02u);
  assert(fl.state.ram[SMBC_FLOATEY_NUM_Y_POS + 0] == 0x10u); /* no rise */
  assert(fl.state.ram[SMBC_SPRITE_TILENUMBER + 0x18] == 0xf7u);
  assert(fl.state.ram[SMBC_SPRITE_TILENUMBER + 0x1c] == 0xfbu);

  printf("tests/core/screen: ok\n");
  return 0;
}
