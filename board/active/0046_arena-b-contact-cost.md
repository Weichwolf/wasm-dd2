Type: Work item
Title: Diagnose and reduce actual Arena-B contact cost
Depends: 0045, 0016

## Contract

Complete the existing original Arena-B O3 ASan/UBSan sixty-second physical AI
scenario within its unchanged 360-second process deadline, preserving the full
2,400 frames, twenty bodies, frame partition/reset checks, physical response/
constraint/model/pass limits, material laws and acceptance tolerances. Measure
actual work before selecting a correction. Retain Native/WASM/instrumented
physical correctness and owned record lifetime; do not substitute a shorter
simulation, relax the deadline or omit instrumentation.

## Evidence

Production d6df3ba separates report storage from unchanged physical work. Its
strict gates, 716 captured physical checks per target, seven old Native owners,
all eleven ground/fleet/actual-input scopes and selected Circuit-2/Circuit-5
races pass. The ordinary Native application completes its first Stockcar season.
The full AI suite passes levels 1..A across all three targets. Native/WASM
Arena-B exporters finish their complete 300-row inventory; the independent
motion gate still rejects Native slot-17 support (1,929 versus 2,280 frames)
under 0016. WASM passes support/travel there. The O3 ASan/UBSan drive exceeds
360 seconds before completing; its last complete exported frame is 1,600.
This is a measured cost failure, not proof of a particular hot path or cause.
Receipt: /tmp/wasm-dd2/rewrite-owned-events-0045/ai-partial-report.json.

A bounded current-source O3 ASan/UBSan profile now reproduces all 108 complete
production rows of the failed source epoch exactly. Its stderr diagnostics
interrupted buffered stdout; reconstructing the two streams removes only each
complete diagnostic object and its newline. The initial parser's apparent
trajectory divergence was an extraction error, not a physical difference.
At frame 1,000, 5,200 fleet steps have issued 266,985 earliest-contact searches
and 264,509 group collections. Thread CPU time is 195.012 seconds for earliest
search, 53.362 for collection and 12.212 for solving. The slowest completed
group solve is 0.000589 seconds. This profile also exceeds the unchanged
360-second deadline; it is diagnostic evidence, not completed AI acceptance.
Receipt: /tmp/wasm-dd2/rewrite-arena-cost-0046/profile-report.json.

A separate 100-second bounded profile preserves all 60 complete exported rows.
At frame 400, earliest search spends 24.577 thread CPU seconds on world queries
and 39.056 on car pairs; group collection spends 14.167 of its 17.580 seconds
on pair neighborhoods. Timers exclude scheduling delays. Pair queries are the
largest measured search component; a solver optimization alone would not
address most of this cost. Receipt: detail/report.json in the same directory.

A private candidate moves the existing exact swept-center sphere rejection
ahead of complete dynamic-state validation, retaining null checks and full
validation before survivor geometry. All 190 pair outputs from one captured
actual Native field, including two hits, match the frozen old function exactly
under O3 ASan/UBSan. Four alternating-order 380,000-call trials reduce thread
CPU time by 29.2..31.4 percent. This is a local measurement only: the complete
unchanged Arena-B scenario remains a separate gate, and the candidate has not
been adopted on master. Receipt: broadphase-prototype/benchmark-report.json.

The candidate's full instrumented case is now terminal: it exceeds the
unchanged 360-second deadline without summaries or reset completion. All 126
complete exported rows match the frozen failed production prefix exactly;
frame 1,000 is completely exported and frame 1,200 is partial. Its isolated
timing gain does not establish a sufficient full-scenario improvement, and no
candidate code has been adopted. Receipt: broadphase-prototype/report.json.

A separate typed batch now validates every immutable body once and reuses
prepared motion/boxes across its unordered pairs. The scalar APIs retain full
validation before sphere rejection. Preparation is call-local, with caller-owned
190-pair inventories, lexicographic order and unchanged per-pair geometry/work
bounds. Fleet invalid preparation rejects the transaction. O3 ASan/UBSan trials
on one actual Native field preserve all 190 frozen scalar results exactly,
including two sweep hits and one neighbor. Four alternating-order trials of
380,000 pairs each reduce thread CPU time by 40.2..41.5 percent for sweeps and
61.3..62.7 percent for neighborhoods. These are scoped query measurements.
Receipt: /tmp/wasm-dd2/rewrite-pair-preparation-0046/benchmark-report.json.

