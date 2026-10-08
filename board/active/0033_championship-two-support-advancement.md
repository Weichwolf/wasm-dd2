Type: Work item
Title: Advance two retained world supports in scheduled racing
Depends: 0032

## Contract

Advance the production-equivalent second-round Stockcar failure at tick 233,358
with finite state, normal complementarity, regularized friction, dissipative
response and exact impulse accounting. Preserve transactional rollback, material
laws, ordinary pass bounds, final tolerances and ordinary controls, AI, damage,
laps and scoring. Diagnose the actual failed stage before choosing a correction.

## Evidence

The terminal Native production and logging-only natural attempts match every JSON
record. Round 1 retains 287,087 ticks and its previous twenty-driver scores.
Round 2 crosses the closed 0032 failure and reaches 233,358, then fails velocity
after 4,096 passes at residual 0.0003393443336907609. Position repair is not reached.
AI, frame and championship state are valid; race/season stay RACING, player health
is 0.40374459157758924 and credited laps are zero. Both actual saved-owner replays
reproduce the failed step without advancing. This is a failed full-season attempt.

The sole checkpoint is 31,216 bytes, SHA-256
d992483fc775d162008674ead942f4a57aeee51e0d4f3c6d1660b5dbb4c27ec0.
Drivers 2/10/14/16/17/18 form fourteen contacts: eight world and six pair.
Query SHA-256: db14009c71b3156ce220a6ad246797f46f0386f77c809376fd5b6495bf91b161.
Failure receipt: /tmp/wasm-dd2/rewrite-season-after-220415-long/report.json.

The unchanged production library also rejects both isolated full-field and
six-body remapped queries. Additional inspection code instead converges in
3,073 passes; those successes cannot prove production advancement. Read-only GDB
inspection obtains the faithful terminal workspace from a debug build whose
35,633-byte solver text is byte-identical to the original diagnostic object.
Its residual matches the actual owner exactly, with one restart and 100 accepted
accelerated corrections. No debugger writes alter the query or solver state.

Independent geometry/inertia response reconstruction agrees with terminal motion
within 2.9e-13. Branch enumeration finds one admissible root retaining wall rows
2/5 and releasing rows 3/4. Released normal speeds are about 0.000203/0.000406;
the retained constitutive matrix condition is about 5,165. A separate finite-
difference root agrees within 5.4e-15, with equation error below 3.6e-14 and all
normal inequalities/friction cones checked. The first three wall points are
collinear; the fourth departs by about 0.02745 in Y. Releasing every matching
support except one gives penetrating contacts; this group needs two retained
supports. This is equation-level evidence, not original-output parity.

A private separating-only patch direction preserves the previous 410 physical
cases but still fails the saved owner and new canonical query. A private bounded
two-endpoint patch direction preserves those cases and passes all 28 rotations/
reversals of the new fourteen-contact query. It advances the Native saved owner
to 233,359; the canonical query uses 513 velocity/83 position passes. Production
sources are unchanged. Native/WASM/sanitizer correction gates and natural
continuation past this new failure remain unproved.
Diagnosis and private-ablation receipt:
/tmp/wasm-dd2/rewrite-season-after-220415-long/diagnosis.json.

Production C now compares each matching patch's single retained support with a
second support at the farthest finite matching point. Both normal equations stay
active, with exact zero targets for the other matching rows. The existing branch
directions and physical acceptance remain intact; bounded storage permits four
models per contact plus two base models. The captured six-body query and all 28
rotations/reversals join the previous physical regression set.

Strict LLVM19 for 157 owned C/header files, all 34 Native and 32 WASM CTests pass.
Twenty-nine captured queries and 410 orderings pass on Native/WASM/ASan/UBSan,
with twenty analytic friction and seventy-two analytic position checks per target.
Native and instrumented production C advance the actual saved owner to 233,359.
The canonical query uses 513 velocity/83 position passes, one restart, velocity
error below 7.6e-15 and position error below 8.0e-10. The Native static solver frame
is 356,168 bytes; actual WASM physical fixtures exercise the reserved 1 MiB stack.
This verifies the saved-input/material correction, not complete seasons or the
still-running broader actual-input/original-data checks.
Component receipt: /tmp/wasm-dd2/rewrite-endpoints-233358/component-report.json.

All eleven actual Native/sanitized/Chromium window/input regressions now pass.
Original-data ground checks pass all eleven levels on three targets, with 79,100
queries and 26,400 physical steps per target. Six scoped race targets pass:
Circuit-2/live and the naturally completed eight-lap Circuit-5 race on each
variant. This is partial race-suite coverage. Successful functional raw output
is removed after storing result/source/binary/image hashes.
The fresh natural production and failure-only companion retain identical round-1
results and exact round-2 prefixes, but have not yet proved continuation past
233,358. The production Python launcher terminated while its child remained
alive; read-only GDB adopts that same identified process without restarting or
writing game state and preserves the original wallclock allowance. Its terminal
exit is still pending; the launcher status does not establish a game failure.
Verified scope: /tmp/wasm-dd2/rewrite-endpoints-233358/gates-report.json.

## Next

Continue the natural production and failure-only diagnostic attempts,
keeping their exact prefixes and source/binary identities. Preserve unchanged
physical bounds and ordinary controls; record its terminal outcome separately.

## Accept

The actual saved step advances with production code on reached targets.
Independent physical checks, existing captured queries and ordering regressions
pass on Native/WASM/ASan/UBSan with mandatory strict gates. Actual input and
original-data behavior remain valid. Ordinary natural racing passes this failure
without position, damage, lap or result injection. Close only this advancement
contract; complete seasons and all-game acceptance require separate full evidence.
