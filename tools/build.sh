#!/usr/bin/env bash
# DD2 WASM build:  transpile (re_out decompile -> build/ wasm source)  ->  emcc compile + link.
# Usage: tools/build.sh [out.js]   (default /tmp/lvltest/dd2run.js)
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUTJS="${1:-/tmp/lvltest/dd2run.js}"
mkdir -p "$(dirname "$OUTJS")"
source "$HOME/Git/emsdk/emsdk_env.sh" >/dev/null 2>&1

# 1) transpile: Ghidra decompile -> WASM source
python3 "$ROOT/tools/transpile.py"

# 1b) lift: P-code -> bit-faithful C (whole-program global-CPU model) into build/
python3 "$ROOT/tools/pcode_lift.py" --global \
  "$ROOT"/re_out/pcode/41397e.txt "$ROOT"/re_out/pcode/413fd2.txt "$ROOT"/re_out/pcode/414055.txt \
  "$ROOT"/re_out/pcode/426ec4.txt "$ROOT"/re_out/pcode/426964.txt \
  > "$ROOT/build/lifted.c"
cat "$ROOT/tools/lifted_glue.c" >> "$ROOT/build/lifted.c"

# 2) compile the linked units (re_out/ also holds unlinked Ghidra copies — sprite_handlers.c etc. — skip them)
F="-std=gnu89 -w -Wno-int-conversion -Wno-incompatible-pointer-types -Wno-implicit-function-declaration -Wno-builtin-declaration-mismatch -Wno-return-type -Wno-return-mismatch"
UNITS="dd2 lifted dd2_dispatch dd2_runtime dd2_buffers dd2_data dd2_win32 dd2_stubs dd2_com dd2_filio"
OBJS=""
err=0
for u in $UNITS; do
  c="$ROOT/build/$u.c"; o="/tmp/$u.o"; [ "$u" = lifted ] &&
  emcc -c $F $xf "$c" -o "$o" 2>/tmp/cc_err.txt || true
  if grep -q 'error:' /tmp/cc_err.txt; then echo "ERROR compiling $u.c:"; grep 'error:' /tmp/cc_err.txt | head -5; err=1; fi
  OBJS="$OBJS $o"
done
[ "$err" = 1 ] && { echo "build aborted (compile errors)"; exit 1; }

# 3) link
emcc $OBJS -o "$OUTJS" \
  -sGLOBAL_BASE=10485760 -sSTACK_SIZE=16777216 -sINITIAL_MEMORY=268435456 \
  -sALLOW_MEMORY_GROWTH=1 -sEXIT_RUNTIME=1 -sERROR_ON_UNDEFINED_SYMBOLS=0 -sNODERAWFS=1 2>/dev/null
echo "built $OUTJS"
