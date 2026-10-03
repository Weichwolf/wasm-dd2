#!/usr/bin/env bash
# Reference prim-type histogram: freeze the original at Draw_All (hbreak 0x420c9c) on level $LVL
# and walk the OT exactly like DrawOTag does (cdb @0x754264/0x7542f2, OT ptr @cdb+0x8a, otsize
# @0x754260), counting the type byte at prim+7. Compare with our build's [HIST] instrumentation.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="${GAME:-$ROOT/DestructionDerby2}"
LVL="${LVL:-2}"
export WINEPREFIX="${WINEPREFIX:-/tmp/wasm-dd2/legacy-wine}"
export WINEARCH=win32
mkdir -p "$WINEPREFIX"
cd "$GAME"
timeout "${RUNSEC:-1400}" xvfb-run -a -s "-screen 0 640x480x16" wine dd2h.exe >/tmp/refhist_wine.log 2>&1 &
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
  if [ "$LV" = "$LVL" ] && [ "$FR" -ge 1 ] && [ "$FR" -le 30 ] 2>/dev/null; then
    cat > /tmp/refhist.gdb <<GEOF
set auto-solib-add off
set pagination off
attach $pid
hbreak *0x420c9c
continue
python
import gdb
inf = gdb.selected_inferior()
def r32(a):
    return int.from_bytes(inf.read_memory(a, 4).tobytes(), 'little')
cf = r32(0x462ff0)
out=[]
gdb.execute("delete")
gdb.execute("hbreak *0x410010")
for i in range(24):
    gdb.execute("continue")
    vals=(r32(0x480044),r32(0x480014),r32(0x48004c),r32(0x480048),r32(0x48002c),r32(0x480028),r32(0x480024),r32(0x480020),r32(0x480030),r32(0x480010))
    out.append(vals)
print(f"[REFDTH] cf{r32(0x462ff0)}")
for v in out:
    print("[REFDTH] tp=%04x cl=%02x u=%08x v=%08x x1=%08x x2=%08x y1=%d y2=%d sh=%d clip=%d"%v)
gdb.execute("detach"); gdb.execute("quit")
end
GEOF
    gdb --nx -batch -x /tmp/refhist.gdb >/tmp/refhist_gdb.log 2>&1; grep -a REFDTH /tmp/refhist_gdb.log | head -30 || tail -5 /tmp/refhist_gdb.log
    break
  fi
done
pkill -x dd2h.exe 2>/dev/null; pkill -x wine 2>/dev/null; sleep 1