Current production passes strict LLVM19 formatting/tidy for 163 C/header files,
37 Native and 35 WASM CTests, all 716 physical checks per Native/WASM/ASan/UBSan
target and six focused instrumented targets. All eleven original-data ground
and fleet levels pass on three targets (79,100 queries/26,400 ground steps and
220 starts/132,000 fleet steps each). Actual Native/sanitized/Chromium input and
presentation pass 31/31/144 comparisons with no browser errors. These verify
the partial repeated-work correction, preserving physical response/constraint/
pass/record bounds; the full deadline contract remains separate and active.
Receipts: native-quality-report.json, wasm-quality-report.json,
physical-report.json, sanitizer-report.json and ground/fleet/window reports
in /tmp/wasm-dd2/rewrite-pair-preparation-0046/.

Complete production Native/WASM Arena-B runs exit zero within the original
180-second exporter bound (97.23/115.25 seconds), each retaining all 300 rows
byte for byte against its previous completed target. Native slot 17 still has
only 1,929 supported frames, so Native motion acceptance remains unsuccessful;
WASM motion passes. The first private instrumented batch attempt times out at
360 seconds with 72 complete matching prefix rows. Its partial prefix and empty
stderr establish neither completion nor a whole-scenario speedup. A separate
current-production O3 ASan/UBSan build records every reached source and measures
child CPU independently of wallclock for the unchanged full case.
Receipts: production-arena-verified/native/report.json,
production-arena-verified/wasm/report.json, report.json and
production-arena-instrumented/build-report.json in the same directory.

The current-production instrumented attempt is now terminal: timeout after
360.172 wallclock seconds and 360.015 child CPU seconds. It retains 232 complete
rows through frame 2,200/slot 11; all 108 available frozen production prefix rows
match exactly. No summary/reset inventory completes and stderr is empty. Every
reached C unit is O3 ASan/UBSan-instrumented. This attempt uses almost all its
wallclock budget as actual CPU work; it still fails the unchanged deadline.
The scoped pair-preparation gain is verified, while 0046 remains active.
Receipts: production-arena-instrumented/report.json and component-report.json
in /tmp/wasm-dd2/rewrite-pair-preparation-0046/.

The separate 51605e8 thread CPU profile preserves all 60 available production
rows exactly through its 100-second diagnostic budget. At frame 400, 90,250
earliest searches have called ground/barrier sweeps 1,805,000 times each.
Ground sweeps consume 19.903 CPU seconds, barriers 2.337, pair inventory 27.433,
pair neighborhoods 4.904 and solving 3.024. This identifies repeated immutable
road-plane preparation as relevant work while pair search remains larger.
Receipt: /tmp/wasm-dd2/rewrite-world-cost-0046/profile-report.json.

The road index now owns each triangle's initial sweep plane/origin and prepares
it once from immutable source geometry. Inactive/degenerate triangles retain
their original rejection. Construction checks allocation bounds and releases
all storage on failure/destruction. Final-point triangle tests, heights, bridge
separation, edge/time tolerance and traversal ties are unchanged. All 79,100
original short/fast sweeps on eleven levels preserve every frozen previous
result field and work count exactly under O3 ASan/UBSan. Four alternating-order
trials reduce query CPU by 19.0..20.8 percent. These are local timings, not a
whole-scenario speedup. Receipt: benchmark-report.json in the same directory.

The private plane candidate's full instrumented attempt still times out after
360.086 wallclock/355.559 CPU seconds. Its 232 complete rows match the prior
production attempt exactly; summaries/reset do not complete and stderr is empty.
Strict LLVM19 requires widening the triangle index before multiplication; the
final production implementation fixes that finding. Its frozen-index comparison
still passes all 79,100 queries. The final source passes 37 Native/35 WASM
CTests, 163-file format/tidy, eight instrumented targets, 716 physical checks
per target and eleven-level original-data ground/fleet checks. Actual window
and final full-case checks are recorded separately before publication.
Receipts: candidate/report.json, native-quality-report.json,
wasm-quality-report.json, physical-report.json, sanitizer-report.json and
ground/fleet reports in /tmp/wasm-dd2/rewrite-world-cost-0046/.

