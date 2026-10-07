# Rewrite work queue

Directory is state: `open/active/closed`. Read `../AGENTS.md` and the current work
item before implementation. IDs remain stable; move the file when its state
changes. This board follows the structure used by `wasm-fist` without importing
its game requirements or acceptance rules.

## Work order

| WI | Deliverable | State |
| --- | --- | --- |
| [0001](closed/0001_rewrite-reference-boundary.md) | master rewrite and ghidra reference | closed |
| [0002](active/0002_natural-races-and-contact-stability.md) | Natural races and contact stability | active |
| [0014](closed/0014_repository-layout-and-backlog.md) | Repository layout and backlog | closed |
| [0015](closed/0015_typed-league-foundation.md) → [0004](open/0004_championships.md) | League foundation and complete championships | closed / open |
| [0003](open/0003_frontend-and-menus.md) | Complete front end and menu actions | open |
| [0005](open/0005_vehicles-and-damage.md) | Every vehicle/class/livery and damage behavior | open |
| [0016](open/0016_ai-tactics-and-offroad-recovery.md) | AI tactics and off-road recovery | open |
| [0006](open/0006_replays.md) | Replays | open |
| [0007](open/0007_keyboard-gamepad-and-platforms.md) | Keyboard/gamepad/platform lifecycle | open |
| [0008](open/0008_settings-save-load-and-records.md) | Settings, save/load and records | open |
| [0009](open/0009_complete-audio-behavior.md) | Complete effects and music behavior | open |
| [0010](open/0010_graphics-and-performance.md) | Continuous graphics improvement | open |
| [0011](open/0011_complete-functional-acceptance.md) | Complete requested game on both targets | open |
| [0012](closed/0012_accepted-motion-stall-detection.md) | Accepted-motion AI stall detection | closed |
| [0013](closed/0013_connected-world-query-pruning.md) | Connected contact world queries | closed |
| [0017](closed/0017_bounded-contact-restart.md) | Bounded restart for cycling accelerated contacts | closed |
| [0018](closed/0018_preserve-converging-contact-iterations.md) | Preserve iterations for converging contact chains | closed |
| [0019](closed/0019_english-repository-and-browser-layout.md) | English repository text and browser layout | closed |
| [0020](closed/0020_delayed-coupled-contact-corrections.md) | Delayed coupled-contact corrections | closed |
| [0021](closed/0021_contact-jacobian-accuracy.md) | Late contact Jacobian accuracy | closed |
| [0022](closed/0022_contact-active-branch-predictions.md) | Contact active-branch predictions | closed |

Continue stabilizing ordinary races, then connect the proven league foundation
to full championship/front-end/save flows. Audio, input and visual work
remain part of the full goal; no component milestone replaces final acceptance.

## Workflow and acceptance

Use RFC 822 headers `Type`, `Title`, optional `Depends`, then **Contract**,
**Evidence**, **Next**, **Accept**. Keep one owner for each behavior contract.

1. Reproduce the current failure and state the intended behavior and evidence.
2. Implement readable typed C11 and meaningful checks for the reached behavior.
3. Run strict LLVM19 and required Native/WASM gates plus relevant functional,
   original-data and actual presentation/output checks.
4. Record source/build identity, commands, scope, results and limitations in the
   work item. Diagnostics remain bounded under `/tmp/wasm-dd2/`.
5. Close only after **Accept** is proved. Commit/push each verified improvement,
   clean completed raw output and update affected work items.

Original runs and reconstructed C supply evidence. They do not replace the
handwritten game. A partial comparison, successful unit test or static preview
cannot prove complete gameplay, original parity or a feature it does not reach.
