#!/usr/bin/env bash
# Add the installed Adobe apps to your desktop's application menu (any freedesktop desktop:
# KDE, GNOME, Xfce, ...). Remove them again with --remove.
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/bin/env.sh"
DATA="${XDG_DATA_HOME:-$HOME/.local/share}"
APPS="$DATA/applications"
ICONS="$DATA/icons"
NAMES="photoshop illustrator premiere audition encoder aftereffects cc"
refresh() {
  update-desktop-database "$APPS" 2>/dev/null || true
  gtk-update-icon-cache -f -t "$ICONS/hicolor" >/dev/null 2>&1 || true
  [ -d "$DATA/mime/packages" ] && { update-mime-database "$DATA/mime" >/dev/null 2>&1 || true; }
  return 0
}
if [ "${1:-}" = --remove ]; then
  for n in $NAMES; do rm -f "$APPS/adobe-$n.desktop" "$ICONS"/hicolor/*/apps/"adobe-$n.png"; done
  rm -f "$DATA/mime/packages/adobe-photoshop.xml"
  refresh
  echo "launchers removed"; exit 0
fi
mkdir -p "$APPS"
PF="$WINEPREFIX/drive_c/Program Files/Adobe"
quote() { printf '"%s"' "$(printf '%s' "$1" | sed -e 's/\\/\\\\\\\\/g' -e 's/["`$]/\\\\&/g' -e 's/%/%%/g')"; }
EXEC="$(quote "$PHOTOSHOPWINE_SRC/bin/photoshopwine")"
CC="$PF/Adobe Creative Cloud/ACC/Creative Cloud.exe"
latest() {  # newest installed release folder of an app, e.g. "Adobe Photoshop 2025"
  local d; d="$(ls -d "$PF/$1 20"[0-9][0-9] 2>/dev/null | sort -V | tail -1)"
  basename "${d:-$1 2026}"
}
icon() {
  local f
  for f in "$2" "$(dirname "$2")"/*Application_Icon*.ico; do
    [ -f "$f" ] && python3 -I "$PHOTOSHOPWINE_SRC/scripts/exe-icon.py" "$f" "$ICONS" "adobe-$1" 2>/dev/null && return
  done
  return 0
}
entry() {
  [ -f "$2" ] || return 0
  icon "$1" "$2"
  local f="$APPS/adobe-$1.desktop" files=" %F" actions="quit-all;" x="$EXEC" app=" $1"
  [ "$1" = cc ] && files=""
  [ "$1" = photoshop ] && app=""
  [ "$1" = photoshop ] && [ -f "$CC" ] && actions="creative-cloud;quit-all;"
  {
    echo "[Desktop Entry]"
    echo "Type=Application"
    echo "Name=$3"
    echo "GenericName=$4"
    echo "Comment=$5"
    echo "Keywords=$6"
    echo "Exec=$x$app$files"
    echo "Icon=adobe-$1"
    echo "Terminal=false"
    echo "StartupNotify=true"
    echo "StartupWMClass=$9"
    echo "SingleMainWindow=true"
    echo "Categories=$7"
    [ -n "$8" ] && echo "MimeType=$8"
    echo "Actions=$actions"
    if [ "$actions" != "quit-all;" ]; then
      echo
      echo "[Desktop Action creative-cloud]"
      echo "Name=Creative Cloud"
      echo "Exec=$x cc"
      echo "Icon=adobe-cc"
    fi
    echo
    echo "[Desktop Action quit-all]"
    echo "Name=Quit All Adobe Apps"
    echo "Exec=$x off"
    echo "Icon=application-exit"
  } > "$f"
  command -v desktop-file-validate >/dev/null && { desktop-file-validate "$f" || true; }
  echo "added $3"
}
entry photoshop "$PF/$(latest "Adobe Photoshop")/Photoshop.exe" \
  "Adobe Photoshop" "Image Editor" "Edit photos and create images" \
  "Photoshop;PSD;PSB;Photo;Image;Retouch;Adobe;" "Graphics;2DGraphics;RasterGraphics;Photography;" \
  "image/vnd.adobe.photoshop;image/png;image/jpeg;image/tiff;image/webp;image/gif;image/bmp;image/x-adobe-dng;" photoshop.exe
entry illustrator "$PF/$(latest "Adobe Illustrator")/Support Files/Contents/Windows/Illustrator.exe" \
  "Adobe Illustrator" "Vector Graphics Editor" "Create vector artwork" \
  "Illustrator;AI;SVG;Vector;Drawing;Adobe;" "Graphics;2DGraphics;VectorGraphics;" \
  "application/illustrator;application/postscript;image/svg+xml;application/pdf;" illustrator.exe
entry premiere "$PF/$(latest "Adobe Premiere Pro")/Adobe Premiere Pro.exe" \
  "Adobe Premiere Pro" "Video Editor" "Edit videos" \
  "Premiere;Video;Film;Editing;Adobe;" "AudioVideo;Video;AudioVideoEditing;" "" "adobe premiere pro.exe"
entry audition "$PF/$(latest "Adobe Audition")/Adobe Audition.exe" \
  "Adobe Audition" "Audio Editor" "Record and edit audio" \
  "Audition;Audio;Sound;Recording;Adobe;" "AudioVideo;Audio;AudioVideoEditing;" \
  "audio/x-wav;audio/wav;audio/mpeg;audio/flac;" "adobe audition.exe"
entry encoder "$PF/$(latest "Adobe Media Encoder")/Adobe Media Encoder.exe" \
  "Adobe Media Encoder" "Media Encoder" "Encode audio and video" \
  "Encoder;Export;Transcode;Video;Adobe;" "AudioVideo;Video;" "" "adobe media encoder.exe"
entry aftereffects "$PF/$(latest "Adobe After Effects")/Support Files/AfterFX.exe" \
  "Adobe After Effects" "Motion Graphics" "Create motion graphics and visual effects" \
  "After Effects;Motion;Animation;VFX;Compositing;Adobe;" "AudioVideo;Video;AudioVideoEditing;" "" afterfx.exe
entry cc "$CC" \
  "Adobe Creative Cloud" "App Manager" "Install and update Adobe apps" \
  "Creative Cloud;Adobe;Apps;Fonts;" "Graphics;" "" "creative cloud.exe"
if [ -f "$APPS/adobe-photoshop.desktop" ]; then
  mkdir -p "$DATA/mime/packages"
  cat > "$DATA/mime/packages/adobe-photoshop.xml" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
  <mime-type type="image/vnd.adobe.photoshop">
    <glob pattern="*.psb"/>
  </mime-type>
</mime-info>
EOF
  if command -v xdg-mime >/dev/null && [ -z "$(xdg-mime query default image/vnd.adobe.photoshop 2>/dev/null)" ]; then
    xdg-mime default adobe-photoshop.desktop image/vnd.adobe.photoshop 2>/dev/null || true
  fi
fi
refresh
echo "launchers installed in $APPS"
