#!/usr/bin/env python3
"""Give the Wine prefix the desktop's colours, dark or light mode and accent colour.

Usage: desktop-settings.py [--force]

On KDE the colours come from the colour scheme (kdeglobals), elsewhere from a built-in
Adwaita dark or Windows light palette chosen by the desktop portal's colour scheme. The
result goes into the registry with regedit; a copy is kept in the prefix and regedit only
runs when something changed (or with --force). Programs started afterwards pick it up.
"""
import configparser
import json
import os
import re
import subprocess
import sys

HOME = os.path.expanduser("~")
CONFIG = os.environ.get("XDG_CONFIG_HOME") or os.path.join(HOME, ".config")
PREFIX = os.environ.get("WINEPREFIX") or os.path.join(HOME, ".local/share/adobewine/prefix")
WHITE, BLACK = (255, 255, 255), (0, 0, 0)


def clamp(v):
    return max(0, min(255, int(round(v))))


def mix(a, b, t):
    return tuple(clamp(x + (y - x) * t) for x, y in zip(a, b))


def lum(c):
    return 0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2]


def portal(key):
    args = ["org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.Settings", "ReadOne", "ss", "org.freedesktop.appearance", key]
    try:
        out = subprocess.run(["busctl", "--user", "--json=short", "call", *args],
                             capture_output=True, text=True, timeout=2).stdout
        data = json.loads(out)["data"][0]["data"]
        return data if isinstance(data, list) else [data]
    except (OSError, subprocess.SubprocessError, ValueError, LookupError, TypeError):
        pass
    try:
        out = subprocess.run(["gdbus", "call", "--session", "-d", args[0], "-o", args[1],
                              "-m", args[2] + "." + args[3], args[5], args[6]],
                             capture_output=True, text=True, timeout=2).stdout
    except (OSError, subprocess.SubprocessError):
        return None
    nums = re.findall(r"(?<![\w.])-?\d+(?:\.\d+)?(?:e-?\d+)?", out.split(">")[0])
    return [float(n) for n in nums] or None


def portal_scheme():
    v = portal("color-scheme")
    return int(v[0]) if v else 0


def portal_accent():
    v = portal("accent-color")
    if v and len(v) == 3 and all(0 <= c <= 1 for c in v):
        return tuple(clamp(c * 255) for c in v)
    return None


def kde_palette():
    kdeglobals = os.path.join(CONFIG, "kdeglobals")
    cp = configparser.RawConfigParser(strict=False, allow_no_value=True, interpolation=None)
    cp.optionxform = str

    def read(path):
        try:
            cp.read(path, encoding="utf-8")
        except (configparser.Error, UnicodeDecodeError):
            pass

    read(kdeglobals)
    scheme = cp.get("General", "ColorScheme", fallback="BreezeLight") or "BreezeLight"
    for d in (os.path.join(HOME, ".local/share/color-schemes"), "/usr/share/color-schemes"):
        if os.path.isfile(os.path.join(d, scheme + ".colors")):
            read(os.path.join(d, scheme + ".colors"))
            read(kdeglobals)
            break

    def get(section, key, default):
        try:
            return tuple(int(v) for v in cp.get(section, key).split(",")[:3])
        except (configparser.Error, ValueError, AttributeError):
            return default

    window = get("Colors:Window", "BackgroundNormal", (239, 240, 241))
    text = get("Colors:Window", "ForegroundNormal", (35, 38, 41))
    inactive = get("Colors:Window", "ForegroundInactive", mix(text, window, 0.4))
    button = get("Colors:Button", "BackgroundNormal", window)
    header = get("Colors:Header", "BackgroundNormal", window)
    tooltip = get("Colors:Tooltip", "BackgroundNormal", button)
    selection = get("Colors:Selection", "BackgroundNormal", (61, 174, 233))
    return {
        "window": window, "window_alt": get("Colors:Window", "BackgroundAlternate", mix(window, text, 0.04)),
        "view": get("Colors:View", "BackgroundNormal", window), "text": text, "inactive": inactive,
        "button": button, "button_alt": get("Colors:Button", "BackgroundAlternate", mix(button, text, 0.1)),
        "button_text": get("Colors:Button", "ForegroundNormal", text), "header": header,
        "tooltip": tooltip, "tooltip_text": get("Colors:Tooltip", "ForegroundNormal", text),
        "selection": selection, "selection_text": get("Colors:Selection", "ForegroundNormal", WHITE),
        "accent": get("General", "AccentColor", get("Colors:Window", "DecorationFocus", selection)),
        "title": get("WM", "activeBackground", header), "title_text": get("WM", "activeForeground", text),
        "title_inactive": get("WM", "inactiveBackground", header),
        "title_inactive_text": get("WM", "inactiveForeground", inactive),
    }


