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

Earlier nine natural ten-lap application sessions passed both modes and
missing-next rollback on Native, Chromium/WASM and ASan/UBSan. The independently
corrected nearest-integer pixel oracle accepted all twenty names/ranks/scores and
actual X11/canvas presentation. Receipt:
/tmp/wasm-dd2/rewrite-championship-application-integration/accepted-report.json.
That receipt predates the ordered-input, visible-track and lifecycle changes.
Fresh Native and Chromium/WASM cases now pass all six cases on the current source;
three instrumented races remain running under
/tmp/wasm-dd2/rewrite-championship-application-current/. No score, lap, position,
damage or result injection supplies natural completion. Reached rewrite units
are sanitizer-instrumented; SDL2 and pinned release SoftGL are uninstrumented.

Actual Redbook output passes the current source in
/tmp/wasm-dd2/rewrite-championship-music-current/report.json. Native effects output
passes, but the instrumented effect test observed PCM before acknowledging the
pause event. The bounded visible-event diagnostic passes:
/tmp/wasm-dd2/rewrite-championship-effects-pause-ack/report.json. The production
verifier still needs this acknowledgement and a complete effects rerun.
Successful raw captures are removed after reports; completed failure diagnoses
retain receipts instead of raw captures.

## Next

Finish the fresh instrumented natural first-round application checks without
changing their source identity. Make the effect pause observer acknowledge actual
presentation, then rerun actual effects output. Follow with physical full-season
continuation/outcome flows, including the next assigned field after promotion.
Full original front end and persistence stay in 0003/0008.

## Accept

Strict LLVM19 and mandatory Native/WASM gates pass. Actual shared application
input enters both championship modes, drives/suspends/restarts without awarding
unfinished points, displays real results/standings, continues to assigned fields,
handles preparation failure without invalid resources and exits safely. Native
and browser presentation and fully instrumented ownership checks are recorded.
No result, damage, lap or score injection substitutes for completed real races.
