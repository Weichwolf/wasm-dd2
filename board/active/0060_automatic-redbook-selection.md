Type: Work item
Title: Select actual Redbook music from game and frontend state
Depends: 0003, 0009

## Contract

Connect automatic music to committed shared Native/browser game state. Select
track 13 in menus/practice results, loaded asset level + 1 for races, 14 for
championship results/intermediate standings and 15 for season completion. Prepare
race music during countdown and start it at GO. Preserve explicit transport,
focus/game suspension, gains and ownership; do not replay a request every frame.
Provide browser access to the needed user-owned Redbook files and reject stale
asynchronous completion. Missing/failed loads preserve accepted audio and show
the actual availability problem. Failed game transitions do not change music.

## Evidence

The existing application owns actual music output, explicit transport and saved
gains; Native initially selects physical track 2 and the browser accepts one
manually staged CDDA file. It does not consume game/frontend music contexts.

`tools/reference/music_selection_fixture.c` runs unmodified original routines
with an isolated imported-MCI observer. Its machine-code pages are read/execute
only; the original executable and provisioned game files remain unchanged.
All eighteen physical selections, start/pause/resume, repeat guards, failed
start, disabled CD and actual pre-green/held/GO countdown calls pass 48 checks.
Original caller disassembly separately distinguishes Practice_Over (13),
Race_Over/Display_Season_Status (14) and End_Of_Season (15), plus Front_End (13)
and Play_Game (loaded level + 1). An independent PE data read matches all eleven
menu-to-asset mappings: [1,2,7,5,3,6,4,10,8,9,11]. Selection clears the playing flag;
GO starts the race title. Explicit pause disables the repeat poll.

Use `make rewrite-original-music-selection-verify`. Receipt:
/tmp/wasm-dd2/rewrite-redbook-policy-0009/verified-3/report.json.
This is an original component/data contract, not implemented automatic rewrite
selection, actual output PCM or complete audio acceptance.

The shared audio owner and application bridge now support transactional READY
preparation independently of immediate loading. Selection retains gain and
does not start or advance the cursor through device callbacks; explicit play
starts it later. Failed preparation retains the whole previous source/transport.
Native and fresh O1 ASan/UBSan exports pass 36 checks each, including failures
in READY/PLAYING/PAUSED, playing replacement, pause/resume and active close.
SDL disk captures independently match constant stereo PCM at the retained gain.
Actual Chromium node buffers prove eight silent READY callbacks, malformed-load
rollback and independent variable-pattern resampling after explicit start at
the default and 48000-Hz device rates. Existing real track 2/3 PCM, immediate
loads, controls, wrap/gain/mute and close/read cancellation remain checked.
Use `make rewrite-music-output-verify`; receipt:
/tmp/wasm-dd2/rewrite-redbook-preparation-0060/report.json.
This is an accepted transport primitive, not automatic context selection or GO
integration; 0060 remains active.
Strict LLVM19 format/tidy passes 208 C/header files; 47 Native and 45 WASM
CTests pass. Actual visual review is separate: 0061 records a browser modal
canvas-loss finding; the audio primitive does not claim full visual acceptance.

## Next

Implement a typed context/request owner using the committed application/session
state. Apply requests once using READY preparation and
preserve manual transport until a real context change. Connect Native file
resolution and browser file availability/asynchronous loading to the existing
locked device owner. Check preparation failure, canceled transitions, stale
browser reads, pause/focus, manual replacement, gain retention and close.
Then verify actual Native/browser PCM and real game/season transitions. Continue
remaining effects, commentary, CD Player metadata and complete menus under
0009/0003; this item does not replace those contracts.

## Accept

The real application plays the context's title on both targets, starts race
music at GO, handles every loaded level and result/season route, and preserves
previous accepted state on failure. Actual output, browser user-file ownership,
transition/request guards and strict Native/WASM/sanitizer gates prove the
contract. Original component checks alone do not close this item.
