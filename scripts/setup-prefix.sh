#!/usr/bin/env bash
# Create or refresh the AdobeWine prefix. Safe to run again.
# Usage: scripts/setup-prefix.sh [--windows-fonts DIR] [--dpi N]
#   --windows-fonts DIR   copy fonts from your own Windows installation (its C:\Windows\Fonts
#                         folder, e.g. a mounted Windows partition). Strongly recommended:
#                         Adobe's UI asks for Segoe UI, and Photoshop's menu bar crashes without it.
#   --dpi N|auto          UI scaling for the Adobe apps in DPI (96 = 100%, 144 = 150%, 192 = 200%),
#                         kept in ~/.config/adobewine/config. Default (auto): your desktop's scale
#                         (Xft.dpi), read again at each launch; 96 when it cannot be read.
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/bin/env.sh"
WINFONTS=""
DPI=""
while [ $# -gt 0 ]; do
  case "$1" in
    --windows-fonts) WINFONTS="$2"; shift 2 ;;
    --dpi) DPI="$2"; shift 2
           case "$DPI" in auto) ;; ''|*[!0-9]*) echo "--dpi takes a number or auto" >&2; exit 1 ;; esac ;;
    *) echo "unknown option $1" >&2; exit 1 ;;
  esac
done
[ -x "$ADOBEWINE_RUNTIME/bin/wine" ] || { echo "no AdobeWine runtime: install the adobewine package or run scripts/build-wine.sh" >&2; exit 1; }
command -v winetricks >/dev/null || { echo "winetricks is required (install it from your distribution)" >&2; exit 1; }
DEPS="$ADOBEWINE_HOME/deps"
ls -d "$DEPS"/dxvk-*/ >/dev/null 2>&1 || "$ADOBEWINE_SRC/scripts/fetch-deps.sh"
SYS="$WINEPREFIX/drive_c/windows/system32"
SYS32="$WINEPREFIX/drive_c/windows/syswow64"

# Browser engine (wine-gecko) and .NET replacement (wine-mono): the Creative Cloud
# installer's pages stay on "Loading..." without gecko.
CACHE="${XDG_CACHE_HOME:-$HOME/.cache}/wine"
mkdir -p "$CACHE"
for a in "gecko 2.47.4 x86" "gecko 2.47.4 x86_64" "mono 11.3.0 x86"; do
  read -r kind ver arch <<< "$a"
  f="wine-$kind-$ver-$arch.msi"
  [ -f "$CACHE/$f" ] && continue
  if [ -f "$ADOBEWINE_HOME/cache/$kind/$f" ]; then mv "$ADOBEWINE_HOME/cache/$kind/$f" "$CACHE/$f"; continue; fi
  curl -fsSL -o "$CACHE/$f.part" "https://dl.winehq.org/wine/wine-$kind/$ver/$f"
  mv "$CACHE/$f.part" "$CACHE/$f"
done
rmdir "$ADOBEWINE_HOME/cache/gecko" "$ADOBEWINE_HOME/cache/mono" "$ADOBEWINE_HOME/cache" 2>/dev/null || true

[ -f "$WINEPREFIX/system.reg" ] || wineboot -i
wineserver -w
[ -d "$SYS/gecko/2.47.4" ] || { wine msiexec /i "$CACHE/wine-gecko-2.47.4-x86_64.msi" /qn; wine msiexec /i "$CACHE/wine-gecko-2.47.4-x86.msi" /qn; }
[ -d "$WINEPREFIX/drive_c/windows/mono" ] || wine msiexec /i "$CACHE/wine-mono-11.3.0-x86.msi" /qn

