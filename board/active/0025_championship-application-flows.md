Type: Work item
Title: Play scheduled championships through the shared application
Depends: 0024, 0023

## Contract

Connect the real championship owner to Native/browser entry, racing, standings,
round/season continuation, restart and exit. Prepare materials/camera together
with the next owned field; failed preparation preserves current gameplay and
visible results. Keep stable IDs, original schedules and exactly-once real score
consumption. Provide visible shared-canvas standings/outcomes and real input.

## Evidence

The reusable application owns scheduled fields, prepared renderer transitions and
named division standings. Original NPC names and default human PLAYER are visible.
Restart preserves completed points; explicit exit restores the retained practice
track. Practice-mode selection prepares the currently visible scheduled track.
Browser close cancels its owned loop; analog advance resumes audio after restart.
Discrete SDL commands execute before later queued control transitions.

Strict LLVM19 covers 157 owned C/header files; all 34 Native and 32 WASM CTests
pass. Focused SDL checks cover order, queued pressure, release, repeat and wheel
input. Quality receipt: /tmp/wasm-dd2/championship-application-quality-current.json.
Prepared-owner checks pass twelve scoped Native/WASM/sanitized cases, including
wrong-owner/stale candidate rejection, cancellation, restart and missing-next
rollback: /tmp/wasm-dd2/rewrite-championship-prepared-transitions/report.json.

Actual Native, ASan/UBSan and Chromium/WASM input/presentation regressions pass:
/tmp/wasm-dd2/rewrite-championship-application-window-current/report.json. These
cover C/N entry, schedule lock, pause/restart, Escape/F7 exit, visible-track practice
selection, owned browser-loop close/reopen and all existing eleven-track
scene/car/driving/race/trial/arena flows. The focus observer requires an actual
letterboxed presentation before comparing released input.

All nine fresh natural ten-lap application cases pass on Native, Chromium/WASM
and ASan/UBSan, covering both modes and missing-next rollback:
/tmp/wasm-dd2/rewrite-championship-application-current/report.json. Source and
binary identities remain unchanged throughout. The independent original-data
score/name and pixel oracles accept all twenty standings and actual X11/canvas
presentation. Frozen results, second-round preparation/restart, unscored exit and
close/reopen pass. No score, lap, position, damage or result injection supplies
natural completion. Reached rewrite units are sanitizer-instrumented; SDL2 and
pinned release SoftGL are uninstrumented. These first-round cases do not establish
complete physical seasons.

Actual effects and Redbook output pass the current runtime on Native and real
WebAudio at 44,100/48,000 Hz, including sanitized application lifetime:
/tmp/wasm-dd2/rewrite-championship-effects-ack-current/report.json and
/tmp/wasm-dd2/rewrite-championship-music-current/report.json. The effects observer
now acknowledges actual letterboxed presentation after Pause before checking PCM,
replacing an unacknowledged short delay. This verification-only change follows
the championship source-identity run; production sources/binaries are unchanged.
Successful raw captures are removed after reports. The earlier pixel-oracle and
pause-observation diagnoses retain compact receipts.

A separate Native headless first Stockcar-season probe naturally completes round
1, then fails advancement in round 2 at tick 59,399. Ordinary AI input, frame input
and championship state remain valid; race/season phase remains RACING, player
engine health is 0.6396117204829093 and no complete lap is credited. No points or
results are manufactured to continue. Source and library identities match the
published runtime. Report:
/tmp/wasm-dd2/rewrite-stock-season-probe/report.json. The sparse diagnostic and
probe remain available for this unresolved simulation failure.

## Next

Locate the failing simulation stage at round-2 tick 59,399 using the retained
bounded probe and only the needed checkpoint. Preserve the regular race rules,
AI controls, damage and score ownership. Follow with complete physical seasons
and continuing/terminal outcome application flows, including the assigned field
after promotion. Full original front end and persistence stay in 0003/0008.

## Accept

Strict LLVM19 and mandatory Native/WASM gates pass. Actual shared application
input enters both championship modes, drives/suspends/restarts without awarding
unfinished points, displays real results/standings, continues to assigned fields,
handles preparation failure without invalid resources and exits safely. Native
and browser presentation and fully instrumented ownership checks are recorded.
No result, damage, lap or score injection substitutes for completed real races.
