#!/usr/bin/env bash
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/bin/env.sh"
ver="$(tr -d '[:space:]' < "$PHOTOSHOPWINE_SRC/VERSION")"
rt="$PHOTOSHOPWINE_HOME/runtime"
out="$(realpath "${1:-.}")"
name="photoshopwine-runtime-$ver-x86_64.tar.zst"
[ -x "$rt/bin/wine" ] || { echo "no runtime in $rt: run scripts/build-wine.sh first" >&2; exit 1; }
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/helpers"
cd "$PHOTOSHOPWINE_SRC"
x86_64-w64-mingw32-gcc -O2 -shared -o "$tmp/helpers/d3d12.dll" shims/d3d12/d3d12shim.c shims/d3d12/d3d12.def \
    -Wl,--enable-stdcall-fixup
x86_64-w64-mingw32-gcc -O2 -o "$tmp/helpers/winclose.exe" tools/winclose/winclose.c
x86_64-w64-mingw32-gcc -O2 -o "$tmp/helpers/d3d12fl.exe" tools/d3d12fl/d3d12fl.c
mkdir -p "$out"
tar -I "zstd -${ZSTD_LEVEL:-19} -T0" -cf "$out/$name.part" --owner=0 --group=0 \
    -C "$PHOTOSHOPWINE_HOME" --exclude=runtime/.photoshopwine --exclude=runtime/include runtime \
    -C "$tmp" helpers
mv "$out/$name.part" "$out/$name"
(cd "$out" && sha256sum "$name" > "$name.sha256")
cat "$out/$name.sha256"
echo "upload $out/$name and $name.sha256 to the v$ver GitHub release"
