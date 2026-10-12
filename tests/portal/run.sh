#!/usr/bin/env bash
# Portal tests for the comdlg32 and shell32 file dialogs, headless (null graphics driver, private
# session bus, mock FileChooser from mock_portal.py):
#
#   tests/portal/run.sh <wine> [outdir]
#
# <wine> is bin/wine of a runtime or the wine loader of a build tree. PORTAL_CASES="legacy hook"
# runs only these cases. Every case runs portal.exe under a timeout (a hang is a failure), and
# check.py compares what the app got with what the mock was asked. Needs dbus-run-session,
# x86_64-w64-mingw32-gcc and -windres, python3 with dbus + gi. Prints PASS/FAIL/SKIP lines; exit
# status = failures.
set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
WINE="$(readlink -f "${1:?usage: run.sh <wine> [outdir]}")"
OUT="$(mkdir -p "${2:-/tmp/adobewine-portal}" && cd "${2:-/tmp/adobewine-portal}" && pwd)"
PY=
for p in ${PORTAL_PYTHON:-} /usr/bin/python3 python3; do
  "$p" -c 'import dbus, dbus.service, dbus.mainloop.glib; from gi.repository import GLib' 2>/dev/null && { PY=$p; break; }
done
[ -n "$PY" ] || { echo "SKIP all: no python with dbus + gi"; exit 0; }
for s in "$(dirname "$WINE")/wineserver" "$(dirname "$WINE")/server/wineserver"; do
  [ -x "$s" ] && { export WINESERVER="$s"; break; }
done
export WINEPREFIX="$OUT/prefix" WINEDEBUG="${WINEDEBUG:--all}" WINEDLLOVERRIDES="mscoree,mshtml=" WINEARCH=win64
unset DISPLAY WAYLAND_DISPLAY WINE_FORCE_PORTAL

x86_64-w64-mingw32-windres "$HERE/portal.rc" -O coff -o "$OUT/portal.res" &&
x86_64-w64-mingw32-gcc -O1 -municode -o "$OUT/portal.exe" "$HERE/portal.c" "$OUT/portal.res" \
  -lcomdlg32 -lole32 -luuid -lshell32 -lcomctl32 || { echo "FAIL build portal.exe"; exit 1; }

reg() { "$WINE" reg "$@" >/dev/null 2>&1; }
if [ ! -f "$WINEPREFIX/system.reg" ]; then
  "$WINE" wineboot -i >/dev/null 2>&1
  reg add 'HKCU\Software\Wine\Drivers' /v Graphics /d null /f
  "${WINESERVER:-wineserver}" -w
fi
reg delete 'HKCU\Software\Wine\AppDefaults\portal.exe' /f
reg delete 'HKCU\Software\Wine\X11 Driver' /v FileDialogPortal /f

F="$OUT/files"
rm -rf "$F"; mkdir -p "$F/host" "$F/other" "$F/out" "$F/run/doc/0a1b2c3d"
for f in host/frame_0001.png host/frame_0002.png other/x.png out/picture.jpg out/existing.png; do echo x >"$F/$f"; done
cp "$F/host/frame_0001.png" "$F/run/doc/0a1b2c3d/frame_0001.png"
ln -s host "$F/link"
"$PY" -c 'import os, sys; os.setxattr(sys.argv[1], "user.document-portal.host-path", sys.argv[2].encode())' \
  "$F/run/doc/0a1b2c3d/frame_0001.png" "$F/host/frame_0001.png" 2>/dev/null && echo 1 >"$OUT/xattr" || rm -f "$OUT/xattr"
WINDIR="Z:$(echo "$F" | tr / '\\')"
cat >"$OUT/bus.conf" <<'EOF'
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <type>session</type>
  <listen>unix:tmpdir=/tmp</listen>
  <auth>EXTERNAL</auth>
  <policy context="default">
    <allow send_destination="*" eavesdrop="true"/>
    <allow eavesdrop="true"/>
    <allow own="*"/>
  </policy>
</busconfig>
EOF
U="file://$F"

