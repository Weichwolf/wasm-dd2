#!/usr/bin/env bash
# DD2 pipeline STAGE 1 (M1): reproduce the named decompile from the binary.
#   dd2h.exe (analyzed Ghidra project) --headless--> re_out/dd2_decomp.c + functions.txt
# Names/types come from the Ghidra project DB; this stage never hand-edits C.
# Idempotent: re-running regenerates the same output from the saved analysis.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# --- toolchain locations (override via env) ---
GHIDRA="${GHIDRA_HOME:-$ROOT/third_party/ghidra_12.1.2_PUBLIC}"
export JAVA_HOME="${JAVA_HOME:-$ROOT/third_party/jdk-21.0.11+10}"
PROJ_DIR="${DD2_GHIDRA_PROJ_DIR:-$ROOT/third_party/dd2_ghidra_proj}"
PROJ_NAME="${DD2_GHIDRA_PROJ_NAME:-dd2}"
PROGRAM="${DD2_GHIDRA_PROGRAM:-dd2h.exe}"
OUT="${1:-$ROOT/re_out}"

echo "[decompile] ghidra=$GHIDRA jdk=$JAVA_HOME proj=$PROJ_DIR/$PROJ_NAME prog=$PROGRAM -> $OUT"
"$GHIDRA/support/analyzeHeadless" \
  "$PROJ_DIR" "$PROJ_NAME" \
  -process "$PROGRAM" -noanalysis \
  -scriptPath "$ROOT/tools/ghidra" \
  -scriptlog /tmp/dd2_decompile.log \
  -postScript ExportDecomp.java \
  2>&1 | grep -iE 'EXPORT_DONE|ERROR REPORT|Exception' || true

[ -s "$OUT/dd2_decomp.c" ] && echo "[decompile] OK: $(grep -cE '/\* ===== .* @ ' "$OUT/dd2_decomp.c") functions -> $OUT/dd2_decomp.c" \
  || { echo "[decompile] FAILED (no output)"; exit 1; }
