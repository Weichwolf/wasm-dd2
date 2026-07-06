#!/usr/bin/env bash
# One comprehensive QA pass: fepass (17 FE+race areas) + adversarial fuzz + a rotating deep probe.
# A component FAILS if its log shows a real crash/error signature, OR it exits non-zero for a reason
# OTHER than a headless-chromium teardown race (long runs intermittently get the renderer reaped at
# the tail AFTER clean work -- a harness artifact, not a game defect; verified no wasm leak via
# qa_memcheck.js). $1 = pass number, $2 = build dir.
N="${1:-0}"; BUILD="${2:-../../web/dd2}"
NODE=$(ls "$HOME"/Git/emsdk/node/*/bin/node 2>/dev/null | head -1); NODE=${NODE:-node}
cd "$(dirname "$0")"
PROBES=(qa_lifecycle.js qa_pause.js qa_subscreens.js qa_kbrebind_hold.js qa_persist_idbfs.js qa_tt.js qa_2p.js qa_attract.js)
ROT="${PROBES[$((N % ${#PROBES[@]}))]}"
BAD='\*\*\* CRASH|PAGEERR|RuntimeError|unreachable|abort\(|SIGSEGV|: FAIL|-> FAIL|not-launched|UNDERFLOW|crashed=true'
TEARDOWN='teardown|browser has been closed|Target (page|closed)|Session closed|context or browser'
# check <logfile> <exitcode> -> echoes "" if ok, else a reason
check(){ local log="$1" rc="$2"
  grep -qE "$BAD" "$log" && { echo "realerr"; return; }
  [ "$rc" = 0 ] && { echo ""; return; }
  grep -qiE "$TEARDOWN" "$log" && { echo ""; return; }   # non-zero exit but only a teardown race -> ok
  echo "exit$rc"; }
fail=""
timeout 520 "$NODE" fepass.js "$BUILD" >/tmp/pass_fepass_$N.log 2>&1; rc=$?
grep -q "17/17 PASS" /tmp/pass_fepass_$N.log || fail="$fail fepass"
[ -n "$(check /tmp/pass_fepass_$N.log $rc)" ] && fail="$fail fepass-err"
timeout 260 "$NODE" qa_fuzz.js "$BUILD" >/tmp/pass_fuzz_$N.log 2>&1; rc=$?
[ -n "$(check /tmp/pass_fuzz_$N.log $rc)" ] && fail="$fail fuzz"
timeout 220 "$NODE" "$ROT" "$BUILD" >/tmp/pass_probe_$N.log 2>&1; rc=$?
[ -n "$(check /tmp/pass_probe_$N.log $rc)" ] && fail="$fail $ROT"
if [ -z "$fail" ]; then echo "PASS $N: ok (fepass 17/17 + fuzz + $ROT)"; else echo "FAIL $N:$fail (see /tmp/pass_*_$N.log)"; fi
