#!/usr/bin/env bash
# Checks the release tarball on several distributions in containers (needs Docker or Podman):
#   tests/distro/check.sh path/to/photoshopwine-runtime-VER-x86_64.tar.zst [image ...]
# This tests installation and that the runtime starts. It does not test graphics or the
# Adobe apps, which need a real GPU and desktop.
set -uo pipefail
tarball="$(realpath "$1")"; shift
here="$(cd "$(dirname "$0")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
engine="$(command -v docker || command -v podman)"
images=("$@")
[ ${#images[@]} -gt 0 ] || images=(archlinux:latest debian:13 ubuntu:24.04 fedora:42 opensuse/tumbleweed)
for img in "${images[@]}"; do
  echo "################ $img"
  "$engine" run --rm -v "$tarball:/release/photoshopwine.tar.zst:ro" -v "$here:/t:ro" -v "$repo:/src:ro" "$img" \
    bash /t/run-in-container.sh "${img%%[:/]*}"
done
