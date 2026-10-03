#!/usr/bin/env bash
# DD2 WASM build:  patch (re_out decompile + patches/ -> build/)  ->  emcc compile + link.
# Usage: tools/build.sh [out.js]   (default /tmp/lvltest/dd2run.js)
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUTJS="${1:-/tmp/lvltest/dd2run.js}"
BUILD_TMP=/tmp/wasm-dd2/node-build
mkdir -p "$BUILD_TMP"
mkdir -p "$(dirname "$OUTJS")"
source "$ROOT/tools/emscripten_env.sh"

# 1) patch: pristine decompile + patches/*.diff -> build/
if [ -z "$DD2_NOPATCH" ]; then bash "$ROOT/tools/patch.sh"; fi

# 2) compile the linked units (re_out/ also holds unlinked Ghidra copies — sprite_handlers.c etc. — skip them)
# FastISel in LLVM 19 folds integer addresses into unsigned WASM memory offsets
# even when the integer addition must wrap (e.g. AI_Com_Server's target-0 lookup).
# SelectionDAG preserves the x86 32-bit address addition; keep the C engine intact.
F="-mllvm -fast-isel=false -std=gnu89 -w -Wno-int-conversion -Wno-incompatible-pointer-types -Wno-implicit-function-declaration -Wno-builtin-declaration-mismatch -Wno-return-type -Wno-return-mismatch"
python3 "$ROOT/tools/generate_cd_toc.py" "$ROOT/DestructionDerby2/Redbook/disc.json" "$ROOT/build/dd2_disc.h"

UNITS="dd2 dd2_dispatch dd2_runtime dd2_buffers dd2_data dd2_win32 dd2_stubs dd2_com dd2_filio dd2_input dd2h_stubs dd2_festate dd2_cd dd2_avi dd2_cinepak dd2_msadpcm dd2_movie dd2_movie_platform dd2_movie_surface dd2_boot"
OBJS=""
err=0
for u in $UNITS; do
  c="$ROOT/build/$u.c"; o="$BUILD_TMP/$u.o"
  if ! emcc -c $F "$c" -o "$o" 2>"$BUILD_TMP/$u.log"; then
    echo "ERROR compiling $u.c:"; cat "$BUILD_TMP/$u.log"; err=1
  fi
  OBJS="$OBJS $o"
done
[ "$err" = 1 ] && { echo "build aborted (compile errors)"; exit 1; }

# 3) link
emcc $OBJS -o "$OUTJS" \
  --pre-js "$ROOT/tools/node_env.js" \
  -sGLOBAL_BASE=10485760 -sSTACK_SIZE=16777216 -sINITIAL_MEMORY=268435456 \
  -sALLOW_MEMORY_GROWTH=1 -sEXIT_RUNTIME=1 -sERROR_ON_UNDEFINED_SYMBOLS=0 -sNODERAWFS=1 --emit-symbol-map \
  2>"$BUILD_TMP/link.log"
echo "built $OUTJS"
