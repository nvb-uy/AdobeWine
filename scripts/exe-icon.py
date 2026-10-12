#!/usr/bin/env python3
"""Write the application icon of a Windows program as hicolor PNGs.

Usage: exe-icon.py <program.exe|icon.ico> <icons dir> <name>

The icon is the program's first icon group, as Windows shows it. Each size goes to
<icons dir>/hicolor/<size>x<size>/apps/<name>.png.
"""
import io
import os
import struct
import sys
import warnings

from PIL import Image

SIZES = (16, 22, 24, 32, 48, 64, 96, 128, 256)
RT_ICON, RT_GROUP_ICON = 3, 14


class PE:
    def __init__(self, data):
        self.data = data
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        if data[pe:pe + 4] != b"PE\0\0":
            raise ValueError("not a PE file")
        nsect, optsize = struct.unpack_from("<H12xH", data, pe + 6)
        opt = pe + 24
        magic = struct.unpack_from("<H", data, opt)[0]
        dirs = opt + (112 if magic == 0x20B else 96)
        self.rsrc = struct.unpack_from("<I", data, dirs + 2 * 8)[0]
        self.sections = []
        for i in range(nsect):
            vsize, va, rawsize, raw = struct.unpack_from("<4I", data, opt + optsize + i * 40 + 8)
            self.sections.append((va, max(vsize, rawsize), raw))
        if not self.rsrc:
            raise ValueError("no resources")
        self.base = self.offset(self.rsrc)

    def offset(self, rva):
        for va, size, raw in self.sections:
            if va <= rva < va + size:
                return rva - va + raw
        raise ValueError("bad address")

    def entries(self, off):
        named, ids = struct.unpack_from("<HH", self.data, self.base + off + 12)
        out = []
        for i in range(named + ids):
            name, target = struct.unpack_from("<II", self.data, self.base + off + 16 + i * 8)
            if name & 0x80000000:
                p = self.base + (name & 0x7FFFFFFF)
                n = struct.unpack_from("<H", self.data, p)[0]
                key = (0, self.data[p + 2:p + 2 + n * 2].decode("utf-16-le", "replace").upper())
            else:
                key = (1, name)
            out.append((key, target))
        return sorted(out, key=lambda e: e[0])

    def subdir(self, off, key):
        for k, target in self.entries(off):
            if k == key and target & 0x80000000:
                return target & 0x7FFFFFFF
        return None

    def first_data(self, off):
        while True:
            entries = self.entries(off)
            if not entries:
                return None
            target = entries[0][1]
            if not target & 0x80000000:
                rva, size = struct.unpack_from("<II", self.data, self.base + target)
                p = self.offset(rva)
                return self.data[p:p + size]
            off = target & 0x7FFFFFFF

    def icon_frames(self):
        groups = self.subdir(0, (1, RT_GROUP_ICON))
        icons = self.subdir(0, (1, RT_ICON))
        if groups is None or icons is None:
            return []
        first = self.entries(groups)
        if not first:
            return []
        group = self.first_data(first[0][1] & 0x7FFFFFFF)
        count = struct.unpack_from("<H", group, 4)[0]
        frames = []
        for i in range(count):
            w, h, colors, planes, bits, size, rid = struct.unpack_from("<BBB1xHHIH", group, 6 + i * 14)
            sub = self.subdir(icons, (1, rid))
            if sub is not None:
                frames.append(((w or 256), colors, planes, bits, self.first_data(sub)))
        return frames


def ico_frames(data):
    count = struct.unpack_from("<H", data, 4)[0]
    frames = []
    for i in range(count):
        w, h, colors, planes, bits, size, off = struct.unpack_from("<BBB1xHHII", data, 6 + i * 16)
        frames.append(((w or 256), colors, planes, bits, data[off:off + size]))
    return frames


def decode(frame):
    size, colors, planes, bits, data = frame
    if data[:8] == b"\x89PNG\r\n\x1a\n":
        im = Image.open(io.BytesIO(data))
    else:
        hdr = struct.pack("<HHH", 0, 1, 1)
        hdr += struct.pack("<BBBBHHII", size % 256, size % 256, colors, 0, planes, bits, len(data), 22)
        im = Image.open(io.BytesIO(hdr + data))
        im.size = (size, size)
    im.load()
    return im.convert("RGBA"), bits


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__.strip())
    src, root, name = sys.argv[1:]
    with open(src, "rb") as f:
        data = f.read()
    warnings.simplefilter("ignore")
    try:
        frames = PE(data).icon_frames() if data[:2] == b"MZ" else ico_frames(data)
    except (ValueError, struct.error):
        frames = []
    images = {}
    for frame in frames:
        try:
            im, bits = decode(frame)
        except Exception:
            continue
        s = im.size[0]
        if s not in images or bits > images[s][1]:
            images[s] = (im, bits)
    if not images:
        sys.exit("no icon in " + src)
    largest = images[max(images)][0]
    for s in SIZES:
        im = images[s][0] if s in images else largest.resize((s, s), Image.LANCZOS)
        out = f"{root}/hicolor/{s}x{s}/apps"
        os.makedirs(out, exist_ok=True)
        im.save(f"{out}/{name}.png")


if __name__ == "__main__":
    main()
