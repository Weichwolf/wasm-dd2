# 03 — Game loop / in-race frame (from Play_Game @0x423a20, game.C)

Functional description (our words) of the in-race tick, for the port. Source: decompiled Play_Game.

## Timing model
- Simulation is a FIXED 50 Hz step. The busy-wait at end of each drawn frame spins until
  `frame_skip * 0x14` (20 ms) has elapsed → base 50 fps, multiplied by frame_skip.
- `frame_skip` is ADAPTIVE: every `frame_wait` drawn frames it is recomputed from measured frame
  time (`tot_time / (draw_frame*20)`, capped at 7, +1), and `frame_wait` doubles up to 32. So on a
  slow machine the sim runs multiple 20 ms steps per rendered frame (frame-skip), keeping sim rate
  constant. The sim is deterministic; rendering cadence is decoupled.
- `current_frame = global_counter / 2`; `global_counter` increments once per SIM step.
- For the WASM port: run the sim at a fixed 50 Hz accumulator; render at display rate. Determinism
  depends on the fixed step + fixed input order (player then AI), NOT on wall-clock.

## Per SIM step (runs frame_skip times per drawn frame)
1. If race countdown finished (`_DAT_0074be98 < 1`):
   - Player car: read input → player car control (FUN_00441394), or pit-lane handler if `PIT_IN`.
   - Each other car: `AI_Com_Server(i)` (AI control). In demo mode ALL cars are AI.
2. `Car_Movement(i)` for every car — vehicle physics integration (see 04-handling).
3. `Do_Car_Collisions()` — car↔car and car↔barrier resolution (see 04-handling).
4. `Car_Camera(camera_car)` — camera follow update.
5. Race bookkeeping: Get_Race_Positions, Strip_Trigger_Handler, Calc_Track_Positions,
   Get_Race_Positions (again), checkpoint scoring (CheckPointScoring).
6. `Update_Scene_Objects()` — animated scenery.
7. Per-car countdown timers decrement (array @0x75a71e, stride 0x1b2 = per-car struct size).
8. `UpdateFlag()` if race finished; Update_Other_Objects; `Bonnet_Smoke(i)` per car (damage smoke).
9. Pause handling (Pause_Mode on pad pause bit); demo timeout (counter iVar5 from 0x5dc).
10. `global_counter++`.

## Per DRAWN frame (once)
- `Draw_Car(i)` for player cars [0..current_player_car), then camera/cockpit overlay, then
  `Draw_Car(i)` for AI cars [current_player_car+1..num_cars). (Cars submit polys to the OT.)
- Update_Flying_Objects, Update_Debris (debris physics + submit).
- Camera_Pad_Control (non-demo free-look).
- Draw_Overlays(camera_car) — HUD (speed/damage/lap/pos).
- DrawFlagObject (if finished), Draw_Sky, DrawLensFlare, DrawParticles, Display_Position_Pointers.
- `Draw_All(2)` — FLUSH: process the PSX ordering-table (OT) and blit. IMPORTANT: submission order
  above is NOT draw order; the OT sorts primitives by z (see 02-gte-gpu). Sky etc. land correctly by z.

## Per-car struct
- Cars are a flat array; per-car stride is `0x1b2` bytes (≈434). Damage/timer/state fields live in
  parallel arrays indexed by `carIdx*0x1b2` (e.g. +0x75a71e timer). `camera_car` selects the viewed car
  (player = `_current_player_car`; demo uses car 4).

## Entry / states (main.C / frontend) — TODO detail in 07-frontend
__WinMain → Init_Main → Play_Intro (movie) → frontend state machine → Play_Game (race) → Race_Over →
results/championship. `_current_level` (hex) selects the LEVx data dir; level loaders set it (0=front-end,
1..B=tracks, C/F=result/special screens) and call Set_Load_Textures + Add_Buffer_Load_ per dir.
