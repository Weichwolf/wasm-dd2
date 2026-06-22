#!/usr/bin/env bash
# WASM build (Emscripten + SDL3 + WebGL2). Shares core + renderer with the native build.
set -euo pipefail
cd "$(dirname "$0")"
source ~/Git/emsdk/emsdk_env.sh >/dev/null 2>&1
mkdir -p web/build
# Preload only the level geometry the playlist needs (keeps the package small).
PRELOAD=()
for L in 1 2 3 4 5 6 7 8 9 A B; do
  PRELOAD+=( --preload-file "assets/raw/LEV$L/LEVEL.DAT@/assets/raw/LEV$L/LEVEL.DAT" )
  # textures for the in-race textured-face renderer (VRAM/CLUT/TDF)
  for F in LEVEL.CLT LEVEL.TDF LEVEL.TX0 LEVEL.TX1 LEVEL.TX2 LEVEL.TX3 LEVEL.TX4; do
    [ -f "assets/raw/LEV$L/$F" ] && PRELOAD+=( --preload-file "assets/raw/LEV$L/$F@/assets/raw/LEV$L/$F" )
  done
done
PRELOAD+=( --preload-file "assets/raw/VAGS/BANK1.SBK@/assets/raw/VAGS/BANK1.SBK" )
# front-end assets
for FE in LEV0/COPYRIGH.BMP LEV0/LOADING.BMP LEV0/FONT.BNK LEV0/LEVEL.SPR LEV0/LEVEL.PAL LEV0/LEVEL.CLT LEV0/LEVEL.TX0 LEV0/LEVEL.TX1 LEV0/LEVEL.TX2 LEV0/LEVEL.TX3 LEV0/LEVEL.TX4; do
  PRELOAD+=( --preload-file "assets/raw/$FE@/assets/raw/$FE" )
done
emcc -O2 -Isrc -DDD2_TIRE -DDD2_GTE -DDD2_FIXED \
  src/main_sdl.c src/core/track.c src/core/vehicle.c src/core/race.c src/core/geo.c src/core/fixed.c src/render/render.c src/render/gte_render.c src/render/ui.c src/render/vram.c \
  -sUSE_SDL=3 -sFULL_ES3 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 \
  -sALLOW_MEMORY_GROWTH=1 -sEXIT_RUNTIME=0 \
  "${PRELOAD[@]}" \
  --shell-file web/shell.html \
  -o web/build/index.html
echo "built web/build/index.html"
