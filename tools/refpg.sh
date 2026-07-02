#!/usr/bin/env bash
# Capture the ORIGINAL's state at Play_Game entry (0x423b50) on the FIRST cold attract race:
# everything the front-end left behind that our CRT-bypass harness must replicate
# (pal_flag class: camera accumulator 0x463ef8, etc.). Dumps lowdata (0x460000-0x470000)
# and 0x77c000-0x785000 at that instant.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="${GAME:-$ROOT/DestructionDerby2}"
OUT="${OUT:-/tmp/refpg}"
export WINEPREFIX="${WINEPREFIX:-$ROOT/.wine-dd2}"
export WINEARCH=win32
mkdir -p "$OUT"
cd "$GAME"
timeout "${RUNSEC:-400}" xvfb-run -a -s "-screen 0 640x480x16" wine dd2h.exe >/tmp/refpg_wine.log 2>&1 &
for j in $(seq 1 60); do
  sleep 1
  pid=$(pgrep -x dd2h.exe | head -1)
  [ -n "$pid" ] && python3 -c "
import sys
try:
    f=open('/proc/$pid/mem','rb'); f.seek(0x462ff0); f.read(4); sys.exit(0)
except Exception: sys.exit(1)" && break
done
echo "attached to pid $pid"
gdb --nx -batch \
  -ex "set auto-solib-add off" -ex "set pagination off" \
  -ex "handle SIGUSR1 nostop noprint pass" -ex "handle SIGUSR2 nostop noprint pass" \
  -ex "handle SIGSEGV nostop noprint pass" -ex "handle SIG33 nostop noprint pass" \
  -ex "attach $pid" -ex "hbreak *0x423b50" -ex continue \
  -ex "dump binary memory $OUT/lowdata_pg.bin 0x460000 0x470000" \
  -ex "dump binary memory $OUT/mid_pg.bin 0x77c000 0x785000" \
  -ex "print/d *(int*)0x936ff4" \
  -ex detach >/tmp/refpg_gdb.log 2>&1
grep -a '^\$1' /tmp/refpg_gdb.log
ls -la "$OUT"
pkill -x dd2h.exe 2>/dev/null; pkill -x wine 2>/dev/null; sleep 1