Final Native/WASM Arena-B exporters complete within their unchanged 180-second
bound (138.20/167.03 seconds) and preserve all 300 preceding rows byte for byte,
including twenty summaries and reset owners. Native support acceptance still
fails at slot 17; WASM motion passes. Current-production O3 ASan/UBSan times out
at 360.062 wallclock seconds with only 217.797 child CPU seconds and 108 complete
matching prefix rows. Its wallclock result cannot isolate a code speedup or
regression relative to the prior mostly-CPU-bound attempts. Neither process
deadlines nor motion requirements are relaxed, and no complete AI acceptance is
claimed. Receipts: production-arena-verified/{native,wasm}/report.json and
production-arena-instrumented/report.json in the same directory.

The first actual-window attempt passes 31 Native/31 instrumented comparisons,
then reaches the browser's unchanged 15-second real reverse-to-arena-8 contact
wait without observing the contact. Its 59 completed browser comparisons and
empty browser error list do not establish a full pass. The complete unchanged
retry passes all 31/31/144 comparisons and input/lifecycle/mode checks, with no
browser errors. No code, wait limit or acceptance threshold changes between
attempts. The terminal negative receipt is retained alongside the passing retry;
the timeout alone does not identify a physical defect or its cause.
Receipts: window-first-attempt-report.json, window-retry/report.json and
component-report.json in /tmp/wasm-dd2/rewrite-world-cost-0046/.

The current 4e66b27 diagnostic counts actual pair geometry without changing
motions, material laws or query bounds. All 60 complete exported rows match the
frozen production prefix exactly. At frame 400, 2,200 fleet steps have built
50,739,303 boxes and visited 7,310,589 refinement windows. Box evaluations at
times 0/0.5/1 total 9,526,789/5,296,626/3,434,588. The zero-time count includes
initial preparation and static neighborhoods; it is not entirely redundant.
Thread CPU time is 31.374 seconds for pair sweeps, 17.839 for ground sweeps,
5.805 for pair neighborhoods, 2.627 for barriers and 3.492 for solving.
Counter/timer overhead makes these diagnostic timings unsuitable for claiming
a gain against an uninstrumented baseline. This 100-second diagnostic is not
full-case acceptance. Receipt:
/tmp/wasm-dd2/rewrite-pair-windows-0046/profile-report.json.

Three private call-local pose reuse designs now preserve all 190 scalar result
fields from the captured actual Native field, including two hits. Four
alternating O3 ASan/UBSan trials of 380,000 pairs compare against the current
frozen batch API rather than the older scalar baseline. Caching only 0/0.5/1
increases query CPU by 4.4..6.7 percent; an eight-entry cache increases it by
4.7..7.8 percent. Separating eight-entry scratch storage from prepared motion
and initializing only written entries passes strict clang-tidy, but increases
CPU by 7.5..10.1 percent on the same field. None is adopted. These timings cover
one field, not the complete scenario. The first eight-entry full attempt still
times out at 360.102 wallclock/250.313 child CPU seconds; all 126 complete rows
match, but summaries and reset do not complete. Receipts:
benchmark-three-report.json, benchmark-report.json,
benchmark-scratch-report.json and candidate8/report.json in the same directory.

The separate scratch-storage candidate's full attempt is also terminal: timeout
at 360.180 wallclock/243.991 child CPU seconds, with all 108 complete prefix rows
matching exactly and empty stderr. Summaries/reset do not complete. Different
CPU availability makes comparing these wallclock attempts unsuitable for a
speedup claim. The three-fixed-pose and first eight-entry designs also fail
strict clang-tidy padding checks; the separate scratch-storage variant passes.
No production C/header, build setting or SoftGL source changes in this diagnosis.
All candidates remain private and 0046 stays active. Receipts:
scratch8/report.json, candidate/tidy-report.json, scratch8/tidy-report.json and
diagnosis-report.json in the same directory.

