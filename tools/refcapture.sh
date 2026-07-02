#!/usr/bin/env bash
# Stage-2 reference capture (WORKS in this env — the previously-flaky wine path, now reliable).
# Runs the reference dd2h.exe under Wine+Xvfb, polls its state via /proc/PID/mem (ptrace_scope=0,
# NO gdb breakpoints — those are unreliable per CLAUDE.md), and at the level-9 checkpoint dumps the
# MPE heap [0x816ff0,0x936ff0) and the game-data/level_data_buffer region [0x796ff0,0x816ff0) to
# $OUT (default /tmp/ref). Compare against our build's DD2_STATEDUMP dump at the same frame.
#
# THE TWO THINGS THAT MADE WINE WORK HERE (both required):
#   1. WINEARCH=win32 prefix in a DIRECTORY YOU OWN (NOT /tmp itself — wine refuses; use the scratchpad
#      or $HOME). dd2h.exe is 32-bit; a default (win64) prefix fails with "could not load kernel32.dll".
#   2. Xvfb with an EXPLICIT screen: `xvfb-run -s "-screen 0 640x480x16"`. Without it the game's
#      DirectDraw SetDisplayMode gets NtUserChangeDisplaySettings -2 and the game never reaches the
#      demo (state stays 0 forever). With it, the reference reaches _current_level==9 in ~25s.
# Data addresses (fixed load base 0x400000, confirmed): _current_level@0x936ff4, current_frame@0x462ff0.
# NEVER `pkill -f dd2h.exe` (kills the agent shell); use `pkill -x dd2h.exe`.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="${GAME:-/home/cosmo/Git/wasm-dd2/DestructionDerby2}"
OUT="${OUT:-/tmp/ref}"
LVL="${LVL:-9}"; FRMIN="${FRMIN:-150}"; FRMAX="${FRMAX:-600}"
export WINEPREFIX="${WINEPREFIX:-$ROOT/.wine-dd2}"   # repo-local prefix (gitignored)
export WINEARCH=win32 WINEPREFIX
mkdir -p "$OUT"
[ -f "$WINEPREFIX/drive_c/windows/system32/kernel32.dll" ] || { echo "init win32 prefix..."; wineboot --init >/dev/null 2>&1; }
cd "$GAME"
timeout "${RUNSEC:-150}" xvfb-run -a -s "-screen 0 640x480x16" wine dd2h.exe >/tmp/refcap_wine.log 2>&1 &
echo "waiting for _current_level==$LVL & frame in [$FRMIN,$FRMAX]..."
for i in $(seq 1 $(( ${RUNSEC:-150} / 4 ))); do   # poll for the whole wine run, not a fixed 160s
  sleep 4
  pid=$(pgrep -x dd2h.exe | head -1); [ -z "$pid" ] && { echo "reference exited early"; break; }
  read -r LV FR < <(python3 - "$pid" <<'PY'
import sys,struct
pid=sys.argv[1]
def rd(a,n=4):
    try:
        with open(f"/proc/{pid}/mem","rb") as f: f.seek(a); return f.read(n)
    except: return None
cl=rd(0x936ff4); cf=rd(0x462ff0)
print(struct.unpack('<i',cl)[0] if cl else -1, struct.unpack('<i',cf)[0] if cf else -1)
PY
)
  echo "  t~$((i*4))s: level=$LV frame=$FR"
  if [ "$LV" = "$LVL" ] && [ "$FR" -ge "$FRMIN" ] && [ "$FR" -le "$FRMAX" ] 2>/dev/null; then
    # tick-precise: busy-poll current_frame until it INCREMENTS, then FREEZE via gdb attach
    # (ptrace stops every thread; SIGSTOP is unreliable under wine -- wineserver resumes it).
    # All regions are then dumped from one consistent instant; the exact frozen frame is recorded.
    python3 - "$pid" <<'PY'
import sys,struct,time
pid=sys.argv[1]
f=open(f"/proc/{pid}/mem","rb")
def cf():
    f.seek(0x462ff0); return struct.unpack('<i',f.read(4))[0]
base=cf(); t0=time.time()
while cf()==base and time.time()-t0<15: pass
PY
    gdb --nx -batch -ex "set auto-solib-add off" -ex "attach $pid" \
      -ex "dump binary memory $OUT/heap.bin 0x816ff0 0x936ff0" \
      -ex "dump binary memory $OUT/gamedata.bin 0x796ff0 0x816ff0" \
      -ex "dump binary memory $OUT/trackstate.bin 0x774900 0x796ff0" \
      -ex "dump binary memory $OUT/framebuf.bin 0x700450 0x74b450" \
      -ex "dump binary memory $OUT/palette.bin 0x700050 0x700450" \
      -ex "dump binary memory $OUT/lowdata.bin 0x460000 0x470000" \
      -ex "print/d *(int*)0x462ff0" -ex detach >/tmp/refcap_gdb.log 2>&1
    FREXACT=$(grep -oE '\$1 = [0-9]+' /tmp/refcap_gdb.log | grep -oE '[0-9]+$')
    echo "level=$LV frame=$FREXACT" > "$OUT/checkpoint.txt"; echo "captured (gdb-frozen at frame $FREXACT) -> $OUT"; break
  fi
done
pkill -x dd2h.exe 2>/dev/null; pkill -x wine 2>/dev/null; sleep 1
