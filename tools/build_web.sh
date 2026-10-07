#!/usr/bin/env bash
# Build the frozen Ghidra browser reference under /tmp.
# Build the handwritten rewrite with make rewrite-wasm.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
REFERENCE="$(python3 "$ROOT/tools/rewrite/reference.py")"
OUTPUT="${1:-$REFERENCE/web/dd2}"
python3 - "$OUTPUT" <<'CHECK'
from pathlib import Path
import sys
output = Path(sys.argv[1]).resolve()
if Path('/tmp') not in output.parents:
    raise SystemExit('Reference browser output must be under /tmp')
CHECK
exec bash "$REFERENCE/tools/build_web.sh" "$OUTPUT"
