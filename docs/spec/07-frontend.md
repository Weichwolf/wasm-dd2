# 07 — Front-end: boot, state flow, level load (main.C, frontend/*.c) — spec (in progress)

Our-words spec from decompiled Init_Main @0x4456e4, Set_Load_Textures @0x445764, Modify_TDF, the per-level
loaders, and the menu strings. Covers boot + how screens/levels are loaded; per-screen menu detail is TODO.

## Boot (Init_Main)
1. Graphics/timer init (FUN_412e8c), set display mode flags.
2. Init_Controller_ (pad/keyboard), Profile_Init (driver profiles), Sound_Init.
3. VSync + VSyncCallback (frame heartbeat), set sound volume.
4. InitCardSystem (memory-card/save), **Read_Directory("DIRINFO")** — loads the Dirinfo TOC into memory.
5. Read_CD_Toc_ (CD audio tracks), Load_Game_Vags (load sound bank, see 08-audio).
Then __WinMain enters the top-level loop → Play_Intro (movie/attract) → front-end menu → Play_Game → results.

## Level / screen load (driven by `_current_level`, hex = LEVx)
- Each screen/level has a loader that sets `_current_level = N` then:
  - `Set_Load_Textures()`: builds path `LEV{X}\LEVEL.TX0` (+ TX1..TXn while present) and queues
    Add_Buffer_Load_ for each TX page → VRAM assembly (see 01-data-io, 02-gte-gpu).
  - `Add_Buffer_Load_("LEV{X}\LEVEL.SPR"/"...DAT"/font)`, then process loads (Load_Completion_Status),
    Init_Graphics, Load_Sprite_Info, Setup_Font(s).
  - `Modify_TDF()`: post-processes the level's TDF texture table — computes per-entry UV min/max (bounding
    the 4 UV bytes @local_14[2..5]) to normalise texture rects for the rasteriser.
- `_current_level`: 0 = front-end (LEV0); 1..0xB = the 11 tracks (LEV1..LEVB); 0xC/0xF = result/special
  screens (LEVC/LEVF sprite sets). The menu→level map (which track index → which LEVx) is in tracksel.c
  (TODO: read for the exact index→LEVx table; note menu order ≠ LEVx order, e.g. LEV6=Pine Hills=trees,
  LEV5=Caprio=desert).

## Front-end screens (menu strings → modules; detail TODO)
- frontend.c — main radial menu: buttons Wrecking Racing / Race Mode / Select Track / Select Car /
  View Statistics / Configuration / Information / CD Audio Player / Go; modes cycle a label; "Go" starts.
- racetype.c/racemode.c — Race Type (Wrecking Racing / Stock Car / Destruction Derby) ×
  Mode (Practice / Time Trials / Championship / Total Destruction / Multiplayer).
- tracksel.c — track select (single cycling preview + name; 11 tracks). carsel.c — car select (Rookie/
  Amateur/Pro classes; accel/topspeed/grip stats). config.c/credits.c/info.c/fastlaps.c/centresc.c —
  options/credits/info/lap-times/screen-centre. name.c — name entry. card/*.c — memory-card save/load.
- results: raceover.c, results/champ.c, endofsea.c — race over → positions/points → championship league
  → end of season (promotion/relegation across divisions).

## Port status / plan
- Already implemented (approx, validated vs ref): title, main radial menu (grey-metal buttons, logo,
  labels), track-select (list — should become single cycling preview to match), HUD, race entry.
- TODO from spec: exact menu state machine + tracksel index→LEVx map + carsel stats + the
  championship/results flow; render front-end screens from the real SPR/FONT as the original sequences them.
