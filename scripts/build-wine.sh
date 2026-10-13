#!/usr/bin/env bash
# Build PhotoshopWine: upstream Wine 11.18 + the patches in patches/, installed into
# $PHOTOSHOPWINE_HOME/runtime. Usage: scripts/build-wine.sh [jobs]
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/bin/env.sh"
JOBS="${1:-$(nproc)}"
TAG=wine-11.18
SRC="$PHOTOSHOPWINE_HOME/src/wine"
BUILD="$PHOTOSHOPWINE_HOME/src/build"
mkdir -p "$PHOTOSHOPWINE_HOME/src"

if [ ! -d "$SRC/.git" ]; then
  git clone --depth 1 --branch "$TAG" https://gitlab.winehq.org/wine/wine.git "$SRC"
fi
cd "$SRC"
# Start from a clean tree every time, so a changed or added patch always applies.
git reset -q --hard "$TAG" 2>/dev/null || git reset -q --hard
git clean -qfdx
for p in "$PHOTOSHOPWINE_SRC"/patches/*.patch; do
  git apply --whitespace=nowarn "$p" || { echo "patch failed: $(basename "$p")" >&2; exit 1; }
done
echo "applied $(ls "$PHOTOSHOPWINE_SRC"/patches/*.patch | wc -l) patches"
python3 -I "$PHOTOSHOPWINE_SRC/scripts/make-dark-theme.py" "$SRC"

# OpenCL headers are not packaged everywhere; use Khronos' copy.
[ -d "$PHOTOSHOPWINE_HOME/src/OpenCL-Headers" ] ||
  git clone --depth 1 https://github.com/KhronosGroup/OpenCL-Headers.git "$PHOTOSHOPWINE_HOME/src/OpenCL-Headers"
export CPPFLAGS="-I$PHOTOSHOPWINE_HOME/src/OpenCL-Headers ${CPPFLAGS:-}"
if command -v ccache >/dev/null; then
  export CC="ccache gcc" x86_64_CC="ccache x86_64-w64-mingw32-gcc" i386_CC="ccache i686-w64-mingw32-gcc"
fi
mkdir -p "$BUILD" && cd "$BUILD"
[ -f Makefile ] || "$SRC/configure" --prefix="$PHOTOSHOPWINE_HOME/runtime" --enable-archs=i386,x86_64 --disable-tests
make -j"$JOBS"
make install -j"$JOBS"
"$PHOTOSHOPWINE_HOME/runtime/bin/wine" --version
