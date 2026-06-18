#!/usr/bin/env bash
# Native headless build (EGL surfaceless + Mesa llvmpipe). For dev/verification.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build-native out
# Faithful DD2 tire-force physics is the default; build with DD2_ARCADE=1 for the old kinematic model.
CFLAGS="-O2 -g -Wall -Wno-misleading-indentation -Isrc ${DD2_ARCADE:+}"
[ -z "${DD2_ARCADE:-}" ] && CFLAGS="$CFLAGS -DDD2_TIRE"
CORE="src/core/track.c src/core/vehicle.c src/core/race.c"
RP="src/render/render.c src/platform/headless.c"
LIBS="-lEGL -lGLESv2 -lm"
gcc $CFLAGS src/main_headless.c $CORE $RP $LIBS -o build-native/dd2_view
gcc $CFLAGS src/main_race.c     $CORE $RP $LIBS -o build-native/dd2_race
echo "built build-native/dd2_view and build-native/dd2_race"
