#!/usr/bin/env bash
# Differential lockstep harness (user's idea): run reference dd2h.exe (Wine) AND our native build,
# both running the deterministic attract demo (same rand LCG 0x41c64e6d, seed 1 -> identical level
# sequence). Dump the MPE heap [0x7debf0,0x8febf0) from BOTH at an ALIGNED checkpoint and diff ->
# the first differing byte localizes the function whose transpile patch diverges.
# Needs: gdb, ptrace_scope=0, Wine, Xvfb.  Usage: tools/lockstep.sh [attach_delay_s]
#
# Three alignment modes (MODE=frame|level|levelframe, default levelframe):
#  - levelframe (RECOMMENDED): gate on _current_level==9 first (skips the title/front-end, whose
#    length varies and whose cycling means raw frame counts land in arbitrary/wrong demo cycles),
#    THEN wait for current_frame to reach F (default 100) WITHIN that same level-9 cycle (confirmed
#    non-hanging: current_frame increments reliably ~15-16/sec under gdb and a full demo race runs
#    ~700 frames before the reference cycles to the next demo level -- so F=100 lands safely inside
#    real, populated gameplay, not the still-mostly-empty state right at Order_Cars/level-select).
#  - level: sync on _current_level (ref @0x936ff4) transitioning to 9 alone (no frame wait after).
#    Measured heap match 58%->74%, game-data match 9%->54% vs raw frame sync, but this checkpoint is
#    EARLY (right at level-select, before Init_Scene/the level file body loads) -- most of the
#    game-data window is still genuinely unwritten at this point in OUR build (see CLAUDE.md "RETRACTED"
#    section), so match numbers from this mode must have their nonzero-fraction checked before trusting.
#  - frame: sync on current_frame (@0x462ff0) reaching F with NO level gate. WEAK: the reference's
#    title screen inflates current_frame before the demo even starts, AND the reference cycles through
#    multiple demo levels so a raw frame threshold can land in the wrong level's cycle entirely (kept
#    for comparison / historical reference only).
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="$ROOT/DestructionDerby2"
NAT="${NAT:-/tmp/dd2_native_na}"
DELAY="${1:-12}"
MODE="${MODE:-levelframe}"
export WINEPREFIX="${WINEPREFIX:-/tmp/dd2_wineprefix}"
export WINEDEBUG=-all
[ -d "$WINEPREFIX" ] || wineboot --init >/dev/null 2>&1

F="${F:-100}"   # (frame mode only) alignment frame both builds reach in the deterministic level-9 demo
cd "$GAME"
rm -f /tmp/ref_heap.mem /tmp/our_heap.mem

if [ "$MODE" = levelframe ]; then
cat > /tmp/refwatch.py <<PY
import gdb
gdb.execute("set pagination off")
gdb.execute("watch *(int*)0x936ff4")
for _ in range(300):
    gdb.execute("continue")
    try: v=int(gdb.parse_and_eval("*(int*)0x936ff4"))
    except: break
    if v==9: break
gdb.execute("delete")
gdb.execute("watch *(int*)0x462ff0")
for _ in range(2000):
    gdb.execute("continue")
    try: v=int(gdb.parse_and_eval("*(int*)0x462ff0"))
    except: break
    if v>=$F:
        gdb.execute("dump binary memory /tmp/ref_heap.mem 0x7debf0 0x8febf0")
        gdb.execute("dump binary memory /tmp/ref_fb.mem 0x700450 0x712450")
        gdb.execute("dump binary memory /tmp/ref_gdb.mem 0x75ebf0 0x7debf0")
        gdb.write("REFFRAME=level9+%d\n"%v); break
gdb.execute("detach"); gdb.execute("quit")
PY
elif [ "$MODE" = level ]; then
cat > /tmp/refwatch.py <<'PY'
import gdb
gdb.execute("set pagination off")
gdb.execute("watch *(int*)0x936ff4")
for _ in range(300):
    gdb.execute("continue")
    try: v=int(gdb.parse_and_eval("*(int*)0x936ff4"))
    except: break
    if v==9:
        gdb.execute("dump binary memory /tmp/ref_heap.mem 0x7debf0 0x8febf0")
        gdb.execute("dump binary memory /tmp/ref_fb.mem 0x700450 0x712450")
        gdb.execute("dump binary memory /tmp/ref_gdb.mem 0x75ebf0 0x7debf0")
        gdb.write("REFFRAME=level9\n"); break
gdb.execute("detach"); gdb.execute("quit")
PY
else
cat > /tmp/refwatch.py <<PY
import gdb
gdb.execute("set pagination off")
gdb.execute("watch *(int*)0x462ff0")
for _ in range(20000):
    gdb.execute("continue")
    try: v=int(gdb.parse_and_eval("*(int*)0x462ff0"))
    except: v=-1
    if v>=$F:
        gdb.execute("dump binary memory /tmp/ref_heap.mem 0x7debf0 0x8febf0")
        gdb.execute("dump binary memory /tmp/ref_fb.mem 0x700450 0x712450")
        gdb.execute("dump binary memory /tmp/ref_gdb.mem 0x75ebf0 0x7debf0")
        gdb.write("REFFRAME=%d\n"%v); break
