#!/usr/bin/env python3
"""Add a "Dark" colour variant to Wine's aero.msstyles.

Usage: make-dark-theme.py <wine source dir>

Writes dark_*.bmp next to the blue_*.bmp files and adds the variant to aero.rc (colour
name, ini file and bitmap resources). The bitmaps are the blue ones with their lightness
inverted and their tint kept, so light faces turn dark, dark glyphs light, and accents stay
blue. Files are only written when their content changes, so rebuilds stay incremental.
The system colours set here are only defaults: the launcher sets the desktop's colours in
the registry (desktop-settings.py), and those win once the theme is loaded.
"""
import glob
import os
import re
import struct
import sys

theme_dir = os.path.join(sys.argv[1], "dlls", "aero.msstyles")
rc_path = os.path.join(theme_dir, "aero.rc")

window_bg = (32, 32, 32)
window_alt = (40, 40, 40)
text = (223, 223, 223)
text_inactive = (150, 150, 150)
button_bg = (58, 58, 60)
button_alt = (72, 72, 74)
selection_bg = (0, 88, 208)
selection_text = (255, 255, 255)
accent = (10, 132, 255)


def clamp(v):
    return max(0, min(255, int(round(v))))


def mix(a, b, t):
    return tuple(clamp(x + (y - x) * t) for x, y in zip(a, b))


def lum(c):
    return 0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2]


L_WHITE, L_BLACK = lum(button_bg), lum(text)


def recolour(r, g, b):
    gray = lum((r, g, b))
    new = L_BLACK + (L_WHITE - L_BLACK) * gray / 255.0
    return tuple(clamp(new + (c - gray) * 0.85) for c in (r, g, b))


def write(path, data):
    try:
        with open(path, "rb") as f:
            if f.read() == data:
                return False
    except FileNotFoundError:
        pass
    with open(path, "wb") as f:
        f.write(data)
    return True


def dark_bmp(path):
    d = bytearray(open(path, "rb").read())
    off = struct.unpack_from("<I", d, 10)[0]
    w, h, _, bpp = struct.unpack_from("<iiHH", d, 18)
    if bpp not in (24, 32):
        raise ValueError(f"{path}: {bpp} bpp")
    step = bpp // 8
    stride = (w * step + 3) & ~3
    for row in range(abs(h)):
        for i in range(off + row * stride, off + row * stride + w * step, step):
            b, g, r = d[i:i + 3]
            if (bpp == 24 and (r, g, b) == (255, 0, 255)) or (bpp == 32 and d[i + 3] == 0):
                continue
            r, g, b = recolour(r, g, b)
            d[i:i + 3] = bytes((b, g, r))
    return bytes(d)


blue_bmps = sorted(glob.glob(os.path.join(theme_dir, "blue_*.bmp")))
dark_names = ["dark_" + os.path.basename(p)[5:] for p in blue_bmps]
changed = 0
for path, name in zip(blue_bmps, dark_names):
    changed += write(os.path.join(theme_dir, name), dark_bmp(path))
for old in glob.glob(os.path.join(theme_dir, "dark_*.bmp")):
    if os.path.basename(old) not in dark_names:
        os.remove(old)

rc_old = open(rc_path, encoding="utf-8").read()
BEGIN, END = "/* Dark theme (generated) */\n", "/* End of dark theme */\n"
rc = re.sub(r"\n*" + re.escape(BEGIN) + ".*?" + re.escape(END), "\n", rc_old, flags=re.S)
rc = rc.replace('"Blue\\0"\n"Dark\\0"\n', '"Blue\\0"\n').replace('"BLUE_INI\\0"\n"DARK_INI\\0"\n', '"BLUE_INI\\0"\n')
blue_ini = re.search(r"BLUE_INI TEXTFILE\n\{\n(.*?)\n\}\n", rc, re.S).group(1)

shadow = mix(button_bg, (0, 0, 0), 0.45)
highlight = mix(button_bg, (255, 255, 255), 0.18)
dk_shadow = mix(window_bg, (0, 0, 0), 0.5)
colours = {
    "Scrollbar": window_alt, "Background": window_bg,
    "ActiveCaption": window_bg, "InactiveCaption": window_bg,
    "GradientActiveCaption": window_bg, "GradientInactiveCaption": window_bg,
    "CaptionText": text, "InactiveCaptionText": text_inactive,
    "Menu": button_bg, "MenuBar": window_bg, "MenuText": text, "MenuHilight": selection_bg,
    "Window": window_bg, "WindowText": text, "WindowFrame": button_alt,
    "ActiveBorder": window_bg, "InactiveBorder": window_bg, "AppWorkSpace": window_bg,
    "Highlight": selection_bg, "HighlightText": selection_text,
    "BtnFace": button_bg, "BtnText": text, "BtnShadow": shadow, "BtnHighlight": highlight,
    "Light3d": button_alt, "DkShadow3d": dk_shadow, "GrayText": text_inactive,
    "InfoText": text, "InfoBk": button_bg, "ButtonAlternateFace": button_alt, "HotTracking": accent,
    "EdgeLightColor": button_alt, "EdgeHighLightColor": highlight, "EdgeShadowColor": shadow,
    "EdgeDkShadowColor": dk_shadow, "EdgeFillColor": button_bg,
}


def dark_line(line):
    m = re.match(r'^"(\w+) = ([0-9]+) ([0-9]+) ([0-9]+)\\r\\n"$', line)
    if not m:
        return re.sub(r"\bblue_", "dark_", line)
    c = colours.get(m.group(1)) or recolour(*(int(m.group(i)) for i in (2, 3, 4)))
    return f'"{m.group(1)} = {c[0]} {c[1]} {c[2]}\\r\\n"'


dark_ini = "\n".join(dark_line(l) for l in blue_ini.split("\n"))
bitmaps = "\n".join(f'/* @makedep: {n} */\n{n[:-4].upper()}_BMP BITMAP "{n}"\n' for n in dark_names)
rc = rc.replace('"Blue\\0"\n', '"Blue\\0"\n"Dark\\0"\n', 1).replace('"BLUE_INI\\0"\n', '"BLUE_INI\\0"\n"DARK_INI\\0"\n', 1)
rc = rc.rstrip("\n") + "\n\n" + BEGIN + "DARK_INI TEXTFILE\n{\n" + dark_ini + "\n}\n\n" + bitmaps + END
changed += write(rc_path, rc.encode("utf-8"))
print(f"dark theme: {len(dark_names)} bitmaps, {changed} files updated")
