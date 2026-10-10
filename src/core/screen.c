/* core/screen.c — see core/screen.h for the contract. */
#include "screen.h"

#include "../hw/ppu.h"
#include "../data/messages.h"
#include "../data/palettes.h"
#include "../data/statusbar.h"
#include "frame.h"

#include <stddef.h>

void write_ppu_reg1(Smbc *smbc, uint8_t value) {
  /* smb.asm: WritePPUReg1 */
  ppu_write(&smbc->ppu, 0, value);
  smbc->state.ram[SMBC_MIRROR_PPU_CTRL1] = value;
}

/* smb.asm: OutputNumbers — append one status bar set to VRAM buffer 1.
 * The selector runs 1-6 after incrementing; six or more prints nothing.
 * Faithful to the original: no bounds check on the buffer. */
static void output_numbers(CoreState *state, uint8_t selector) {
  selector = (uint8_t)((selector + 1u) & 0x0fu);
  if (selector >= 6u) return; /* smb.asm: ExitOutputN */
  uint8_t y = (uint8_t)(selector * 2u);
  uint8_t x = state->ram[SMBC_VRAM_BUFFER1_OFFSET];
  uint16_t base = (uint16_t)(SMBC_VRAM_BUFFER1 + x);
  /* smb.asm: top score display goes to $22f0, everything else to $20xx */
  state->ram[base] = (y == 0u) ? 0x22u : 0x20u;
  state->ram[base + 1] = SMBC_STATUS_BAR_DATA[y];
  state->ram[base + 2] = SMBC_STATUS_BAR_DATA[y + 1];
  uint8_t len = state->ram[base + 2];
  uint8_t off = (uint8_t)(SMBC_STATUS_BAR_OFFSET[selector] -
                          SMBC_STATUS_BAR_DATA[y + 1]);
  for (uint8_t i = 0; i < len; i++) { /* smb.asm: DigitPLoop */
    state->ram[base + 3 + i] = state->ram[SMBC_DISPLAY_DIGITS + off + i];
  }
  state->ram[base + 3 + len] = 0x00u; /* smb.asm: zero terminator */
  state->ram[SMBC_VRAM_BUFFER1_OFFSET] = (uint8_t)(x + 3u + len);
}

void print_status_bar_numbers(CoreState *state, uint8_t nybbles) {
  /* smb.asm: PrintStatusBarNumbers */
  state->ram[SMBC_TEMP_VRAM_PTR] = nybbles; /* smb.asm: sta $00 */
  output_numbers(state, nybbles);           /* smb.asm: coin display */
  output_numbers(state, (uint8_t)(nybbles >> 4)); /* smb.asm: score */
}

void digits_math_routine(CoreState *state, uint8_t y) {
  /* smb.asm: DigitsMathRoutine */
  if (state->ram[SMBC_OPER_MODE] != SMBC_MODE_TITLE) {
    for (int x = 5; x >= 0; x--) {           /* smb.asm: AddModLoop */
      uint8_t v = (uint8_t)(state->ram[SMBC_DIGIT_MODIFIER + x] +
                            state->ram[SMBC_DISPLAY_DIGITS + y]);
      if (v & 0x80u) { /* smb.asm: BorrowOne — negative means borrow */
        state->ram[SMBC_DIGIT_MODIFIER + x - 1]--;
        v = 0x09u;
      } else if (v >= 10u) { /* smb.asm: CarryOne */
        v = (uint8_t)(v - 10u);
        state->ram[SMBC_DIGIT_MODIFIER + x - 1]++;
      }
      state->ram[SMBC_DISPLAY_DIGITS + y] = v; /* smb.asm: StoreNewD */
      y--;
    }
  }
  for (size_t x = 0; x < 6u; x++) { /* smb.asm: EraseMLoop */
    state->ram[SMBC_DIGIT_MODIFIER + x] = 0x00u;
  }
}

void init_scroll(Smbc *smbc, uint8_t value) {
  /* smb.asm: InitScroll */
  ppu_write(&smbc->ppu, 5, value);
  ppu_write(&smbc->ppu, 5, value);
}

/* smb.asm: VRAM_AddrTable_Low / VRAM_AddrTable_High — the pointers the NMI
 * loads into the $00/$01 indirect. RAM entries are the two VRAM buffers.
 * The ROM entries point at palette and message data (data/palettes,
 * data/messages); the host cannot express those as 6502 address halves, so
 * the low/high bytes stay zero here and the actual data is reached through
 * VRAM_ADDR_ROM_DATA below. */
