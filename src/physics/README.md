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

Real driving input, vehicle/wheel rendering and chase camera are integrated in
free driving. Track barrier response is now a separate post-integration step.
Distinct vehicle classes, damage, detached
wheels, off-road recovery, AI and race rules remain pending. The current probes
can leave the road and fall; they establish cross-target dynamics and the named
synthetic behavior, not original driving parity or complete race correctness.

`barrier_world.c` borrows immutable source barriers and owns a balanced 3D bounds
index. A swept circular horizontal footprint and vertical interval intersect the
finite line part and rounded endpoints. Enumerated circle/line/height intervals
catch fast travel across a thin barrier without per-frame position sampling.
Ray coordinates avoid distant quadratic discriminant cancellation. Earliest
contact selection uses two passes, followed by lowest owned barrier-ID ties
within 1e-10; depth-first traversal is bounded and allocates nothing. Arena
contacts use the original analytic radius, at any height as in the reference.
Road walls have a tuned height of 400 units to keep bridge levels independent.

`vehicle_collision.c` resolves the proposed fixed step using five overlapping
rounded body lobes (186-unit radius along Z ±264, matching a 372 x 900 footprint)
and a 130-unit vertical half-height. Lobes follow full body orientation; rotation
arcs conservatively inflate the swept chords. These are gameplay proxies, not
exact mesh collision shapes. Earliest contacts split the remaining motion;
restitution 0.2, bounded friction 0.25 and mass-normalized world inertia apply
linear/angular impulses at the contact point. Initial overlaps are corrected,
and moving away from a touching surface is allowed. After at most 16 responses,
the last corrected pose is kept rather than accepting unchecked residual travel.
Failure leaves the proposed vehicle untouched. The game owner retains a
resettable 64-bit count of collision contacts for events and verification.

`make rewrite-barrier-verify` independently checks all 6,492 original collision
lines (including diagonal widening/narrowing boundaries) and four arena radii,
plus 13,048 earliest swept queries per target against exhaustive geometric
boundary-event enumeration. Native, Node/WASM and ASan/UBSan agree on discrete
selection/work counts and numerical contacts. CTests exercise fast head-on and
glancing response, energy loss, overlap correction, escaping contact, rotation,
separate heights, distant tangency and nonfinite rollback. Full original-data
free-driving snapshots and real window/browser lifecycle/input checks run too;
a real browser reverse key drives into an arena boundary. This establishes
barrier geometry/response, not original fixed-point collision/damage parity,
other-car collision or complete race correctness.

## Body ground support

The surface index also supports allocation-free one-sided point/triangle sweeps.
It enumerates active triangles in intersecting XZ bounds, intersects the motion
with their planes and tests the crossing point inside the actual triangle.
This catches a fast crossing even when both endpoints lie outside the triangle.
Two passes anchor time ties to the global earliest contact (1e-10), then prefer
the lowest cell and triangle. Bounded initial overlap recovery ignores higher
surfaces already above the start; upward motion from a touching road is allowed.
Finite input validation remains effective under fast-math. Vertical samples
retain their existing height-window and preferred-cell semantics.

`dd2_vehicle_collide_world` sweeps the eight source contact-box corners from
`Get_Corner_Positions` (X ±186, Y ±130, Z ±450). These are source collision
dimensions, separate from the visual body offset and mesh bounds. Inverted,
sideways and spinning cars now have body support, using inelastic normal impulses
and tuned friction 0.8 with the same world inertia as the suspension/barriers.
Initial ground overlap recovery is limited to 25 units along the normal. Short
angular chords approximate rotated corner paths (target 0.02 radians, at most
16 pieces). Corner time ties prefer source order. This is a corner-contact proxy,
not exact mesh/triangle collision or an exact continuous rotating-box solver.

