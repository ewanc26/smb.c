/* hw/internal.h — NES register addresses, shared by the hardware atoms.
 *
 * Owns: the memory-mapped register addresses the CPU sees. Values are the
 * hardware addresses themselves, taken from the disassembly's DEFINES
 * section (smb.asm: "NES specific hardware defines").
 *
 * No behaviour lives here; the atoms model the side effects.
 */
#ifndef SMBC_HW_INTERNAL_H
#define SMBC_HW_INTERNAL_H

#include <stdint.h>

/* smb.asm: PPU register defines */
#define SMBC_PPU_CTRL1   0x2000u
#define SMBC_PPU_CTRL2   0x2001u
#define SMBC_PPU_STATUS  0x2002u
#define SMBC_PPU_SPR_ADDR 0x2003u
#define SMBC_PPU_SPR_DATA 0x2004u
#define SMBC_PPU_SCROLL  0x2005u
#define SMBC_PPU_ADDRESS 0x2006u
#define SMBC_PPU_DATA    0x2007u

/* smb.asm: APU / IO register defines */
#define SMBC_SND_SQUARE1   0x4000u
#define SMBC_SND_SQUARE2   0x4004u
#define SMBC_SND_TRIANGLE  0x4008u
#define SMBC_SND_NOISE     0x400cu
#define SMBC_SND_DELTA     0x4010u
#define SMBC_SPR_DMA       0x4014u
#define SMBC_SND_MASTERCTRL 0x4015u
#define SMBC_JOYPAD1       0x4016u
#define SMBC_JOYPAD2       0x4017u

#endif /* SMBC_HW_INTERNAL_H */
