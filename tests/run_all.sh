#!/usr/bin/env bash
# Verify every level over multiple seeds: completion + determinism. Classifies
# circuits / arenas / non-tracks. Exit 0 only if all playable levels pass all seeds.
cd "$(dirname "$0")/.."
BIN=build-native/dd2_race
SEEDS="1 2 3"
fail=0
for L in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do
  D="assets/raw/LEV$L/LEVEL.DAT"
  if [ ! -f "$D" ]; then printf "LEV%-2s  ABSENT\n" "$L"; continue; fi
  line="LEV$L "
  bad=0; nontrack=0
  for s in $SEEDS; do
    OUT=$("$BIN" "$D" "out/verify/LEV${L}_s${s}" 2 6 "$s" 999 2>/dev/null)
    if ! echo "$OUT" | grep -q 'race time'; then nontrack=1; break; fi
    fin=$(echo "$OUT" | grep -c finished@)
    det=$(echo "$OUT" | grep -oE 'MATCH|MISMATCH')
    line+=" s$s:$fin/6,$det"
    [ "$fin" = 6 ] || bad=1
    [ "$det" = MATCH ] || bad=1
  done
  if [ "$nontrack" = 1 ]; then printf "%-6s NON-TRACK (no drivable loop)\n" "LEV$L"; continue; fi
  if [ "$bad" = 1 ]; then echo "$line  <-- FAIL"; fail=1; else echo "$line  OK"; fi
done
echo "================"
[ "$fail" = 0 ] && echo "ALL PLAYABLE LEVELS PASS" || echo "SOME FAILURES"
exit $fail