# run_case <name> <mode> <spec json or "none"> [VAR=value ...]
run_case() {
  local name=$1 mode=$2 spec=$3; shift 3
  [ -z "${PORTAL_CASES:-}" ] || [[ " $PORTAL_CASES " == *" $name "* ]] || return 0
  rm -f "$OUT/$name".*
  local mock="true"
  if [ "$spec" != none ]; then
    echo "$spec" >"$OUT/$name.spec"
    mock="'$PY' '$HERE/mock_portal.py' '$OUT/$name.json' '$OUT/$name.spec' >'$OUT/$name.mock' 2>&1 &
      for i in \$(seq 50); do grep -q ready '$OUT/$name.mock' && break; sleep 0.1; done"
  fi
  env "$@" timeout 90 dbus-run-session --config-file="$OUT/bus.conf" -- sh -c "$mock
    '$WINE' '$OUT/portal.exe' $mode '$WINDIR'" >"$OUT/$name.raw" 2>"$OUT/$name.err"
  echo $? >"$OUT/$name.status"
  tr -d '\r' <"$OUT/$name.raw" >"$OUT/$name.out"
  [ "$(cat "$OUT/$name.status")" = 124 ] && "${WINESERVER:-wineserver}" -k 2>/dev/null
  touch "$OUT/$name.json"
}

F1="\"$U/host/frame_0001.png\""; F2="\"$U/host/frame_0002.png\""
run_case legacy legacy "[{\"uris\": [$F1]}]" WINE_FORCE_PORTAL=1
run_case save save "[{\"uris\": [\"$U/out/new%20project.aep\"]}]" WINE_FORCE_PORTAL=1
run_case savedef savedef "[{\"uris\": [\"$U/out/picture\"], \"filter\": \"Images (*.png;*.jpg)\"}]" WINE_FORCE_PORTAL=1
run_case multi multi "[{\"uris\": [$F1, $F2, \"$U/other/x.png\"]}]" WINE_FORCE_PORTAL=1
run_case hook hook "[{\"uris\": [$F2], \"filter\": 1}]" WINE_FORCE_PORTAL=1
run_case reject reject "[{\"uris\": [$F1], \"choices\": \"keep\"}, {\"uris\": [$F2], \"choices\": \"keep\"}]" WINE_FORCE_PORTAL=1
run_case hooka hooka "[{\"uris\": [\"$U/out/existing.png\"]}]" WINE_FORCE_PORTAL=1
run_case item item "[{\"uris\": [$F1]}]" WINE_FORCE_PORTAL=1
run_case docportal item "[{\"uris\": [\"$U/run/doc/0a1b2c3d/frame_0001.png\"]}]" WINE_FORCE_PORTAL=1
run_case itemsave itemsave "[{\"uris\": [\"$U/out/picture\"], \"filter\": \"JPEG (*.JPG;*.JPEG;*.JPE)\"}]" WINE_FORCE_PORTAL=1
run_case itemreopen itemreopen "[{\"uris\": [$F1]}, {\"uris\": [$F2]}]" WINE_FORCE_PORTAL=1
run_case folder folder "[{\"uris\": [\"$U/other\"]}]" WINE_FORCE_PORTAL=1
run_case browse browse "[{\"uris\": [\"$U/other\"]}]" WINE_FORCE_PORTAL=1
run_case modal modal "[{\"uris\": [$F1], \"delay_ms\": 1500}]" WINE_FORCE_PORTAL=1
run_case fallback fallback none WINE_FORCE_PORTAL=1
run_case itemfallback itemfallback none WINE_FORCE_PORTAL=1
run_case browsefallback browsefallback none WINE_FORCE_PORTAL=1
run_case vanish vanish "[{\"code\": \"exit\"}]" WINE_FORCE_PORTAL=1
run_case off off "[{\"uris\": [$F1]}]" WINE_FORCE_PORTAL=0
reg add 'HKCU\Software\Wine\X11 Driver' /v FileDialogPortal /d auto /f
run_case hookauto hookauto "[{\"uris\": [$F1]}]"
run_case itemauto itemlegacy "[{\"uris\": [$F1]}]"
reg add 'HKCU\Software\Wine\X11 Driver' /v FileDialogPortal /d never /f
reg add 'HKCU\Software\Wine\AppDefaults\portal.exe\X11 Driver' /v FileDialogPortal /d always /f
run_case appdefault hook "[{\"uris\": [$F1]}]"
reg delete 'HKCU\Software\Wine\AppDefaults\portal.exe' /f
reg delete 'HKCU\Software\Wine\X11 Driver' /v FileDialogPortal /f
"${WINESERVER:-wineserver}" -k 2>/dev/null

"$PY" "$HERE/check.py" "$OUT" "$F" "$WINDIR"
