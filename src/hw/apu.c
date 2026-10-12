/* hw/apu.c — see hw/apu.h for the contract. */
#include "apu.h"

void apu_init(Apu *apu) {
  *apu = (Apu){0};
}

void apu_write(Apu *apu, uint16_t addr, uint8_t value) {
  if (addr < 0x4000u || addr >= 0x4000u + SMBC_APU_REGISTER_COUNT) {
    return;
  }
  apu->regs[addr - 0x4000u] = value;
  if (apu->log_len < SMBC_APU_WRITE_LOG) {
    apu->log[apu->log_len++] =
        (ApuWrite){.addr = (uint16_t)(addr - 0x4000u), .value = value};
  }
}

uint8_t apu_read(const Apu *apu, uint16_t addr) {
  if (addr < 0x4000u || addr >= 0x4000u + SMBC_APU_REGISTER_COUNT) {
    return 0;
  }
  if (addr == 0x4015u) {
    /* status read: length-counter-active bits (d0-d4), maintained by
     * the synth; the game never reads $4015, so this exists for
     * hardware correctness rather than gameplay */
    return apu->status4015;
  }
  return apu->regs[addr - 0x4000u];
}
