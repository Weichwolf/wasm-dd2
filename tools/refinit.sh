#!/usr/bin/env bash
# Capture the ORIGINAL's stack state at FUN_004431e8 entry (car-grid init) for level $LVL:
# the function builds the car fd in uninitialized stack slots ([ebp-0x70..-0x45]); the original's
# "garbage" there is deterministic leftovers that influence the init Track_Follow settle (camera
# start strip). Dump the frame window + registers so the exact original values can be replicated.
# Also dumps the camera fd right when Init_Track_Objects (0x430de8) runs.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="${GAME:-$ROOT/DestructionDerby2}"
OUT="${OUT:-/tmp/refinit}"
LVL="${LVL:-2}"
export WINEPREFIX="${WINEPREFIX:-$ROOT/.wine-dd2}"
export WINEARCH=win32
mkdir -p "$OUT"
cd "$GAME"
timeout "${RUNSEC:-1400}" xvfb-run -a -s "-screen 0 640x480x16" wine dd2h.exe >/tmp/refinit_wine.log 2>&1 &
echo "waiting for a demo to run (attach during ANY level, then wait for level $LVL init)..."
for i in $(seq 1 60); do
  sleep 4
  pid=$(pgrep -x dd2h.exe | head -1); [ -z "$pid" ] && { echo "exited early"; exit 1; }
  FR=$(python3 -c "
import struct
f=open('/proc/$pid/mem','rb'); f.seek(0x462ff0); print(struct.unpack('<i',f.read(4))[0])" 2>/dev/null || echo -1)
  [ "$FR" -ge 10 ] 2>/dev/null && break
done
echo "attaching (current frame $FR); waiting for FUN_004431e8 with level==$LVL"
cat > /tmp/refinit.gdb <<GEOF
set auto-solib-add off
set pagination off
attach $pid
hbreak *0x4431e8
python
import gdb
out = "$OUT"
lvl = $LVL
phase = [0]
def on_stop(event):
    if not isinstance(event, gdb.BreakpointEvent):
        return
    if phase[0] == 0:
        cur = int(gdb.parse_and_eval("*(int*)0x936ff4"))
        if cur != lvl:
            gdb.execute("continue"); return
        sp = int(gdb.parse_and_eval("\$esp"))
        gdb.execute(f"dump binary memory {out}/initframe.bin {sp-0x90} {sp+0x10}")
        with open(f"{out}/initregs.txt","w") as f:
            for r in ("eax","ebx","ecx","edx","esi","edi","esp"):
                f.write(f"{r}={int(gdb.parse_and_eval('\$'+r))&0xffffffff:#x}\n")
        gdb.execute("delete")
        gdb.execute("hbreak *0x430de8")
        phase[0] = 1
        gdb.execute("continue"); return
    camfd = int(gdb.parse_and_eval("*(int*)0x77cf70"))
    strip = int(gdb.parse_and_eval(f"*(int*)({camfd}+0x14)"))
    with open(f"{out}/inittrack.txt","w") as f:
        f.write(f"camera_fd={camfd:#x} strip={strip:#x}\n")
    gdb.execute(f"dump binary memory {out}/carfd.bin 0x792690 0x792a00")
    gdb.execute("detach"); gdb.execute("quit")
gdb.events.stop.connect(on_stop)
end
continue
GEOF
gdb --nx -batch -x /tmp/refinit.gdb >/tmp/refinit_gdb.log 2>&1
ls -la "$OUT"
pkill -x dd2h.exe 2>/dev/null; pkill -x wine 2>/dev/null; sleep 1
