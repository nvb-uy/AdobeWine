# shellcheck shell=bash
# PhotoshopWine environment. Source this file; it never touches a system-wide Wine.
#   PHOTOSHOPWINE_HOME  where the runtime, dependencies and prefix live
#                       (default: ${XDG_DATA_HOME:-~/.local/share}/photoshopwine)
#   WINEPREFIX          the Windows prefix (default: $PHOTOSHOPWINE_HOME/prefix)
if [ -n "${BASH_SOURCE[0]:-}" ]; then _psw_env="${BASH_SOURCE[0]}"
elif [ -n "${ZSH_VERSION:-}" ]; then eval '_psw_env="${(%):-%x}"'
else _psw_env="$0"; fi
PHOTOSHOPWINE_SRC="$(cd "$(dirname "$_psw_env")/.." && pwd)"; unset _psw_env
_psw_running() {
  local s p
  s="$(stat -c '%D %i' "$1/prefix" 2>/dev/null)" || return 1
  s="/tmp/.wine-$(id -u)/server-${s% *}-$(printf '%x' "${s#* }")"
  for p in /proc/[0-9]*; do
    [ "$(readlink "$p/cwd" 2>/dev/null)" = "$s" ] && return 0
  done
  return 1
}
_psw_move() {
  [ -d "$1/adobewine" ] && [ ! -L "$1/adobewine" ] && [ ! -e "$1/photoshopwine" ] && [ ! -L "$1/photoshopwine" ] || return 0
  mv "$1/adobewine" "$1/photoshopwine" && ln -s photoshopwine "$1/adobewine"
}
if [ -z "${PHOTOSHOPWINE_HOME:-}" ]; then
  _psw_data="${XDG_DATA_HOME:-$HOME/.local/share}"
  PHOTOSHOPWINE_HOME="$_psw_data/photoshopwine"
  if [ ! -e "$PHOTOSHOPWINE_HOME" ] && [ -d "$_psw_data/adobewine" ] && [ ! -L "$_psw_data/adobewine" ]; then
    if _psw_running "$_psw_data/adobewine" || ! _psw_move "$_psw_data"; then
      PHOTOSHOPWINE_HOME="$_psw_data/adobewine"
    fi
  fi
  unset _psw_data
fi
_psw_move "${XDG_CONFIG_HOME:-$HOME/.config}" 2>/dev/null || true
PHOTOSHOPWINE_CONFIG="${XDG_CONFIG_HOME:-$HOME/.config}/photoshopwine/config"
unset -f _psw_running _psw_move
export PHOTOSHOPWINE_SRC PHOTOSHOPWINE_HOME PHOTOSHOPWINE_CONFIG
export WINEPREFIX="${WINEPREFIX:-$PHOTOSHOPWINE_HOME/prefix}"
if [ -x "$PHOTOSHOPWINE_SRC/runtime/bin/wine" ]; then PHOTOSHOPWINE_RUNTIME="$PHOTOSHOPWINE_SRC/runtime"
else PHOTOSHOPWINE_RUNTIME="$PHOTOSHOPWINE_HOME/runtime"; fi
export PHOTOSHOPWINE_RUNTIME
export PATH="$PHOTOSHOPWINE_RUNTIME/bin:$PATH"
export WINEARCH=win64
export WINEDEBUG="${WINEDEBUG:--all,+err}"
export WINEDLLOVERRIDES="${WINEDLLOVERRIDES:-winemenubuilder.exe=d}"
export FONTCONFIG_FILE="${FONTCONFIG_FILE:-$PHOTOSHOPWINE_SRC/config/fonts.conf}"
mkdir -p "$PHOTOSHOPWINE_HOME/logs" "$PHOTOSHOPWINE_HOME/cache/shaders"
export DXVK_LOG_PATH="$PHOTOSHOPWINE_HOME/logs"
export VKD3D_SHADER_CACHE_PATH="${VKD3D_SHADER_CACHE_PATH:-$PHOTOSHOPWINE_HOME/cache/shaders}"
if [ "${PHOTOSHOPWINE_NVIDIA_OFFLOAD:-auto}" = 1 ] || { [ "${PHOTOSHOPWINE_NVIDIA_OFFLOAD:-auto}" = auto ] &&
     [ -e /proc/driver/nvidia/version ] && [ "$(ls -d /sys/class/drm/card[0-9] 2>/dev/null | wc -l)" -gt 1 ]; }; then
  export __NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia __VK_LAYER_NV_optimus=NVIDIA_only
fi
[ -e /proc/driver/nvidia/version ] && export DXVK_ENABLE_NVAPI=1
export WINE_HEAP_QUARANTINE="${WINE_HEAP_QUARANTINE-Creative Cloud.exe;Creative Cloud Helper.exe;Creative Cloud UI Helper.exe;Adobe Desktop Service.exe;node.exe;CCXProcess.exe;CoreSync.exe;CRLogTransport.exe;HDHelper.exe;AdobeIPCBroker.exe;AdobeUpdateService.exe}"
