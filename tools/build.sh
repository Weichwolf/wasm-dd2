#!/usr/bin/env bash
# DD2 WASM build:  patch (re_out decompile + patches/ -> build/)  ->  emcc compile + link.
# Usage: tools/build.sh [out.js]   (default /tmp/lvltest/dd2run.js)
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUTJS="${1:-/tmp/lvltest/dd2run.js}"
mkdir -p "$(dirname "$OUTJS")"
source "$HOME/Git/emsdk/emsdk_env.sh" >/dev/null 2>&1

# 1) patch: pristine decompile + patches/*.diff -> build/
bash "$ROOT/tools/patch.sh"

# 2) compile the linked units (re_out/ also holds unlinked Ghidra copies — sprite_handlers.c etc. — skip them)
F="-std=gnu89 -w -Wno-int-conversion -Wno-incompatible-pointer-types -Wno-implicit-function-declaration -Wno-builtin-declaration-mismatch -Wno-return-type -Wno-return-mismatch"
UNITS="dd2 dd2_dispatch dd2_runtime dd2_buffers dd2_data dd2_win32 dd2_stubs dd2_com dd2_filio dd2_input dd2h_stubs"
OBJS=""
err=0
for u in $UNITS; do
  c="$ROOT/build/$u.c"; o="/tmp/$u.o"
  emcc -c $F "$c" -o "$o" 2>/tmp/cc_err.txt || true
  if grep -q 'error:' /tmp/cc_err.txt; then echo "ERROR compiling $u.c:"; grep 'error:' /tmp/cc_err.txt | head -5; err=1; fi
  OBJS="$OBJS $o"
done
[ "$err" = 1 ] && { echo "build aborted (compile errors)"; exit 1; }

# 3) link
emcc $OBJS -o "$OUTJS" \
  -sGLOBAL_BASE=10485760 -sSTACK_SIZE=16777216 -sINITIAL_MEMORY=268435456 \
  -sALLOW_MEMORY_GROWTH=1 -sEXIT_RUNTIME=1 -sERROR_ON_UNDEFINED_SYMBOLS=0 -sNODERAWFS=1 --emit-symbol-map 2>/dev/null
echo "built $OUTJS"
