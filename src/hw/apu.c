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
}

uint8_t apu_read(const Apu *apu, uint16_t addr) {
  if (addr < 0x4000u || addr >= 0x4000u + SMBC_APU_REGISTER_COUNT) {
    return 0;
  }
  return apu->regs[addr - 0x4000u];
}
