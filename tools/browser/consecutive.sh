#!/usr/bin/env bash
# Drive consecutive comprehensive QA passes toward the 10-clean-pass goal. Runs passrun.sh
# repeatedly, appending one PASS/FAIL line per pass to the log. STOPS on the first FAIL (so the
# main agent can root-cause + fix + restart the count). $1 = start index, $2 = count, $3 = build.
START="${1:-1}"; COUNT="${2:-10}"; BUILD="${3:-../../web/dd2}"
cd "$(dirname "$0")"
LOG=/tmp/consecutive.log
for i in $(seq "$START" $((START+COUNT-1))); do
  line=$(bash passrun.sh "$i" "$BUILD")
  echo "$line" | tee -a "$LOG"
  echo "$line" | grep -q "^PASS" || { echo "STOP: pass $i failed" | tee -a "$LOG"; exit 2; }
done
echo "DONE: $COUNT consecutive clean passes from $START" | tee -a "$LOG"
