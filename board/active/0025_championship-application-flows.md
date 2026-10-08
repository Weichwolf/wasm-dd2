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
published runtime before correction selection. Report:
/tmp/wasm-dd2/rewrite-stock-season-probe/report.json. The sparse diagnostic and
probe supplied the reproduction for 0026.

The correction-selection improvement in 0026 advances that exact saved step and
the unchanged-bound Native season probe proceeds to second-round tick 66,449.
Its first ten-lap round still completes naturally at tick 287,081. A separate
four-world-contact velocity failure now stops driver 8; player engine health is
0.5166984465536384. The failed step is reproduced from one 31,216-byte checkpoint
and supplied the now-proved contact contract under 0027. Evidence:
/tmp/wasm-dd2/rewrite-world-branch-selection/selection-report.json and
/tmp/wasm-dd2/rewrite-stock-season-next-failure/report.json. Neither this progress
nor the separately passing eight-lap Circuit-5 race proves complete seasons.

The current contact-selection runtime also passes all eleven Native, sanitized
window and Chromium/WASM input/presentation regressions:
/tmp/wasm-dd2/rewrite-world-branch-selection/window-final/report.json. Native
championship entry waits for changed presentation before pause observation.
Browser restart checks the trusted click's exact zero tick and field ownership;
a later Pause click may legitimately occur after the countdown. Both observers
retain real input, score ownership and existing timeout bounds. These current
window regressions are separate from the earlier nine natural first-round
application cases and the still-open complete-season contract.

The shared low-speed contact law in 0027 passes its saved input and all current
Native/browser/sanitized window flows. The fresh Native season completes its first
round at tick 287,087, then reaches the coupled drivers-11/13 support failure at
second-round tick 86,643. The regular race bound is unchanged. Current scoped
quality/physical/presentation receipt:
/tmp/wasm-dd2/rewrite-world-friction-transition/report.json. The next retained
checkpoint and diagnosis are /tmp/wasm-dd2/rewrite-season-86643/report.json.
Complete seasons and the nine earlier application first-round cases keep their
separate scopes.

Closed 0028 adds analytic mixed-contact corrections without changing the material
law or race rules. Eighteen captured queries, fourteen reorderings, mandatory
Native/WASM/LLVM19 gates and all eleven actual window/browser flows pass. The
saved tick-86,643 input advances; the fresh Native season proceeds to tick
107,550 before another advancement failure. Its exact current-source receipt is
/tmp/wasm-dd2/rewrite-coupled-86643-final/report.json. The next sparse diagnosis
matches production under 0029: velocity converges, then position repair exceeds
its 4,096-pass bound. Complete physical seasons remain unproved.

Closed 0029 now proves the saved advancement cases through tick 93,231, with
25 captured queries, 334 orderings and strict Native/WASM/sanitizer gates.
Actual window/input and original-data ground checks pass all eleven levels;
six selected race target scopes include the complete eight-lap Circuit-5 race.
The unchanged-bound Native season passes every previously recorded failure tick,
including 107,550, then fails round 2 at 118,647. Sparse production-equivalent
capture and actual replays identify a three-world-contact velocity failure on
driver 15 under 0030. Complete physical seasons remain unproved.
Current evidence: /tmp/wasm-dd2/rewrite-release-93231/report.json and
/tmp/wasm-dd2/rewrite-season-after-93231/report.json.

Closed 0030 now proves the saved tick-118,647 step and natural advancement past
it through bounded private linear world-friction refinement. Twenty-six captured
queries, 340 orderings, mandatory LLVM19/Native/WASM gates, eleven-level actual
window/input and original-data ground checks, and six scoped race targets pass
on all three variants. The unchanged-bound Native season next fails round 2 at
128,036. Sparse production-equivalent capture and actual replays identify six
cars with eleven contacts (six world/five pair), failing the velocity stage.
The new advancement owner is 0031; complete physical seasons remain unproved.
Current receipts: /tmp/wasm-dd2/rewrite-linear-118647/report.json and
/tmp/wasm-dd2/rewrite-season-after-118647/report.json.

Closed 0031/0032 now prove saved-step corrections at 128,036/220,415, with strict
gates and reached three-target physical/original-data/actual-input regressions.
The unmodified Native first-season attempt and logging-only companion reach
round-2 tick 230,000 with matching prefixes and identical previous round-1 results.
Full-season completion remains unproved. Current advancement receipt:
/tmp/wasm-dd2/rewrite-patch-220415/advancement-report.json.

Those longer attempts now terminate at round-2 tick 233,358 with matching records.
The saved-owner failure, faithful isolated query and independent two-support
diagnosis are tracked under active 0033. A private Native correction advances
the saved step; production/multi-target/natural continuation remain unproved.
Current diagnosis: /tmp/wasm-dd2/rewrite-season-after-220415-long/diagnosis.json.

Closed 0033 now proves the production saved-input correction and natural
continuation through round-2 tick 240,000, with identical production/diagnostic
prefixes and previous first-round results. Strict gates, three-target captured
physics, actual eleven-level input/original-data and six scoped race target checks
pass. Full physical seasons remain unproved; the same natural processes continue.
Current advancement: /tmp/wasm-dd2/rewrite-endpoints-233358/advancement-report.json.

Those attempts are now terminal at the unchanged second-round 300,000-tick bound.
All 120 production/diagnostic records match, with no contact abort or failure
checkpoint. The player's engine remains healthy at 0.4003601829670652 but laps
remain zero; no scores/results are manufactured. Diagnose current movement and
controller behavior under active 0016. Terminal receipt:
/tmp/wasm-dd2/rewrite-endpoints-233358/report.json.

## Next

Resolve the current healthy natural noncompletion under active 0016, then require
complete physical seasons at the unchanged ordinary bounds and real score rules.
Preserve regular race rules, AI controls, damage and score ownership.
Follow with complete physical seasons
and continuing/terminal outcome application flows, including the assigned field
after promotion. Full original front end and persistence stay in 0003/0008.

## Accept

Strict LLVM19 and mandatory Native/WASM gates pass. Actual shared application
input enters both championship modes, drives/suspends/restarts without awarding
unfinished points, displays real results/standings, continues to assigned fields,
handles preparation failure without invalid resources and exits safely. Native
and browser presentation and fully instrumented ownership checks are recorded.
No result, damage, lap or score injection substitutes for completed real races.