#define VRAM_ADDR_TABLE_SIZE 19u
static const uint8_t VRAM_ADDR_TABLE_LOW[VRAM_ADDR_TABLE_SIZE] = {
    0x01u, /*  0: VRAM_Buffer1 */
    0x00u, /*  1: WaterPaletteData (ROM) */
    0x00u, /*  2: GroundPaletteData (ROM) */
    0x00u, /*  3: UndergroundPaletteData (ROM) */
    0x00u, /*  4: CastlePaletteData (ROM) */
    0x00u, /*  5: VRAM_Buffer1_Offset */
    0x41u, /*  6: VRAM_Buffer2 */
    0x41u, /*  7: VRAM_Buffer2 */
    0x00u, /*  8: BowserPaletteData (ROM) */
    0x00u, /*  9: DaySnowPaletteData (ROM) */
    0x00u, /* 10: NightSnowPaletteData (ROM) */
    0x00u, /* 11: MushroomPaletteData (ROM) */
    0x00u, /* 12: MarioThanksMessage (ROM) */
    0x00u, /* 13: LuigiThanksMessage (ROM) */
    0x00u, /* 14: MushroomRetainerSaved (ROM) */
    0x00u, /* 15: PrincessSaved1 (ROM) */
    0x00u, /* 16: PrincessSaved2 (ROM) */
    0x00u, /* 17: WorldSelectMessage1 (ROM) */
    0x00u, /* 18: WorldSelectMessage2 (ROM) */
};
static const uint8_t VRAM_ADDR_TABLE_HIGH[VRAM_ADDR_TABLE_SIZE] = {
    0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x03u, 0x03u, 0x03u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
};

/* the host-side counterparts of the ROM addr table entries; NULL marks the
 * RAM entries, which read through $00/$01 as the original does */
static const uint8_t *const VRAM_ADDR_ROM_DATA[VRAM_ADDR_TABLE_SIZE] = {
    NULL,                             /*  0: VRAM_Buffer1 */
    SMBC_WATER_PALETTE_DATA,          /*  1 */
    SMBC_GROUND_PALETTE_DATA,         /*  2 */
    SMBC_UNDERGROUND_PALETTE_DATA,    /*  3 */
    SMBC_CASTLE_PALETTE_DATA,         /*  4 */
    NULL,                             /*  5: VRAM_Buffer1_Offset */
    NULL,                             /*  6: VRAM_Buffer2 */
    NULL,                             /*  7: VRAM_Buffer2 */
    SMBC_BOWSER_PALETTE_DATA,         /*  8 */
    SMBC_DAY_SNOW_PALETTE_DATA,       /*  9 */
    SMBC_NIGHT_SNOW_PALETTE_DATA,     /* 10 */
    SMBC_MUSHROOM_PALETTE_DATA,       /* 11 */
    SMBC_MARIO_THANKS_MESSAGE,        /* 12 */
    SMBC_LUIGI_THANKS_MESSAGE,        /* 13 */
    SMBC_MUSHROOM_RETAINER_SAVED,     /* 14 */
    SMBC_PRINCESS_SAVED1,             /* 15 */
    SMBC_PRINCESS_SAVED2,             /* 16 */
    SMBC_WORLD_SELECT_MESSAGE1,       /* 17 */
    SMBC_WORLD_SELECT_MESSAGE2,       /* 18 */
};

void load_vram_addr_table(CoreState *state) {
  /* smb.asm: ldx VRAM_Buffer_AddrCtrl; lda VRAM_AddrTable_Low,x ... */
  uint8_t ctrl = state->ram[SMBC_VRAM_BUFFER_ADDR_CTRL];
  if (ctrl >= VRAM_ADDR_TABLE_SIZE) {
    ctrl = 0; /* the original trusts the index; clamp rather than read OOB */
  }
  state->ram[SMBC_TEMP_VRAM_PTR] = VRAM_ADDR_TABLE_LOW[ctrl];
  state->ram[SMBC_TEMP_VRAM_PTR + 1] = VRAM_ADDR_TABLE_HIGH[ctrl];
}

/* smb.asm: the update-set reader. The original walks a 6502 ($00/$01)
 * indirect pointer through the update sets, whether they live in RAM (the two
 * VRAM buffers) or ROM (palette and message data). RAM entries keep that
 * pointer in RAM exactly as the original does; ROM entries cannot be
 * expressed as two RAM bytes, so the host walks an index into the ROM table
 * instead. `pos` is the offset of the next byte within the active source. */
