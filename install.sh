#!/usr/bin/env bash
# Install PhotoshopWine, or continue an install that stopped. Safe to run again.
#   curl -fsSL https://raw.githubusercontent.com/nvb-uy/PhotoshopWine/master/install.sh | bash
#   curl -fsSL .../install.sh | bash -s -- /path/to/Creative_Cloud_Set-Up.exe
#   ./install.sh [Creative_Cloud_Set-Up.exe]    from a checkout
#   ./install.sh --update                        what photoshopwine update runs
set -euo pipefail

REPO="${PHOTOSHOPWINE_REPO:-https://github.com/nvb-uy/PhotoshopWine.git}"
BRANCH="${PHOTOSHOPWINE_BRANCH:-master}"
RELEASES="${PHOTOSHOPWINE_RELEASES:-https://github.com/nvb-uy/PhotoshopWine/releases/download}"
DATA="${XDG_DATA_HOME:-$HOME/.local/share}"

pick_home() {
  local old="$DATA/adobewine" new="$DATA/photoshopwine" s p
  if [ -e "$new" ] || [ -L "$new" ] || [ ! -d "$old" ] || [ -L "$old" ]; then echo "$new"; return; fi
  s="$(stat -c '%D %i' "$old/prefix" 2>/dev/null || true)"
  if [ -n "$s" ]; then
    s="/tmp/.wine-$(id -u)/server-${s% *}-$(printf '%x' "${s#* }")"
    for p in /proc/[0-9]*; do
      if [ "$(readlink "$p/cwd" 2>/dev/null)" = "$s" ]; then echo "$old"; return; fi
    done
  fi
  if mv "$old" "$new" 2>/dev/null; then
    ln -s photoshopwine "$old" 2>/dev/null || true
    echo "$new"
  else
    echo "$old"
  fi
}

export PHOTOSHOPWINE_HOME="${PHOTOSHOPWINE_HOME:-$(pick_home)}"
export WINEPREFIX="${WINEPREFIX:-$PHOTOSHOPWINE_HOME/prefix}"
RUNTIME="$PHOTOSHOPWINE_HOME/runtime"
STAMP="$RUNTIME/.photoshopwine"
MARKER="$WINEPREFIX/.photoshopwine-setup"
PF="$WINEPREFIX/drive_c/Program Files/Adobe"
CCEXE="$PF/Adobe Creative Cloud/ACC/Creative Cloud.exe"
CCURL="https://creativecloud.adobe.com/apps/download/creative-cloud"
LOG="$PHOTOSHOPWINE_HOME/logs/install-$(date +%Y%m%d-%H%M%S).log"
MODE=install
SETUP_ARG=""
CC_SETUP=""
SRC=""
QUIET=""

say() { printf '==> %s\n' "$*"; }
note() { printf '    %s\n' "$@"; }
die() { QUIET=1; printf '%s\n' "$@" >&2; exit 1; }
log() { "$@" >>"$LOG" 2>&1 </dev/null; }

on_exit() {
  local rc=$?
  [ "$rc" = 0 ] || [ -n "$QUIET" ] || printf '\nSomething went wrong. Details are in %s\n' "$LOG" >&2
}

on_int() {
  QUIET=1
  printf '\nStopped. Run the installer again to continue where it left off.\n' >&2
  exit 130
}

package_manager() {
  local w
  for w in $(sed -n 's/^ID\(_LIKE\)\{0,1\}=//p' "${PHOTOSHOPWINE_OS_RELEASE:-/etc/os-release}" 2>/dev/null | tr -d "\"'"); do
    case "$w" in
      debian|ubuntu|linuxmint|pop) echo apt; return ;;
      fedora|rhel|centos) echo dnf; return ;;
      arch|archlinux|manjaro|endeavouros|cachyos) echo pacman; return ;;
      opensuse*|suse|sles) echo zypper; return ;;
      gentoo) echo emerge; return ;;
    esac
  done
  echo unknown
}

