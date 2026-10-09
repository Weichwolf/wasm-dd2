Use English for repository documentation, comments, commit messages and UI text.

Use `/tmp/wasm-dd2/` for verification captures, generated diagnostics and logs.
Never place these outputs in the repository or `deps/verification-artifacts`.
Original provisioned game data and reusable build dependencies may remain ignored
in the repository. `/tmp` is a 16 GiB tmpfs on this machine: keep each verification
run below 2 GiB and leave at least 1 GiB free. Capture only the memory checkpoints
needed for the current diagnosis; do not dump full memory images at every frame.
Delete successful raw comparison output after writing the report. Remove completed
captures when their diagnosis is finished, and run `make clean-logs` before and
after verification work. Do not delete files needed by running processes.

After each successfully verified improvement, commit and push it as requested
by the user. Do not claim original parity for a diagnostic or partial comparison.

On the `master` branch, read `docs/rewrite.md` before implementation. New game
modules are handwritten C11 under `src/`, using typed state and explicit ownership
instead of original absolute addresses or emulated registers. Keep the reference
reconstruction and original game data available for functional comparisons.
`re_out/`, `patches/` and the legacy root `web/` belong only to `ghidra`.
The rewrite browser UI lives in `src/platform/web/`; generated builds stay in
`/tmp/wasm-dd2/`. Use `make reference-prepare`
and frozen reference commands under `/tmp/wasm-dd2/`, never recreate them on `master`.
The rewrite may improve graphics and audio; bitidentical original output is not
an acceptance requirement. Functional correctness and native/WASM stability are.

The expanded goal is a fully standalone game. Native and WASM must build,
launch and provide every game feature without original DD2 game files, Dirinfo,
sound banks, CD images or Redbook files. Original data/reconstruction is optional
development/reference evidence only; it must never be a required runtime input,
distributed dependency or fallback that masks missing replacement content.

Rebuild all game assets and keep their editable sources and generated runtime
assets in the repository under `assets/`. Use Blender for models/scenes, including
complete cockpits/interiors; generate textures procedurally and create new audio
(effects, engines, ambience, commentary and composed music). Include vehicles,
tracks, scenery, characters/props where needed, fonts, UI and visual effects in
the replacement inventory. Authored/generated game content belongs in Git;
temporary baking/build diagnostics and verification captures still belong in
`/tmp/wasm-dd2/`. Keep generation scripts, parameters and exports reproducible.
Do not treat decoded/extracted original assets as replacement content.

Target a substantial quality improvement over the original in every area,
including geometry, materials, cockpit presentation, resolution, lighting,
shadows, effects, UI and audio. Judge actual player-visible/audible results on
both targets with concrete before/after evidence and performance measurements.
Functional similarity or passing physics tests alone cannot close this contract.

Use the pinned `deps/softgl` submodule by default. Do not alter SoftGL sources
as part of a game change; deliberate dependency changes need their own evidence.
Plan authored scenes against a 640x360, 4x MSAA, four-total-render-thread profile
targeting 60 FPS (16.67 ms for the complete frame) on this machine. Start with
100,000-200,000 submitted triangles and 20-30 materials per frame, retaining
headroom for overdraw, alpha tests and expensive materials. These are planning
budgets, not measured throughput guarantees. Measure Native and actual browser
frames including simulation, resolve/readback and presentation; include the
calling thread in the render-thread count. Keep asset detail scalable with LOD
and visibility rather than assuming every detailed model is visible at once.
All rewrite C/header files must pass clang-format 19. All rewrite C units must
pass strict clang-tidy 19 and the compiler flags defined in `CMakeLists.txt`.
Fix findings instead of disabling checks or adding blanket suppressions. Run
`make rewrite-check`, `make rewrite-wasm` and `ctest --preset rewrite-wasm` for
changes affecting the shared build/renderer. Preserve functional comparisons
and add focused checks when a new gameplay subsystem is implemented.

Treat visual quality as a primary acceptance criterion. Inspect actual Native
windows and browser canvas captures with real input, including relevant moving
gameplay, camera, vehicle/texture appearance, HUD/results and menu readability.
Pixel regressions complement human visual inspection; numerical physics checks
alone do not establish an acceptable player experience. Record concrete visual
findings and follow-up work on the board, and remove completed captures after
writing the review evidence.

Maintain `board/README.md` and the current work item. Directory determines work
item state (`open/active/closed`); preserve IDs and update evidence, next steps
and acceptance after each verified improvement. Close only the proved contract.
