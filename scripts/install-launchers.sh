#!/usr/bin/env bash
# Add the installed Adobe apps to your desktop's application menu (any freedesktop desktop:
# KDE, GNOME, Xfce, ...). Remove them again with --remove.
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/bin/env.sh"
APPS="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
ICONS="${XDG_DATA_HOME:-$HOME/.local/share}/icons"
if [ "${1:-}" = --remove ]; then
  rm -f "$APPS"/adobewine-*.desktop "$ICONS"/hicolor/*/apps/adobewine-*.png
  update-desktop-database "$APPS" 2>/dev/null || true
  gtk-update-icon-cache -f -t "$ICONS/hicolor" >/dev/null 2>&1 || true
  echo "launchers removed"; exit 0
fi
mkdir -p "$APPS"
PF="$WINEPREFIX/drive_c/Program Files/Adobe"
EXEC="\"$(printf '%s' "$ADOBEWINE_SRC/bin/adobewine" | sed -e 's/\\/\\\\\\\\/g' -e 's/["`$]/\\\\&/g' -e 's/%/%%/g')\""
latest() {  # newest installed release folder of an app, e.g. "Adobe Photoshop 2025"
  local d; d="$(ls -d "$PF/$1 20"[0-9][0-9] 2>/dev/null | sort -V | tail -1)"
  basename "${d:-$1 2026}"
}
icon() {
  local f
  for f in "$2" "$(dirname "$2")"/*Application_Icon*.ico; do
    [ -f "$f" ] && python3 -I "$ADOBEWINE_SRC/scripts/exe-icon.py" "$f" "$ICONS" "adobewine-$1" 2>/dev/null && return
  done
  return 0
}
entry() {  # entry <name> <title> <categories> <mime> <wmclass> <exe>
  [ -f "$6" ] || return 0
  icon "$1" "$6"
  cat > "$APPS/adobewine-$1.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=$2 (AdobeWine)
Comment=$2 through the AdobeWine Wine build
Exec=$EXEC $1 %F
Icon=adobewine-$1
Terminal=false
Categories=$3
MimeType=$4
StartupWMClass=$5
EOF
  echo "added $2"
}
entry photoshop "Adobe Photoshop" "Graphics;2DGraphics;RasterGraphics;" "image/vnd.adobe.photoshop;image/png;image/jpeg;image/tiff;" photoshop.exe "$PF/$(latest "Adobe Photoshop")/Photoshop.exe"
entry illustrator "Adobe Illustrator" "Graphics;2DGraphics;VectorGraphics;" "application/illustrator;application/postscript;image/svg+xml;application/pdf;" illustrator.exe "$PF/$(latest "Adobe Illustrator")/Support Files/Contents/Windows/Illustrator.exe"
entry premiere "Adobe Premiere Pro" "AudioVideo;Video;AudioVideoEditing;" "" "adobe premiere pro.exe" "$PF/$(latest "Adobe Premiere Pro")/Adobe Premiere Pro.exe"
entry audition "Adobe Audition" "AudioVideo;Audio;AudioVideoEditing;" "audio/x-wav;audio/wav;audio/mpeg;audio/flac;" "adobe audition.exe" "$PF/$(latest "Adobe Audition")/Adobe Audition.exe"
entry encoder "Adobe Media Encoder" "AudioVideo;Video;" "" "adobe media encoder.exe" "$PF/$(latest "Adobe Media Encoder")/Adobe Media Encoder.exe"
entry aftereffects "Adobe After Effects" "AudioVideo;Video;AudioVideoEditing;" "" "afterfx.exe" "$PF/$(latest "Adobe After Effects")/Support Files/AfterFX.exe"
entry cc "Adobe Creative Cloud" "Graphics;" "" "creative cloud.exe" "$PF/Adobe Creative Cloud/ACC/Creative Cloud.exe"
cat > "$APPS/adobewine-off.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Stop all Adobe apps (AdobeWine)
Exec=$EXEC off
Icon=process-stop
Terminal=false
Categories=Utility;
EOF
update-desktop-database "$APPS" 2>/dev/null || true
gtk-update-icon-cache -f -t "$ICONS/hicolor" >/dev/null 2>&1 || true
echo "launchers installed in $APPS"