package_name() {
  case "$2:$1" in
    pacman:python3) echo python ;;
    pacman:mingw) echo mingw-w64-gcc ;;
    apt:mingw) echo mingw-w64 ;;
    dnf:mingw) echo mingw64-gcc mingw32-gcc ;;
    zypper:mingw) echo mingw64-cross-gcc mingw32-cross-gcc ;;
    emerge:git) echo dev-vcs/git ;;
    emerge:curl) echo net-misc/curl ;;
    emerge:python3) echo dev-lang/python ;;
    emerge:winetricks) echo app-emulation/winetricks ;;
    emerge:tar) echo app-arch/tar ;;
    emerge:zstd) echo app-arch/zstd ;;
    emerge:mingw) echo sys-devel/crossdev ;;
    *) echo "$1" ;;
  esac
}

install_hint() {
  local pm pkgs=() p=() t
  pm="$(package_manager)"
  for t in "$@"; do read -ra p <<< "$(package_name "$t" "$pm")"; pkgs+=("${p[@]}"); done
  case "$pm" in
    apt) echo "  sudo apt install ${pkgs[*]}" ;;
    dnf) echo "  sudo dnf install ${pkgs[*]}" ;;
    pacman) echo "  sudo pacman -S --needed ${pkgs[*]}" ;;
    zypper) echo "  sudo zypper install ${pkgs[*]}" ;;
    emerge) echo "  sudo emerge --ask ${pkgs[*]}" ;;
    *) echo "  ${pkgs[*]}  (with your distribution's package manager)" ;;
  esac
  if [ "$pm" = apt ] && [[ " $* " = *" winetricks "* ]] && grep -qs '^ID=debian' "${PHOTOSHOPWINE_OS_RELEASE:-/etc/os-release}"; then
    echo "  (on Debian, winetricks is in the \"contrib\" section: enable it first)"
  fi
}

