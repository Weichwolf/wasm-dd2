# Readable C rewrite

## Expanded standalone scope

The required product runs completely without original DD2 game files on Native
and WASM. All game content is rebuilt and versioned in `assets/`: Blender models
and scenes with complete cockpits/interiors, procedurally generated textures,
new effects/engine/ambient/commentary audio and newly composed music, plus all
fonts, UI and visual-effect assets. Keep editable inputs, generation scripts and
runtime exports reproducible. Original decoded/extracted content does not count
as replacement content. Original files and `ghidra` remain optional development
references and must not be shipped, requested at startup or used as a hidden
fallback for missing authored assets.

Visual and acoustic quality must substantially exceed the original in every
area. Review actual gameplay and cockpit views, materials, lighting/shadows,
effects, menus/HUD and audible output on both targets, with before/after evidence
and measured performance. Functional rules remain mandatory; original pixel,
PCM and asset-byte identity are reference diagnostics rather than product
acceptance requirements.

This is the expanded target, not implemented standalone acceptance. The current
application still requires original Dirinfo and sound-bank/CDDA content; the
reference-dependent component evidence below describes that current migration
state. Work item 0062 owns the asset inventory, Blender/procedural/audio pipeline
and removal of all mandatory original runtime inputs. Preserve existing useful
functional comparisons as optional reference checks while making the default
build, launch and acceptance use the replacement content.

The initial authored pipeline now commits one editable Racer R1 vehicle/cockpit,
three owned float-mesh LODs, six procedural 1024x1024 material sets and twelve
new PCM clips including an original music arrangement. Independent generation
reproduces all 38 runtime files without original input; geometry/material/PCM
checks and direct Blender-image review pass their bounded offline scope.
Owned C mesh/PNG loaders now validate all three LODs and eighteen maps on Native,
Node/WASM and fresh O1 ASan/UBSan. Every typed mesh field/index re-encodes exactly
after releasing source bytes; every decoded PNG sample agrees with Pillow.
The original-free authored vehicle preview now renders all LODs and the cockpit
at 640x360/4x MSAA in real Native/browser windows, with camera/detail/pose/reset
and close controls. It uses batched geometry, filtered albedo mips, basic lighting
and transparency. Actual images remain too dark in the cockpit and lack the
required material/environment/shadow quality. The default game still does not
select this content, and full gameplay/audio/60-FPS acceptance remains open.
See `assets/README.md`,
`assets/inventory.json` and active 0062 for evidence and missing integration.

The `master` branch is the new implementation. `ghidra` retains the executable
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
| `src/assets/` | Owned authored-content loaders and optional original reference decoding |
| `src/platform/` | Native/browser windows, input, timing and persistence |
| `tests/` | Functional, rendering and platform regression tests |

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
Owned car material bindings now cover all twenty stable drivers and the three
human class paint families. The actual fleet renderer uses individual palettes
and number sprites; static car preview uses the selected class. Regional deformation retains
the selected livery. Unmodified original high-detail remapping matches all
driver/class bindings on eleven levels; cross-target images separately check
rendering and intact/damaged/restored ownership. The human now selects Rookie,
Amateur or Pro outside play; NPCs use original Pro handling. Owned class identity
survives mode/track changes and session restarts. Original drive scaling, rear
grip and drive-dependent front/rear lateral weights operate inside the tunable
rewrite rigid body. Displayed acceleration/speed/grip ratings remain separate
from physical multipliers. Full class race/damage coverage and detached parts
remain under 0005; this is no original force or pixel parity claim.
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
lap clocks and timed results on withdrawal or engine retirement. Total Destruction
now starts twenty cars in each arena, keeps every opponent targeting the player
and displays surviving engines and the player survival clock through results. Detached parts,
smoke, billboard orientation, lighting and blending remain to be implemented. A shared C application now presents the
eleven track/car views in a native SDL window and a browser canvas, with orbit,
tilt, pan, zoom, reset and track/view selection. The same application now offers
free driving on all eleven levels, using
all twenty source-grid positions with road-aligned initial orientation, fixed-step frame
accumulation, pause/reset and focus-loss suspension. Other cars are rendered and
respond to impacts. Opponents now follow source road paths, brake for curves,
avoid traffic and pursue other cars in arenas, including timed reverse maneuvers.
Finished opponents continue normal AI steering and motion while lap records and
finish places stay latched. An original-circuit-5 natural race checks continued
post-finish movement and final scores on Native/WASM/ASan/UBSan. The current
ordinary Native/WASM/fresh O3 ASan/UBSan first Stockcar seasons naturally complete
all four scheduled rounds (three player finishes and one engine retirement each)
and open season 2. The unchanged limits and actual result consumption are proved
under closed 0050; broader campaigns and the original front end remain open.
Passing retains immediate body-heading avoidance and checks the base-lane
return path before merging back from a clear passing lane. Existing lane choices, rate
and physical limits are preserved. Stalling is detected from both low forward
speed and insufficient accepted horizontal movement, so retained solver velocities cannot mask a blocked car.
Damage-aware tactics, off-road recovery,
complete physical campaigns and the original front end remain pending.
Handwritten single-player league rules now own score/rank/division permutations,
stable-ID grid mapping, tie sorting and promotion/relegation. The production
Native/WASM component and fully instrumented ASan/UBSan version match 6,241
targeted original-x86 cases. Use `make rewrite-league-verify`; see
`src/game/league.md`. Driving now consumes a copied physical-slot permutation
while keeping driver IDs stable and human zero independent of start position.
`make rewrite-grid-verify` checks assigned starts, actual short physical drives
and mode/reset ownership on all eleven original levels. Its promotion scores
initialize fixtures; they do not establish completed championships.
A typed single-player championship controller now owns original schedules,
exactly-once actual result consumption, standings, continuing/terminal outcomes,
unlocks and the five-record history window including the current season. Its
session owns decoded tracks and assigned physical fields; failed next-track
preparation preserves current results. `make rewrite-championship-verify` checks
synthetic season rules separately from actual first scheduled ten-lap circuit
sessions and missing-next-track rollback on Native, WASM and ASan/UBSan. See
`src/game/championship.md`. The shared application now connects Native C/N and browser entry to this owner,
with scheduled track locking, unscored restart/exit, prepared renderer resources,
real named division standings and explicit result continuation. Original NPC
names and default human PLAYER are implemented; the player/car/audio profile actions
below now own configurable identity. `make rewrite-championship-application-verify`
checks natural first-round
application results/presentation on Native, Chromium/WASM and ASan/UBSan, separate
from complete physical campaigns, original menus, multiplayer and saved seasons.
Supported overturned cars now have a separate temporary availability state and
source-timed two-second recovery, with distance-gated opponent righting. Landing
preserves damage, progress and physical clocks and refreshes real road support.
See `src/game/recovery.md`. This is not yet a complete racing game.

