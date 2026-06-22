#!/bin/bash
# Build the DECOMPILED dd2h code (re_out/dd2.c + scaffolding) to a runnable WASM module.
set -e
source ~/Git/emsdk/emsdk_env.sh >/dev/null 2>&1
F="-std=gnu89 -w -Wno-int-conversion -Wno-incompatible-pointer-types -Wno-implicit-function-declaration -Wno-builtin-declaration-mismatch -Wno-return-type -Wno-return-mismatch"
for f in dd2 dd2_dispatch dd2_runtime dd2_buffers dd2_data dd2_win32 dd2_stubs dd2_com; do emcc -c $F re_out/$f.c -o /tmp/$f.o; done
emcc /tmp/dd2.o /tmp/dd2_dispatch.o /tmp/dd2_runtime.o /tmp/dd2_buffers.o /tmp/dd2_data.o /tmp/dd2_win32.o /tmp/dd2_stubs.o /tmp/dd2_com.o \
  -o out/dd2_decomp.js -sGLOBAL_BASE=10485760 -sSTACK_SIZE=1048576 -sINITIAL_MEMORY=33554432 \
  -sALLOW_MEMORY_GROWTH=1 -sEXIT_RUNTIME=1 -sERROR_ON_UNDEFINED_SYMBOLS=0 -sNODERAWFS=1
echo "built out/dd2_decomp.js"