check_system() {
  [ "$(uname -s)" = Linux ] && [ "$(uname -m)" = x86_64 ] || die "PhotoshopWine needs a 64-bit x86 PC running Linux."
  [ "$(id -u)" != 0 ] || die "Run the installer as your normal user, without sudo."
  local t missing=()
  for t in git curl python3 winetricks tar zstd; do
    command -v "$t" >/dev/null || missing+=("$t")
  done
  [ ${#missing[@]} = 0 ] && return
  die "A few programs are missing: ${missing[*]}" "Install them with:" "$(install_hint "${missing[@]}")" "Then run the installer again."
}

photoshop_present() {
  local f
  for f in "$PF"/"Adobe Photoshop 20"[0-9][0-9]/Photoshop.exe; do
    if [ -f "$f" ]; then return 0; fi
  done
  return 1
}

photoshop_ready() {
  local f
  for f in "$PF"/"Adobe Photoshop 20"[0-9][0-9]/Photoshop.exe; do
    if [ -f "$f" ] && [ -z "$(find "${f%/*}" -mmin -1 -print -quit 2>/dev/null)" ]; then return 0; fi
  done
  return 1
}

find_cc_setup() {
  if photoshop_present || [ -f "$CCEXE" ]; then return; fi
  if [ -n "$SETUP_ARG" ]; then
    [ -f "$SETUP_ARG" ] || die "Can't find $SETUP_ARG"
    CC_SETUP="$(readlink -f "$SETUP_ARG")"
    return
  fi
  local dir f
  dir="$(xdg-user-dir DOWNLOAD 2>/dev/null || true)"
  if [ -z "$dir" ] || [ "$dir" = "$HOME" ] || [ ! -d "$dir" ]; then dir="$HOME/Downloads"; fi
  for f in "$dir"/Creative_Cloud_Set-Up*.exe; do
    if [ -f "$f" ] && { [ -z "$CC_SETUP" ] || [ "$f" -nt "$CC_SETUP" ]; }; then CC_SETUP="$f"; fi
  done
  [ -n "$CC_SETUP" ] || die "First, download the Creative Cloud installer from Adobe:" \
    "  $CCURL" \
    "Save it in $dir and run this installer again." \
    "(Or give its path: curl -fsSL ... | bash -s -- /path/to/Creative_Cloud_Set-Up.exe)"
}

get_source() {
  local self="${BASH_SOURCE[0]:-}" dest="$PHOTOSHOPWINE_HOME/checkout"
  if [ -n "$self" ] && [ -f "$self" ] && [ -x "$(dirname "$self")/bin/photoshopwine" ]; then
    SRC="$(cd "$(dirname "$self")" && pwd -P)"
    return
  fi
  if [ -d "$dest/.git" ]; then
    say "Updating PhotoshopWine"
    log git -C "$dest" pull -q --ff-only "$REPO" "$BRANCH" || note "Could not update $dest, using the version already there."
  elif [ -e "$dest" ] && [ -n "$(ls -A "$dest")" ]; then
    die "$dest already exists and is not a PhotoshopWine download." "Move it away and run the installer again."
  else
    say "Getting PhotoshopWine"
    log git clone -q --depth 1 --branch "$BRANCH" "$REPO" "$dest"
  fi
  SRC="$dest"
}

update_source() {
  [ -z "${PHOTOSHOPWINE_PULLED:-}" ] || return 0
  [ "$(git -C "$SRC" rev-parse --show-toplevel 2>/dev/null)" = "$SRC" ] || return 0
  if ! git -C "$SRC" diff --quiet HEAD -- 2>/dev/null; then
    note "$SRC has local changes, so its files are left as they are."
    return 0
  fi
  say "Updating PhotoshopWine"
  local old new
  old="$(git -C "$SRC" rev-parse HEAD)"
  if ! log git -C "$SRC" pull -q --ff-only; then
    note "Could not get the latest version (details in $LOG), keeping this one."
    return 0
  fi
  new="$(git -C "$SRC" rev-parse HEAD)"
  [ "$old" = "$new" ] || PHOTOSHOPWINE_PULLED=1 exec bash "$SRC/install.sh" --update
}

runtime_id() {
  local h
  h="$({ grep -m1 '^TAG=' "$SRC/scripts/build-wine.sh"; cat "$SRC"/patches/*.patch "$SRC/scripts/make-dark-theme.py"; } | sha256sum | cut -c1-12)"
  echo "$VERSION+$h"
}

runtime_current() {
  [ -x "$SRC/runtime/bin/wine" ] && return 0
  [ -x "$RUNTIME/bin/wine" ] || return 1
  if [ ! -f "$STAMP" ]; then echo "build $(runtime_id)" > "$STAMP"; return 0; fi
  local kind id
  read -r kind id < "$STAMP" || true
  [ "$id" = "$(runtime_id)" ] && return 0
  [ "$kind" = release ] && [ "$id" = "$VERSION" ] && return 0
  return 1
}

apps_running() {
  local rt="$RUNTIME"
  [ -x "$SRC/runtime/bin/wine" ] && rt="$SRC/runtime"
  pgrep -u "$(id -u)" -f -- "$(printf '%s/' "$rt" | sed 's/[][\.*^$+?(){}|]/\\&/g')" >/dev/null
}

check_apps_closed() {
  apps_running || return 0
  die "Photoshop or Creative Cloud is still open. Close them first (photoshopwine off), then run this again."
}

release_available() {
  mkdir -p "$DL"
  local code
  code="$(curl -sSL -o "$DL/$NAME.sha256" -w '%{http_code}' "$URL.sha256" 2>>"$LOG" || true)"
  case "$code" in
    200) return 0 ;;
    404) rm -f "$DL/$NAME.sha256"; return 1 ;;
    *) die "Could not download $URL.sha256" "Check your internet connection and run the installer again." ;;
  esac
}

fetch_runtime() {
  local new="$PHOTOSHOPWINE_HOME/runtime.new" want got
  say "Downloading the Wine runtime"
  if [ -t 2 ]; then curl -fL --retry 3 --progress-bar -o "$DL/$NAME.part" "$URL"
  else log curl -fsSL --retry 3 -o "$DL/$NAME.part" "$URL"; fi
  want="$(cut -d' ' -f1 < "$DL/$NAME.sha256")"
  got="$(sha256sum "$DL/$NAME.part" | cut -d' ' -f1)"
  if [ "$want" != "$got" ]; then
    rm -f "$DL/$NAME.part"
    die "The download was damaged (checksum mismatch). Run the installer again."
  fi
  rm -rf "$new"
  mkdir -p "$new"
  log tar -C "$new" -I zstd -xf "$DL/$NAME.part"
  [ -x "$new/runtime/bin/wine" ] || die "The downloaded runtime is incomplete. Details are in $LOG"
  rm -rf "$PHOTOSHOPWINE_HOME/runtime.old"
  if [ -e "$RUNTIME" ] || [ -L "$RUNTIME" ]; then mv "$RUNTIME" "$PHOTOSHOPWINE_HOME/runtime.old"; fi
  mv "$new/runtime" "$RUNTIME"
  if [ -d "$new/helpers" ]; then mkdir -p "$SRC/lib"; cp "$new/helpers"/* "$SRC/lib/"; fi
  echo "release $VERSION" > "$STAMP"
  rm -rf "$new" "$PHOTOSHOPWINE_HOME/runtime.old" "$DL/$NAME.part" "$DL/$NAME.sha256"
}

build_runtime() {
  local t missing=() pm why="There is no prebuilt runtime for version $VERSION, so it is built on this computer."
  [ "${1:-}" = rebuild ] && why="Your runtime was built on this computer, so it is built again with the new changes."
  for t in gcc make flex bison perl pkg-config x86_64-w64-mingw32-gcc i686-w64-mingw32-gcc; do
    command -v "$t" >/dev/null || missing+=("$t")
  done
  if [ ${#missing[@]} -gt 0 ]; then
    pm="$(package_manager)"
    printf '%s\n' "$why" "Some build tools are missing (${missing[*]}). Install them with:" >&2
    case "$pm" in
      apt) note "sudo apt build-dep wine    (enable the source code repositories first)" "sudo apt install git ccache mingw-w64" ;;
      dnf) note "sudo dnf builddep wine" "sudo dnf install git ccache mingw64-gcc mingw32-gcc" ;;
      zypper) note "sudo zypper source-install --build-deps-only wine" "sudo zypper install git ccache mingw64-cross-gcc mingw32-cross-gcc" ;;
      pacman) note "sudo pacman -S --needed base-devel git ccache mingw-w64-gcc ffmpeg libcups libgphoto2 libxcomposite \\" \
                   "    libxinerama libxxf86vm mesa opencl-headers opencl-icd-loader pcsclite perl samba sane sdl2 \\" \
                   "    unixodbc v4l-utils vulkan-headers" ;;
      emerge) note "sudo emerge --ask --onlydeps app-emulation/wine-vanilla" "sudo emerge --ask sys-devel/crossdev && sudo crossdev --target x86_64-w64-mingw32 && sudo crossdev --target i686-w64-mingw32" ;;
      *) note "the build dependencies of Wine and a MinGW cross-compiler (x86_64 and i686)" ;;
    esac
    die "Then run the installer again."
  fi
  say "Building the Wine runtime"
  note "$why" \
       "This takes a long time (1-3 hours). To follow it: tail -f \"$LOG\""
  log "$SRC/scripts/build-wine.sh"
  echo "build $(runtime_id)" > "$STAMP"
}

get_runtime() {
  runtime_current && return 0
  check_apps_closed
  local kind=""
  [ -f "$STAMP" ] && { read -r kind _ < "$STAMP" || true; }
  if [ "$kind" = build ]; then build_runtime rebuild; return 0; fi
  if release_available; then fetch_runtime; else build_runtime; fi
}

link_command() {
  local bin="$HOME/.local/bin"
  mkdir -p "$bin"
  if [ "$(readlink -f "$bin/photoshopwine" 2>/dev/null)" != "$SRC/bin/photoshopwine" ]; then
    say "Adding the photoshopwine command"
    ln -sfn "$SRC/bin/photoshopwine" "$bin/photoshopwine"
  fi
}

path_hint() {
  case ":$PATH:" in *":$HOME/.local/bin:"*) return ;; esac
  echo
  echo "To use the photoshopwine command in a terminal, add ~/.local/bin to your PATH:"
  case "${SHELL##*/}" in
    zsh) echo "  echo 'export PATH=\"\$HOME/.local/bin:\$PATH\"' >> ~/.zshrc" ;;
    fish) echo "  fish_add_path ~/.local/bin" ;;
    *) echo "  echo 'export PATH=\"\$HOME/.local/bin:\$PATH\"' >> ~/.bashrc" ;;
  esac
}

