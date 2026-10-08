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
This inventory is static evidence; the reconstruction has known incomplete
coverage. A separate original Wine run observes the two initially ambiguous
main slots using real X11 input, an isolated save copy and bounded read-only
state/framebuffer observations. It opens/cancels the File Manager and opens the
CD Player. Receipt: `/tmp/wasm-dd2/rewrite-frontend-card-link/report.json`.
No debugger or engine-state writes are used, and the provisioned save is unchanged.
This establishes those entry routes, rather than all submenu actions or playback.
A further real-input original run opens/cancels Information and Configuration.
Their initial selections are View Lap Times and Select Control Method. Seven
key actions preserve the selected main entries and both the isolated/provisioned
save files. Receipt: `/tmp/wasm-dd2/rewrite-frontend-info-config/report.json`.
Its completed images/logs are removed after recording hashes and interpretations.

The original image supplies eight main-menu slots. These identifiers name
rendering assets, rather than final English UI labels:

| Slot | Asset identifier | Rewrite requirement |
| --- | --- | --- |
| 0 | `RACETYPE` | Choose the actual race mode and session type |
| 1 | `CAR` | Select a supported car with its real class/livery/damage data |
| 2 | `TRACK` | Select an eligible circuit or arena; preserve scheduled championship tracks |
| 3 | `CARD` | File Manager: connect real load/save/delete and failure behavior |
| 4 | `LINK` | CD Audio Player: select titles and control real Redbook transport |
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

The observed File Manager opens a Load view with fifteen slots; Escape returns
to its selected main-menu entry. Save/delete, occupied-slot selection and error
paths still need direct observation. The observed CD Player displays the selected
track, title and group, with previous/play/stop/next controls. Metadata/navigation
observation does not prove accepted PCM or playback behavior. Its actions must
connect to the existing Redbook mixer/device, not a separate simulated player.

The observed Information entry presents View Lap Times by default on the empty
initial profile. `Show_Information` selects that entry when no statistics are
recorded and uses a different navigation table when statistics are unavailable.
Configuration initially selects Control Method. Both screens return to their
retained main-menu entry on Escape. These entry/cancel observations do not prove
record/statistics actions, remapping, audio edits, credits or saved settings.

A subsequent 19-key original run opens the five-row lap-times table and Credits
from Information, then the Keyboard remapping prompt through Control Method and
Audio Volume through Configuration. Right from View Lap Times selects View
Credits on the empty-statistics profile. The Keyboard prompt initially asks for
Left; Audio Volume initially presents Sound Effects. Escape unwinds each route
to its retained parent and main selection. Both save files remain unchanged.
Receipt: `/tmp/wasm-dd2/rewrite-frontend-submenus/report.json`. These observations
establish actual subordinate entry/cancel routes; record updates, submitted
bindings, volume changes, joystick actions and persistence remain unproved.

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
route inventory, remaining original observations and backend implementation remain open.
