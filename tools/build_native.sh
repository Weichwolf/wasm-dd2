#!/usr/bin/env bash
# Native 32-bit debug build of the DD2 engine (same C as the wasm build; native_main.c replaces
# the wasm dd2_runtime.c entry). ASan on -> the Track_Follow OOB becomes a real backtrace.
# Needs: gcc-multilib. Run the result from DestructionDerby2/ (it fopens dd2_image.bin + Dirinfo).
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-/tmp/dd2_native}"
ASAN="${ASAN:--fsanitize=address}"

python3 "$ROOT/tools/transpile.py" >/dev/null

# 32-bit, no-pie (so the 0x400000 image region is free below the 0x08048000 text base),
# match wasm32's 32-bit pointer model. ASan finds the OOB; -g for line-level backtrace.
F="-m32 -no-pie -g -O0 $ASAN -std=gnu89 -w \
  -Wno-int-conversion -Wno-implicit-function-declaration -Wno-builtin-declaration-mismatch \
  -Wno-return-type -Wno-return-mismatch -Wno-incompatible-pointer-types"

# engine units (dd2_runtime EXCLUDED — native_main.c provides main + CRT helpers)
UNITS="dd2 dd2_dispatch dd2_buffers dd2_data dd2_win32 dd2_stubs dd2_com dd2_filio"
OBJS=""
for u in $UNITS; do
  gcc $F -c "$ROOT/build/$u.c" -o "/tmp/n_$u.o" 2>/tmp/ne_$u.txt || true
  if grep -q 'error:' /tmp/ne_$u.txt; then echo "ERROR $u.c:"; grep 'error:' /tmp/ne_$u.txt|head; exit 1; fi
  OBJS="$OBJS /tmp/n_$u.o"
done
gcc $F -c "$ROOT/tools/native_main.c" -o /tmp/n_main.o
gcc $F $OBJS /tmp/n_main.o -o "$OUT" -lm
echo "built $OUT (32-bit, ASan)"
