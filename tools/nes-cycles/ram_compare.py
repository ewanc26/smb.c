#!/usr/bin/env python3
"""Compare two per-frame RAM dumps (2 KB per frame) and report, for the game
state the issue cares about, which frames disagree (#41).

    ram_compare.py emulator-ram.bin harness-ram.bin
"""
import sys

RAM = 2048
FRAME_COUNTER, OPER_MODE, OPER_MODE_TASK = 0x09, 0x0770, 0x0772


def frames(path):
    data = open(path, "rb").read()
    return [data[i:i + RAM] for i in range(0, len(data) - RAM + 1, RAM)]


def main(argv):
    emu, hw = frames(argv[0]), frames(argv[1])
    n = min(len(emu), len(hw))
    identical = sum(1 for f in range(n) if emu[f] == hw[f])

    def mismatches(addr):
        return [f for f in range(n) if emu[f][addr] != hw[f][addr]]

    fc = mismatches(FRAME_COUNTER)
    print(f"frames compared: {n}, identical RAM: {identical}")
    print(f"FrameCounter mismatches: {len(fc)} first {fc[:8]}")
    print(f"OperMode mismatches: {len(mismatches(OPER_MODE))}")
    print(f"OperMode_Task mismatches: {len(mismatches(OPER_MODE_TASK))}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
