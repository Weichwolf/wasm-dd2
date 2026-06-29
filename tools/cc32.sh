#!/usr/bin/env bash
# 32-bit native compiler wrapper for the DD2 engine (matches wasm32's 32-bit pointer model so the
# engine's int-stored pointers + 0x400000 image VAs work natively -> debuggable with gdb/ASan).
# Toolchain bootstrapped from apt-downloaded .debs (libc6-dev-i386, libgcc-14-dev:i386) -> /tmp/m32.
L=/tmp/m32/lib
exec gcc -m32 -no-pie -fno-pie -g -O0 \
  -std=gnu89 -w \
  -Wno-int-conversion -Wno-incompatible-pointer-types -Wno-implicit-function-declaration \
  -Wno-builtin-declaration-mismatch -Wno-return-type -Wno-return-mismatch \
  -B$L -L$L "$@"
