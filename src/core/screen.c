/* core/screen.c — see core/screen.h for the contract. */
#include "screen.h"

#include "../hw/ppu.h"
#include "frame.h"

void write_ppu_reg1(Smbc *smbc, uint8_t value) {
  /* smb.asm: WritePPUReg1 */
  ppu_write(&smbc->ppu, 0, value);
  smbc->state.ram[SMBC_MIRROR_PPU_CTRL1] = value;
}

void init_scroll(Smbc *smbc, uint8_t value) {
  /* smb.asm: InitScroll */
  ppu_write(&smbc->ppu, 5, value);
  ppu_write(&smbc->ppu, 5, value);
}

/* smb.asm: VRAM_AddrTable_Low / VRAM_AddrTable_High — the pointers the NMI
 * loads into the $00/$01 indirect. RAM entries are the two VRAM buffers.
 * TODO(data): the ROM entries point at palette and message data in the
 * disassembly; they are zero here until those tables are translated, which
 * reads as an empty update set. */
#define VRAM_ADDR_TABLE_SIZE 19u
static const uint8_t VRAM_ADDR_TABLE_LOW[VRAM_ADDR_TABLE_SIZE] = {
    0x01u, /*  0: VRAM_Buffer1 */
    0x00u, /*  1: WaterPaletteData (ROM, pending) */
    0x00u, /*  2: GroundPaletteData (ROM, pending) */
    0x00u, /*  3: UndergroundPaletteData (ROM, pending) */
    0x00u, /*  4: CastlePaletteData (ROM, pending) */
    0x00u, /*  5: VRAM_Buffer1_Offset */
    0x41u, /*  6: VRAM_Buffer2 */
    0x41u, /*  7: VRAM_Buffer2 */
    0x00u, /*  8: BowserPaletteData (ROM, pending) */
    0x00u, /*  9: DaySnowPaletteData (ROM, pending) */
    0x00u, /* 10: NightSnowPaletteData (ROM, pending) */
    0x00u, /* 11: MushroomPaletteData (ROM, pending) */
    0x00u, /* 12: MarioThanksMessage (ROM, pending) */
    0x00u, /* 13: LuigiThanksMessage (ROM, pending) */
    0x00u, /* 14: MushroomRetainerSaved (ROM, pending) */
    0x00u, /* 15: PrincessSaved1 (ROM, pending) */
    0x00u, /* 16: PrincessSaved2 (ROM, pending) */
    0x00u, /* 17: WorldSelectMessage1 (ROM, pending) */
    0x00u, /* 18: WorldSelectMessage2 (ROM, pending) */
};
static const uint8_t VRAM_ADDR_TABLE_HIGH[VRAM_ADDR_TABLE_SIZE] = {
    0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x03u, 0x03u, 0x03u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
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

static uint8_t read_indirect(const CoreState *state, uint8_t y) {
  uint16_t base = (uint16_t)(state->ram[SMBC_TEMP_VRAM_PTR] |
                             (state->ram[SMBC_TEMP_VRAM_PTR + 1] << 8));
  return state->ram[(uint16_t)(base + y)];
}

/* smb.asm: sec; tya; adc $00; sta $00; lda #$00; adc $01; sta $01 */
static void advance_indirect(CoreState *state, uint8_t y) {
  uint16_t lo = (uint16_t)(state->ram[SMBC_TEMP_VRAM_PTR] + y + 1u);
  state->ram[SMBC_TEMP_VRAM_PTR] = (uint8_t)lo;
  state->ram[SMBC_TEMP_VRAM_PTR + 1] =
      (uint8_t)(state->ram[SMBC_TEMP_VRAM_PTR + 1] + (lo > 0xffu ? 1u : 0u));
}

void update_screen(Smbc *smbc) {
  /* smb.asm: UpdateScreen */
  CoreState *state = &smbc->state;
  Ppu *ppu = &smbc->ppu;
  (void)ppu_read(ppu, 2); /* ldx PPU_STATUS: reset the flip-flop */

  uint8_t first = read_indirect(state, 0);
  while (first != 0) {
    /* smb.asm: WriteBufferToScreen — one update set:
     *   byte 1-2: VRAM address, high then low
     *   byte 3:   d7 increment-by-32, d6 repeat, d5-d0 length
     *   then the data bytes */
    uint8_t y = 0;
    ppu_write(ppu, 6, first);
    ppu_write(ppu, 6, read_indirect(state, (uint8_t)(y + 1u)));
    y = (uint8_t)(y + 2u);
    uint8_t third = read_indirect(state, y);

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
      ppu_write(ppu, 7, read_indirect(state, y));
    }

    advance_indirect(state, y); /* past this set, onto the next header */
    ppu_write(ppu, 6, 0x3fu);   /* smb.asm: the $3F00/$00/$00 sequence */
    ppu_write(ppu, 6, 0x00u);
    ppu_write(ppu, 6, 0x00u);
    ppu_write(ppu, 6, 0x00u);
    first = read_indirect(state, 0);
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
