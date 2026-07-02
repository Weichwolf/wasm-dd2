#!/usr/bin/env bash
# Dump the ORIGINAL's texturespace (pointer @0x74c4cc) and clutspace (@0x74c4d4) while frozen
# in a race, to diff the texture PAGE LAYOUT against ours (ground prims sample tpage 0xa; our
# page 0xa holds clouds+decals instead of the floor -> upload placement diverges).
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="${GAME:-$ROOT/DestructionDerby2}"
OUT="${OUT:-/tmp/reftex}"
LVL="${LVL:-9}"
export WINEPREFIX="${WINEPREFIX:-$ROOT/.wine-dd2}"
export WINEARCH=win32
mkdir -p "$OUT"
cd "$GAME"
timeout "${RUNSEC:-1400}" xvfb-run -a -s "-screen 0 640x480x16" wine dd2h.exe >/tmp/reftex_wine.log 2>&1 &
echo "waiting for level $LVL ..."
for i in $(seq 1 $(( ${RUNSEC:-1400} / 4 ))); do
  sleep 4
  pid=$(pgrep -x dd2h.exe | head -1); [ -z "$pid" ] && { echo "exited"; exit 1; }
  read -r LV FR < <(python3 - "$pid" <<'PY'
import sys,struct
pid=sys.argv[1]
def rd(a):
    try:
        with open(f"/proc/{pid}/mem","rb") as f: f.seek(a); return struct.unpack('<i',f.read(4))[0]
    except: return -1
print(rd(0x936ff4), rd(0x462ff0))
PY
)
  echo "  t~$((i*4))s: level=$LV frame=$FR"
  if [ "$LV" = "$LVL" ] && [ "$FR" -ge 1 ] 2>/dev/null; then
    TEX=$(python3 - "$pid" <<'PY'
import sys,struct
pid=sys.argv[1]
with open(f"/proc/{pid}/mem","rb") as f:
    f.seek(0x74c4cc); tex=struct.unpack('<I',f.read(4))[0]
    f.seek(0x74c4d4); clut=struct.unpack('<I',f.read(4))[0]
print(f"{tex:#x} {clut:#x}")
PY
)
    set -- $TEX
    echo "texspace=$1 clutspace=$2" | tee "$OUT/ptrs.txt"
    gdb --nx -batch -ex "set auto-solib-add off" -ex "attach $pid" \
      -ex "dump binary memory $OUT/texspace.bin $1 $(($1 + 0x200000))" \
      -ex "dump binary memory $OUT/clutspace.bin $2 $(($2 + 0x10000))" \
      -ex detach >/tmp/reftex_gdb.log 2>&1
    ls -la "$OUT"; break
  fi
done
pkill -x dd2h.exe 2>/dev/null; pkill -x wine 2>/dev/null; sleep 1
