#!/usr/bin/env bash
# Native headless build (EGL surfaceless + Mesa llvmpipe). For dev/verification.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build-native out
CFLAGS="-O2 -g -Wall -Wno-misleading-indentation -Isrc"
SRC="src/main_headless.c src/core/track.c src/render/render.c src/platform/headless.c"
gcc $CFLAGS $SRC -lEGL -lGLESv2 -lm -o build-native/dd2_headless
echo "built build-native/dd2_headless"
