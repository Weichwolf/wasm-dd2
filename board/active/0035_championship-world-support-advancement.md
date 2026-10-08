Type: Work item
Title: Advance the championship world-support failure after reverse retry
Depends: 0016

## Contract

Resolve the actual round-2 step-83,450 world-contact failure exposed after the
corrected forward/reverse timer. Preserve all physical inequalities, material
acceptance and ordinary driving/damage/lap/score ownership. Verify the saved
owner step, three-target query behavior and subsequent natural continuation.

## Evidence

The corrected controller's natural first Stockcar round retains 287,087 ticks
and its original twenty-driver scores. Round 2 fails at tick 83,450 with valid
AI/frame/championship inputs. Failure-only diagnostics match all 76 production
records and capture one 31,216-byte owner input plus the contact query.

Three static-world contacts involve physical body 14: two distinct upward
road normals and a horizontal wall normal. Velocity exhausts 4,096 passes at
residual 0.000335284234228977; position is not reached. Both the saved actual
owner and the isolated full-field / one-body-remapped query reproduce failure
against unchanged production libraries. A production correction has not yet
been proved.

Receipt: /tmp/wasm-dd2/rewrite-reverse-expiry/failure-capture/report.json.
Replay/query receipts: production-query/owner-replay-report.json and
production-query/report.json within the same capture directory.

A read-only GDB observer uses instruction bytes identical to the production
contact object and reproduces its exact 4,096-pass terminal residual. An
independent geometry/inertia mobility reconstructs terminal contact velocity
within 1.19e-14 and enumerates every normal active set. It finds an admissible
regularized root with the wall released, both road supports loaded, wall
separation speed 0.0899708 and full physical residual below 3.6e-15. The two
road slip speeds straddle the unchanged 0.1 transition (0.173528/0.095204).
This proves a feasible branch, not a production fix or original parity.
Receipts: debug-query/identity.json, debug-query/terminal.json and
independent-root/report.json within the same capture directory.

## Next

Trace the current released-wall refinement through the sliding/linear road
transition. Implement a bounded correction preserving the independently proved
root and physical acceptance. Add this input to focused physical checks,
retain existing captured queries and verify actual owner advancement followed
by ordinary natural continuation. Full campaigns remain under 0025/0004.

## Accept

The actual saved owner advances, the captured query and independent physical
checks pass on Native/WASM/ASan, and the same ordinary natural campaign continues
beyond this failure with normal controls and no state or score injection.
