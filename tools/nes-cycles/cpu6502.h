/* tools/nes-cycles/cpu6502.h — a cycle-counting MOS 6502 (dev-only, #41).
 *
 * Runs the official opcodes with their documented base cycle counts and the
 * page-cross and taken-branch penalties. Memory goes through a bus the caller
 * supplies, so the NES machine (nes_machine.c) decides what each address
 * does. NMI is edge-triggered by the caller through cpu6502_nmi.
 *
 * Not a game-build file: nothing under src/ depends on it. */
#ifndef SMBC_TOOLS_CPU6502_H
#define SMBC_TOOLS_CPU6502_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Cpu6502 Cpu6502;

typedef struct {
  uint8_t (*read)(void *ctx, uint16_t addr);
  void (*write)(void *ctx, uint16_t addr, uint8_t value);
  void *ctx;
} Cpu6502Bus;

struct Cpu6502 {
  uint8_t a, x, y, sp, p;
  uint16_t pc;
  uint64_t cycles;      /* total CPU cycles since power-on */
  bool nmi_requested;   /* set by the machine; serviced before the next opcode */
  bool jammed;          /* an illegal opcode was hit; execution stopped */
  uint16_t jam_pc;
  uint8_t jam_opcode;
  Cpu6502Bus bus;
};

#define CPU_FLAG_C 0x01u
#define CPU_FLAG_Z 0x02u
#define CPU_FLAG_I 0x04u
#define CPU_FLAG_D 0x08u
#define CPU_FLAG_B 0x10u
#define CPU_FLAG_U 0x20u
#define CPU_FLAG_V 0x40u
#define CPU_FLAG_N 0x80u

/* Reset: loads PC from $FFFC, sets I, and takes the reset cycles. */
void cpu6502_reset(Cpu6502 *cpu, Cpu6502Bus bus);

/* Execute one instruction (or one NMI entry if one is pending). Returns the
 * cycles it took, and advances cpu->cycles by the same amount. */
unsigned cpu6502_step(Cpu6502 *cpu);

/* Charge extra cycles that the bus takes outside the CPU (OAM DMA stalls). */
void cpu6502_stall(Cpu6502 *cpu, unsigned cycles);

#endif
