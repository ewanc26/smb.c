#!/usr/bin/env python3
"""Compare the port's rendered frames with the reference emulator's screens (#31).

The port's frame f is the picture the emulator shows at step f+1: game state
written in NMI f is first displayed one frame later. So the default offset is 1.

Colours: the port's palette indices are mapped to RGB by the majority colour
the emulator shows for each index, as emulator_compare.py does.

    compare_pictures.py emulator.npy port-fb.bin [offset] [--png frame,...]

Writes side-by-side PNGs (emulator | port | diff) for the frames given.
"""
import struct
import sys
import zlib
from collections import Counter, defaultdict

import numpy as np


def write_png(path, rgb):
    h, w, _ = rgb.shape
    raw = b"".join(b"\x00" + rgb[y].tobytes() for y in range(h))

    def chunk(tag, data):
        body = tag + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b"")
    open(path, "wb").write(png)


def main(argv):
    png_frames = []
    if "--png" in argv:
        i = argv.index("--png")
        png_frames = [int(x) for x in argv[i + 1].split(",")]
        argv = argv[:i] + argv[i + 2:]
    emu = np.load(argv[0], mmap_mode="r")
    port = np.fromfile(argv[1], dtype=np.uint8).reshape(-1, 240, 256)
    off = int(argv[2]) if len(argv) > 2 else 1
    n = min(len(port), len(emu) - off)

    pairs = defaultdict(Counter)
    for i in range(0, n, 3):
        e = np.asarray(emu[i + off]).reshape(-1, 3)
        p = port[i].reshape(-1)
        keys = (e[:, 0].astype(np.int64) << 16) | (e[:, 1].astype(np.int64) << 8) | e[:, 2]
        for idx in np.unique(p):
            vals, cnt = np.unique(keys[p == idx], return_counts=True)
            for k, c in zip(vals, cnt):
                pairs[int(idx)][int(k)] += int(c)
    lut = np.zeros((64, 3), dtype=np.uint8)
    for idx, counts in pairs.items():
        k = counts.most_common(1)[0][0]
        lut[idx] = [(k >> 16) & 255, (k >> 8) & 255, k & 255]

    bad = [int(np.any(lut[port[i]] != np.asarray(emu[i + off]), axis=-1).sum()) for i in range(n)]
    exact = sum(1 for b in bad if b == 0)
    print(f"offset {off}: exact {exact}/{n} frames")
    print("first mismatching frames:", [(i, b) for i, b in enumerate(bad) if b][:20])

    for f in png_frames:
        e = np.asarray(emu[f + off]).astype(np.uint8)
        p = lut[port[f]]
        d = np.any(p != e, axis=-1)
        dv = e.copy()
        dv[d] = [255, 0, 0]
        gap = np.full((240, 4, 3), 255, np.uint8)
        panel = np.concatenate([e, gap, p, gap, dv], axis=1)
        write_png(f"picture_{f}.png", panel)
        print(f"frame {f}: {int(d.sum())} pixels differ -> picture_{f}.png")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
