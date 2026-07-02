#!/usr/bin/env bash
# Pipeline stage `make patch`:  re_out/ (pristine decompile + shim)  +  patches/NNN-*.diff  ->  build/
#
# The patch series is ORDERED (numeric prefix = application order; later patches may depend on the
# text produced by earlier ones). Each .diff carries its rationale as a header above the unified diff.
# Application is EXACT (-F0 --fuzz=0): any decompile drift breaks loudly here instead of silently
# mis-applying — this stage is the anchor check of the pipeline.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

rm -rf "$ROOT/build"
mkdir -p "$ROOT/build"
cp "$ROOT"/re_out/*.c "$ROOT"/re_out/*.h "$ROOT/build/"

n=0
for p in "$ROOT"/patches/*.diff; do
  if ! patch -p1 -s -F0 --fuzz=0 -d "$ROOT/build" < "$p"; then
    echo "PATCH FAILED: $(basename "$p")  (decompile drift? fix or regenerate the patch)" >&2
    exit 1
  fi
  n=$((n+1))
done
rm -f "$ROOT"/build/*.orig
echo "patch: $n patches applied -> build/"
