/* tools/nes-cycles/cpu6502.c — see cpu6502.h. Dev-only (#41). */
#include "cpu6502.h"

#include <stdio.h>

/* Base cycles for the official opcodes; 0 marks an illegal opcode. */
static const uint8_t BASE[256] = {
    /*00*/ 7,6,0,0,0,3,5,0, 3,2,2,0,0,4,6,0,
    /*10*/ 2,5,0,0,0,4,6,0, 2,4,0,0,0,4,7,0,
    /*20*/ 6,6,0,0,3,3,5,0, 4,2,2,0,4,4,6,0,
    /*30*/ 2,5,0,0,0,4,6,0, 2,4,0,0,0,4,7,0,
    /*40*/ 6,6,0,0,0,3,5,0, 3,2,2,0,3,4,6,0,
    /*50*/ 2,5,0,0,0,4,6,0, 2,4,0,0,0,4,7,0,
    /*60*/ 6,6,0,0,0,3,5,0, 4,2,2,0,5,4,6,0,
    /*70*/ 2,5,0,0,0,4,6,0, 2,4,0,0,0,4,7,0,
    /*80*/ 0,6,0,0,3,3,3,0, 2,0,2,0,4,4,4,0,
    /*90*/ 2,6,0,0,4,4,4,0, 2,5,2,0,0,5,0,0,
    /*A0*/ 2,6,2,0,3,3,3,0, 2,2,2,0,4,4,4,0,
    /*B0*/ 2,5,0,0,4,4,4,0, 2,4,2,0,4,4,4,0,
    /*C0*/ 2,6,0,0,3,3,5,0, 2,2,2,0,4,4,6,0,
    /*D0*/ 2,5,0,0,0,4,6,0, 2,4,0,0,0,4,7,0,
    /*E0*/ 2,6,0,0,3,3,5,0, 2,2,2,0,4,4,6,0,
    /*F0*/ 2,5,0,0,0,4,6,0, 2,4,0,0,0,4,7,0,
};

static uint8_t rd(Cpu6502 *c, uint16_t a) { return c->bus.read(c->bus.ctx, a); }
static void wr(Cpu6502 *c, uint16_t a, uint8_t v) { c->bus.write(c->bus.ctx, a, v); }
static uint8_t fetch(Cpu6502 *c) { return rd(c, c->pc++); }
static uint16_t rd16(Cpu6502 *c, uint16_t a) {
  return (uint16_t)(rd(c, a) | (rd(c, (uint16_t)(a + 1)) << 8));
}
static void push(Cpu6502 *c, uint8_t v) { wr(c, (uint16_t)(0x100u | c->sp--), v); }
static uint8_t pop(Cpu6502 *c) { return rd(c, (uint16_t)(0x100u | ++c->sp)); }
/* Two-byte operands and pulls are read low byte first, one call at a time:
 * C leaves the order of calls within one expression unspecified, and bus
 * reads can have side effects. */
static uint16_t fetch16(Cpu6502 *c) {
  uint16_t lo = fetch(c);
  return (uint16_t)(lo | (fetch(c) << 8));
}
static uint16_t pop16(Cpu6502 *c) {
  uint16_t lo = pop(c);
  return (uint16_t)(lo | (pop(c) << 8));
}

static void setzn(Cpu6502 *c, uint8_t v) {
  c->p = (uint8_t)((c->p & ~(CPU_FLAG_Z | CPU_FLAG_N)) |
                   (v == 0 ? CPU_FLAG_Z : 0) | (v & 0x80u));
}

static void set_flag(Cpu6502 *c, uint8_t f, bool on) {
  c->p = on ? (uint8_t)(c->p | f) : (uint8_t)(c->p & ~f);
}

static void adc(Cpu6502 *c, uint8_t v) {
  unsigned sum = c->a + v + (c->p & CPU_FLAG_C ? 1u : 0u);
  set_flag(c, CPU_FLAG_C, sum > 0xffu);
  set_flag(c, CPU_FLAG_V, ((c->a ^ sum) & (v ^ sum) & 0x80u) != 0);
  c->a = (uint8_t)sum;
  setzn(c, c->a);
}

static void compare(Cpu6502 *c, uint8_t reg, uint8_t v) {
  set_flag(c, CPU_FLAG_C, reg >= v);
  setzn(c, (uint8_t)(reg - v));
}