# Microsoft's shader compiler (Audition compiles HLSL at run time) and the core fonts
# (the Creative Cloud renderer crashes without real Windows fonts).
[ -f "$WINEPREFIX/.adobewine-d3dcompiler" ] || { winetricks -q d3dcompiler_47 && touch "$WINEPREFIX/.adobewine-d3dcompiler"; }
[ -f "$WINEPREFIX/drive_c/windows/Fonts/arial.ttf" ] || winetricks -q corefonts tahoma
# WinRT classes added by the patches (0031 ToastNotificationManager) need registering.
wine regsvr32 /s windows.ui.dll

override() { wine reg add 'HKCU\Software\Wine\DllOverrides' /v "$1" /d native,builtin /f >/dev/null; }
# DXVK: Direct3D 8-11 -> Vulkan
for d in d3d8 d3d9 d3d10core d3d11 dxgi; do
  cp "$DEPS"/dxvk-*/x64/$d.dll "$SYS/"; cp "$DEPS"/dxvk-*/x32/$d.dll "$SYS32/"; override $d
done
# vkd3d-proton: Direct3D 12 -> Vulkan, behind a small shim that retries
# D3D_FEATURE_LEVEL_1_0_CORE device creation at 11_0 (Adobe's AI features use core devices).
for d in d3d12 d3d12core; do
  cp "$DEPS"/vkd3d-proton-*/x64/$d.dll "$SYS/"; cp "$DEPS"/vkd3d-proton-*/x86/$d.dll "$SYS32/"; override $d
done
if [ -f "$ADOBEWINE_SRC/lib/d3d12.dll" ]; then
  mv "$SYS/d3d12.dll" "$SYS/d3d12_vkd3d.dll"; cp "$ADOBEWINE_SRC/lib/d3d12.dll" "$SYS/d3d12.dll"
elif command -v x86_64-w64-mingw32-gcc >/dev/null; then
  mv "$SYS/d3d12.dll" "$SYS/d3d12_vkd3d.dll"
  x86_64-w64-mingw32-gcc -O2 -shared -o "$SYS/d3d12.dll" "$ADOBEWINE_SRC/shims/d3d12/d3d12shim.c" \
      "$ADOBEWINE_SRC/shims/d3d12/d3d12.def" -Wl,--enable-stdcall-fixup
else
  echo "note: no MinGW compiler, skipping the d3d12 shim (Adobe AI features may be unavailable)" >&2
fi
# Photoshop's GPU check (sniffer.exe) wants a Direct3D 12 device at feature level 12_0 and
# reports no GPU at all without one; Camera Raw wants 12_1 on Intel GPUs. vkd3d-proton
# offers 12_0 only with Vulkan sparse residency, which some drivers lack although the Windows
# driver for the same GPU offers 12_x (e.g. Mesa on Gen9 Intel graphics). Such GPUs get
# VKD3D_FEATURE_LEVEL in the prefix's environment: 12_1 when they have the rest of 12_1
# (ROVs, conservative rasterization), as on Windows, else 12_0. Other GPUs get nothing, as
# the override would cap them.
ENVKEY='HKCU\Environment'
wine reg delete "$ENVKEY" /v VKD3D_FEATURE_LEVEL /f >/dev/null 2>&1 || true
FL="$ADOBEWINE_SRC/lib/d3d12fl.exe"
[ -f "$FL" ] || FL="$ADOBEWINE_HOME/d3d12fl.exe"
if [ ! -f "$FL" ] && command -v x86_64-w64-mingw32-gcc >/dev/null; then
  x86_64-w64-mingw32-gcc -O2 -o "$FL" "$ADOBEWINE_SRC/tools/d3d12fl/d3d12fl.c"
fi
if [ -f "$FL" ]; then
  levels="$(env -u VKD3D_FEATURE_LEVEL wine "$FL" 2>/dev/null | tr -d '\r')" && rc=0 || rc=$?
  if [ "$rc" = 2 ]; then
    wine reg add "$ENVKEY" /v VKD3D_FEATURE_LEVEL /d "${levels##* }" /f >/dev/null
    echo "note: this GPU offers Direct3D 12 feature level ${levels%% *} only; reporting ${levels##* } so Photoshop and Camera Raw use it." >&2
  fi
