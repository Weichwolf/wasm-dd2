#!/usr/bin/env bash
# Reference OUTPUT-STREAM capture: per-frame framebuffer/palette/audio-staging from the ORIGINAL
# dd2h.exe under wine. Rationale (the bit-identity acceptance test): if the presented framebuffer
# and the audio stream match byte-for-byte per frame, everything upstream is correct by definition.
#
# Method: wait until the attract demo enters $LVL, then gdb-attach (freezes all threads) and set a
# HARDWARE breakpoint on Draw_All @0x420c9c (sw breakpoints are unreliable under wine; hbreak uses
# debug registers). Every hit dumps, keyed by the engine frame counter @0x462ff0:
#   $OUT/cfNNNNN.fb   640x480 8bpp framebuffer @0x700450   (compare: our DD2_CFDUMP cfNNNNN.bin)
#   $OUT/cfNNNNN.pal  palette @0x700050
#   $OUT/audio.bin    PCM staging [0x803160,0x816ff0) once at the end (Stage-3 audio reference)
# Draw_All runs twice per engine frame; the dump script skips already-captured frames.
# Usage: LVL=2 NFRAMES=64 OUT=/tmp/refstream RUNSEC=1400 tools/refstream.sh
# NEVER `pkill -f dd2h.exe` (kills the agent shell); use `pkill -x dd2h.exe`.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="${GAME:-$ROOT/DestructionDerby2}"
OUT="${OUT:-/tmp/refstream}"
LVL="${LVL:-2}"
NFRAMES="${NFRAMES:-64}"
export WINEPREFIX="${WINEPREFIX:-$ROOT/.wine-dd2}"
export WINEARCH=win32
mkdir -p "$OUT"
[ -f "$WINEPREFIX/drive_c/windows/system32/kernel32.dll" ] || { echo "init win32 prefix..."; wineboot --init >/dev/null 2>&1; }
cd "$GAME"
timeout "${RUNSEC:-1400}" xvfb-run -a -s "-screen 0 640x480x16" wine dd2h.exe >/tmp/refstream_wine.log 2>&1 &
echo "waiting for _current_level==$LVL ..."
if [ "${FIRSTBOOT:-0}" = "1" ]; then
  # attach as soon as the process exists and the image is loaded; the Draw_All hook
  # skips frames until the FIRST race level begins (menu frames have level==0).
  for j in $(seq 1 60); do
    sleep 1
    pid=$(pgrep -x dd2h.exe | head -1)
    [ -n "$pid" ] && python3 -c "
import struct,sys
try:
    f=open('/proc/$pid/mem','rb'); f.seek(0x462ff0); f.read(4); sys.exit(0)
except Exception: sys.exit(1)" && break
  done
  echo "early attach to pid $pid"
  LV=boot; FR=0
  set -- dummy
fi
for i in $(seq 1 $(( ${RUNSEC:-1400} / 4 ))); do
  if [ "${FIRSTBOOT:-0}" != "1" ]; then
  sleep 4
  pid=$(pgrep -x dd2h.exe | head -1); [ -z "$pid" ] && { echo "reference exited early"; exit 1; }
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
  fi
  if { [ "$LVL" = "0" ] && [ "$LV" -ge 1 ] 2>/dev/null || [ "$LV" = "$LVL" ]; } && [ "$FR" -ge 0 ] && [ "$FR" -le 5 ] 2>/dev/null || [ "${FIRSTBOOT:-0}" = "1" ]; then
    echo "entering level $LVL (frame $FR) -- attaching stream capture"
    cat > /tmp/refstream.gdb <<GEOF
set auto-solib-add off
set pagination off
handle SIGUSR1 nostop noprint pass
handle SIGUSR2 nostop noprint pass
handle SIGSEGV nostop noprint pass
handle SIGSYS nostop noprint pass
handle SIG33 nostop noprint pass
handle SIG34 nostop noprint pass
handle SIG35 nostop noprint pass
attach $pid
hbreak *0x4431e8
python
import gdb
def arm(event):
    # first hit = race init (runs once, no slowdown in menus); switch to the Draw_All stream bp
    gdb.events.stop.disconnect(arm)
    gdb.execute("delete")
    gdb.execute("hbreak *0x420c9c")
    gdb.execute("continue")
gdb.events.stop.connect(arm)
end
continue
python
import gdb, os
out = "$OUT"
seen = set()
count = 0
target = $NFRAMES
def on_stop(event):
    global count
    if not isinstance(event, gdb.BreakpointEvent):
        try: gdb.execute("continue")
        except Exception: pass
        return
    lv = int(gdb.parse_and_eval("*(int*)0x936ff4"))
    if lv < 1:
        gdb.execute("continue"); return
    cf = int(gdb.parse_and_eval("*(int*)0x462ff0"))
    if cf not in seen:
        seen.add(cf)
        gdb.execute(f"dump binary memory {out}/cf{cf:05d}.fb 0x700450 0x74b450")
        gdb.execute(f"dump binary memory {out}/cf{cf:05d}.pal 0x700050 0x700450")
        if count < 12:
            gdb.execute(f"dump binary memory {out}/cf{cf:05d}.ts 0x774900 0x796ff0")
        count += 1
        if count >= target:
            gdb.execute(f"dump binary memory {out}/audio.bin 0x803160 0x816ff0")
            gdb.execute("detach")
            gdb.execute("quit")
    gdb.execute("continue")
gdb.events.stop.connect(on_stop)
end
continue
GEOF
    gdb --nx -batch -x /tmp/refstream.gdb >/tmp/refstream_gdb.log 2>&1
    echo "captured $(ls "$OUT" | grep -c '\.fb$') frames -> $OUT"
    break
  fi
done
pkill -x dd2h.exe 2>/dev/null; pkill -x wine 2>/dev/null; sleep 1