Ground and barrier contacts share earliest selection and the remaining fixed
step. Rebound/friction can change both linear and angular travel before the next
sweep. At most 64 combined responses keep the last checked pose on exhaustion;
the barrier-only entry point retains its 16-response budget. Failure preserves
the proposed state. Ground contact is now integrated in free driving and reset
settling. Car-pair collision is integrated below; damage and automatic off-road
recovery remain pending.

`make rewrite-ground-verify` independently reads the source box and original
road planes. Short/fast vertical sweeps at both triangle centroids of every cell
include missing triangles and bridge overlap. Each level also runs three seconds
of upright, inverted, sliding and spinning drops from its original grid position.
Native, Node/WASM and ASan/UBSan compare contacts and sampled motion. Synthetic
tests additionally check an outside-to-outside crossing, bridge separation,
upward escape, bounded overlap repair, inverted/sideways rest, slope/sliding
friction, a 200,000-unit/s drop, fast rotation, impulse energy loss, unchanged
upright suspension and transactional nonfinite rejection. These checks establish
body-ground behavior, not original collision parity or complete races.

The original-data check covers 79,100 sweeps and 26,400 dynamics steps per target.
Analytic plane comparisons require 1e-8 absolute or 1e-10 relative error and exact
source contact selection. Dynamics use explicit absolute bounds: position 0.1
world units, velocity 0.2 units/s, quaternion components 1e-4 and angular velocity
1e-3 radians/s (plus 1e-8 relative). Spinning drops permit two grazing solver
responses' difference while requiring the same support outcome; other drops
require equal response counts. The observed maximum difference is about 0.1245
units/s on the Arena A spinning drop. These are sub-coordinate movement
tolerances, not a claim of bitidentical dynamics or exact replay determinism.

## Vehicle pairs and starter field

`car_contact.c` sweeps the source 372 x 260 x 900 oriented contact boxes against
each other. A relative-motion sphere test prunes distant pairs. Fifteen separating
axes provide continuous translation intervals within each angular piece. Rotation
uses midpoint orientations with conservative chord padding, targeting 0.01 radians
per piece and at most 64 pieces. This gives a small contact skin for normal fixed
steps, not exact continuously rotating mesh collision. Face contacts clip the
incident face against reference side planes and use its area centroid, so adding
duplicate or collinear clipping vertices cannot move the impulse point. Relative
quaternion difference/sum gives stable angles even for tiny rotations. Edge contacts use closest supported
edge points. Bridge-separated boxes do not collide merely because their XZ
footprints intersect. Geometry queries allocate nothing.

`dd2_vehicle_collide_fleet` resolves up to twenty already integrated bodies using
one earliest-event clock for ground, barriers and pairs. It anchors time ties to
the global earliest event, then uses body/pair order. Equal-mass pair response
uses restitution 0.2, friction 0.25, world inertia and opposite impulses at a
shared contact point. Each response rechecks every body's remaining motion;
initial overlaps receive symmetric separation. The 64-response budget keeps
the last checked poses on exhaustion. Validation or final-state failure preserves
every proposed body and clears known-size event outputs. Typed stack copies keep
the solve transactional without per-step allocations; WASM consumers reserve
256 KiB stack space. `collision_math.h` shares the existing vector, rotation and
impulse calculations with single-body ground/barrier entry points.

Synthetic Native/WASM/ASan tests check analytic fast frontal contact times and
rebound speed, glancing angular response, energy loss, rotating impact, three-car
momentum transfer, separate bridge heights and invalid-state rollback.
`make rewrite-fleet-verify` checks the full original starter field on all eleven
levels for 132,000 vehicle steps per target. Component bounds are 0.1 world units
for position, 0.2 units/s for velocity, 1e-4 for quaternion/wheel roll and 1e-3
radians/s for angular velocity, plus 1e-8 relative tolerance. Other cars currently
hold their brakes; driving AI, distinct masses/classes, damage and collision
sound remain pending. Counts record individual solver responses, not unique
accidents. These checks establish rewrite behavior, not original collision parity.
