#!/usr/bin/env bash
# Unmodified Windows reference with a verified virtual CD and scoped cleanup.
# All Wine processes belong to third_party/wine-reference/prefix. No global pkill.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [ "${LVL:-9}" != 9 ]; then
    echo "FE attract capture supports level 9; use menu mode for menu checkpoints." >&2
    exit 2
fi
exec python3 "$ROOT/tools/reference/capture.py" \
    --game-dir "${GAME:-$ROOT/DestructionDerby2}" \
    --output "${OUT:-/tmp/dd2-reference}" \
    --mode "${REFMODE:-attract}" \
    --frame "${FRMIN:-150}" \
    --timeout "${RUNSEC:-90}"
