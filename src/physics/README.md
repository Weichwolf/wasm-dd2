# Physics

Own vehicle motion, collision and damage with an explicit simulation step.
Keep rendering and operating-system calls out of this module. Verify stable
behavior on native and WASM, including fast-math edge cases.

`road_contact.c` provides vertical contact with one known road/arena lane cell.
It selects an active triangle in XZ, includes shared edges, rejects missing or
zero-area triangles and returns its interpolated height and unit normal with
positive Y. Calculations use double precision and convert source coordinates
before subtracting, avoiding signed integer overflow at coordinate extremes.
Queries must be finite. Edge tolerance is 1e-6 in world coordinates, converted
to each edge's barycentric scale; it cannot grow with the cell size. An expanded
triangle bounding rectangle also limits extrapolation at acute corners. Points
beyond this numerical tolerance fail. Signed-coordinate extremes and a point
one unit outside an extremely large cell are checked on both targets.

`road_surface.c` selects a contact without knowing its cell beforehand. Create
one surface index for a decoded road and destroy it before destroying that road:
the index owns its balanced XZ bounds hierarchy and borrows immutable geometry.
Construction and destruction allocate; queries have no allocations, rendering
or operating-system calls. Both construction and traversal are iterative.

A query supplies XZ coordinates, a finite inclusive height window and an optional
preferred cell (`DD2_ROAD_NO_STRIP` for none). It returns the highest eligible
contact. Contacts within 1e-6 below the global maximum form a tie: the preferred
cell wins if eligible, otherwise the lowest cell index wins. Two traversal
passes anchor ties to the global maximum, avoiding order-dependent tolerance
chains. Height windows distinguish stacked road surfaces; they do not model
wheel travel, motion through walls or recovery outside the track. Invalid
coordinates/windows fail and clear the result and optional statistics. An
integer byte-representation check rejects NaN and infinity even with the
required fast-math compiler flags.

Synthetic CTests run on native and WASM. `make rewrite-road-verify` checks the
original contact geometry against independent plane calculations with ASan/UBSan
as well. Source contact normals are not used, and original fixed-point height
rounding is not a rewrite requirement. `make rewrite-surface-verify` also compares
surface selection with an exhaustive, ID-ordered cell search on all eleven
original levels, on native, Node/WASM and ASan/UBSan. Each target checks 197,750
queries covering both triangle centroids with three height windows and all cell
corners. The oracle uses the separately verified contact primitive, without the
index hierarchy. Selection traces and work counts agree across targets. The
index performs about 4.1–4.8 actual cell contact tests per query on average,
including both passes, and the verifier requires at least 95% pruning relative
to a full cell scan. These counts describe the sampled queries, not a guarantee
for arbitrary geometry or a timing benchmark.

Synthetic tests include stacked planes, strict height boundaries, shared edges,
preferred-cell ties, tolerance chains, acute corners, coordinate extremes and
NaN/infinity under fast-math. These checks establish contact selection, not
driving parity.

## Vehicle core

`vehicle.c` advances a typed four-wheel rigid body in fixed 5 ms steps. State is
position, linear/world angular velocity, a unit quaternion, front steering angle
and four wheel contact/force records. It owns no allocations and retains no
geometry pointers. `dd2_vehicle_reset` accepts structured position/yaw start
data; `dd2_vehicle_step` borrows a road and its matching surface index for one
step. The caller must preserve that geometry and accumulate simulation time
independently of presentation. Wheel outputs describe the beginning of the last
step. Invalid controls/core state and out-of-bounds results fail transactionally.
Start/control data use structs so finite-value checks remain effective under
fast-math; a standalone floating yaw argument was experimentally rejected after
its NaN check was optimized away.

Coordinates retain +Y up, local +Z forward and local +X right. The original wheel
rig (`FUN_00440bf4 @ 00440bf4`, tables in the provisioned image) has local centers
at X ±186, Y -130, Z ±450. The rewrite uses those XZ locations and the same fully
extended center Y, with mounts at -60, 70 units of suspension travel and a tuned
60-unit wheel radius. Wheel order matches the rig: front right, rear right,
front left, rear left. These dimensions are separate from pending animated
wheel geometry.

Each wheel samples a finite vertical window and preferred previous cell. A
nonnegative spring/damper load acts along the road normal. Projected wheel
forward/right directions apply drive, braking, rolling resistance and lateral
grip, bounded together by available load and surface friction. The original
playable roads contain surface classes 0 and 1; class 1 has half grip, matching
the ratio of the original friction/traction tables. Front wheels steer with a
limited slew rate; reverse steering follows force direction naturally. No tire
force acts without a reachable upright contact. Linear gravity and drag,
rigid-body torque including the gyroscopic term, and normalized quaternion
integration update the pose. A swept vertical landing window limits penetration
at full compression and keeps a vehicle below a bridge on its existing surface.
This is vertical road contact, not a general swept body/wall collision solver.

The current default spring, damper, acceleration, steering and speed curve are
rewrite tuning in world units/seconds. They are not a transcription of the
original fixed-point update cadence or its visual suspension recurrence. On a
flat road, spring coefficient 90 and gravity 2500 give a rest height of
`190 - 2500 / (4 * 90) = 183.055555...`, checked independently in the synthetic
tests. Tests also cover acceleration, stopping, reverse motion/steering, slopes,
reduced grip, flight without ground controls, bridge selection, fast downward
travel, rotation and transactional NaN/infinity rejection.

`make rewrite-vehicle-verify` exercises 24 distributed cell starts on each of the
eleven original tracks/arenas, alternating nominal height and drops. Five seconds
per start include settling, forward throttle/steering, braking and reverse:
264,000 simulation steps per target. Every 20 steps, native, Node/WASM and
ASan/UBSan compare pose, velocity, orientation, steering and wheel load/compression
within absolute 1e-5 or relative 1e-8 tolerance; discrete contacts and summaries
must match exactly. The largest observed numeric difference is below 2e-8.
Completed raw samples and instrumented binaries are removed after the report.

The inspection viewer does not yet drive this vehicle. Real driving input,
vehicle/wheel rendering and chase camera are the next integration work. Distinct
vehicle classes, body/wall/car collisions, upside-down support, damage, detached
wheels, off-road recovery, AI and race rules remain pending. The current probes
can leave the road and fall; they establish cross-target dynamics and the named
synthetic behavior, not original driving parity or complete race correctness.
