# Readable C rewrite

The `rewrite` branch is the new implementation. `master` retains the executable
reconstruction; the annotated `reconstruction-baseline` tag records the exact
starting point (`b1111bd`). That reference has known incomplete full-game
coverage. It is evidence for behavior, not a claim that every feature is correct.

The active worktree is `/home/cosmo/Git/wasm-dd2`; the reference worktree is
`/home/cosmo/Git/wasm-dd2-reference`. The local reference shares immutable
provisioned assets through ignored symlinks and has its own `SaveGames` copy.
Never modify original assets through those links. Elsewhere, provision the game
in each reference checkout using its existing `make provision` target.

## Boundaries

New game code is C11 with meaningful types, explicit ownership and named
functions. Game code must not depend on original absolute addresses, emulated
x86 registers or the global memory image. Keep the decompilation, patches and
original comparison tools as reference material during migration.

| Directory | Responsibility |
| --- | --- |
| `src/game/` | Game state, menus, race/season transitions, results and replays |
| `src/physics/` | Vehicle motion, collision and damage |
| `src/ai/` | Opponent behavior and driving decisions |
| `src/render/` | SoftGL adapter, camera, geometry, materials and visual effects |
| `src/audio/` | Effects, CD music, mixing and playback state |
| `src/assets/` | Typed loaders for original tracks, cars, textures and data |
| `src/platform/` | Native/browser windows, input, timing and persistence |
| `tests/rewrite/` | Functional, rendering and platform regression tests |

The renderer adapter has an actual triangle/pixel/lifetime bootstrap. The first
asset module validates the original `Dirinfo` archive and exposes borrowed file
views; all 114 original entries have been checked on native, Node/WASM and
ASan/UBSan against an independent reader. Typed level sections, signed road
vertices, UV definitions and all original texture pages are also decoded and
checked for all 13 level containers. A real SoftGL texture-upload/cutout pixel
test runs on both targets. PAL colors use BGR source order, checked against all
256 loaded entries in an unmodified original race under Wine. See
`src/assets/README.md` for format and ownership
details. Compressed scene blocks, object placements and polygon meshes are now
decoded into owned C structures and independently checked for all eleven playable
level containers, including car/sky/wheel shapes. SoftGL now renders stored mesh
geometry with texture/material selection, cutout and depth testing; static scene
and car previews run for all eleven levels on both targets. Material opacity
uses original UV/CLUT selection, and the preview camera preserves positive Y
upward. Scene vertices use their static raster-cell or local object origin,
separate from the retained bounding center. Owned road graphs and lane cells now
cover all eleven playable tracks/arenas, with vertical triangle contact heights
and normals. A balanced spatial index now selects the highest road contact
within a height window, with stable shared-edge ties and finite-input validation.
The vehicle core now advances a four-wheel rigid body with suspension, traction,
steering, braking, reverse and vertical landings in fixed steps. Driving
presentation now connects that core to a perspective chase camera,
full body orientation and four suspended/steered/rolling wheel models.
Source track barriers and analytic arena boundaries now participate in free
driving, with continuous collision sweeps, restitution, friction and angular
response. Body/ground support now uses the original eight-corner contact box,
with one-sided swept road contacts and a shared ground/barrier solver.
Car-pair collisions now share the ground/barrier event clock for a 20-car starter
field, with swept oriented boxes and equal/opposite impulses. Six regional crush
zones now reduce engine power, retire cars with exhausted front zones and deform
the rendered body; a player HUD shows regional damage and engine health. Accident
attribution now credits 90/180/360-degree spins and destroyed opponents, with
visible points/destruction counts. Racing levels now track source-equivalent
checkpoints, laps, individual completion and lap times for all twenty cars, with
a lap/finish HUD. Wrecking/Stockcar practice races now have a source-timed
start countdown, persistent finishing order, retirement/survival endings,
an explicit coasting phase and frozen twenty-driver results. Time Trial now uses
one physical car on all seven circuits, continuous laps, current/last/best
lap clocks and timed results on withdrawal or engine retirement. Detached parts,
smoke, billboard orientation, lighting and blending remain to be implemented. A shared C application now presents the
eleven track/car views in a native SDL window and a browser canvas, with orbit,
tilt, pan, zoom, reset and track/view selection. The same application now offers
free driving on all eleven levels, using
all twenty source-grid positions with road-aligned initial orientation, fixed-step frame
accumulation, pause/reset and focus-loss suspension. Other cars are rendered and
respond to impacts. Opponents now follow source road paths, brake for curves,
avoid traffic and pursue other cars in arenas, including timed reverse maneuvers.
Damage-aware tactics, off-road recovery, Total Destruction,
championships and the complete original front end remain pending.
Supported overturned cars now have a separate temporary availability state and
source-timed two-second recovery, with distance-gated opponent righting. Landing
preserves damage, progress and physical clocks and refreshes real road support.
See `src/game/recovery.md`. This is not yet a complete racing game.