static uint8_t shift_op(Cpu6502 *c, int kind, uint8_t v) {
  uint8_t out;
  bool carry_out;
  switch (kind) {
  case 0: /* ASL */
    carry_out = (v & 0x80u) != 0;
    out = (uint8_t)(v << 1);
    break;
  case 1: /* ROL */
    carry_out = (v & 0x80u) != 0;
    out = (uint8_t)((v << 1) | (c->p & CPU_FLAG_C ? 1u : 0u));
    break;
  case 2: /* LSR */
    carry_out = (v & 0x01u) != 0;
    out = (uint8_t)(v >> 1);
    break;
  default: /* ROR */
    carry_out = (v & 0x01u) != 0;
    out = (uint8_t)((v >> 1) | (c->p & CPU_FLAG_C ? 0x80u : 0u));
    break;
  }
  set_flag(c, CPU_FLAG_C, carry_out);
  setzn(c, out);
  return out;
}

/* Effective address for the memory modes. page_cross is set when an indexed
 * read crosses a page, which costs one extra cycle on read instructions. */
typedef enum { M_ZP, M_ZPX, M_ZPY, M_ABS, M_ABX, M_ABY, M_IZX, M_IZY } Mode;

static uint16_t effective(Cpu6502 *c, Mode m, bool *page_cross) {
  *page_cross = false;
  switch (m) {
  case M_ZP:
    return fetch(c);
  case M_ZPX:
    return (uint8_t)(fetch(c) + c->x);
  case M_ZPY:
    return (uint8_t)(fetch(c) + c->y);
  case M_ABS: {
    uint16_t lo = fetch(c);
    return (uint16_t)(lo | (fetch(c) << 8));
  }
  case M_ABX:
  case M_ABY: {
    uint16_t base = fetch(c);
    base = (uint16_t)(base | (fetch(c) << 8));
    uint16_t index = m == M_ABX ? c->x : c->y;
    uint16_t addr = (uint16_t)(base + index);
    *page_cross = (addr & 0xff00u) != (base & 0xff00u);
    return addr;
  }
  case M_IZX: {
    uint8_t zp = (uint8_t)(fetch(c) + c->x);
    return (uint16_t)(rd(c, zp) | (rd(c, (uint8_t)(zp + 1)) << 8));
  }
  case M_IZY: {
    uint8_t zp = fetch(c);
    uint16_t base = (uint16_t)(rd(c, zp) | (rd(c, (uint8_t)(zp + 1)) << 8));
    uint16_t addr = (uint16_t)(base + c->y);
    *page_cross = (addr & 0xff00u) != (base & 0xff00u);
    return addr;
  }
  }
  return 0;
}

static void branch(Cpu6502 *c, bool taken, unsigned *cycles) {
  int8_t off = (int8_t)fetch(c);
  if (!taken) return;
  uint16_t target = (uint16_t)(c->pc + off);
  *cycles += 1;
  if ((target & 0xff00u) != (c->pc & 0xff00u)) *cycles += 1;
  c->pc = target;
}

void cpu6502_reset(Cpu6502 *cpu, Cpu6502Bus bus) {
  cpu->a = cpu->x = cpu->y = 0;
  cpu->sp = 0xfd;
  cpu->p = CPU_FLAG_I | CPU_FLAG_U;
  cpu->bus = bus;
  cpu->nmi_requested = false;
  cpu->jammed = false;
  cpu->cycles = 0;
  cpu->pc = rd16(cpu, 0xfffc);
  cpu->cycles += 7;
}

void cpu6502_stall(Cpu6502 *cpu, unsigned cycles) {
  cpu->cycles += cycles;
}

static unsigned service_nmi(Cpu6502 *c) {
  c->nmi_requested = false;
  push(c, (uint8_t)(c->pc >> 8));
  push(c, (uint8_t)c->pc);
  push(c, (uint8_t)((c->p | CPU_FLAG_U) & ~CPU_FLAG_B));
  c->p |= CPU_FLAG_I;
  c->pc = rd16(c, 0xfffa);
  return 7;
}

