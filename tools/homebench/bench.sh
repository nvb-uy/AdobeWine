#!/bin/bash
# usage: bench.sh [wheel|pwheel|move|pmove] [events] [interval ms] [runs]
# Photoshop must be running with its Home screen open.
D=$(cd "$(dirname "$0")" && pwd)
source "$D/../../bin/env.sh"
mode=${1:-pwheel}; n=${2:-300}; iv=${3:-16}; runs=${4:-2}
[ "$D/drive.exe" -nt "$D/drive.c" ] || x86_64-w64-mingw32-gcc -O2 -o "$D/drive.exe" "$D/drive.c" -lwinmm || exit 1
for i in $(seq 90); do
    XW=$(xwininfo -root -tree | awk '/"Adobe Photoshop 2026"/{print $1; exit}')
    [ -n "$XW" ] && break
    sleep 2
done
[ -n "$XW" ] || { echo "no Photoshop window"; exit 1; }
sleep "${SETTLE:-0}"
PSPID=$(pgrep -f 'Photoshop 2026\\Photoshop.exe' | head -1)
read X Y <<< "$(xwininfo -id "$XW" | awk '/Absolute upper-left X/{x=$4}/Absolute upper-left Y/{y=$4}END{print x, y}')"
if [ "${mode#p}" = wheel ]; then
    args="$((X+900)) $((Y+600)) $((X+900)) $((Y+600))"
else
    args="$((X+320)) $((Y+690)) $((X+1690)) $((Y+690))"
fi
dur=${DUR:-$(( n * (iv + 2) / 1000 + 4 ))}
for r in $(seq "$runs"); do
    out=$(mktemp -d)
    : > "$out/posts"
    python3 -I "$D/damage.py" "$XW" "$dur" "$out/posts" > "$out/damage" &
    python3 -I "$D/tcpu.py" "$PSPID" $((dur - 1)) > "$out/cpu" &
    sleep 0.3
    wine "$D/drive.exe" "$mode" "$n" "$iv" $args 2>/dev/null | python3 -I "$D/damage.py" stamp "$out/posts" | grep -v '^target'
    wait
    cat "$out/damage"
    head -4 "$out/cpu"
    rm -r "$out"
    sleep 3
done
