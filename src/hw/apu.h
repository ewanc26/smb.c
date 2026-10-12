/* hw/apu.h — APU register capture and the write log.
 *
 * Owns: the $4000-$4017 audio register image, plus a per-frame log of
 * the writes the translated sound engine makes. The image holds the
 * last written value; the log carries the write EVENTS, because APU
 * write side effects (envelope restart, length reload) trigger on the
 * write itself, not on a value change — a snapshot cannot represent
 * writing the same value twice. Synthesis (#35, hw/apu_synth.c)
 * drains the log once per frame; the core never reads it, and it is
 * not part of the replay state hash.
 */
#ifndef SMBC_HW_APU_H
#define SMBC_HW_APU_H

#include <stdint.h>

#define SMBC_APU_REGISTER_COUNT 0x18u
#define SMBC_APU_WRITE_LOG 64u /* per-frame write events; past the cap
                                * edge semantics drop (register values
                                * still land) — only reached when
                                * nothing drains the log */

typedef struct {
  uint16_t addr; /* offset from $4000 */
  uint8_t value;
} ApuWrite;

typedef struct {
  uint8_t regs[SMBC_APU_REGISTER_COUNT]; /* $4000-$4017 */
  ApuWrite log[SMBC_APU_WRITE_LOG];      /* this frame's writes, in
                                          * order; drained by the
                                          * synth */
  uint8_t log_len;
  uint8_t status4015; /* length-active bits, maintained by the synth;
                       * what a $4015 read reports */
} Apu;

void apu_init(Apu *apu);

/* Address is the hardware address ($4000-$4017); out-of-range writes are
 * dropped, matching how the game only ever touches these. */
void apu_write(Apu *apu, uint16_t addr, uint8_t value);
uint8_t apu_read(const Apu *apu, uint16_t addr);

#endif /* SMBC_HW_APU_H */