gdb.execute("detach"); gdb.execute("quit")
PY
fi
pkill -x dd2h.exe 2>/dev/null; pkill -x Xvfb 2>/dev/null; sleep 1
Xvfb :66 -screen 0 640x480x16 >/dev/null 2>&1 & XPID=$!
sleep 2
DISPLAY=:66 wine dd2h.exe >/tmp/wr.log 2>&1 &
PID=""; for i in $(seq 1 300); do PID=$(pgrep -x dd2h.exe | head -1); [ -n "$PID" ] && break; done
echo "[lockstep] reference PE pid=$PID, MODE=$MODE"
timeout 280 gdb -batch -p "$PID" -x /tmp/refwatch.py 2>&1 | grep -E "REFFRAME" | head -1
kill "$XPID" 2>/dev/null; pkill -x dd2h.exe 2>/dev/null

# our build: run to the SAME aligned checkpoint, dump the same heap region
if [ "$MODE" = levelframe ]; then
cat > /tmp/ours.gdb <<EOF
set pagination off
break ids_flip if *(int*)0x462ff0 >= $F
run
dump binary memory /tmp/our_heap.mem 0x7debf0 0x8febf0
dump binary memory /tmp/our_fb.mem 0x700450 0x712450
dump binary memory /tmp/our_gdb.mem 0x75ebf0 0x7debf0
printf "our current_frame=%d _current_level=%d\n", *(int*)0x462ff0, _current_level
kill
quit
EOF
DD2_LEVEL=9 timeout 120 gdb -batch -x /tmp/ours.gdb --args "$NAT" 2>&1 | grep -iE "our current_frame" | head -1
elif [ "$MODE" = level ]; then
cat > /tmp/ours.gdb <<EOF
set pagination off
break Order_Cars
run
dump binary memory /tmp/our_heap.mem 0x7debf0 0x8febf0
dump binary memory /tmp/our_fb.mem 0x700450 0x712450
dump binary memory /tmp/our_gdb.mem 0x75ebf0 0x7debf0
printf "our _current_level=%d\n", _current_level
kill
quit
EOF
DD2_LEVEL=9 timeout 120 gdb -batch -x /tmp/ours.gdb --args "$NAT" 2>&1 | grep -iE "our _current_level" | head -1
else
cat > /tmp/ours.gdb <<EOF
set pagination off
break ids_flip if *(int*)0x462ff0 >= $F
run
dump binary memory /tmp/our_heap.mem 0x7debf0 0x8febf0
dump binary memory /tmp/our_fb.mem 0x700450 0x712450
dump binary memory /tmp/our_gdb.mem 0x75ebf0 0x7debf0
printf "our current_frame=%d\n", *(int*)0x462ff0
kill
quit
EOF
timeout 120 gdb -batch -x /tmp/ours.gdb --args "$NAT" 2>&1 | grep -iE "our current_frame" | head -1
fi

python3 - <<PY
a=open('/tmp/ref_heap.mem','rb').read(); b=open('/tmp/our_heap.mem','rb').read()
n=min(len(a),len(b)); same=sum(1 for i in range(n) if a[i]==b[i])
first=next((i for i in range(n) if a[i]!=b[i]), -1)
nza=sum(1 for x in a if x!=0); nzb=sum(1 for x in b if x!=0)
print(f"[lockstep] heap nonzero: ref={nza}/{n}({100*nza/n:.1f}%) our={nzb}/{n}({100*nzb/n:.1f}%)  "
      f"(LESSON: large zero regions trivially 'match' -- distrust high% if nonzero%% is low)")
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
# game-data buffer region (level data @0x75ebf0..heap) — STATIC, alignment-insensitive
try:
    ga=open('/tmp/ref_gdb.mem','rb').read(); gb=open('/tmp/our_gdb.mem','rb').read()
    m=min(len(ga),len(gb)); gs=sum(1 for i in range(m) if ga[i]==gb[i]); base=0x75ebf0
    nzga=sum(1 for x in ga if x!=0); nzgb=sum(1 for x in gb if x!=0)
    print(f"[lockstep] game-data nonzero: ref={nzga}/{m}({100*nzga/m:.1f}%) our={nzgb}/{m}({100*nzgb/m:.1f}%)")
    print(f"[lockstep] game-data(0x75ebf0..0x7debf0) match: {gs}/{m} = {100*gs/m:.2f}%")
    print("  per-64KB region (diverging only):")
    for off in range(0,m,0x10000):
        sa=ga[off:off+0x10000]; sb=gb[off:off+0x10000]; mm=min(len(sa),len(sb))
        if mm==0: continue
        sm=sum(1 for i in range(mm) if sa[i]==sb[i])
        tag = "MATCH" if sm/mm>0.99 else ("%.0f%%"%(100*sm/mm))
        print(f"    0x{base+off:x}: {tag}")
except Exception as e: print("[lockstep] game-data compare skipped:",e)
PY
