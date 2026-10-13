#!/usr/bin/env python3
"""Dump the reference emulator's 2 KB of CPU RAM after each frame (#41).

Dev-only. Needs the pinned emulator (nes-py 8.2.1, numpy 1.26.x). The ROM is
read from SMBC_ROM and never copied into the repository; its header bytes
12-15 are zeroed in a temporary copy, as emulator_compare.py does.

    SMBC_ROM=/path/to/smb.nes emu_ram.py inputs.bin frames out.bin
"""
import os
import sys
import tempfile

import numpy as np  # noqa: F401  (nes-py needs numpy at import time)
from nes_py import NESEnv


def main(argv):
    rom = os.environ.get("SMBC_ROM")
    if not rom or not os.path.isfile(rom):
        sys.exit("emu_ram: set SMBC_ROM to the path of the cartridge (.nes)")
    inputs_path, frames, out_path = argv[0], int(argv[1]), argv[2]
    inputs = open(inputs_path, "rb").read()
    data = bytearray(open(rom, "rb").read())
    data[12:16] = b"\x00\x00\x00\x00"
    with tempfile.TemporaryDirectory() as tmp:
        fixed = os.path.join(tmp, "fixed.nes")
        open(fixed, "wb").write(bytes(data))
        env = NESEnv(fixed)
        env.reset()
        with open(out_path, "wb") as out:
            for i in range(frames):
                env.step(inputs[i] if i < len(inputs) else 0)
                out.write(bytes(env.ram))
        env.close()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
