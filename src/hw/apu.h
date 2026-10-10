/* hw/apu.h — APU register capture.
 *
 * Owns: the $4000-$4017 audio register image. Sound synthesis is not
 * modelled; writes are captured so the translated music engine can run
 * unmodified, and a host audio backend can consume the register image
 * later.
 */
#ifndef SMBC_HW_APU_H
#define SMBC_HW_APU_H

#include <stdint.h>

#define SMBC_APU_REGISTER_COUNT 0x18u

typedef struct {
  uint8_t regs[SMBC_APU_REGISTER_COUNT]; /* $4000-$4017 */
} Apu;

void apu_init(Apu *apu);

/* Address is the hardware address ($4000-$4017); out-of-range writes are
 * dropped, matching how the game only ever touches these. */
void apu_write(Apu *apu, uint16_t addr, uint8_t value);
uint8_t apu_read(const Apu *apu, uint16_t addr);

#endif /* SMBC_HW_APU_H */