static uint8_t source_byte(const CoreState *state, const uint8_t *rom,
                           uint16_t pos, uint8_t y) {
  if (rom != NULL) {
    return rom[(uint16_t)(pos + y)];
  }
  /* RAM: pos IS the $00/$01 pointer, and y is the offset within the set */
  return state->ram[(uint16_t)(pos + y)];
}

void update_screen(Smbc *smbc) {
  /* smb.asm: UpdateScreen */
  CoreState *state = &smbc->state;
  Ppu *ppu = &smbc->ppu;
  (void)ppu_read(ppu, 2); /* ldx PPU_STATUS: reset the flip-flop */

  uint8_t ctrl = state->ram[SMBC_VRAM_BUFFER_ADDR_CTRL];
  const uint8_t *rom =
      ctrl < VRAM_ADDR_TABLE_SIZE ? VRAM_ADDR_ROM_DATA[ctrl] : NULL;

  uint16_t pos;
  if (rom != NULL) {
    pos = 0u; /* ROM: index into the table */
  } else {
    pos = (uint16_t)(state->ram[SMBC_TEMP_VRAM_PTR] |
                    ((uint16_t)state->ram[SMBC_TEMP_VRAM_PTR + 1] << 8));
  }

  uint8_t first = source_byte(state, rom, pos, 0);
  while (first != 0) {
    /* smb.asm: WriteBufferToScreen — one update set:
     *   byte 1-2: VRAM address, high then low
     *   byte 3:   d7 increment-by-32, d6 repeat, d5-d0 length
     *   then the data bytes */
    uint8_t y = 0;
    ppu_write(ppu, 6, first);
    ppu_write(ppu, 6, source_byte(state, rom, pos, (uint8_t)(y + 1u)));
    y = (uint8_t)(y + 2u);
    uint8_t third = source_byte(state, rom, pos, y);

    uint8_t mirror = (uint8_t)(state->ram[SMBC_MIRROR_PPU_CTRL1] | 0x04u);
    if ((third & 0x80u) == 0) {
      mirror &= 0xfbu; /* smb.asm: bcs SetupWrites / and #%11111011 */
    }
    write_ppu_reg1(smbc, mirror);

    bool repeat = (third & 0x40u) != 0; /* d6 of the third byte */
    uint8_t length = (uint8_t)(third & 0x3fu);
    if (repeat) {
      y++; /* smb.asm: ora #%00000010; iny */
    }
    for (; length != 0; length--) {
      /* smb.asm: OutputToVRAM — repeat mode holds Y on the data byte */
      if (!repeat) {
        y++;
      }
      ppu_write(ppu, 7, source_byte(state, rom, pos, y));
    }

    /* smb.asm: sec; tya; adc $00; sta $00; lda #$00; adc $01; sta $01 —
     * advance past this set onto the next header. RAM entries keep the
     * pointer in $00/$01 as the original does. */
    pos = (uint16_t)(pos + (uint16_t)y + 1u);
    if (rom == NULL) {
      state->ram[SMBC_TEMP_VRAM_PTR] = (uint8_t)pos;
      state->ram[SMBC_TEMP_VRAM_PTR + 1] = (uint8_t)(pos >> 8);
    }
    ppu_write(ppu, 6, 0x3fu);   /* smb.asm: the $3F00/$00/$00 sequence */
    ppu_write(ppu, 6, 0x00u);
    ppu_write(ppu, 6, 0x00u);
    ppu_write(ppu, 6, 0x00u);
    first = source_byte(state, rom, pos, 0);
  }
  init_scroll(smbc, first); /* smb.asm: falls through to InitScroll, A=0 */
}

/* smb.asm: TopScoreCheck — six-digit subtraction, lowest digit first; the
 * score is copied only if no borrow survives the final digit. */
static void top_score_check(CoreState *state, int x) {
  int y = 5;
  int no_borrow = 1; /* sec on entry */
  for (int i = 0; i < 6; i++) {
    int diff = state->ram[SMBC_PLAYER_SCORE_DISPLAY + x] -
               state->ram[SMBC_TOP_SCORE_DISPLAY + y] - (1 - no_borrow);
    no_borrow = diff >= 0;
    x--;
    y--;
  }
  if (!no_borrow) {
    return; /* smb.asm: NoTopSc */
  }
  x++;
  y++;
  for (int i = 0; i < 6; i++) {
    state->ram[SMBC_TOP_SCORE_DISPLAY + y] =
        state->ram[SMBC_PLAYER_SCORE_DISPLAY + x];
    x++;
    y++;
  }
}

void update_top_score(CoreState *state) {
  /* smb.asm: UpdateTopScore — mario's score, then luigi's */
  top_score_check(state, 0x05);
  top_score_check(state, 0x0b);
}
