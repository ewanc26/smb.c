/* core/memory.c — see core/memory.h for the contract. */
#include "memory.h"

#include "frame.h"
#include "screen.h"
#include "state.h"

void initialize_memory(CoreState *state, uint8_t offset) {
  /* smb.asm: InitializeMemory. X walks pages $07..$00. Y starts at the
   * caller's boot offset on the first page and runs down to $00; the
   * loop then decrements it once more to $ff, so every later page is
   * cleared in full. Only the first page stops at the offset. */
  for (int page = 0x07; page >= 0; page--) {
    for (int y = (page == 0x07) ? offset : 0xff; y >= 0; y--) {
      if (page == 0x01 && y >= 0x60) {
        continue; /* smb.asm: SkipByte — the stack at $0160-$01FF */
      }
      state->ram[(page << 8) | y] = 0x00;
    }
  }
}

/* smb.asm: WriteNTAddr — clear one name table, then the buffer header and
 * the scroll variables. The 6502's X is zero after the tile loop, which is
 * what the buffer and scroll writes store. */
static void write_nt_addr(Smbc *smbc, uint8_t high) {
  CoreState *state = &smbc->state;
  Ppu *ppu = &smbc->ppu;
  ppu_write(ppu, 6, high);
  ppu_write(ppu, 6, 0x00u);
  /* smb.asm: InitNTLoop — X=$04, Y=$C0: exactly 768 blank-tile (#$24)
   * writes, verified against the donor ROM */
  for (int x = 0x04; x > 0; x--) {
    for (int y = 0xc0; y > 0; y--) {
      ppu_write(ppu, 7, 0x24u);
    }
  }
  state->ram[SMBC_VRAM_BUFFER1_OFFSET] = 0x00u; /* txa */
  state->ram[SMBC_VRAM_BUFFER1] = 0x00u;
  for (int y = 64; y > 0; y--) {
    ppu_write(ppu, 7, 0x00u); /* smb.asm: InitATLoop */
  }
  state->ram[SMBC_HORIZONTAL_SCROLL] = 0x00u;
  state->ram[SMBC_VERTICAL_SCROLL] = 0x00u;
  init_scroll(smbc, 0x00u); /* smb.asm: jmp InitScroll */
}

void initialize_name_tables(Smbc *smbc) {
  /* smb.asm: InitializeNameTables */
  CoreState *state = &smbc->state;
  (void)ppu_read(&smbc->ppu, 2); /* lda PPU_STATUS: reset the flip-flop */
  /* sprites from the first 4K, background from the second; increment 1 */
  write_ppu_reg1(
      smbc, (uint8_t)((state->ram[SMBC_MIRROR_PPU_CTRL1] | 0x10u) & 0xf0u));
  write_nt_addr(smbc, 0x24u); /* smb.asm: name table 1 first */
  write_nt_addr(smbc, 0x20u); /* smb.asm: then name table 0 */
}