## SoftGL

`vendor/softgl` is a submodule pinned to
`7963be1d5b5e1bebbe97ece2c655228c8bc0a838`, the latest published upstream
version checked during preparation. Subsequent upgrades remain explicit and
reproducible. To initialize it:

```sh
git submodule update --init --recursive
```

For local library development, configure with
`-DDD2_SOFTGL_DIR=/home/cosmo/Git/softgl`. Published verification uses the pinned
submodule, and deliberate updates must include renderer regression evidence.
The renderer and rewrite are C11. Meshoptimizer belongs to SoftGL's separate
offline asset-preparation tools and is not linked into either runtime. The
rewrite build does not enable a C++ compiler or build those tools.
The initial WASM bootstrap uses SIMD and a prestarted eight-worker pthread pool,
covering SoftGL's maximum render pool. Native/browser presentation uses SDL2
software surfaces, with shared keyboard input and monotonic timing. Gamepad,
persistence and audio remain pending. Browser pthread builds
need HTTPS/localhost and COOP/COEP response headers.
Physics consumers reserve a 256 KiB WASM stack for the transactional twenty-body
solver and nested geometry queries; the SDK's default 64 KiB is insufficient.

## Build and mandatory quality gates

Use CMake 3.25+, Ninja, Python 3, LLVM 19 (`clang`, `clang-format`, `clang-tidy`),
Emscripten, Node and SDL2 development headers. Debian's package names are
`cmake ninja-build python3 clang-19 clang-format-19 clang-tidy-19 emscripten
nodejs libsdl2-dev`. The window verifier additionally needs Pillow, Xvfb,
xdotool, Chromium and Playwright (the repository's browser launcher supports
Debian's system Chromium).

```sh
make clean-logs
make rewrite-check
make rewrite-wasm
ctest --preset rewrite-wasm
make clean-logs
```

`make rewrite-play LEVEL=1` opens the native track/car/free-driving application. `make rewrite-web`
serves the browser viewer at `http://127.0.0.1:8080/`; select the provisioned
`DestructionDerby2/Dirinfo` file in the page. Assets remain local and are not
included in the browser distribution. Arrow keys orbit/tilt, WASD pans, plus/minus
or the wheel zooms, R resets, Tab switches track/car, Page Up/Down changes level,
and Escape closes the view. Enter starts/exits free driving; W/Up gives gas,
S/Down reverses, A/D or Left/Right steers, Space brakes, P pauses and R returns
to the settled grid start. The browser has equivalent selection/pause/reset
controls and can reopen the archive after closing. The damage icon points forward
upward: green/yellow/red indicate increasing regional crush, and the lower bar
shows remaining engine health. PTS shows accident points and KO shows credited
destructions. Reset restores all twenty cars and clears their damage, scores and
attribution windows. Racing tracks also show LAP current/required at the upper
left, followed by FIN when that driver completes the required laps. The initial
grid approach starts lap 1 without crediting a complete lap; reverse finish
crossings cannot earn another lap. Arenas omit the lap display. F5 starts Wrecking
Racing on a circuit or a destruction arena; F6 starts Stockcar on a circuit.
F8 starts Time Trial with one car and unlimited laps on a circuit. The browser
view selector offers the same modes. The two-second start countdown
holds the settled field and all physics/lap/damage/accident clocks until GO;
P and loss of focus freeze the countdown as well as an active race. Road position
uses credited laps and finish-relative progress; finish places latch on each
crossing tick, with prior position resolving simultaneous crossings. Player
completion or retirement ends a circuit session. Arenas end on player retirement
or fewer than two available cars with functioning engines; a supported resting
overturn makes a car temporarily unavailable without engine retirement. A tuned three-second coasting
phase allows later finishers to cross before results freeze. F7 or the browser's
race-exit button publishes provisional DNF results immediately. R restarts the
same field/countdown; Enter returns to inspection, and selecting Freifahrt clears
the race rules. Stockcar awards original position bonuses (100/75/50/.../0);
Wrecking adds actual simulated accident points to the original 50/25/10 circuit
bonuses, capped at 999. Arena results use accident points without circuit bonuses.
No randomized post-race NPC score or progress boost is manufactured. The result
list sorts total points, breaking ties by road/survival position; the player is
highlighted yellow. These are practice results; season totals are still pending. Time Trial displays
CURRENT, LAST and BEST in MM:SS.mmm (5 ms resolution, display capped at
99:59.995), excluding the partial grid approach. A complete lap updates last/best
without ending the session. Retirement freezes the current timer before the
coasting phase; F7 retains the session times immediately. Time Trial awards no
placement/accident points. R clears the session record and restarts the countdown.
Returning to Freifahrt or a circuit race restores twenty cars and the original
finite lap count; selecting an arena changes to Wrecking. Persistent track
records, their original file compatibility and the full front end remain pending.

The WASM builder uses a private writable ports cache under
`/tmp/wasm-dd2/emscripten-cache/`, including with Debian's frozen system SDK.
Existing system headers/libraries are reused through file links; SDL is built
privately. An explicit `EM_CACHE` overrides this default. The SDK package files
are not modified. Builds and reusable dependencies remain outside the source tree.

`make` builds the native rewrite. Existing explicit reference targets, such as
`make native`, `make wasm` and the original verification targets, remain
available. They build the reconstructed engine, not the new implementation.

All rewrite C and header files under `src/` and `tests/rewrite/` are covered by
`make rewrite-format-check`; `make rewrite-format` applies formatting. Both
default CMake builds require the format gate before compiling the rewrite. Native
targets run clang-tidy by default, and `make rewrite-tidy` independently checks
every translation unit against `compile_commands.json`. LLVM 19 is required so
formatter changes do not silently alter the style. Analyzer, bugprone, CERT,
miscellaneous, performance, portability and readability findings are errors.
Do not add suppressions or `NOLINT` merely to make a gate green.

The requested compile flags apply to the rewrite and SoftGL:

```text
-Wall -Wextra -Wpedantic
-Wno-unused-parameter -Wno-unused-function
-fno-strict-aliasing -ffast-math
```

Rewrite targets additionally use `-Werror -Wshadow -Wconversion
-Wstrict-prototypes -Wmissing-prototypes -Wformat=2`. Vendor and reference code
retain their own quality rules. Fast-math permits arithmetic changes; new physics
must be verified for stable behavior on native and WASM, including edge cases.

CI on `rewrite` checks formatting, strict native build/analysis and both renderer
bootstraps, plus archive/level/texture bounds and texture rendering checks without proprietary data.
With the original data provisioned, `make rewrite-archive-verify` also compares
all archive entries on both targets and an ASan/UBSan build.
`make rewrite-level-verify` independently compares every level section extent,
decoded vertex/UV field, the full 2 MiB index atlas and all 32 RGBA pages for each
of the 13 original level containers. These checks cover data decoding and renderer
plumbing; complete scene rendering and gameplay coverage are still pending.
`make rewrite-mesh-verify` compares every decoded object placement, mesh vector,
normal and polygon field in all eleven playable levels on native, Node/WASM and
ASan/UBSan, and rejects six corrupted original scene/mesh streams.
`make rewrite-scene-verify` renders scene/car previews for all eleven playable
levels on native, Node/WASM and a sanitized rewrite build, checks visible output
and cross-target image consistency, and removes completed raw frames. Its scope
is static scene meshes and neutral palette shade with selected cutout, without
billboard orientation, lighting or blending. Free driving is checked separately;
a complete race remains pending. See `src/render/README.md`.
`make rewrite-road-verify` checks every playable road graph and contact cell
against an independent original-data reader on native, Node/WASM and ASan/UBSan.
It includes source attributes, links, missing edge triangles, grid geometry and
vertical plane samples, plus invalid-link/count/vertex/branch-cycle rejection.
This is contact geometry coverage; the separately tested suspension/vehicle core,
off-road recovery and
lap/checkpoint equivalence are outside this geometry check.
`make rewrite-surface-verify` additionally compares indexed contact selection
against an exhaustive cell search for 197,750 queries per target across all
eleven playable levels, including height windows and shared corners. Native,
Node/WASM and ASan/UBSan agree on selection and work counts. Synthetic checks
cover stacked surfaces, tolerance chains and nonfinite inputs under fast-math;
the original-data verifier requires at least 95% pruning of contact tests. See
`src/physics/README.md` for ownership and tie rules. This index supplies the
vehicle simulation used in free driving.
`make rewrite-vehicle-verify` exercises the vehicle core for 264,000 fixed steps
per target, using 24 distributed starts on every original track/arena. It
compares sampled positions, velocities, orientations, steering and wheel state
on native, Node/WASM and ASan/UBSan. Synthetic tests cover independent rest-height
calculations, acceleration/braking/reverse, steering, grip, slopes, flight,
bridges, fast landings and invalid-input rejection. Parameters are rewrite tuning;
this establishes the simulation core, not original driving parity or a complete
race. Vehicle rendering/input/chase-camera integration is checked separately;
Barrier, body/ground and car-pair response are checked separately; the regional damage core is tested separately. See `src/physics/README.md`.
`make rewrite-window-verify` compares all eleven actual native window and browser
canvas track/car views against the shared C preview, exercises real keyboard and
wheel events, focus-loss release, camera reset, selection synchronization,
native resizing/letterboxing, invalid browser archives and close/reopen. The
native lifecycle also runs with ASan/UBSan-instrumented rewrite C modules;
system SDL2 and pinned release SoftGL remain uninstrumented. The
native framebuffer comparison is exact; cross-target comparisons use the same
recorded SIMD edge bounds as the scene verifier. This establishes inspection/free-driving behavior,
not original rendering parity or complete-race coverage.

`make rewrite-driving-verify` checks fixed-step accumulation, equivalent frame
partitions, reset, pause and invalid timing/control rejection on both targets.
An independent original-data reader checks all eleven first-grid positions,
including generated alternate-branch IDs and the arena height map. Before/after
acceleration snapshots compare pose/contact state and perspective images on
native, Node/WASM and ASan/UBSan. The actual window/browser verifier additionally
checks all eleven paused starts, real throttle events, pause/focus-loss freezing,
reset, track wrapping and inspection-mode restoration. This covers free driving;
it does not establish original handling parity or complete races.

`make rewrite-barrier-verify` checks all 6,492 source collision segments and four
analytic arena radii against an independent original-data reader. For each target,
13,048 swept queries compare indexed earliest contacts against exhaustive shape
intersection, including the height separation between bridge levels. Native,
Node/WASM and ASan/UBSan agree on contact selection and work counts. Synthetic
checks cover fast head-on/glancing impacts, distant endpoint tangency, overlap
correction, escape, angular travel, energy loss and invalid-state rollback.
The game owner invokes barrier response after every fixed vehicle step; the
browser additionally uses a real reverse key to hit an arena boundary and checks
that reset clears accumulated collision events. Rounded body proxies, road wall
height, restitution and friction are rewrite tuning. Body/ground contact is
checked separately; detached parts and a complete race remain pending.

`make rewrite-ground-verify` independently checks the eight original body-contact
corners and short/fast sweeps at both triangle centroids of all 19,775 road cells
on Native, Node/WASM and ASan/UBSan. Four original-grid drops per level exercise
upright, inverted, sliding and spinning bodies; existing free-driving window
and browser checks run as well. Ground and barriers now share earliest-contact
response, including during reset settling. Body corners and bounded angular
chords form a collision proxy; this is not exact mesh collision or original
handling parity. See `src/physics/README.md` for solver bounds and tuning.

`make rewrite-fleet-verify` independently reconstructs all 220 starter slots from
the original road graphs, lane quads, headings and special SCA grid. Arena layouts
use continuous trigonometry rather than original integer sine/height rounding.
Three-second drives exercise all twenty bodies on every playable level, comparing
132,000 vehicle steps per target on Native, Node/WASM and ASan/UBSan, with complete
reset restoration. Synthetic checks cover fast frontal and glancing collisions,
rotating contacts, three-car impulse transfer, bridge separation, energy loss and
transactional rejection. Browser checks use a real reverse key to hit another
car and verify counter reset. This establishes starter-field collision behavior.
Stationary-field comparisons explicitly disable driving decisions. Each 5 ms
step also checks the full typed contact report: source obstacles, partners,
chronology, local contact points and reduction back to aggregate impacts. Reports
publish with all validated bodies; repair-only contacts carry no impulse. Distinct
vehicle classes, liveries, detached parts and race rules remain pending.

`make rewrite-ai-verify` independently checks source-linked guidance from every
original racing strip, including branches and loop wrap, with three lane fractions
and three lookahead lengths. Native, Node/WASM and ASan/UBSan also run sixty-second
twenty-car scenarios on all eleven levels: 2,640,000 vehicle steps per target.
Checks cover supported movement, decision cadence, reset, stalled reversing and
frame-partition independence of every vehicle/controller. Real native/browser
driving checks run too. Controller tuning and floating-point-sensitive traffic
choices can produce different encounters across targets; these long guidance
scenarios disable damage to isolate controller behavior. This verifies behavior,
not original AI parity, exact cross-target trajectories or complete race rules.
See `src/ai/README.md` for parameters and remaining tactics/recovery work.

`make rewrite-accidents-verify` checks source-inspired 10/25/50 spin points, 25
destruction points, the 999 cap and first-contact attribution on both targets.
Each victim's 1.5-second window remains attached to the first responding partner;
subsequent contacts do not steal or extend it. Retired instigators cancel credits.
Six-second twenty-car drives on all eleven original tracks/arenas supply poses,
retirement flags and actual pair contacts to an independent Python angle-history
oracle. It checks every scoring state through 264,000 vehicle steps per target
on Native, Node/WASM and ASan/UBSan, including reset. Real browser input checks
active attribution, pause and reset; SoftGL pixel tests check the PTS/KO display.
Two controlled side-impact cases on original arena 8 additionally verify actual
180/360-degree point awards and engine retirement/destruction credit through the
physical solver, damage and controls (16,000 further vehicle steps per target).
Assigned high initial speeds are diagnostic stress inputs. The ordinary short
drives exercise attribution/expiration and can finish without point awards.
This verifies tuned rewrite accident rules, not original physics/damage parity,
lap/race rules, championship standings or complete racing gameplay.

`make rewrite-laps-verify` independently reads all 3,246 racing strips and the
original finish/default-lap table. It checks progress equivalents on Native,
Node/WASM and ASan/UBSan, including longer-branch shared units and nested nodes.
All 32 combinations of main-route branch choices complete the required laps
through ordered source-cell rule traces, with first-crossing, lap timing and
finish flags. These traces are rule/data checks, not physically driven complete
races. Six-second twenty-car physical scenarios on every racing level check
168,000 vehicle steps per target against an independent geometric road-contact
and sequential-checkpoint oracle; a second field checks equivalent frame
partitions. Synthetic tests cover reverse/re-crossing, missing units, unsupported
contacts, retirement, completion freeze and transactional rejection. Actual
native/browser windows verify the lap overlay, real start-line crossing, paused
timing and reset; pixel tests check LAP/FIN glyphs. Complete races and results
are covered by the separate race verifier.

`make rewrite-race-verify` checks all eleven original levels in Wrecking mode
and all seven circuits in Stockcar mode on Native, Node/WASM and ASan/UBSan.
Each short physical scenario holds the complete field during countdown, drives
six seconds, checks every driver's geometric checkpoint/lap progress and race
position against independent Python oracles, then verifies DNF publication,
result freeze, frame partitioning, pause/rejection and full reset. Independent
readers supply the original lap/course and both placement-point tables. Fourteen
ordered twenty-driver source-cell scenarios complete every circuit/mode through
lap rules and coasting results; these are rules traces, not physical races.
A separate complete Stockcar race on original circuit 5 uses the real twenty-car
physics, AI, damage, accidents, checkpoints and race owner through all eight
required laps and result publication. The existing AI supplies only ordinary
player control inputs; no position, lap, damage or race-state injection is used.
An independent geometric oracle checks the player at every fixed step; final
field placement/scoring is checked against crossing ticks and source tables.
This establishes one physically completed circuit race, not complete gameplay or
all-track completion. Additional complete races, overturned-car elimination,
recovery, Total Destruction and championships remain to be implemented
or verified. Native/browser window checks cover all eleven countdown/results
views, real mode/exit keys and selectors, held throttle until GO, pause, frozen
results, invalid arena Stockcar rejection and restart. SoftGL pixel tests check
source-timed red lights, GO, the yellow player result row and exact Time Trial
timestamp pixels, including punctuation. Seven additional short single-car
scenarios check Time Trial on every circuit, with pause/results/reset and
restoration of the twenty-car finite-lap field. A long physical Time Trial on
circuit 5 completes nine laps, continuing beyond its original eight-lap race
limit. Independent geometry/checkpoint and lap-clock oracles check every tick
on Native, Node/WASM and ASan/UBSan. Native/browser checks cover all seven
single-car starts and timed result views, F8/selector entry, real throttle,
paused clocks, arena rejection and finite-rule restoration. This verifies
session timing; persistent records and full-game coverage remain open. Source-timed lights
are currently visual; sound cues and music remain pending.

## Migration and acceptance

First decode original assets into documented structures, then render one
original track/car through SoftGL. Add native and browser presentation/input,
then one complete playable race with physics, opponents, effects and music.
Expand to all tracks, modes, menus, championships, replays, settings and
save/load, using the reference and original to resolve uncertain behavior.

Keep simulation independent of rendering and give it explicit input and timing.
Functional tests cover rules, outcomes, navigation and data compatibility.
Renderer tests cover the new renderer's intended output; original framebuffer
and PCM equality are not acceptance requirements for this rewrite. Make visual
improvements incrementally: resolution/filtering, materials/lighting, shadows
and effects, with image regression and performance evidence on both targets.

After each verified improvement, commit and push. Captures, generated build
output and logs stay under `/tmp/wasm-dd2/`, below 2 GiB per verification run
with at least 1 GiB free. Clean logs before/after verification, retain reports
and remove completed raw captures without deleting files in use.

## Goal

Implementiere im Branch `rewrite` von `/home/cosmo/Git/wasm-dd2` eine vollständig
spielbare, gut lesbare und modular aufgebaute C11-Neuimplementierung von
Destruction Derby 2 für Native und WebAssembly mit der gepinnten SoftGL-Bibliothek
als Renderer; nutze `master`, den Tag `reconstruction-baseline` und das laufende
Original als Referenzen für Spielverhalten und Datenformate, ersetze schrittweise
absolute Speicheradressen und Registeremulation durch dokumentierte Typen und
klare Schnittstellen, implementiere und prüfe alle Strecken, Fahrzeuge, Physik,
Schadensmodelle, KI, Spielmodi, Menüs, Rennen, Meisterschaften, Wiederholungen,
Einstellungen, Tastatur- und Gamepad-Steuerung sowie Speichern und Laden
einschließlich relevanter Randfälle, stelle funktionierende Effekte und
Redbook-Musik sicher und verbessere die Darstellung kontinuierlich durch höhere
Auflösung, bessere Texturen, Beleuchtung, Schatten und Effekte; Bitidentität von
Bild und Ton zum Original ist keine Anforderung, funktionale Korrektheit,
Stabilität, Datenkompatibilität und gutes Spielverhalten auf beiden Plattformen
sind verbindlich, ebenso strenges clang-tidy und clang-format mit LLVM 19, die
vereinbarten Compilerflags, reproduzierbare automatisierte Prüfungen, begrenzte
Diagnostik unter `/tmp/wasm-dd2/` und Commit plus Push nach jedem verifizierten
Fortschritt, bis sämtliche Spielfunktionen umgesetzt und keine bekannten
Funktions- oder Kompatibilitätsfehler mehr offen sind.

`make rewrite-recovery-verify` exercises supported rest/righting boundaries and
220 controlled physical roof-down drops per target at every original grid slot,
with independently decoded source-plane and wheel contact checks and subsequent
acceleration. It covers Native, Node/WASM and ASan/UBSan. These are recovery
subsystem checks, not naturally completed arena races or original handling parity.
