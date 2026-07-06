#!/usr/bin/env bash
# One comprehensive QA pass: fepass (17 FE+race areas) + adversarial fuzz + a rotating deep probe.
# Uses exit codes (fepass/qa_* exit 0=pass, 2=fail). Emits "PASS N: ..." / "FAIL N: ...".
# $1 = pass number, $2 = build dir.
N="${1:-0}"; BUILD="${2:-../../web/dd2}"
NODE=$(ls "$HOME"/Git/emsdk/node/*/bin/node 2>/dev/null | head -1); NODE=${NODE:-node}
cd "$(dirname "$0")"
PROBES=(qa_lifecycle.js qa_pause.js qa_subscreens.js qa_kbrebind_hold.js qa_persist_idbfs.js qa_tt.js qa_2p.js qa_attract.js)
ROT="${PROBES[$((N % ${#PROBES[@]}))]}"
fail=""
timeout 520 "$NODE" fepass.js "$BUILD" >/tmp/pass_fepass_$N.log 2>&1 || fail="$fail fepass"
grep -q "17/17 PASS" /tmp/pass_fepass_$N.log || fail="$fail fepass!17"
timeout 320 "$NODE" qa_fuzz.js "$BUILD" >/tmp/pass_fuzz_$N.log 2>&1 || fail="$fail fuzz"
timeout 220 "$NODE" "$ROT" "$BUILD" >/tmp/pass_probe_$N.log 2>&1 || fail="$fail $ROT"
if [ -z "$fail" ]; then echo "PASS $N: ok (fepass 17/17 + fuzz + $ROT)"; else echo "FAIL $N:$fail (see /tmp/pass_*_$N.log)"; fi