## SoftGL

SoftGL is our rendering library and lives in `deps/softgl`. Other reusable build
dependencies remain ignored under `deps/`.

`deps/softgl` is a submodule pinned to
`a95534e8ac9f061a2e86286ffd4d80aa69c10bfa`, the published upstream `master`
tip fetched on 2026-10-09. Subsequent upgrades remain explicit and
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
The WASM build uses SIMD and a prestarted eight-worker pthread capacity.
The updated library's automatic WASM render pool uses at most three helpers,
with the calling thread participating: four total render threads. Native still
uses the library's automatic core-based pool; a shared explicit thread-selection
interface remains pending. Pool capacity is not the active render-thread count.
Native/browser presentation uses SDL2
software surfaces, with shared keyboard input and monotonic timing. Gamepad
remains pending. An owned Windows save-card container now preserves
all fifteen physical blocks, exposes compact logical entries and stages complete
borrowed replacement inputs. Original image reads and put/delete transitions
are independently checked on Native/WASM/fresh sanitized C. A typed original
configuration/profile codec now owns settings, five season records, current
drivers, names, lap tables and bindings while retaining reserved bytes. All
fields and encoded blocks match independently decoded original-x86 packs and
actual A/B cards on Native/WASM/fresh sanitized C. A polling save-store owner now
publishes complete candidates after synchronized Native atomic replacement or
strict IndexedDB transaction completion. Ninety-two Native/sanitized cases and
25 actual Chromium checks prove full images, failure rollback, conflicting
owners, real writer interruption and browser-process restart. Indeterminate
Native directory synchronization requires reload and never reports success.
Audio preference actions now consume that store through the Native application C
bridge and actual browser controls, with exact source effects volume preservation
and a separate versioned music extension. Restoring preferences preserves the
active session; pending writes refuse application close. See
`src/game/configuration.md` and `make rewrite-preferences-verify`.
Player/car/audio profile actions now own the active human name and class and provide F2
name entry, F3 save selection/filename/overwrite confirmation and F4 explicit
reload/restore on Native and the browser canvas. Browser controls expose these
actions alongside audio-only restore. New names use eight printable ASCII
characters; imported eleven-character names remain intact. Modal input suspends
simulation/music and reuses the frozen world image, avoiding a full scene redraw
for every character. An active championship locks identity while allowing audio
restores. Profile saves capture the live class; restores validate it before
preparing a replacement field. Different-class loads are rejected during play;
same-class loads retain active motion. Audio-only actions leave live class/name
unchanged and preserve dormant source fields. NPC names and stable IDs remain unchanged.
`make rewrite-player-profile-verify` checks unmodified original rosters, complete
independent cards, actual X11/browser input, restart and sanitized ownership.
Complete configuration consumption, playable-state translation and replay codecs
remain open; see
`src/assets/README.md`, `make rewrite-save-card-verify` and
`make rewrite-save-profile-verify`, plus `src/platform/README.md` and
`make rewrite-save-store-verify`. Typed sound-bank/WAVE readers
now preserve all 45 original effects, including their independent playback
frequencies and loop/channel metadata. Raw CDDA views expose the 18 provisioned
Redbook tracks; source/binary-identified three-target verification compares every
decoded sample byte. See `make rewrite-audio-assets-verify` and
`src/assets/README.md`. A typed audio runtime now mixes four effect channels
and a separate Redbook track with rational linear resampling, gain/pan,
saturating summation and explicit start/pause/resume/repeat/stop state. Analytic
tests cover playback cursors, endpoints and channel locks on both targets.
See `src/audio/README.md`. SDL2 now sends mixed PCM to Native/browser output.
The application loads a repeating Redbook title, supports pause/resume and
volume, and freezes output on game pause/focus loss. Native resolves all physical
tracks beside Dirinfo under `Redbook/`; F10 pauses/resumes and F11/F12 selects
the previous/next title. The browser opens local `track02.cdda`..`track19.cdda`
files and provides music/volume controls. Source PCM remains owned until a locked
replacement or device close; failed loads preserve playing music.
`make rewrite-music-output-verify` checks independent original-CDDA resampling
against Native SDL disk output and real browser WebAudio node buffers, plus
repeat, pause/focus, gain/mute, invalid-load rollback and close/reopen. The Native
application is also checked under ASan/UBSan. The original sound bank now supplies
the player motor, spatial impacts and THREE/TWO/ONE/GO cues. Fixed-step events
survive render frame batching; channel reservations protect motor/countdown
from automatic impact replacement. Browser controls offer independent music and
effects volume. `make rewrite-effects-output-verify` checks actual Native output
and browser four-channel PCM at 44,100/48,000 Hz with an independent WAVE oracle,
plus pause, mute, reset, close and sanitized lifetime. Pitch, levels and impact
attenuation are rewrite tuning. Skid/crowd/commentary/menu effects, automatic
game-state music selection and saved audio settings remain pending. Browser pthread builds
need HTTPS/localhost and COOP/COEP response headers.
The audio owner and shared application bridge additionally expose transactional
READY preparation without immediate playback. Native/sanitized callbacks and
actual Chromium output check silence, cursor zero, retained gain, rollback and
explicit start. Context selection and countdown/GO integration remain under
active 0060; this primitive does not implement automatic race/menu music.
Physics consumers reserve a 1 MiB WASM stack for the bounded coupled-contact
matrix, transactional twenty-body solver and nested geometry queries; the SDK's
default 64 KiB is insufficient.

