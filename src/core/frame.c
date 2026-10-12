/* core/frame.c — see core/frame.h for the contract. */
#include "frame.h"

#include "../data/chr.h"
#include "boot.h"
#include "nmi.h"

void smbc_init(Smbc *smbc) {
  state_init(&smbc->state);
  joypad_init(&smbc->joypad1);
  joypad_init(&smbc->joypad2);
  ppu_init(&smbc->ppu);
  ppu_set_chr(&smbc->ppu, SMBC_CHR_ROM); /* the donor cartridge's pattern tables */
  apu_init(&smbc->apu);
  boot_run(smbc);
}

void smbc_frame(Smbc *smbc) {
  /* the timing contract (hw/ppu.h): one call is one NTSC frame. The
   * clock enters vblank, the translated NMI runs — its sprite-0 waits
   * advance the clock through the pre-render line and the hit — and
   * whatever remains runs out to the frame's end. */
  ppu_enter_vblank(&smbc->ppu);
  nmi_run(smbc);
  ppu_end_frame(&smbc->ppu);
}
