# 04 — Vehicle handling / physics (car.C, handling.C, suspensn.C, collsn3d.C) — spec (in progress)

Our-words functional spec from decompiled Car_Movement @0x442cdc + neighbours. Fixed-point throughout.

## Per-car state + cadence
- Cars are a flat array; per-car stride **0x1b2** bytes. Parallel global arrays are indexed by
  `carIdx*0x1b2` (e.g. state @0x75a692, out-of-bounds flags @0x75a6c6/0x75a6ca, ground heights
  @0x75a6ee/0x75a6f6, sparking dir @0x75a6be, barrier-hit mask @0x75a7ae).
- Driving INPUT + drive forces are applied every OTHER sim step (`DAT_0073c2c0 & 1`) → ~25 Hz control,
  while the 3D motion integrators + collisions run every sim step (50 Hz, see 03-gameloop).

## Motion state machine (Car_Movement switch on state @0x75a692)
- **case 0 — grounded driving**: Car_Drive_Motion (apply input→drive forces) → FUN_442b08 (common
  pre-step) → Car_Drive_Motion_3D (integrate) → Barrier_Collision for sides 0..3 (OR'd into hit mask).
- **case 1 — 2-point**: Car_Drive_2pt_Motion → FUN_442b08 → FUN_43dd00 (2pt integrate).
- **case 2 — 2pt 3D**: FUN_44282c (input) → FUN_442b08 → Car_2pt_Motion_3D.
- **case 3 — 1pt 3D**: FUN_44282c → FUN_442b08 → Car_1pt_Motion_3D.
- **case 4 — flying** (airborne after big hit/jump): decays two spin terms (@0x75a610/0x75a618) toward 0
  by a Q12 fraction each control step, FUN_444e3c (reset/relax denting matrix), FUN_442b08,
  Car_Fly_Motion_3D (ballistic integrate).
- **case 5 — grounded 3D** (normal on-track): FUN_44282c → FUN_442b08 → Car_Grounded_Motion_3D →
  8× Barrier_Corner_Collision → Sparking (when scraping).
- All non-1/2/3 states then run 8× Barrier_Corner_Collision (car's 8 hull corners vs track barriers).

## Post-step bookkeeping
- Out-of-bounds: if ground-probe heights (@0x75a6ee/0x75a6f6) exceed limits → set OOB flag (reset/respawn).
  Thresholds: <0x1000 in-range; >0xe63 near-edge warn; else OOB.
- Death Bowl (LEVB) special: any car whose tracked Z (@0x752348, stride 0x27c) < -300 → OOB (fell out).

## Sub-functions to spec next (read in full, then add here)
- Car_Drive_Motion / Car_Drive_2pt_Motion / FUN_44282c: pad/AI input → engine torque, brake, steer angle.
- Car_Grounded_Motion_3D (the main integrator): suspension per wheel, tyre traction/slip, gravity,
  velocity + orientation (Q12 matrix) integration. + Calc_Suspension_Right_Wheels / Calc_Null_Suspension.
- Car_Fly_Motion_3D: ballistic + angular momentum (the spin decay above).
- collsn3d.C: Do_Car_Collisions, Check_2D/Ground/Space_Car_Collision (broad+narrow phase car↔car),
  Ground_Collision (car↔terrain), Car_Rolled_Edge_Onto_Wheels (roll recovery).
- handling/barrclsn.C: Barrier_Collision (side) + Barrier_Corner_Collision (8 corners) vs track edge segments.
- damage.C/denting.C: hit impulse → damage value + mesh denting (mid/high_car_vertices, FUN_444e3c).

## Grounded integrator (Car_Grounded_Motion_3D @0x43d8e4) — the on-track model
NOT a free rigid body — it's a TRACK-RELATIVE, TERRAIN-CONFORMING model:
- Two parallel per-car structs: `car_fd` (working frame, stride **0x2c**: position/orientation the
  renderer uses) and a track-position struct (stride **0x27c** @0x752344: track-relative state — segment,
  distance, heading, lateral).
- Each step: copy track-frame → car_fd; `Track_Follow(car_fd)` advances the car along the track path
  segments; `Map_Height(car_fd)` samples terrain height under the car; `Get_Corner_Positions` computes the
  4 wheel-corner world positions.
- Body height = average of the 4 corner ground heights (sum of corner Y >>2) + body offset → written back
  to the track-pos Y (@0x752348) and car_fd height (@0x75a634, ×0x1000 Q12).
- `Calc_Car_Angles_Square(4 corners, dir vectors @0x466b28)` derives pitch+roll from the 4-corner footprint
  on the terrain (the car tilts to match the ground under its wheels). Direction/corner basis from a table
  @0x466ad8 (indexed by the car's facing octant @0x75a6be).
- So: speed/steer (from Car_Drive_Motion, below) drive the TRACK-RELATIVE position; the 3D pose is then
  reconstructed by snapping to the track surface + tilting to the local terrain. This is why cars hug the
  track and bank on slopes without a full suspension sim (suspension = visual wheel travel, Calc_Suspension_*).
- Fly/2pt/1pt states replace this with ballistic/partial integration when airborne or wrecked.

## Drive forces (Car_Drive_Motion @0x4413ac) — Q12 fixed-point velocity model
- Car velocity is a 2-component vector in the ground plane: vx @0x75a610, vz @0x75a618 (Q12). A lateral/
  spin term decays toward 0 each step (`v += -v>>3`-style, ~12.5%/step) when below a speed threshold or on
  low-grip — this is the grip/slip + natural deceleration.
- Control source ptr @0x75a2a0 (per-car pad/AI input block): throttle/brake field, steer field, etc.
- THROTTLE/accel: `force = input * 0xccb0` (Q16 engine scale ≈0.8) `>>0xc` added to the velocity components
  along the car's facing → forward acceleration; top speed emerges from accel vs the per-step decay.
- STEERING: a turn delta applied to heading; rate scaled by current speed and clamped to ±0x2000 (Q12 angle
  rate) — fast = wider turn radius. When a collision flag (@0x75a6c6) is set, steering switches to a
  recovery/counter response instead of player/AI steer.
- The resulting velocity integrates the track-relative position (@0x752344 struct) consumed by
  Car_Grounded_Motion_3D (above). Brake/reverse mirror throttle with opposite/À scaled force.
- Per-class tuning (Rookie/Amateur/Pro) scales accel/top-speed/grip (from car-select); damage reduces
  engine force + can lock steering (the @0x75a6a6.. = -0x3fff wheel-lock writes seen on heavy hits).
- ALL integer Q12/Q16; no float. Bit-exact port must use the same scales (0xccb0, >>0xc, ±0x2000, >>3 decay)
  and the same per-step order to match trajectories.

## Port note
Determinism requires the exact fixed-point math + the fixed per-step order (Car_Movement all cars →
Do_Car_Collisions). Implement integrators in Q-format integers, not float, to match the original bit-for-bit.