## Build and mandatory quality gates

Use CMake 3.25+, Ninja, Python 3, LLVM 19 (`clang`, `clang-format`, `clang-tidy`),
Emscripten, Node, SDL2 and zlib development headers. Debian's package names are
`cmake ninja-build python3 clang-19 clang-format-19 clang-tidy-19 emscripten
nodejs libsdl2-dev zlib1g-dev`. WASM obtains zlib through the SDK port in the
private `/tmp/wasm-dd2/` cache. Authored-content verification additionally needs
NumPy and Pillow; normal builds consume the committed exports without Blender.
The window verifier additionally needs Pillow, Xvfb,
xdotool, Chromium and Playwright (the repository's browser launcher supports
Debian's system Chromium).

Optional Native diagnostics use Debian's `linux-perf`, `valgrind` and Intel
`pcm` packages. PCM executables are installed under `/usr/sbin`; use their
absolute paths when that directory is absent from the user's PATH. Installation
does not establish counter access: on the current host, unprivileged software
and hardware perf probes fail with `perf_event_paranoid=3`, and PCM cannot access
MSRs/PCI configuration. Keep host security settings unchanged during game work.
Valgrind Memcheck and Callgrind run without those counter permissions. Prefer
bounded captured contact cases for Callgrind; its instrumented cost is not an
ordinary Native wall-clock benchmark or a WASM performance measurement. Keep
diagnostics under `/tmp/wasm-dd2/` and remove raw profiles after summarizing them.

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
and Escape closes the view. C starts Wrecking Championship and N starts Stockcar
Championship. During a championship, tracks follow the original schedule; R
restarts an unfinished round, Enter continues results, and Escape/F7 restores the
previous practice track without scoring an unfinished round. Named league totals
appear after results. The browser offers the same modes and continuation/exit
buttons. Complete physical campaigns and the original front end remain open.
F1 cycles Rookie/Amateur/Pro outside driving and championships. The browser has
a Car class selector; car preview shows the selected name and original ratings.
Playing, profile dialogs and pending storage operations reject class changes
without replacing the live field. Selection prepares the complete replacement
before publishing and preserves the class through resets and scheduled owners.
Saved class restoration now shares the player/car/audio actions under 0058;
audio-only loads keep class selection unchanged. Complete configuration and
playable saved-state restoration remain open. `make rewrite-car-class-verify` checks thirty isolated
unmodified-original rating/force cases, class force behavior, 120 short original
level/mode/class prefixes per target, session restarts and real Native/browser
selection. These prefixes do not establish completed races or campaigns. The
unchanged strict free-driving comparison still fails LEVB after 800 steps;
the other 21 level/mode cases pass. See active 0002 for the current diagnostic.
Enter starts/exits free driving; W/Up gives gas,
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
F8 starts Time Trial with one car and unlimited laps on a circuit; F9 starts
Total Destruction on an arena. The browser
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
same field/countdown; Enter returns to inspection, and selecting Free driving clears
the race rules. Stockcar awards original position bonuses (100/75/50/.../0);
Wrecking adds actual simulated accident points to the original 50/25/10 circuit
bonuses, capped at 999. Wrecking arena results use accident points without circuit bonuses.
No randomized post-race NPC score or progress boost is manufactured. The result
list sorts total points, breaking ties by road/survival position; the player is
highlighted yellow. These are practice results; championships separately retain actual season totals. Time Trial displays
CURRENT, LAST and BEST in MM:SS.mmm (5 ms resolution, display capped at
99:59.995), excluding the partial grid approach. A complete lap updates last/best
without ending the session. Retirement freezes the current timer before the
coasting phase; F7 retains the session times immediately. Time Trial awards no
placement/accident points. R clears the session record and restarts the countdown.
Returning to Free driving or a circuit race restores twenty cars and the original
finite lap count; selecting an arena changes to Wrecking. Persistent track
records, their original file compatibility and the full front end remain pending.

Total Destruction uses twenty physical cars; every arena opponent retains player
slot zero as its pursuit target, including periodic retarget ticks and inverted
braking. SURVIVAL counts 5 ms ticks after GO while the player engine works,
including the coasting phase for a surviving winner; player retirement freezes
it before that tick. Withdrawal/results freeze it immediately. The original
99:00 timer cap is preserved. ALIVE counts available cars with functioning engines;
supported resting overturns are temporarily excluded until recovery. Timed results
replace placement/accident scores. Selection stays in Total Destruction across
arenas and switches to Wrecking on a circuit. R resets time, damage and all
pursuit targets. Dense pursuit fields can exhaust the shared 64-event solver
budget; the solver then retains checked poses instead of accepting residual
travel. The fleet solver now collects connected support neighborhoods at the actual
swept event time, applies primary restitution once, and jointly solves inelastic
normal/friction support and position repair. Pair proximity now determines the primary contact’s connected component before
querying world supports, avoiding road/barrier queries for unrelated cars while
retaining contact order and solver budgets. World and car-pair supports share a
0.1 world units/s linear low-speed friction transition, with unchanged Coulomb
saturation above that threshold. This bounds microscopic creep while stabilizing
almost parallel ground/wall supports. Captured dense-contact
queries and independent analytic/material checks cover this rewrite tuning.
A stalled accelerated solve restarts once from its exact input motion and zero
impulses, retaining all constraints and the shared 4,096-pass bound. Converging
accelerated solves retain their remaining iterations. After 512 ordinary sweeps,
guarded Newton corrections can resolve slow coupled normal/friction
modes in either phase, using bounded automatic storage and no allocations. Every
accepted correction reduces the finite physical residual and clears ordinary
secant history; rejection restores exact motion.
The direction search refits released normal constraints and compares projected
and alternate directions for loaded world supports from the same exact input.
World-only groups retain the zero-friction-gradient trial; mixed car/world
supports use an analytic constitutive Jacobian with cached unit-impulse mobility.
Twenty-eight reorderings of the two captured seven-contact championship queries
preserve independent physical checks. A bounded position prediction transfers
multipliers toward stronger near-parallel inequalities, rebuilds response offsets
and projects recipients. It retains all contacts and accepts only a smaller
finite full residual; rejection restores exact state. Twenty-four independent
analytic position cases check least-norm translation and unchanged motion/clocks.
If needed, an active-position equation fit uses exact coupled translation
responses, releases negative pressure and refits within contact_count + 1 solves.
Singular or non-improving fits preserve exact state. All twenty-four row
orderings of the new four-contact query and forty-eight analytic tilted-plane
cases check clearance, least-norm translation and released supports.
After the existing 512-pass delay in either phase, a mixed-group speculative branch can use up to sixteen bounded
Newton refinements while contact loads and friction directions settle.
Intermediate states remain private; only a finite lower full physical residual
can replace the outer iterate. Co-oriented world patches permit a projected
Jacobian fallback if the analytic seed is singular or refinement cannot improve.
Sixteen additional orderings
of the captured eight-contact query preserve independent physical checks.
A loaded small-slip branch also permits a private saturated constitutive
direction after that delay; the original regularized physical residual still
controls final acceptance. All 120 permutations of a five-contact natural
championship query check the transition without changing material rules.
Mixed fields can also compare selective saturated directions for each loaded
small-slip car pair, retaining the other contacts' constitutive directions.
The selection belongs to a local typed model; every trial starts from the same
outer state and remains subject to the existing bounds and physical residual.
Twenty rotations/reversals of a four-body, ten-contact championship query check
these independent transitions.
After the delay, world-only and mixed fields also compare private pressure seeds:
double each loaded small-slip world support or release each loaded world support.
Higher-load seeds equilibrate linear friction with all normal loads held fixed,
then apply the bounded analytic constitutive refinement. Release directions
instead fit the selected support's normal and tangent impulses to zero while
refitting the remaining coupled equations, starting from the same exact outer
state. Released contacts retain their normal inequalities in physical acceptance
and can reload in ordinary sweeps. The fixed-load solve and refinement reuse one
matrix; every candidate remains subject to the unchanged final physical law,
finite residual reduction and exact rollback. At most four models per contact
plus four base models use bounded automatic storage; ordinary pass limits and
material acceptance remain unchanged. All six orderings of a three-world-contact
query check higher pressure, and all 120 permutations of a three-body/five-contact
query check released world support with loaded car pairs. Twenty-two additional
rotations/reversals of a six-body/eleven-contact query check coupled release
without a fixed-load seed.
Private branch eligibility does not require a coordinate restart: slowly improving
accelerated solves can exhaust their pass budget without one. Existing model,
refinement and backtracking bounds remain; every final candidate still reduces
full physical error and requires ordinary certification. Sixteen orderings of
the three-car/eight-contact tick-168287 query check this initial-phase route.
The released-wall fixture keeps its independent physical checks while allowing
completion before restart; other required-restart fixtures remain unchanged.
A final private warm trial freezes the current projected normal active set.
Released rows have exact zero normal/tangent targets; retained rows keep their
normal equations while bounded backtracking handles negative full-step
predictions. It preserves the previous warm and cold models. Only a complete
physical root can replace the outer state; a partial improvement restores exact
motion and impulses. This adds one base model (260 maximum), while keeping one
matrix, 4,096 shared passes and sixteen refinement/backtracking steps. The actual
five-car/fourteen-contact tick-223337 query and twenty-eight rotations/reversals
exercise this branch without changing material rules or physical tolerances.
Loaded sliding world supports also compare a higher-pressure and
fixed-load friction seed through a private constitutive refinement. The fitted
equation merit and tangent direction permit a return to the linear branch while
coupled pressures settle. Sliding/unloaded world supports use the larger of
their current load and mean positive incident load only as a private pressure
scale. Small-slip LOAD seeds retain their original current-load scale. Their
selected normal equation stays active without provisional release during that
trial; candidate normal impulses
remain nonnegative. Final cone projection and the unchanged full physical
tolerance remain mandatory; partial alternate roots restore exact warm state.
Twenty-two rotations/reversals of the five-car/eleven-contact tick-108716 query
check this transition. This alternate occupies the existing pressure-model slot,
and unloaded supports omit the redundant release model, preserving the
four-model-per-contact, pass, matrix and refinement bounds.
Doubling a tiny sliding load can miss a higher-pressure linear-branch root.
The incident scale adds no model or matrix and still requires complete physical
acceptance and exact rollback. The actual three-world-contact tick-194353 and
tick-142893 queries and all six permutations of each exercise that basin.
Positive linear world supports now also compare that private incident-load
pressure basin alongside their existing small-load trial. The level-2 round-2
tick-137455 Native/WASM captures need this stronger linear root; an independent
mobility solve and all six contact permutations per capture check it. The model
array reserves five entries per contact plus four base entries (324 maximum).
Material, pass, search, refinement, matrix and final acceptance limits remain
unchanged. Complete seasons and movement remain separate acceptance work.

Selected world-release refinements also retain their fitted tangent directions
through private sliding/linear transitions. Before outer acceptance, every
friction impulse is projected into its original cone and the full physical
residual is recomputed. Finite reduction, exact rollback, material rules and
ordinary/refinement bounds remain unchanged. All six permutations of the
three-world-contact tick-83,450 query independently check the released wall
and its two road supports.
Within private release refinement, fitted complementarity/constitutive equation
merit also guides bounded backtracking across physical-error ridges. Projected
fallback steps keep their physical merit. Only the projected finite lower full
physical residual can replace the outer state; final rules and every ordinary,
refinement and backtracking bound stay unchanged. All 120 permutations of the
two-body/five-contact tick-190,078 query check the released road/wall branch.
Co-oriented world patches additionally compare each retained support against a
fixed private set of released matching contacts. The retained normal uses its
active equation even from zero pressure; every released normal/tangent impulse
has an exact zero full-step target. Backtracking scales its prior impulses toward
zero, avoiding tiny reconstructed loads amplified by friction softness. All
contacts retain their physical inequalities and material law; only a finite
lower full residual is accepted. Twenty rotations/reversals of the tick-220,415
four-body/ten-contact query check dependent normal rows and simultaneous release.
Matching world normals with distinct arms can require two loaded supports.
Each retained support also compares a second support at the farthest finite
matching point, retaining both active normal equations and exact zero targets
for the other matching rows. This bounded direction preserves the previous
single-support models and all final physical acceptance. Twenty-eight rotations/
reversals of the six-body/fourteen-contact tick-233,358 query check the branch;
independent analytic and finite-difference roots check its admissibility.
World-only fields also privately refine the existing linear friction direction
after restart, allowing a sliding support to return to the linear branch while
pressures settle. The full seed and up to sixteen steps retain all contact,
cone, finite-residual and rollback checks. This base model fits existing bounded
storage and leaves mixed-field models, ordinary pass limits and final material
acceptance unchanged. All six orderings of the tick-118,647 world query check
the transition.
The solver retains the stronger finite physical correction.
See `src/physics/README.md`. Complete natural arena behavior remains a separate gate. A 456-case synthetic convoy
regression checks full common motion and centered separation without impulses or
clock changes. Groups beyond the 64-constraint group limit retain conservative
serial response. A separate owned 4,096-entry record buffer stores all permitted
responses as a borrowed view. Driving swaps working/published buffers only after
whole-frame validation, preserving old entries on rejection. Neither the
64-event physical work limit nor the 1 MiB WASM stack is increased. Full natural arena completion and dense-field behavior remain subject
to original-data race verification. Supported resting overturns now participate in arena availability and recover
after two seconds; distant opponents retry placement after the deadline. Complete arena behavior still requires natural race verification.

The WASM builder uses a private writable ports cache under
`/tmp/wasm-dd2/emscripten-cache/`, including with Debian's frozen system SDK.
Existing system headers/libraries are reused through file links; SDL is built
privately. An explicit `EM_CACHE` overrides this default. The SDK package files
are not modified. Builds and reusable dependencies remain outside the source tree.

`make` builds the native rewrite. `make reference-prepare` exports the immutable `reconstruction-baseline` commit
with Git-blob and SHA-256 checks to `/tmp/wasm-dd2/reference/<commit>/`. The
reference owns its mutable save card and borrows immutable game data. Use
`make reference REFERENCE_TARGET=verify-season-transition` for legacy tests, or
`make native`, `make wasm`, `make patch` and `make check` for frozen reconstruction
targets. They run entirely under `/tmp`, independently of the rewrite.
`re_out/` and `patches/` are retained only on `ghidra`, not on `master`.
Provisioning uses the verified frozen image extractor and never recreates these
directories in the rewrite.

All rewrite C and header files under `src/` and `tests/` are covered by
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
-Wstrict-prototypes -Wmissing-prototypes -Wformat=2`. SoftGL and reference code
retain their own quality rules. Fast-math permits arithmetic changes; new physics
must be verified for stable behavior on native and WASM, including edge cases.

The mandatory local gates check formatting, strict native build/analysis and both
renderer bootstraps, plus archive/level/texture bounds and texture rendering
without proprietary data. There are no GitHub workflow files on `master`.
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

The fleet suspension also supports reachable finite faces of other moving
vehicles through `body_surface.c` and `dd2_vehicle_step_field`. Every prediction
uses the immutable initial field; spring and tire forces exchange momentum at a
common contact point. Suspension-axis sphere/face geometry prevents spurious
loads from steep side faces. Focused Native/WASM and instrumented checks cover
banked contact, shared motion, conservation, rejection and physical roof escape.
The full unchanged sixty-second AI suite now passes all eleven playable levels
on Native, WASM and fresh O3 ASan/UBSan, including the previous Arena-B support
failure. This establishes sustained driving coverage with damage disabled;
natural races, damage-aware tactics and complete championships remain open.
Receipt: `/tmp/wasm-dd2/rewrite-strut-support-0016/component-report.json`.

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
all-track completion. Additional complete races, off-road rescue and championships remain to be
implemented or verified. Supported overturn availability/righting is checked
separately by `make rewrite-recovery-verify`. Native/browser window checks cover all eleven countdown/results
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
session timing; persistent records and full-game coverage remain open. Four additional
short twenty-car scenarios check Total Destruction pursuit, countdown, timer,
frame partitioning, pause, withdrawal/results and reset on every original arena.
Separate physical arena completion checks use ordinary player steering/weaving
and shunting commands with pursuing AI through contact/engine damage. They require natural
coasting results and check each 5 ms tick with independent race/timer oracles on
all three targets; those completion checks are still being diagnosed. The fixed
inputs switch to alternating steering and twenty-second forward/reverse periods
after 30,000 fixture ticks (including the countdown), so a single steady steering
profile cannot define the entire long run. Native/browser
checks cover all four arena starts and results, F9/selector entry, pause, restart
and circuit rejection. The browser additionally checks actual timer progress
after GO and frozen survival time on pause/withdrawal. Pixel charts check exact live
and result SURVIVAL/ALIVE digits, including the 99:00 cap. These checks establish
arena session timing/pursuit, not complete Total Destruction gameplay. Supported
overturn availability/righting has its own controlled verification. To diagnose individual scenarios, run
`python3 tools/rewrite/verify_race.py --case 8-total-survive --output /tmp/wasm-dd2/arena-probe`.
Repeat `--case` for multiple scenarios; omitting it keeps the full suite. Scoped
reports explicitly mark partial coverage, and each target writes its own result
receipt against recorded source/binary hashes before another target can fail.
Source changes during a run invalidate its final report. Source-timed lights now
have matching sound cues; automatic game-state music selection remains pending.

## Authored scene performance target

For this machine, plan around **640x360, 4x MSAA and four total render threads**
at **60 FPS**, allowing **16.67 ms per complete frame**. The initial content
budget is **100,000-200,000 submitted triangles and 20-30 materials per frame**,
with reserve for overdraw, alpha tests and expensive material evaluation.
These are user-supplied planning values, not measured guarantees at 60 FPS.
Count the calling thread as part of the four threads. Use LOD, cockpit/exterior
visibility and material sharing to keep detailed authored assets within the
visible scene budget.

The current reference-backed viewer still renders 640x480 single-sample frames;
this dependency upgrade does not change its framebuffer or HUD layout. Enable
the target profile in the authored renderer alongside aspect-correct cameras,
HUD/menu layout and explicit Native thread selection. Measure actual production
Native/browser gameplay with simulation, drawing, MSAA resolve, readback and
presentation included. Record median and tail frame times, scene/material counts,
CPU/browser identity and visual review evidence before accepting 60 FPS.

## Migration and acceptance

Reference decoding and initial original-backed rendering/presentation are
already established. Next build the owned Blender/procedural/audio pipeline
and a playable authored track, vehicle and cockpit with new effects/music,
then make Native/browser default launch independent of original files. Expand
replacement content and functionality to all tracks, modes, menus,
championships, replays, settings and save/load. Optional reference comparisons
can resolve uncertain behavior; they must not become runtime dependencies.

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

Implement a fully playable, readable and modular standalone C11 Destruction Derby 2 reimplementation on `master` in `/home/cosmo/Git/wasm-dd2` for Native and WebAssembly with the pinned SoftGL renderer; target 60 FPS on this machine at 640x360, 4x MSAA and four total render threads including the caller, using an initial planning budget of 100,000-200,000 triangles and 20-30 materials per frame with overdraw/alpha-test/material headroom, validated by actual Native/browser complete-frame measurements; rebuild every game asset and keep editable Blender models/scenes, complete cockpits/interiors, procedural texture generators, newly created effects/engine/ambient/commentary audio, newly composed music and all generated runtime content in the repository. Native and WASM must build, launch and provide every feature without original DD2 game files, archives, sound banks, CD images or Redbook content; use `ghidra`, the reconstruction baseline and the original only as optional development references. Replace absolute addresses/register emulation with documented types and explicit ownership; implement every track, vehicle, physics/damage model, AI behavior, mode, menu, race, championship, replay, setting, keyboard/gamepad control, save/load operation and relevant edge case, plus working cockpit views and all audio behavior. Substantially surpass the original in every visual, acoustic and gameplay-quality area through high-quality geometry, procedural materials/textures, resolution, lighting, shadows, effects, UI and newly authored audio; assess actual Native/browser gameplay, screenshots and audible output with concrete before/after evidence and measured performance, rather than relying only on physics or pixel tests. Original bitidentity is not required; standalone completeness, functional correctness, stability, appropriate data compatibility and demonstrably superior player experience on both targets are mandatory. Enforce strict clang-tidy/clang-format 19 and the agreed compiler flags, keep diagnostics bounded under `/tmp/wasm-dd2/`, maintain the backlog, remove completed captures/logs and commit/push every verified improvement until the entire expanded goal is implemented and no known functional, compatibility, content or visual/audio defects remain.

`make rewrite-recovery-verify` exercises supported rest/righting boundaries and
220 controlled physical roof-down drops per target at every original grid slot,
with independently decoded source-plane and wheel contact checks and subsequent
acceleration. It covers Native, Node/WASM and ASan/UBSan. These are recovery
subsystem checks, not naturally completed arena races or original handling parity.

The captured ordinary round-2 tick-155992 world-pressure query now preserves
projected warm normal branches during private refinement. Its nineteen contacts
among nine bodies reach an independently verified physical root; full/remapped
Native/WASM/O1 ASan/UBSan cases, all 742 physical checks and unchanged eleven-level
AI contracts pass, with strict LLVM19/169-file and 38/36 CTest gates. This proves
the captured field component, not a full natural owner campaign. The independent
Arena-A spinning-body discrepancy was unchanged by that correction. Closed 0052
now uses the existing one-car joint world-contact solver and passes the unchanged
eleven-level ground and 220-case recovery contracts on all three targets, plus
169-file strict gates, 38/36 CTests and all 45 previously proved race scenarios.
The full natural-arena suite still fails independently: Native Arena 8 rejects
position correction at owner step 10574; WASM exceeds the unchanged 1800-second
deadline, while sanitized reaches natural engine retirement. Active 0053 owns the
exact Native query, with converged velocity and zero calls to the changed world API. Fresh
570df9d ordinary baselines instead stop at round-1 or round-3 tick bounds behind
late-race traffic. Closed 0050 now proves continued finished-NPC controls and naturally completed
first Stockcar seasons on all three targets; broader campaign coverage remains
open. See the board for these separate acceptance contracts.


The captured Arena-8 owner-step-10574 position failure now has an independently
checked component correction under active 0053. The private equality refit releases
one warm multiplier boundary before refitting, retaining supports whose fitted
pressure becomes positive after their neighbor leaves. Released identity rows
prescribe exact zero even when elimination roundoff is slightly negative. The
captured full query reaches its independent twelve-row position root at 512
passes on Native/WASM/sanitized C; all 48 geometry reorder/remap checks, 742 prior
physical cases, eleven original ground levels, 220 recovery cases and strict
170-file/38/36 gates pass. All 45 previously proved race scenarios pass per target
again. On 6efaa44, Native Arena 8 naturally reaches engine-retirement/coasting results
at tick 81219 (401.09 seconds survival), within the unchanged scenario deadline;
fresh O1 ASan/UBSan completes at tick 13083 (60.41 seconds survival). WASM exceeds
the unchanged 1800-second deadline after 77251 complete ticks. Its independently
checked prefix passes the former rejection but supplies no natural-result
acceptance. Active 0053 retains that missing contract; all-arena completion
remains unproved. Receipt:
`/tmp/wasm-dd2/rewrite-arena8-rejection-0053/terminal-owner-report.json`.

Internal contact scratch now initializes live counted prefixes instead of
clearing unused capacity. Degenerate static SAT axes explicitly clear their
selection flag; tie selection reads the other fields only on live axes. Eight
ordinary 6000-tick prefixes preserve all per-target public fields. Two
reversed-order comparisons reduce child CPU by 1.48..1.92 percent on Native and
5.50..6.01 percent on a private named WASM link; these are scoped prefix costs.
Fresh patterned O1 ASan/UBSan and Native Memcheck cover the physical/ownership
corpora, with strict LLVM19/170-file, 38/36 gates, all eleven original ground
levels, 220 recovery cases and 45 prior race scenarios passing again per target.
Closed 0054 proves this scratch contract. At source 0323ba9, actual Native and
fresh O1 sanitized Arena-8 owners repeat their natural results at ticks
81219/13083 (401.09/60.41 seconds survival). WASM times out at the unchanged
1800-second deadline after 86042 complete ticks (428.21 seconds survival, twenty
available engines). Every complete prefix row passes the independent oracle;
the full public tick-77251 checkpoint matches the prior source exactly. Neither
check proves a natural WASM result or isolated whole-case speed improvement.
Active 0053 remains open for late field motion/cost diagnosis. Completed raw
output and private binaries are removed after retaining hashes and receipts.
Receipts: `/tmp/wasm-dd2/rewrite-arena8-profile-0053/`
`{terminal-owner-report,late-checkpoint-report,terminal-cleanup-report}.json`.
Scratch verification receipt:
`/tmp/wasm-dd2/rewrite-arena8-profile-0053/verification-report.json`.

The bounded late Arena-8 WASM observer stops as planned at tick 83591 without
natural results. Its full public tick-77251 checkpoint matches production exactly.
Across ticks 70000..83591, the field averages 58.411 response events per step;
the player travels 24557.623 units with nearly all wheel steps supported and only
five damage-eligible contact records. Late V8 samples identify fleet resolution,
static car SAT and source-surface traversal, including inlined callees. This is
scoped moving-field/cost evidence, not retirement or an isolated speed comparison.
Active 0053 now needs one actual late fleet step with previous/proposed typed
bodies and contact times/rows to distinguish repeated supports from legitimate
dense pursuit. Budgets/material/health/controls remain unchanged. Completed raw
profiling output is removed; three required typed checkpoints and receipts remain.
Receipt: /tmp/wasm-dd2/rewrite-arena8-late-0053/diagnosis-report.json.

Fleet response-budget exhaustion now certifies independent residual motion with
the existing ground/barrier and pair sweeps. Potentially contacting or unresolved
bodies keep checked poses; newly frozen obstructions propagate to followers.
The shared 64-event limit and material laws remain unchanged. This fixes the
LEVB player being stopped by five unrelated NPC pairs at driving tick 534.
All 22 original-level drive/start comparisons pass again on Native/WASM/fresh O1
ASan/UBSan, with unchanged state and image tolerances. Four focused budget-tail
geometry cases, 48 position boundary checks, 742 prior material cases, all eleven
original ground levels and 220 recovery drops pass per target, with zero Memcheck
errors. Strict LLVM19 checks cover 207 C/header files; 47 Native and 45 WASM CTests
pass. Work item 0059 owns this bounded contract; complete natural arenas and
campaigns remain under 0002/0053. Receipts:
/tmp/wasm-dd2/rewrite-fleet-remainder-0059-gates/ and
/tmp/wasm-dd2/rewrite-fleet-remainder-0059-{driving-2,ground,recovery}/report.json.