run_setup() {
  local id=packaged
  [ -f "$STAMP" ] && id="$(cat "$STAMP")"
  if [ "$MODE" != update ] && [ -f "$MARKER" ] && [ "$(cat "$MARKER")" = "$id" ]; then return 0; fi
  check_apps_closed
  say "Setting up (a few minutes the first time)"
  log "$SRC/bin/photoshopwine" setup
  echo "$id" > "$MARKER"
}

install_photoshop() {
  photoshop_present && return 0
  if [ ! -f "$CCEXE" ]; then
    say "Installing Creative Cloud"
    note "Sign in with your Adobe account when it asks."
    log "$SRC/bin/photoshopwine" run "$CC_SETUP" || true
  fi
  say "Waiting for Photoshop"
  note "In Creative Cloud, sign in and install Photoshop from the Apps list." \
       "This window waits until Photoshop is installed. Ctrl+C is safe: run the installer" \
       "again later and it continues from here."
  local started=""
  until photoshop_ready; do
    if [ -z "$started" ] && [ -f "$CCEXE" ] && ! pgrep -u "$(id -u)" -f 'Creative Cloud\.exe' >/dev/null; then
      started=1
      setsid "$SRC/bin/photoshopwine" cc >>"$LOG" 2>&1 </dev/null &
    fi
    sleep 10
  done
}

