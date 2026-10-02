#!/usr/bin/env bash
# Native 32-bit debug build of the DD2 engine (same C as the wasm build; native_main.c replaces
# the wasm dd2_runtime.c entry). ASan on -> the Track_Follow OOB becomes a real backtrace.
# Needs: gcc-multilib. Run the result from DestructionDerby2/ (it fopens dd2_image.bin + Dirinfo).
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-/tmp/dd2_native}"
ASAN="${ASAN:--fsanitize=address}"

bash "$ROOT/tools/patch.sh" >/dev/null

# 32-bit, no-pie (so the 0x400000 image region is free below the 0x08048000 text base),
# match wasm32's 32-bit pointer model. ASan finds the OOB; -g for line-level backtrace.
read -r -a SANFLAGS <<< "$ASAN"
F=(-m32 -no-pie -g -O0 "${SANFLAGS[@]}" -std=gnu89 -w
  -Wno-int-conversion -Wno-implicit-function-declaration -Wno-builtin-declaration-mismatch
  -Wno-return-type -Wno-return-mismatch -Wno-incompatible-pointer-types -I"$ROOT/build")
SDL_LIBS=()
if [ -z "$DD2_BUILD_HEADLESS" ]; then
  SDL_FLAGS_TXT=$(python3 "$ROOT/tools/native_sdl_config.py" cflags)
  SDL_LIBS_TXT=$(python3 "$ROOT/tools/native_sdl_config.py" libs)
  SDL_FLAGS=()
  if [ -n "$SDL_FLAGS_TXT" ]; then mapfile -t SDL_FLAGS <<< "$SDL_FLAGS_TXT"; fi
  mapfile -t SDL_LIBS <<< "$SDL_LIBS_TXT"
  F+=(-DDD2_NATIVE_SDL "${SDL_FLAGS[@]}")
fi

# engine units (dd2_runtime EXCLUDED — native_main.c provides main + CRT helpers)
python3 "$ROOT/tools/generate_cd_toc.py" "$ROOT/DestructionDerby2/Redbook/disc.json" "$ROOT/build/dd2_disc.h"

UNITS="dd2 dd2_dispatch dd2_buffers dd2_data dd2_win32 dd2_stubs dd2_com dd2_filio dd2_input dd2h_stubs dd2_festate dd2_cd dd2_avi dd2_cinepak dd2_msadpcm dd2_movie dd2_movie_platform dd2_movie_surface"
if [ -z "$DD2_BUILD_HEADLESS" ]; then UNITS="$UNITS dd2_native"; fi
OBJS=""
for u in $UNITS; do
  UNIT_FLAGS=()
  # The exact integer implementation of Wine's x87 FIR arithmetic must keep
  # up with the real audio clock. At -O0 its processing time feeds back into
  # the next elapsed-time block and starves live races. Keep the reconstructed
  # engine at -O0; optimize the handwritten mixer/transport shim without
  # fast-math or aliasing assumptions about its Win32 buffer structures.
  case "$u" in dd2h_stubs|dd2_avi|dd2_cinepak|dd2_msadpcm|dd2_movie_surface) UNIT_FLAGS=(-O2 -fno-strict-aliasing);; esac
  if ! gcc "${F[@]}" "${UNIT_FLAGS[@]}" -c "$ROOT/build/$u.c" -o "/tmp/n_$u.o" 2>"/tmp/ne_$u.txt"; then
    cat "/tmp/ne_$u.txt";exit 1
  fi
  OBJS="$OBJS /tmp/n_$u.o"
done
gcc "${F[@]}" -c "$ROOT/tools/native_main.c" -o /tmp/n_main.o
gcc "${F[@]}" $OBJS /tmp/n_main.o -o "$OUT" -lm "${SDL_LIBS[@]}" \
  -Wl,--version-script="$ROOT/tools/native_symbols.map"
if [ -n "$DD2_BUILD_HEADLESS" ]; then
  echo "built $OUT (32-bit headless)"
else
  echo "built $OUT (32-bit; DD2_WINDOW=1 enables native SDL)"
fi
