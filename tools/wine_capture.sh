#!/usr/bin/env bash
# Run dd2h.exe under Wine in a headless Xvfb, capture frames. Requires: wine xvfb x11-utils imagemagick (+xdotool).
set -e
GAME=/home/cosmo/Git/wasm-dd2/DestructionDerby2
OUT=${1:-/home/cosmo/Git/wasm-dd2/out/ref}; mkdir -p "$OUT"
export WINEPREFIX=$HOME/.wine-dd2 WINEDEBUG=-all DISPLAY=:99
pgrep -f "Xvfb :99" >/dev/null || (Xvfb :99 -screen 0 640x480x16 & sleep 2)
command -v wineboot >/dev/null && wineboot -u 2>/dev/null || true
cd "$GAME"
echo "launching dd2h.exe under wine..."; wine dd2h.exe >/tmp/wine.log 2>&1 &
WPID=$!; sleep 25
shot(){ xwd -root -display :99 2>/dev/null | convert xwd:- "$OUT/$1.png" 2>/dev/null || import -window root -display :99 "$OUT/$1.png"; echo "shot $1"; }
shot title; sleep 2
command -v xdotool >/dev/null && { xdotool key --display :99 Return; sleep 3; shot menu; }
echo "frames in $OUT (adjust timing/keys per the original's flow)"
