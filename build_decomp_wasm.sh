#!/bin/bash
# Build the DECOMPILED dd2h code (re_out/dd2.c + scaffolding; handler .c files are appended INTO dd2.c) to runnable WASM.
set -e
source ~/Git/emsdk/emsdk_env.sh >/dev/null 2>&1
F="-std=gnu89 -w -Wno-int-conversion -Wno-incompatible-pointer-types -Wno-implicit-function-declaration -Wno-builtin-declaration-mismatch -Wno-return-type -Wno-return-mismatch"
OBJ=""
for f in dd2 dd2_dispatch dd2_runtime dd2_buffers dd2_data dd2_win32 dd2_stubs dd2_com dd2_filio; do emcc -c $F re_out/$f.c -o /tmp/$f.o; OBJ="$OBJ /tmp/$f.o"; done
emcc $OBJ -o DestructionDerby2/dd2run.js \
  -sGLOBAL_BASE=10485760 -sSTACK_SIZE=16777216 -sINITIAL_MEMORY=268435456 \
  -sALLOW_MEMORY_GROWTH=1 -sEXIT_RUNTIME=1 -sERROR_ON_UNDEFINED_SYMBOLS=0 -sNODERAWFS=1 --profiling-funcs
echo "built DestructionDerby2/dd2run.js"
