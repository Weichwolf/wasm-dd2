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

## Next

Use the terminal negative candidate receipt before choosing production work.
Measure remaining repeated pair validation, pose construction and neighborhood
search against actual inputs. Consider bounded, typed fleet query preparation
that validates each immutable body/motion once and reuses geometry across its
pairs, preserving all public invalid-input behavior and full instrumentation.
Prove output equivalence and the unchanged full-scenario deadline before
adoption. Keep slot-17 roof riding under 0016 and
actual-owner continuation/public-session seasons separate. Capture only the
typed inputs needed for a proved correction.

## Accept

A causal redundant-work defect and its correction are demonstrated against
frozen actual inputs. The full unchanged Arena-B gate completes under its
original deadline with all reached C instrumented. Native/WASM physical,
material, chronology, ownership and rollback checks pass alongside strict
LLVM19 and required build/test gates. No support/AI/mode/campaign or full-game
contract is closed by a cost measurement alone.
