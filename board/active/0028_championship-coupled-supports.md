Type: Work item
Title: Resolve coupled supports in the second scheduled circuit
Depends: 0027

## Contract

Advance the reproduced Stockcar step at second-round tick 86,643 with the shared
bounded low-speed friction model. Preserve finite state, normal complementarity,
friction cones, dissipative response, impulse accounting, rollback, convergence
tolerances and the shared 4,096-pass budget. Keep regular controls, AI, damage,
laps, championship scoring and race bounds.

## Evidence

The current Native natural season completes the first ten-lap round at tick
287,087 and reaches second-round tick 86,643 before advancement fails. AI, input,
race and championship states remain valid. Player engine health is
0.37294417079115094; zero complete laps are credited on this second circuit.

One 31,216-byte input checkpoint reproduces the actual failing step. Drivers 11
and 13 form a seven-contact group with six world supports and one car pair.
Velocity response exhausts 4,096 passes with residual 0.0034676348145833424;
position correction is not reached. The sparse diagnostic reproduces the
production failure and prior checkpoints exactly. No result or retirement is
manufactured to continue. Source/binary/checkpoint identity and scoped evidence:
/tmp/wasm-dd2/rewrite-season-86643/report.json. The needed input checkpoint and
query are retained. This is not a completed season or original physics parity.

## Next

Freeze the remapped failing query. Inspect coupled contact mobility, Newton
Jacobian conditioning and normal-active set predictions. Verify a general
correction against independent physical conditions for all existing captured
cases on Native/WASM and instrumented C. Follow with natural scheduled racing;
complete campaigns and front-end/save flows remain under their existing owners.

## Accept

The reproduced query and all prior physical fixtures pass on all three targets.
The saved actual input advances and ordinary racing continues past this failure
without state, lap, health or result injection. Mandatory LLVM19/Native/WASM and
relevant actual application checks pass. Close only this coupled-support contract.