add_launchers() {
  say "Adding Photoshop to your app menu"
  log "$SRC/bin/photoshopwine" launchers
}

usage() {
  sed -n '2,6p' "${BASH_SOURCE[0]:-install.sh}" 2>/dev/null | sed 's/^# \{0,1\}//'
}

main() {
  while [ $# -gt 0 ]; do
    case "$1" in
      --update) MODE=update ;;
      -h|--help) usage; return 0 ;;
      *) SETUP_ARG="$1" ;;
    esac
    shift
  done
  mkdir -p "${LOG%/*}"
  : > "$LOG"
  trap on_exit EXIT
  trap on_int INT
  check_system
  [ "$MODE" = update ] || find_cc_setup
  get_source
  if [ "$MODE" = update ]; then
    check_apps_closed
    update_source
  fi
  VERSION="$(tr -d '[:space:]' < "$SRC/VERSION")"
  NAME="photoshopwine-runtime-$VERSION-x86_64.tar.zst"
  URL="$RELEASES/v$VERSION/$NAME"
  DL="$PHOTOSHOPWINE_HOME/downloads"
  get_runtime
  link_command
  run_setup
  if [ "$MODE" = update ]; then
    if photoshop_present; then add_launchers; fi
    echo
    echo "PhotoshopWine is up to date."
    path_hint
    return 0
  fi
  install_photoshop
  add_launchers
  echo
  echo "Done. Open Adobe Photoshop from your app menu, or run photoshopwine."
  echo "Optional: if you have a Windows installation, its fonts make Photoshop's interface"
  echo "look exactly as on Windows: photoshopwine setup --windows-fonts /path/to/Windows/Fonts"
  path_hint
}

main "$@"
