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
Distinct vehicle classes, detached
wheels, off-road recovery and race rules remain pending. Driving decisions are
now implemented separately under `src/ai/`. The current probes
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
settling. Car-pair collision is integrated below; detached parts and automatic off-road
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
axes provide continuous translation/rotation intervals. Each face/cross axis
follows the two interpolated poses rather than holding the midpoint direction.
Endpoint projection chords have a conservative quadratic curvature allowance;
possible time intervals are refined earliest first. An actual-pose SAT check
confirms both geometry and the normal before a physical contact is returned.
The quaternion path bounds include the small endpoint norm error accepted by
`dd2_vehicle_valid`. Initial pieces target 0.01 radians, capped at 64. Each query
uses an explicit depth stack (48 levels) and at most 512 refinement windows.
Exhaustion returns an `unresolved` time bound instead of pretending there is no
contact. This remains source-sized box collision, not exact visual-mesh collision.

For an axis with speed bound W and second-derivative bound C, the signed center
projection has curvature at most 2|d'|W + |d|C. A body's signed support term has
curvature at most C_body + 2W_body W + C. Their weighted sum times interval
width squared / 8 bounds the endpoint chord error. This also bounds the
cross-product axes without normalizing their changing directions; a constant
midpoint scale is applied to the whole inequality. A query first excludes
certified empty ranges, then confirms a contact within the 1e-6 spatial tolerance.
Touching/escaping ranges must stay within that tolerance along the same signed
axis at both endpoints, including curvature. Opposite endpoint sides cannot
certify escape through the other box. Relative quaternion difference/sum gives
stable initial piece counts even for tiny rotations.

Face contacts clip the incident polygon against reference side and front planes
in coordinates relative to the incident face center. This preserves the small
normal offsets of nearly parallel faces even at distant world positions. The
impulse uses the area centroid of a clipped band near the deepest contact edge,
retaining the whole face for parallel contacts. Selecting only vertices in that
band would abruptly discard a distant edge when a tiny tilt crosses its depth
tolerance. Each face direction continuously subtracts a flat-edge allowance of
half the 1e-6 depth tolerance before selecting the band. The front patch uses
the same allowance to prevent numerical skew in a very thin first-contact
slice, anchored at the true deepest incident corner. Replacing slopes around
only the face center can otherwise remove the whole valid patch and return a
fallback point outside both bodies. The corner anchor retains every original
front-patch point and extends allowed projected height by at most one contact
tolerance; the final contact midpoint halves that normal offset; the SAT normal and
time still use the actual geometry. This keeps almost parallel
supporting edges stable without moving the impulse outside the clipped contact
patch. Edge contacts use closest supported edge points.
Bridge-separated boxes do not collide merely because their XZ footprints
intersect. Queries allocate nothing. A twenty-body regression covers five
headings with common lateral motion; a low-speed collision preserves real
near-contact impacts. Sixty analytic cases cover positive/negative rotations
around all three axes from 0.0005 to 1 radian, with both true corner contacts and
nearby misses. They independently solve corner reach and nlerp contact time.
A separate forced-budget test verifies the conservative time bound and the
absence of invented impulse/contact records on Native and WASM.
Another 120 analytic face cases cover pitch/roll in both directions, the band
thresholds and translations to distant coordinates. They independently intersect
a tilted rectangle's bounds and height band. Two physical closing collisions
check that crossing the former vertex threshold cannot create a large spin. A
frozen pair trajectory from original arena 8 additionally checks that a thin
first-contact patch stays inside both oriented boxes and produces the required
closing impulse, instead of repeatedly reporting separating fallback contacts.
Collective overlap correction and coupled support response are integrated below;
continuous face points alone do not establish complete arena gameplay.

`dd2_car_contact_proximity` queries a static contact neighborhood for the
collective solver. Every normalized separating axis must have a gap at most the
requested margin (finite, 0 through the 504-unit body radius). This defines SAT
proximity; a diagonally separated pair's Euclidean distance can exceed that
margin. The result retains signed minimum axis depth, so a negative penetration
describes a nearby gap. Its face clipping admits that margin while retaining the
same deepest-edge band. A successful static query has time zero and is resolved;
stationary touching or separating neighbors are eligible support constraints,
without becoming primary swept collision events. Inputs remain untouched and
failed/invalid queries clear the output. The existing sweep still uses its 1e-6
tolerance and earliest-event clock. `rewrite_car_proximity` checks 108 analytic
face contacts (all local axes, both directions, full orientations and distant
translations), 2,304 independent eight-corner projection queries over all
fifteen axes, exact margin boundaries, reversed pairs and fast-math nonfinite
rejection. The fleet solver uses this query to collect support constraints after
an actual earliest swept event.