unsigned cpu6502_step(Cpu6502 *c) {
  if (c->jammed) return 0;
  if (c->nmi_requested) {
    unsigned n = service_nmi(c);
    c->cycles += n;
    return n;
  }

  uint16_t op_pc = c->pc;
  uint8_t op = fetch(c);
  unsigned cycles = BASE[op];
  if (cycles == 0) {
    c->jammed = true;
    c->jam_pc = op_pc;
    c->jam_opcode = op;
    return 0;
  }
  bool cross = false;
  uint16_t addr;
  uint8_t v;

  switch (op) {
  /* --- control and stack --- */
  case 0x00: /* BRK */
    c->pc++;
    push(c, (uint8_t)(c->pc >> 8));
    push(c, (uint8_t)c->pc);
    push(c, (uint8_t)(c->p | CPU_FLAG_B | CPU_FLAG_U));
    c->p |= CPU_FLAG_I;
    c->pc = rd16(c, 0xfffe);
    break;
  case 0x20: /* JSR */
    addr = fetch(c);
    addr = (uint16_t)(addr | (fetch(c) << 8));
    push(c, (uint8_t)((c->pc - 1) >> 8));
    push(c, (uint8_t)((c->pc - 1)));
    c->pc = addr;
    break;
  case 0x40: /* RTI */
    c->p = (uint8_t)((pop(c) & ~CPU_FLAG_B) | CPU_FLAG_U);
    c->pc = pop16(c);
    break;
  case 0x60: /* RTS */
    c->pc = (uint16_t)(pop16(c) + 1);
    break;
  case 0x08: push(c, (uint8_t)(c->p | CPU_FLAG_B | CPU_FLAG_U)); break;   /* PHP */
  case 0x28: c->p = (uint8_t)((pop(c) & ~CPU_FLAG_B) | CPU_FLAG_U); break; /* PLP */
  case 0x48: push(c, c->a); break;                                        /* PHA */
  case 0x68: c->a = pop(c); setzn(c, c->a); break;                        /* PLA */
  case 0x18: set_flag(c, CPU_FLAG_C, false); break;                       /* CLC */
  case 0x38: set_flag(c, CPU_FLAG_C, true); break;                        /* SEC */
  case 0x58: set_flag(c, CPU_FLAG_I, false); break;                       /* CLI */
  case 0x78: set_flag(c, CPU_FLAG_I, true); break;                        /* SEI */
  case 0xb8: set_flag(c, CPU_FLAG_V, false); break;                       /* CLV */
  case 0xd8: set_flag(c, CPU_FLAG_D, false); break;                       /* CLD */
  case 0xf8: set_flag(c, CPU_FLAG_D, true); break;                        /* SED */
  case 0xaa: c->x = c->a; setzn(c, c->x); break;                          /* TAX */
  case 0xa8: c->y = c->a; setzn(c, c->y); break;                          /* TAY */
  case 0x8a: c->a = c->x; setzn(c, c->a); break;                          /* TXA */
  case 0x98: c->a = c->y; setzn(c, c->a); break;                          /* TYA */
  case 0xba: c->x = c->sp; setzn(c, c->x); break;                         /* TSX */
  case 0x9a: c->sp = c->x; break;                                         /* TXS */
  case 0xca: c->x--; setzn(c, c->x); break;                               /* DEX */
  case 0xe8: c->x++; setzn(c, c->x); break;                               /* INX */
  case 0x88: c->y--; setzn(c, c->y); break;                               /* DEY */
  case 0xc8: c->y++; setzn(c, c->y); break;                               /* INY */
  case 0xea: break;                                                       /* NOP */

  /* --- branches --- */
  case 0x10: branch(c, !(c->p & CPU_FLAG_N), &cycles); break; /* BPL */
  case 0x30: branch(c, (c->p & CPU_FLAG_N) != 0, &cycles); break; /* BMI */
  case 0x50: branch(c, !(c->p & CPU_FLAG_V), &cycles); break; /* BVC */
  case 0x70: branch(c, (c->p & CPU_FLAG_V) != 0, &cycles); break; /* BVS */
  case 0x90: branch(c, !(c->p & CPU_FLAG_C), &cycles); break; /* BCC */
  case 0xb0: branch(c, (c->p & CPU_FLAG_C) != 0, &cycles); break; /* BCS */
  case 0xd0: branch(c, !(c->p & CPU_FLAG_Z), &cycles); break; /* BNE */
  case 0xf0: branch(c, (c->p & CPU_FLAG_Z) != 0, &cycles); break; /* BEQ */

  /* --- jumps --- */
  case 0x4c: /* JMP abs */
    addr = fetch(c);
    c->pc = (uint16_t)(addr | (fetch(c) << 8));
    break;
  case 0x6c: { /* JMP (ind), with the 6502's page-wrap bug */
    uint16_t ptr = fetch(c);
    ptr = (uint16_t)(ptr | (fetch(c) << 8));
    uint16_t hi_addr = (uint16_t)((ptr & 0xff00u) | ((ptr + 1) & 0x00ffu));
    c->pc = (uint16_t)(rd(c, ptr) | (rd(c, hi_addr) << 8));
    break;
  }

  /* --- immediate and misc two-byte forms --- */
  case 0x09: case 0x29: case 0x49: case 0x69: case 0xa9: case 0xc9: case 0xe9:
  case 0xa0: case 0xa2: case 0xc0: case 0xe0: {
    v = fetch(c);
    switch (op) {
    case 0x09: c->a |= v; setzn(c, c->a); break;
    case 0x29: c->a &= v; setzn(c, c->a); break;
    case 0x49: c->a ^= v; setzn(c, c->a); break;
    case 0x69: adc(c, v); break;
    case 0xa9: c->a = v; setzn(c, c->a); break;
    case 0xc9: compare(c, c->a, v); break;
    case 0xe9: adc(c, (uint8_t)~v); break;
    case 0xa0: c->y = v; setzn(c, c->y); break;
    case 0xa2: c->x = v; setzn(c, c->x); break;
    case 0xc0: compare(c, c->y, v); break;
    default: /* 0xe0 CPX */ compare(c, c->x, v); break;
    }
    break;
  }

  /* --- BIT --- */
  case 0x24: case 0x2c:
    addr = op == 0x24 ? fetch(c) : fetch16(c);
    v = rd(c, addr);
    set_flag(c, CPU_FLAG_Z, (c->a & v) == 0);
    set_flag(c, CPU_FLAG_N, (v & 0x80u) != 0);
    set_flag(c, CPU_FLAG_V, (v & 0x40u) != 0);
    break;

  /* --- CPX/CPY memory forms --- */
  case 0xc4: case 0xcc: case 0xe4: case 0xec: {
    addr = op == 0xc4 || op == 0xe4 ? fetch(c) : fetch16(c);
    v = rd(c, addr);
    compare(c, op == 0xc4 || op == 0xcc ? c->y : c->x, v);
    break;
  }

  /* --- zero page and absolute stores and read-modify-write --- */
  case 0x84: case 0x8c: case 0x85: case 0x8d: case 0x95: case 0x9d:
  case 0x99: case 0x81: case 0x91: case 0x86: case 0x8e: case 0x94: case 0x96: {
    Mode m;
    uint8_t value = 0;
    switch (op) {
    case 0x84: m = M_ZP; value = c->y; break;
    case 0x8c: m = M_ABS; value = c->y; break;
    case 0x85: m = M_ZP; value = c->a; break;
    case 0x8d: m = M_ABS; value = c->a; break;
    case 0x94: m = M_ZPX; value = c->y; break;
    case 0x95: m = M_ZPX; value = c->a; break;
    case 0x9d: m = M_ABX; value = c->a; break;
    case 0x99: m = M_ABY; value = c->a; break;
    case 0x81: m = M_IZX; value = c->a; break;
    case 0x91: m = M_IZY; value = c->a; break;
    case 0x86: m = M_ZP; value = c->x; break;
    case 0x8e: m = M_ABS; value = c->x; break;
    default: /* 0x96 STX zp,Y */ m = M_ZPY; value = c->x; break;
    }
    addr = effective(c, m, &cross);
    wr(c, addr, value);
    break;
  }

  case 0x06: case 0x16: case 0x0e: case 0x1e:   /* ASL */
  case 0x26: case 0x36: case 0x2e: case 0x3e:   /* ROL */
  case 0x46: case 0x56: case 0x4e: case 0x5e:   /* LSR */
  case 0x66: case 0x76: case 0x6e: case 0x7e:   /* ROR */
  case 0xc6: case 0xd6: case 0xce: case 0xde:   /* DEC */
  case 0xe6: case 0xf6: case 0xee: case 0xfe: { /* INC */
    Mode m;
    switch (op & 0x1fu) {
    case 0x06: m = M_ZP; break;
    case 0x16: m = M_ZPX; break;
    case 0x0e: m = M_ABS; break;
    default: m = M_ABX; break; /* 0x1e */
    }
    addr = effective(c, m, &cross);
    v = rd(c, addr);
    uint8_t out;
    switch (op & 0xe0u) {
    case 0x00: out = shift_op(c, 0, v); break;
    case 0x20: out = shift_op(c, 1, v); break;
    case 0x40: out = shift_op(c, 2, v); break;
    case 0x60: out = shift_op(c, 3, v); break;
    case 0xc0: out = (uint8_t)(v - 1); setzn(c, out); break;
    default:   out = (uint8_t)(v + 1); setzn(c, out); break;
    }
    wr(c, addr, out);
    break;
  }

  /* --- accumulator shifts --- */
  case 0x0a: c->a = shift_op(c, 0, c->a); break;
  case 0x2a: c->a = shift_op(c, 1, c->a); break;
  case 0x4a: c->a = shift_op(c, 2, c->a); break;
  case 0x6a: c->a = shift_op(c, 3, c->a); break;

  /* --- the regular ALU and load groups (aaa bbb 01) --- */
  default: {
    if ((op & 0x03u) != 0x01u) {
      /* LDX/LDY/STX zero-page-indexed and absolute-indexed forms not yet
       * covered above */
      switch (op) {
      case 0xb6: addr = effective(c, M_ZPY, &cross); c->x = rd(c, addr); setzn(c, c->x); break;
      case 0xbe: addr = effective(c, M_ABY, &cross); c->x = rd(c, addr); setzn(c, c->x); break;
      case 0xb4: addr = effective(c, M_ZPX, &cross); c->y = rd(c, addr); setzn(c, c->y); break;
      case 0xbc: addr = effective(c, M_ABX, &cross); c->y = rd(c, addr); setzn(c, c->y); break;
      case 0xa6: addr = effective(c, M_ZP, &cross); c->x = rd(c, addr); setzn(c, c->x); break;
      case 0xae: addr = effective(c, M_ABS, &cross); c->x = rd(c, addr); setzn(c, c->x); break;
      case 0xa4: addr = effective(c, M_ZP, &cross); c->y = rd(c, addr); setzn(c, c->y); break;
      case 0xac: addr = effective(c, M_ABS, &cross); c->y = rd(c, addr); setzn(c, c->y); break;
      default:
        c->jammed = true;
        c->jam_pc = op_pc;
        c->jam_opcode = op;
        return 0;
      }
      if (cross) cycles += 1;
      break;
    }
    unsigned aaa = (op >> 5) & 7u;
    unsigned bbb = (op >> 2) & 7u;
    Mode m;
    switch (bbb) {
    case 0: m = M_IZX; break;
    case 1: m = M_ZP; break;
    case 2: m = M_ZP; break; /* immediate: handled below */
    case 3: m = M_ABS; break;
    case 4: m = M_IZY; break;
    case 5: m = M_ZPX; break;
    case 6: m = M_ABY; break;
    default: m = M_ABX; break;
    }
    if (bbb == 2) {
      /* 0x89 is an illegal immediate NOP: not used by the game */
      if (aaa == 4) {
        c->jammed = true;
        c->jam_pc = op_pc;
        c->jam_opcode = op;
        return 0;
      }
      v = fetch(c);
    } else {
      addr = effective(c, m, &cross);
      if (aaa == 4) { /* STA */
        wr(c, addr, c->a);
        break;
      }
      v = rd(c, addr);
    }
    switch (aaa) {
    case 0: c->a |= v; setzn(c, c->a); break;   /* ORA */
    case 1: c->a &= v; setzn(c, c->a); break;   /* AND */
    case 2: c->a ^= v; setzn(c, c->a); break;   /* EOR */
    case 3: adc(c, v); break;                   /* ADC */
    case 5: c->a = v; setzn(c, c->a); break;    /* LDA */
    case 6: compare(c, c->a, v); break;         /* CMP */
    default: adc(c, (uint8_t)~v); break;        /* SBC */
    }
    if (cross) cycles += 1;
    break;
  }
  }

  c->cycles += cycles;
  return cycles;
}
