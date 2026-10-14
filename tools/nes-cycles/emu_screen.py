#!/usr/bin/env python3
"""Dump the reference emulator's screen for each frame, as RGB (#31).

Dev-only. Needs the pinned emulator (nes-py 8.2.1, numpy 1.26.x). The ROM is
read from SMBC_ROM and never copied into the repository.

    SMBC_ROM=/path/to/smb.nes emu_screen.py inputs.bin frames out.npy
"""
import os
import sys
import tempfile

import numpy as np
from nes_py import NESEnv


def main(argv):
    rom = os.environ.get("SMBC_ROM")
    if not rom or not os.path.isfile(rom):
        sys.exit("emu_screen: set SMBC_ROM to the path of the cartridge (.nes)")
    inputs = open(argv[0], "rb").read()
    frames, out = int(argv[1]), argv[2]
    data = bytearray(open(rom, "rb").read())
    data[12:16] = b"\x00\x00\x00\x00"
    with tempfile.TemporaryDirectory() as tmp:
        fixed = os.path.join(tmp, "fixed.nes")
        open(fixed, "wb").write(bytes(data))
        env = NESEnv(fixed)
        env.reset()
        screens = np.zeros((frames, 240, 256, 3), dtype=np.uint8)
        for i in range(frames):
            env.step(inputs[i] if i < len(inputs) else 0)
            screens[i] = env.screen
        env.close()
    np.save(out, screens)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
