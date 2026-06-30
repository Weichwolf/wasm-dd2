#!/usr/bin/env bash
# Differential lockstep harness (user's idea): run reference dd2h.exe (Wine) AND our native build,
# both running the deterministic attract demo (same rand LCG 0x41c64e6d, seed 1 -> identical level
# sequence). Dump the MPE heap [0x7debf0,0x8febf0) from BOTH at the SAME engine frame (current_frame
# @0x462ff0) and diff -> the first differing byte localizes the function whose transpile patch diverges.
# Needs: gdb, ptrace_scope=0, Wine, Xvfb.  Usage: tools/lockstep.sh [attach_delay_s]
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="$ROOT/DestructionDerby2"
NAT="${NAT:-/tmp/dd2_native_na}"
DELAY="${1:-12}"
export WINEPREFIX="${WINEPREFIX:-/tmp/dd2_wineprefix}"
export WINEDEBUG=-all
[ -d "$WINEPREFIX" ] || wineboot --init >/dev/null 2>&1

F="${F:-100}"   # alignment frame both builds reach in the deterministic level-9 demo
cd "$GAME"
rm -f /tmp/ref_heap.mem /tmp/our_heap.mem
# gdb python: watch current_frame, continue until it reaches F, then dump the heap
cat > /tmp/refwatch.py <<PY
import gdb,struct
gdb.execute("set pagination off")
gdb.execute("watch *(int*)0x462ff0")
for _ in range(20000):
    gdb.execute("continue")
    try: v=int(gdb.parse_and_eval("*(int*)0x462ff0"))
    except: v=-1
    if v>=$F:
        gdb.execute("dump binary memory /tmp/ref_heap.mem 0x7debf0 0x8febf0")
        gdb.execute("dump binary memory /tmp/ref_fb.mem 0x700450 0x712450")
        gdb.write("REFFRAME=%d\n"%v); break
gdb.execute("detach"); gdb.execute("quit")
PY
Xvfb :66 -screen 0 640x480x16 >/dev/null 2>&1 & XPID=$!
sleep 2
DISPLAY=:66 wine dd2h.exe >/tmp/wr.log 2>&1 &
sleep 5   # attach during the title (current_frame still 0), then watch up to F
PID=""; for p in $(pgrep -f dd2h 2>/dev/null); do [ "$(cat /proc/$p/comm 2>/dev/null)" = "dd2h.exe" ] && PID=$p; done
echo "[lockstep] reference PE pid=$PID, watching current_frame to F=$F"
timeout 120 gdb -batch -p "$PID" -x /tmp/refwatch.py 2>&1 | grep -E "REFFRAME" | head -1
kill "$XPID" 2>/dev/null; pkill -f dd2h.exe 2>/dev/null

# our build: run the SAME DemoMode, break at the same frame, dump the same heap region
cat > /tmp/ours.gdb <<EOF
set pagination off
break ids_flip if *(int*)0x462ff0 >= $F
run
dump binary memory /tmp/our_heap.mem 0x7debf0 0x8febf0
dump binary memory /tmp/our_fb.mem 0x700450 0x712450
printf "our current_frame=%d\n", *(int*)0x462ff0
kill
quit
EOF
timeout 120 gdb -batch -x /tmp/ours.gdb --args "$NAT" 2>&1 | grep -iE "our current_frame" | head -1

python3 - <<PY
a=open('/tmp/ref_heap.mem','rb').read(); b=open('/tmp/our_heap.mem','rb').read()
n=min(len(a),len(b)); same=sum(1 for i in range(n) if a[i]==b[i])
first=next((i for i in range(n) if a[i]!=b[i]), -1)
print(f"[lockstep] heap match: {same}/{n} = {100*same/n:.2f}%")
if first>=0:
    va=0x7debf0+first
    print(f"[lockstep] FIRST DIVERGENCE at heap VA 0x{va:x}: ref={a[first]:#04x} ours={b[first]:#04x}")
    print(f"           ref[{first}:{first+16}]= "+' '.join('%02x'%x for x in a[first:first+16]))
    print(f"           our[{first}:{first+16}]= "+' '.join('%02x'%x for x in b[first:first+16]))
else:
    print("[lockstep] heaps IDENTICAL")
try:
    fa=open('/tmp/ref_fb.mem','rb').read(); fb=open('/tmp/our_fb.mem','rb').read()
    m=min(len(fa),len(fb)); fs=sum(1 for i in range(m) if fa[i]==fb[i])
    print(f"[lockstep] framebuffer(0x700450) match: {fs}/{m} = {100*fs/m:.2f}%  "
          f"(ref distinct={len(set(fa[:m]))} our distinct={len(set(fb[:m]))})")
except Exception as e: print("[lockstep] fb compare skipped:",e)
PY