fi
# NVIDIA only: CUDA / NVENC / NVAPI / NVML bridges to the Linux driver.
if [ -e /proc/driver/nvidia/version ]; then
  "$(ls -d "$DEPS"/nvidia-libs-*/ | head -1)/setup_nvlibs.sh" install
fi

# Windows version: 11 (build 26100, patch 0018) for everything; Photoshop 27 divides by
# zero at launch on that build, so Photoshop alone reports Windows 10.
wine winecfg /v win11 >/dev/null 2>&1
wine reg add 'HKCU\Software\Wine\AppDefaults\Photoshop.exe' /v Version /d win10 /f >/dev/null
# WebView2 (the Home screens) presents through a DirectComposition swapchain; only Wine's
# own dxgi has composition swapchains (patches 0050, 0051), DXVK does not.
wine reg add 'HKCU\Software\Wine\AppDefaults\msedgewebview2.exe\DllOverrides' /v dxgi /d builtin /f >/dev/null
wine reg add 'HKCU\Software\Wine\AppDefaults\msedgewebview2.exe\DllOverrides' /v d3d11 /d builtin /f >/dev/null
# X11 driver (through XWayland on Wayland desktops), and no window-manager decorations:
# Adobe apps start maximised and a title bar would force a resize that leaves panels undrawn.
wine reg add 'HKCU\Software\Wine\Drivers' /v Graphics /d x11 /f >/dev/null
wine reg add 'HKCU\Software\Wine\X11 Driver' /v Decorated /d N /f >/dev/null
wine reg add 'HKCU\Software\Wine\WineDbg' /v ShowCrashDialog /t REG_DWORD /d 0 /f >/dev/null
wine reg add 'HKCU\Software\Wine\Explorer' /v ShowSystray /t REG_DWORD /d 0 /f >/dev/null
if [ -n "$DPI" ]; then
  conf="${XDG_CONFIG_HOME:-$HOME/.config}/adobewine/config"
  mkdir -p "${conf%/*}"; touch "$conf"
  sed -i '/^DPI=/d' "$conf"
  [ "$DPI" = auto ] || echo "DPI=$DPI" >> "$conf"
fi

# Fonts from your own Windows installation (Segoe UI and friends).
if [ -n "$WINFONTS" ]; then
  [ -d "$WINFONTS" ] || { echo "not a folder: $WINFONTS" >&2; exit 1; }
  cp -n "$WINFONTS"/*.[tToO][tT][fFcC] "$WINEPREFIX/drive_c/windows/Fonts/" 2>/dev/null || true
elif ! ls "$WINEPREFIX/drive_c/windows/Fonts/"segoeui.ttf >/dev/null 2>&1; then
  # No Windows fonts: Segoe UI made from Selawik, Microsoft's open metric-compatible
  # fallback, if the system has it. It must really be named "Segoe UI": DirectWrite ignores
  # FontSubstitutes, and Photoshop's menu bar hangs the UI without that family.
  mapfile -t selawik < <(fc-list -f '%{file}\n' :family=Selawik 2>/dev/null)
  if [ "${#selawik[@]}" -gt 0 ] &&
     python3 "$ADOBEWINE_SRC/scripts/selawik-as-segoe.py" "$WINEPREFIX/drive_c/windows/Fonts" "${selawik[@]}" >/dev/null; then
    echo "note: no Windows fonts given; Segoe UI was made from Selawik." >&2
  else
    echo "note: Segoe UI is missing. Run again with --windows-fonts <your Windows Fonts folder>," >&2
    echo "      or install the Selawik fonts and run setup again." >&2
  fi
fi
"$ADOBEWINE_SRC/scripts/register-fonts.sh"
python3 -I "$ADOBEWINE_SRC/scripts/desktop-settings.py" --force
wineserver -w
echo "prefix ready: $WINEPREFIX ($(wine --version))"
