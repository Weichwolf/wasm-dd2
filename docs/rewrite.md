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
Vehicle suspension, billboard orientation,
lighting, blending and gameplay remain to be implemented. A shared C application now presents the
eleven track/car views in a native SDL window and a browser canvas, with orbit,
tilt, pan, zoom, reset and track/view selection. This is an inspection viewer;
vehicle simulation, opponents and race rules remain pending.
This is not yet a playable game.

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

`make rewrite-play LEVEL=1` opens the native track/car viewer. `make rewrite-web`
serves the browser viewer at `http://127.0.0.1:8080/`; select the provisioned
`DestructionDerby2/Dirinfo` file in the page. Assets remain local and are not
included in the browser distribution. Arrow keys orbit/tilt, WASD pans, plus/minus
or the wheel zooms, R resets, Tab switches track/car, Page Up/Down changes level,
and Escape closes the view. The browser can reopen the archive after closing.

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
billboard orientation, lighting or blending. Vehicle simulation and a complete
race remain pending. See `src/render/README.md`.
`make rewrite-road-verify` checks every playable road graph and contact cell
against an independent original-data reader on native, Node/WASM and ASan/UBSan.
It includes source attributes, links, missing edge triangles, grid geometry and
vertical plane samples, plus invalid-link/count/vertex/branch-cycle rejection.
This is contact geometry coverage; suspension, vehicle motion, off-road behavior
and lap/checkpoint equivalence remain pending.
`make rewrite-surface-verify` additionally compares indexed contact selection
against an exhaustive cell search for 197,750 queries per target across all
eleven playable levels, including height windows and shared corners. Native,
Node/WASM and ASan/UBSan agree on selection and work counts. Synthetic checks
cover stacked surfaces, tolerance chains and nonfinite inputs under fast-math;
the original-data verifier requires at least 95% pruning of contact tests. See
`src/physics/README.md` for ownership and tie rules. This index is a foundation
for vehicle simulation and is not yet used by the inspection viewer.
`make rewrite-window-verify` compares all eleven actual native window and browser
canvas track/car views against the shared C preview, exercises real keyboard and
wheel events, focus-loss release, camera reset, selection synchronization,
native resizing/letterboxing, invalid browser archives and close/reopen. The
native lifecycle also runs with ASan/UBSan-instrumented rewrite C modules;
system SDL2 and pinned release SoftGL remain uninstrumented. The
native framebuffer comparison is exact; cross-target comparisons use the same
recorded SIMD edge bounds as the scene verifier. This establishes viewer behavior,
not original rendering parity or playable-race coverage.

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