`dd2_vehicle_collide_fleet` resolves up to twenty already integrated bodies using
one earliest-event clock for ground, barriers and pairs. It anchors time ties to
the global earliest event, then uses body/pair order. Equal-mass pair response
uses restitution 0.2, friction 0.25, world inertia and opposite impulses at a
shared contact point. At a physical event, the solver collects nearby rounded
barrier probes, road corners and box pairs within twice its 1e-4 clearance, keeps
the primary body's connected component and deduplicates coincident supports.
Pair identity ignores unused barrier metadata. World contacts retain body
corner/probe identity, so swept chord error cannot duplicate an instantaneous
support. The retained contact preserves primary impact metadata and the deepest
queried gap for position repair. A rotating roof-down regression checks distinct
support records, subsequent event chronology, energy loss and road clearance. Primary restitution/friction is
applied once; `contact_group.c` then solves inelastic normal and friction
constraints together. Static world supports use Coulomb sliding/sticking. Car
pairs use a continuous low-speed friction law: with cone radius `L = mu * N`,
the tangential impulse is `-L * slip / max(0.1, length(slip))`. Below 0.1 world
units/s the opposing impulse grows linearly with final slip; above that threshold
it saturates at the same Coulomb limit. This permits bounded microscopic creep
(1/9,000 of the 900-unit body length per second) between cars. It is documented
rewrite material tuning, rather than original physics parity or ideal sticking
between cars. The implicit friction softness is capped at 1e12 to keep vanishing
pressure finite; the cap affects only cone radii at or below 1e-13 impulse units.
Friction uses a fixed step bounded by the trace of the tangential mobility
matrix; a slip-direction mass can oscillate when that
matrix has unequal eigenvalues. A frozen three-car chain from the native
frame-partition test checks convergence, normal complementarity, the car-pair
friction law and conserved linear momentum. Normal complementarity and projected
friction must converge within 1e-7 units/s, with a 4,096-pass bound. Consecutive impulse steps also provide a
secant prediction for slowly converging coupled supports. Its normal impulses
and tangential friction are projected into their admissible cones, and every
quantity is checked before application. A prediction is accepted only with a
strictly smaller finite physical residual. Rejection restores exact saved motion
and impulses. If the best physical residual does not improve by 0.1% over
64 accelerated passes, the solver restarts once from exact original velocities
and angular velocities, clearing all accumulated impulses. It then uses ordinary
coordinate response without secants. Acceleration also stops at pass 2,048 to
reserve half the unchanged 4,096 total-pass bound for that response. All contacts,
material laws, initial-normal-speed metadata and position repair are retained;
reported velocity passes include work before and after restart. The result
reports the single restart explicitly. Two arena-B regressions cover a captured
five-contact cycle and simultaneous sloped-ground/wall/flat-ground support, with
independent contact, impulse, friction, energy and physical-clock checks. The
velocity/position tolerances and pass bound are unchanged.
Normal coordinate responses use their exact unilateral impulse without
over-relaxation. Over-relaxation can amplify modes shared with Coulomb friction,
even while individual secant predictions reduce the residual. A frozen
eight-body/eight-support WASM event from arena 8 previously exceeded the bound
despite those predictions; the exact coordinate response converges with the
same residual tolerance and iteration bound.
`rewrite_contact_group_conditioning` freezes that event and the original-data
arena-9 four-car rewrite event which previously exceeded the bound. Both check
released/active normals,
the car-pair friction law, conserved linear/angular momentum, energy loss,
position clearance, centered repair and unchanged orientation/physical clocks,
including accepted and rejected predictions. These checks cover response at a
certified pose; natural arena completion is verified separately.
`rewrite_contact_friction` adds ten analytic two-body cases across the linear/saturated transition and six
frozen original-data arena-9 rewrite queries with 15/7/22/10/19/11 contacts. Its
independent checks reconstruct linear/angular impulses from the published
responses, check the constitutive law in final velocity units, and retain normal
complementarity, friction cones, energy loss, clearance, orientation and clocks.
The stored poses retain their captured values; they are rewrite simulation
states, not original executable trajectories. Passing these cases does not
establish complete natural arena behavior. A separate equal-mass, least-norm
position solve requires complementarity within 1e-9 world units, uses signed support gaps and
preserves shared motion without changing velocity, spin or physical clocks.
Every body's remaining motion is swept again after correction.

Report incident speed retains actual initial closing/primary impact speed, and
also represents pressure transferred through another contact as effective normal
mass times the inelastic support impulse. Positive impulses remain represented
by positive incident speed; zero-impulse overlap repairs do not cause damage.
Each support is reported at the primary event time and uncorrected contact pose.
Groups which cannot fit the remaining 64 entries retain the previous conservative
serial response for the rest of that step; no successful response is dropped.
A failed joint solve preserves all proposed bodies and clears the public output.
The 64-response budget keeps the last checked poses on exhaustion. An unresolved sweep also stops at its
conservative time bound, increments `unresolved_sweeps`, and leaves velocity,
health and accident attribution without a manufactured response. Validation or final-state failure preserves
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
radians/s for angular velocity, plus 1e-8 relative tolerance. Stationary-field
comparisons disable driving decisions; distinct masses/classes and collision
sound remain pending. Counts record individual solver responses, not unique
accidents. These checks establish rewrite behavior, not original collision parity.

