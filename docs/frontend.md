# Frontend migration

The shared application currently provides track inspection, free driving and
practice/championship sessions. It does not implement the original frontend.
Work item 0003 owns complete navigation and working actions; 0004, 0007, 0008 and
0009 own the championship/multiplayer, input, persistence and audio contracts it
must connect. A visible button alone does not satisfy an action.

## Original evidence

The frozen `reconstruction-baseline` reference contains `Front_End`,
`Button_Pressed`, the race selectors, driver-name entry, track/car selection,
configuration and result/season screens. Prepare it with
`make reference-prepare`; source and original-image inventory is recorded at
`/tmp/wasm-dd2/rewrite-frontend-inventory/report.json`.
This is static evidence. Actual original navigation and rendered labels still
need observation; the reconstruction has known incomplete coverage.

The original image supplies eight main-menu slots. These identifiers name
rendering assets, rather than final English UI labels:

| Slot | Asset identifier | Rewrite requirement |
| --- | --- | --- |
| 0 | `RACETYPE` | Choose the actual race mode and session type |
| 1 | `CAR` | Select a supported car with its real class/livery/damage data |
| 2 | `TRACK` | Select an eligible circuit or arena; preserve scheduled championship tracks |
| 3 | `CARD` | Observe the original action and connect the persistence owner |
| 4 | `LINK` | Observe the original action before assigning semantics |
| 5 | `INFO` | Provide the original information/statistics/record flows after inventory |
| 6 | `CONFIG` | Edit actual input, audio, display and game settings after inventory |
| 7 | `GO` | Start the selected, fully prepared session |

`Front_End` excludes the track slot from directional navigation for the original
championship and multiplayer session types. Its accept/toggle actions are distinct.
`Button_Pressed` waits for confirm release before returning; held input must not
accidentally accept successive screens in the rewrite.

The source selectors establish these session constraints:

| Selector | State established by the reference | Handwritten owner |
| --- | --- | --- |
| `Select_Champ` | One entered driver, twenty cars, scheduled tracks | Single-player championship |
| `Select_ChampQS` | Default player name, twenty cars, scheduled tracks | Championship quick start |
| `Select_Multi` | Name entry for one to ten human participants, twenty cars, scheduled tracks | Multiplayer championship; input/turn topology still needs observation |
| `Select_Pract` | One human, twenty cars, circuit selection | Circuit practice race |
| `Select_TimeT` | One human/car, circuit selection | Continuous Time Trial |
| `Select_DDPract` | One human, twenty cars, arena selection | Destruction practice |
| `Select_Total` | One human, twenty cars, arena selection | Total Destruction |

`Select_Track` retains the entry selection and restores it on cancel. Race-mode
selectors commit their state only after successful subordinate selection.
These source observations do not establish complete playable original menus or
the correctness of the current reference's championship scores.

## Shared ownership and acceptance

Implement typed navigation and explicit actions in `src/game/`, with the same
state transitions on Native and WASM. Rendering uses SoftGL; platform adapters
supply keyboard/gamepad/pointer events. Keep selection/edit state separate from
the committed session or profile so back/cancel and failed preparation preserve
the previous working state. Starting a race must prepare its real assets and
owners before publishing the transition.

Standings and season/result views consume the actual championship/race owner;
record and file screens consume persistent data. No menu may invent scores,
completed laps, damage or a successful save. Replay, audio and settings actions
must control their implemented subsystems. Preserve pause/focus behavior and
prevent held confirmation from leaking into the next screen.

Acceptance requires actual Native/browser navigation through every inventoried
entry, visible feedback and the intended game-state change, including cancel,
invalid input, unavailable data, reload and failed save/load/preparation. Complete
route inventory, original observations and backend implementation remain open.
