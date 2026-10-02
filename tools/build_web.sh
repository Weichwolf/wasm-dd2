#!/usr/bin/env bash
# DD2 decompiled-engine BROWSER build: same C as the headless node build (build.sh) but rendered to a
# <canvas> and driven by real keyboard input. Differs from web/build (a SEPARATE from-scratch WebGL
# reimplementation) -- this is the actual dd2h.exe -> WASM port running interactively.
#   - ids_flip (dd2_com.c, #ifdef DD2_BROWSER) blits g_pixels@0x700450 -> canvas via g_palette, then
#     emscripten_sleep(0) yields to the browser (ASYNCIFY) so it paints + delivers key events.
#   - keyboard: shell JS calls _dd2_browser_key_event(codePtr, down) on keydown/keyup.
# Usage: tools/build_web.sh [outdir]   (default web/dd2)
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUTDIR="${1:-$ROOT/web/dd2}"
mkdir -p "$OUTDIR"
source "$ROOT/tools/emscripten_env.sh"
GAME="$ROOT/DestructionDerby2"

bash "$ROOT/tools/patch.sh"

# Match build.sh: preserve wrapping 32-bit addresses instead of FastISel offsets.
F="-mllvm -fast-isel=false -std=gnu89 -w -DDD2_BROWSER -Wno-int-conversion -Wno-incompatible-pointer-types -Wno-implicit-function-declaration -Wno-builtin-declaration-mismatch -Wno-return-type -Wno-return-mismatch"
python3 "$ROOT/tools/generate_cd_toc.py" "$ROOT/DestructionDerby2/Redbook/disc.json" "$ROOT/build/dd2_disc.h"

UNITS="dd2 dd2_dispatch dd2_runtime dd2_buffers dd2_data dd2_win32 dd2_stubs dd2_com dd2_filio dd2_input dd2h_stubs dd2_festate dd2_cd dd2_avi dd2_cinepak dd2_msadpcm dd2_movie dd2_movie_platform dd2_movie_surface"
OBJS=""; err=0
for u in $UNITS; do
  c="$ROOT/build/$u.c"; o="/tmp/web_$u.o"
  emcc -c $F "$c" -o "$o" 2>/tmp/wcc_err.txt || true
  if grep -q 'error:' /tmp/wcc_err.txt; then echo "ERROR compiling $u.c:"; grep 'error:' /tmp/wcc_err.txt | head -5; err=1; fi
  OBJS="$OBJS $o"
done
[ "$err" = 1 ] && { echo "build aborted (compile errors)"; exit 1; }

emcc $OBJS -o "$OUTDIR/index.html" \
  -sGLOBAL_BASE=10485760 -sSTACK_SIZE=16777216 -sINITIAL_MEMORY=268435456 \
  -sALLOW_MEMORY_GROWTH=1 -sEXIT_RUNTIME=0 -sERROR_ON_UNDEFINED_SYMBOLS=0 \
  -sASYNCIFY -sASYNCIFY_STACK_SIZE=131072 \
  -sEXPORTED_FUNCTIONS='["_main","_dd2_browser_key_event","_dd2_pad_update","_malloc","_free"]' \
  -sEXPORTED_RUNTIME_METHODS='["ccall","cwrap","stringToUTF8","lengthBytesUTF8","ENV","FS"]' \
  -lidbfs.js \
  --shell-file "$ROOT/web/shell_port.html" \
  --preload-file "$GAME/Dirinfo@Dirinfo" \
  --preload-file "$GAME/dd2_image.bin@dd2_image.bin" \
  2>/tmp/wlink_err.txt || { echo "LINK FAILED:"; tail -20 /tmp/wlink_err.txt; exit 1; }
# SaveGames is NOT preloaded: it is IDBFS-backed (shell_port.html mounts /persist and symlinks
# /SaveGames -> /persist/SaveGames). On a first-ever run the file is absent, so the engine's
# InitCardSystem @0x423220 recreates a fresh 128KB card from the image baseline (proven native:
# the recreated file is byte-identical to the shipped SaveGames). The shell creates an empty
# target before linking so older MEMFS versions can open it; then it persists to IndexedDB.
cp -a "$GAME/Redbook" "$OUTDIR/"
cp "$GAME/Intro.avi" "$OUTDIR/INTRO.AVI"
cp "$GAME/Outro.avi" "$OUTDIR/OUTRO.AVI"
echo "built browser port -> $OUTDIR/index.html"