The separate refinement diagnostic now measures reuse within each pair rather
than across a whole prepared fleet. At frame 400, 12,738,122 of 43,863,534 window
box samples could reuse a previous window or the already prepared initial pose.
A private three-entry pair-local cache actually reduces total box construction
from 50,739,303 to 36,888,795 at that frame, preserving all 60 available rows.
Its captured-field query CPU still increases by 4.8..8.4 percent, and its full
instrumented case times out at 360.110 wallclock/356.004 cumulative child CPU
seconds with 215 matching complete rows and no summaries/reset. It is not
adopted. Eliminating box construction alone is insufficient evidence for a
useful speed improvement. Receipts: profile-report.json,
profile-candidate/profile-report.json, benchmark-local-report.json and
candidate/report.json in /tmp/wasm-dd2/rewrite-refinement-reuse-0046/.

The same actual trajectory identifies a different redundant operation:
21,919,160 of 109,577,906 swept-axis evaluations at frame 400 happen after the
intersection of their time intervals is already empty. Further intersections
can only shrink that interval. Production now returns the unchanged no-contact
result at that point, retaining source-axis order for surviving contacts and
every angular/refinement/response/material bound. There is no cache or new
allocation. Corrected instrumentation measures exactly 87,658,746 axis calls and
zero calls after empty intervals, with identical box/window/world/group counts
and all 80 available production rows matching. Receipts:
axis-profile/profile-report.json and axis-corrected-profile/profile-report.json
in the same directory. Diagnostic child CPU totals inherited from combined
build/run shells are explicitly marked unsuitable for isolated process timing;
the corrected profile and full axis attempt measure a per-process usage delta.

Four alternating O3 ASan/UBSan trials preserve all 190 actual-field results and
reduce query CPU by 19.0..19.5 percent against frozen current batch preparation.
Another 4,096 finite translated/rotated queries, including swapped body order,
match every frozen result field (1,296 hits). Strict LLVM19 format/tidy for 163
C/header files, 37 Native/35 WASM CTests and all 716 physical checks per target
pass. Eight focused instrumented targets pass with all 46 reached rewrite C
units freshly compiled under O1 ASan/UBSan; pinned release SoftGL/system SDL
remain outside instrumentation. Receipts: benchmark-axis-report.json,
frozen-pairs-report.json, native-quality-report.json, wasm-quality-report.json,
physical-report.json and sanitizer-report.json in the same directory.

Current production Native/WASM Arena-B exporters complete within the unchanged
180-second bound (79.48/97.20 seconds), each preserving all 300 preceding rows
byte for byte. Native slot-17 support still fails at 1,929 frames; WASM motion
passes. The production-equivalent axis candidate uses a byte-identical private
copy of the production car-query C unit, instrumenting every reached C unit at
O3. It times out at 360.249 wallclock/319.821 child CPU seconds, with all 215
complete prefix rows matching and empty stderr. Summaries/reset do not complete.
This proves a component gain, not the full deadline contract or whole-game
acceptance. Receipts: production-arena-verified/{native,wasm}/report.json,
axis-candidate/report.json and component-report.json in the same directory.

## Next

Retain the verified partial pair-preparation correction without closing the
full deadline contract. Use the terminal current-production receipt and its
separate child CPU measurement to diagnose remaining pair-window/pose and
world-query work from actual inputs before further changes. The local pose
cache also fails measured adoption criteria. Retain the proved empty-interval
axis rejection without closing the full deadline. Measure per-pair refinement
work and unresolved-query frequency to locate the remaining dense-field cost;
evaluate shared relative-distance bounds only after measuring their actual
repetition. Keep initial rejection cheap, query ownership explicit and stack
use bounded. Prepared road planes and skipped empty intervals are verified local
gains; they do not close the full deadline.
Preserve every existing
acceptance bound and instrument every reached C unit. Keep slot-17 roof riding
under 0016 and actual-owner continuation/public-session seasons separate;
the older independent WASM/instrumented seasons both timed out in round 2.
Capture only typed inputs needed for a proved correction.

## Accept

A causal redundant-work defect and its correction are demonstrated against
frozen actual inputs. The full unchanged Arena-B gate completes under its
original deadline with all reached C instrumented. Native/WASM physical,
material, chronology, ownership and rollback checks pass alongside strict
LLVM19 and required build/test gates. No support/AI/mode/campaign or full-game
contract is closed by a cost measurement alone.