def builtin_palette(dark, accent):
    if dark:
        window, view, button, text, inactive, tooltip = (36,) * 3, (30,) * 3, (58,) * 3, WHITE, (153,) * 3, (56,) * 3
        accent = accent or (53, 132, 228)
    else:
        window, view, button, text, inactive, tooltip = (240,) * 3, WHITE, (225,) * 3, BLACK, (109,) * 3, WHITE
        accent = accent or (0, 120, 215)
    return {
        "window": window, "window_alt": mix(window, text, 0.04), "view": view, "text": text,
        "inactive": inactive, "button": button, "button_alt": mix(button, text, 0.1), "button_text": text,
        "header": window, "tooltip": tooltip, "tooltip_text": text, "selection": accent,
        "selection_text": WHITE, "accent": accent, "title": window, "title_text": text,
        "title_inactive": window, "title_inactive_text": inactive,
    }


def sys_colours(p):
    shadow = mix(p["button"], BLACK, 0.45)
    return {
        "Scrollbar": p["window_alt"], "Background": p["window"],
        "ActiveTitle": p["title"], "InactiveTitle": p["title_inactive"],
        "Menu": p["tooltip"], "Window": p["view"], "WindowFrame": p["button_alt"],
        "MenuText": p["text"], "WindowText": p["text"], "TitleText": p["title_text"],
        "ActiveBorder": p["header"], "InactiveBorder": p["header"], "AppWorkSpace": p["window"],
        "Hilight": p["selection"], "HilightText": p["selection_text"], "ButtonFace": p["button"],
        "ButtonShadow": shadow, "GrayText": p["inactive"], "ButtonText": p["button_text"],
        "InactiveTitleText": p["title_inactive_text"], "ButtonHilight": mix(p["button"], WHITE, 0.18),
        "ButtonDkShadow": mix(p["window"], BLACK, 0.5), "ButtonLight": p["button_alt"],
        "InfoText": p["tooltip_text"], "InfoWindow": p["tooltip"],
        "ButtonAlternateFace": p["button_alt"], "HotTrackingColor": p["accent"],
        "GradientActiveTitle": p["title"], "GradientInactiveTitle": p["title_inactive"],
        "MenuHilight": p["selection"], "MenuBar": p["window"],
    }


def registry():
    scheme = portal_scheme()
    if ":KDE:" in ":%s:" % os.environ.get("XDG_CURRENT_DESKTOP", ""):
        p = kde_palette()
        dark = scheme == 1 if scheme else lum(p["window"]) < 128
    else:
        dark = scheme == 1
        p = builtin_palette(dark, portal_accent())
    a = p["accent"]
    shades = [mix(a, WHITE, t) for t in (0.6, 0.4, 0.2)] + [a] + [mix(a, BLACK, t) for t in (0.2, 0.4, 0.6)]
    light = "dword:%08x" % (not dark)
    reg = ["REGEDIT4", "",
           r"[HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\ThemeManager]",
           '"ColorName"="%s"' % ("Dark" if dark else "Blue"), '"ThemeActive"="1"', '"LoadedBefore"="1"', "",
           r"[HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize]",
           '"AppsUseLightTheme"=' + light, '"SystemUsesLightTheme"=' + light, "",
           r"[HKEY_CURRENT_USER\Control Panel\Colors]"]
    reg += ['"%s"="%d %d %d"' % (k, *v) for k, v in sys_colours(p).items()]
    reg += ['"%s"=-' % k for k in ("ActiveCaption", "InactiveCaption", "GradientActiveCaption",
                                   "GradientInactiveCaption", "CaptionText", "InactiveCaptionText")]
    reg += ["", r"[HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Accent]",
            '"AccentPalette"=hex:' + ",".join("%02x,%02x,%02x,00" % c for c in shades), "",
            r"[HKEY_CURRENT_USER\Software\Microsoft\Windows\DWM]",
            '"AccentColor"=dword:ff%02x%02x%02x' % a[::-1],
            '"ColorizationColor"=dword:ff%02x%02x%02x' % a]
    return "\n".join(reg) + "\n"


def main():
    reg = registry()
    saved = os.path.join(PREFIX, ".desktop-settings.reg")
    try:
        if "--force" not in sys.argv[1:] and open(saved, encoding="utf-8").read() == reg:
            return 0
    except OSError:
        pass
    new = saved + ".new"
    with open(new, "w", encoding="utf-8") as f:
        f.write(reg)
    rc = subprocess.run(["wine", "regedit", "/S", "Z:" + new.replace("/", "\\")],
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode
    if rc == 0:
        os.replace(new, saved)
    else:
        os.remove(new)
    return rc


if __name__ == "__main__":
    sys.exit(main())
