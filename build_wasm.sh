#!/usr/bin/env bash
# WASM build (Emscripten + SDL3 + WebGL2). Shares core + renderer with the native build.
set -euo pipefail
cd "$(dirname "$0")"
source ~/Git/emsdk/emsdk_env.sh >/dev/null 2>&1
mkdir -p web/build
# Preload only the level geometry the playlist needs (keeps the package small).
PRELOAD=()
for L in 1 3 4 5 6 8 9 A B; do
  PRELOAD+=( --preload-file "assets/raw/LEV$L/LEVEL.DAT@/assets/raw/LEV$L/LEVEL.DAT" )
done
emcc -O2 -Isrc \
  src/main_sdl.c src/core/track.c src/core/vehicle.c src/core/race.c src/render/render.c \
  -sUSE_SDL=3 -sFULL_ES3 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 \
  -sALLOW_MEMORY_GROWTH=1 -sEXIT_RUNTIME=0 \
  "${PRELOAD[@]}" \
  --shell-file web/shell.html \
  -o web/build/index.html
echo "built web/build/index.html"