`dd2_vehicle_collide_fleet_report` exposes every selected ground, barrier and
car-pair contact of one fixed step in a caller-owned typed report. The bounded
64-entry array matches the solver's iteration budget: successful responses are
never silently discarded. Each record identifies the bodies/source obstacle,
full-step contact fraction, world point/impulse normal, and both body-local
contact arms at the interpolated pose before overlap correction. Closing speed
and normal impulse exclude friction and distinguish an impact from zero-impulse
overlap repair. Ground/barrier contacts use `DD2_VEHICLE_NO_PARTNER` and a zero
second local point; car pairs use `UINT32_MAX` instead of an obstacle ID. The
legacy aggregate API uses the same solver. Reports and bodies publish together
after validation; a failure clears the report and preserves all proposed bodies.
No callbacks, retained pointers or allocations are involved.

The fleet verifier now reads each 5 ms step's contact report. It independently
reduces records back to all twenty bodies' aggregate counters/strongest impacts,
checks chronology, finite geometry, unit normals and source obstacle bounds on
all eleven levels and three targets. Analytic tests cover frontal/side/rotated
car contacts, a two-impact chain's global times, a fast roof landing, wall contact
arms before rebound, zero-impulse recovery, 64-event exhaustion and invalid-state
rollback. This provides the physical input for damage and accident attribution;
the report itself does not implement damage, scoring or race rules.

Separate controlled roof landings and wall strikes at 5000 world units/s on each
original level exercise road-cell, strip-wall and analytic-arena obstacle IDs.
They check both world report types independently of the coupled three-second
field, which primarily exercises car pairs. The much faster 200000-unit/s
analytic cases remain synthetic tests, not an arbitrary-speed guarantee on
every source slope.

## Regional crush and engine failure

`damage.h` owns six normalized crush regions per car: front, middle and rear,
each split left/right. `dd2_damage_step` consumes every fixed-step contact report
inside the driving transaction. Validation precedes all writes, including counter
overflow checks. Support/repair contacts do not damage a car. For each region,
the strongest weighted impulse in a step contributes once, avoiding duplicate
loads from repeated solver responses. Reset clears all twenty damage states.

The original retires cars when either front zone is exhausted; this rule is
retained. Other damage distributions and magnitudes are rewrite tuning: closing
speed must exceed 500 and impulse per mass must exceed 250; crush is
`min(1, (impulse - 250) / 6000)`. Lateral weights interpolate across X ±186;
longitudinal weights interpolate front/middle/rear over Z ±300. Saturation snaps
within 1e-12 of one for stable fast-math retirement on both targets. Engine health
is one minus the largest front crush. Partial damage reduces throttle by up to
40%; retirement suppresses throttle/steering and applies full braking. Wrecks
remain physical bodies in the shared collision solver.

`dd2_driving_set_damage` disables/freeze damage for isolated diagnostics; actual
application driving enables damage by default. `rewrite_damage` tests regional
impacts, duplicate contact suppression, harmless support, engine failure,
transactional invalid-input rejection and deformation on Native and WASM.
Detached panels/wheels, smoke, repair/pits and race rules are still pending.
Accident points now consume contact reports and retirement state in `src/game/accidents.c`. These tuned checks do not establish original damage parity.

After discontinuous game recovery, `dd2_vehicle_refresh_wheels` rebuilds real
wheel mounts, centers, compression, loads and source contacts without advancing
body motion, steering or the vehicle clock. The game owns the rest deadline and
landing decision; see `src/game/recovery.md`.

`rewrite_convoy` checks 456 complete fixed steps for 2..20 equal-mass boxes,
four overlap depths, three headings and both row axes. An independent closed-form
position solution requires centered repair and full common tangential travel,
with exactly zero impulse and no clock/spin changes. `rewrite_contact_group`
checks an analytic wall/pair pressure cascade, sliding/sticking Coulomb friction,
two-point rotational support with active/released contacts, invalid queries and
bounded failure for contradictory position constraints. These establish targeted
joint-solver behavior; complete original-data arena coverage remains required.

Contact collection first determines the primary event’s connected component from
car-pair proximity. Only that component queries road and barrier support; world
contacts cannot connect separate cars. Primary, barrier, ground and pair insertion
order, support deduplication, contact budgets and response rules are preserved.
This avoids repeated world queries for unrelated cars during dense-field events.
