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
constraints together. World supports and car pairs share a continuous low-speed
friction law: with cone radius `L = mu * N`,
the tangential impulse is `-L * slip / max(0.1, length(slip))`. Below 0.1 world
units/s the opposing impulse grows linearly with final slip; above that threshold
it saturates at the same Coulomb limit. This permits bounded microscopic creep
(1/9,000 of the 900-unit body length per second) at nearly stationary contacts.
This is deliberate rewrite material tuning. It avoids ambiguous ideal sticking
at almost parallel ground/wall supports while preserving normal complementarity,
the friction cone, dissipative response and Coulomb saturation above 0.1 units/s.
The implicit friction softness is capped at 1e12 to keep vanishing
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
coordinate response without secants. A converging accelerated solve retains its
remaining iterations; an unconditional halfway restart can interrupt a healthy
slow mode. Both phases share the unchanged 4,096 total-pass bound. All contacts,
material laws, initial-normal-speed metadata and position repair are retained;
reported velocity passes include work before and after restart. The result
reports the single restart explicitly.

After 512 ordinary sweeps, a bounded Newton correction is available
every 32 passes, including after restart. Ordinary sweeps first settle active
normal/friction branches. A three-axis contact basis describes the projected
normal complementarity and tangential friction equations; finite differences
form their coupled Jacobian. The relative forward-difference step is 1e-6,
scaled by the impulse coordinate or one. Larger probes distort late stick/slip
directions; smaller probes can lose accuracy in accumulated body motion.
Pivoted elimination skips singular directions. A direction predicting negative
normal pressure is refitted with that contact's three impulse coordinates set
to zero; simply clipping it would invalidate every coupled equation. Releases
are monotone within the prediction, bounding refits to the contact count plus
one. They do not remove contacts from the physical solve.

Loaded world supports compare an alternate direction with the ordinary projected
root from the same exact motion and impulses. World-only groups retain their
zero-friction-gradient trial. Groups with car partners instead use the full
constitutive friction root, including the linear/saturated transition and
pressure coupling for every contact. If `L = mu * N` is the cone radius,
`u` is final tangential velocity, and `e = min(0.1, 1e12 * L)`, its root is
`F + L * u / max(e, |u|)`. Zero load and zero slip give zero material response.
This is the existing capped material law, not a new friction coefficient or
creep threshold. An analytic Jacobian differentiates this root and normal
complementarity, using unit-impulse body motion for exact contact mobility.
Each column's response is cached for its one or two receiving bodies; accumulated
velocities are not perturbed to obtain these derivatives. Singular directions
remain skipped by the existing pivoted solve; contacts are never dropped.
The accepted direction with the smaller full physical residual wins. Both
trials retain cone projection, active-set refits, exact rollback and the
unchanged final complementarity/friction tolerances.
The proposed impulses are projected into their cones and backtracked up to
sixteen times. Only finite motion with a strictly smaller physical residual is
accepted; rejected probes restore exact velocities and accumulated impulses.
Convergence is certified after an ordinary sweep, so tiny predicted impulses
at separating contacts are cleared by their unilateral response. That final
sweep counts toward the existing pass bound.
An accepted correction clears ordinary-sweep secant history so the two kinds of
displacement cannot form a false fixed-point prediction. The dense matrix has
at most 192 axes in automatic storage, with no allocation or contact omission.
Corrections and restarts share the existing 4,096-pass bound.

After the 512-pass delay in either phase, mixed car/world groups compare a refined
speculative branch from the same exact input. A full fitted seed may require
previously separating supports to load while friction directions change. Up to
sixteen Newton steps re-evaluate those branches, each with existing pressure
refits, cone projection and bounded backtracking. Intermediate seed/refinement
states remain private and may exceed the outer error; the final candidate must
have finite motion and a strictly smaller full physical residual. Failure
restores exact input, and a weaker candidate restores the previously better
correction. Co-oriented world supports on one body permit the existing
projected Jacobian when the analytic seed is singular or refinement cannot
improve.
Groups without that patch retain the analytic path. One bounded matrix is reused
for every refinement; there are no additional ordinary sweeps or allocations.
Material laws, final tolerances and the total ordinary-pass bound remain unchanged.
Slow progress can consume the entire accelerated phase without triggering a
restart. Private models are therefore eligible in both phases after the existing
delay, rather than requiring a restart that may never happen. Ordinary
certification and exact rejection/rollback still apply. The actual tick-168287
three-car/eight-contact query and sixteen orderings cover this initial phase.
A released-wall fixture can now finish before restarting; its physical/material,
energy, impulse and position checks remain, while the obsolete exact restart
expectation is removed only for that fixture. Other required-restart cases and
the global single-restart bound remain checked.

A regularized direction can remain on a small-slip linear branch even when
its valid solution requires saturated friction. After the delay, mixed groups
with a positive regularized load also compare a saturated constitutive branch
from the same exact outer state. Its private direction uses the normalized
nonzero slip; zero slip keeps the regularized denominator. Both branches retain
at most sixteen refinements, fitted nonnegative pressures, cone projection,
bounded backtracking and exact rollback. The original regularized physical
residual chooses the better final candidate. This changes search directions,
not the 0.1-unit transition or any material law, tolerance or ordinary-pass bound.

Some mixed groups need one car-pair support to leave the linear branch while
other loaded contacts remain regularized. The search additionally enumerates
each loaded small-slip car pair as a selective saturated direction, with every
other contact retaining its constitutive direction. A local typed model records
the selected contact; there is no persistent solver selection state. Every model
starts from the exact same outer motion and impulses and uses the existing
refinement, pressure-fit and backtracking bounds. Only a smaller finite residual
under the unchanged physical law can replace the best candidate.

World loads can approach an inadmissible pressure root while a valid root exists
on another active branch. After the delay, local typed models try doubling each
loaded small-slip world support and releasing each loaded world support, in both
world-only and mixed fields. A higher-load seed applies the changed normal
impulse and solves linear friction equilibrium with every normal load held fixed,
then projects friction into the cones and applies analytic constitutive refinement.
A release direction instead fits the selected support's normal and tangential
impulses to zero while refitting the remaining coupled equations. It starts
from the exact outer state, avoiding the fixed-load friction seed's different
active branch. Subsequent release directions retain those zero-impulse rows;
bounded backtracking uses the fitted complementarity/constitutive equation merit
inside this private refinement. That merit permits a physical-error increase
between private steps while the material root settles; nonfinite equations are
rejected. Projected fallback steps retain their physical-error merit, then
recompute the private equation merit. Their private
constitutive steps retain the fitted tangent direction across sliding/linear
transitions, instead of clipping that direction before the coupled equations
settle. After the bounded refinement, every tangent impulse is projected back
into its cone and the full physical error is recomputed. Only a finite smaller
error after that projection can replace the outer state; rejection restores
exact input. Other trial methods keep their existing per-step cone projection.
Every original contact, including the released support's normal inequality,
contributes to final physical acceptance. Ordinary sweeps can reload a support.
One matrix is reused for the fixed-load solve and at most sixteen refinements.
All intermediate motion/impulses remain private; only a smaller finite physical
residual replaces the outer state. Rejection restores the exact input or the
previously better correction. The model array holds at most four models per
contact plus four base models (260 total). Ordinary sweeps, material laws and
final tolerances are unchanged.

Co-oriented world supports can have dependent normal equations: collinear points
on one rigid body have affine normal velocities. A valid active set may require
several interior supports to release together. After the delay, the search also
tries each world support as the retained member of its matching normal patch,
using the existing normal-axis tolerance and a local bounded contact mask.
The other matching supports retain fixed zero-normal/tangent rows during private
refinement. The retained support uses its active normal equation even when its
initial pressure is zero. All original contacts remain in physical acceptance.

Matching normals do not always permit a single loaded support. Distinct contact
arms can require two loaded endpoints to balance torque while interior rows
release. Each retained world support also compares a second support at the
farthest finite matching point. Both retained normals use active equations,
including from zero pressure; the other matching supports have fixed zero
impulse targets. This adds at most one model per contact, avoiding an unbounded
subset search. It preserves the single-support directions, all normal
inequalities, material law, finite-residual acceptance and exact rollback.

A full fitted release must have exactly zero impulse. Reconstructing it from
nearly cancelling basis components can leave tiny positive pressure/friction;
the pressure-dependent softness then amplifies that roundoff. Released patch
and selected world-release
candidates instead scale their saved normal/tangent impulses by one minus the
backtracking factor, reaching exact zero at the full step. This preserves cones,
finite checks, impulse accounting, the final material law and exact rollback.
The bounded model array holds at most four models per contact plus four base
models (260 total); ordinary pass/refinement/backtracking limits are unchanged.

Both initial refitted directions still compete before branch-model search.
If that accepted correction already has full physical error below the existing
1e-7 velocity tolerance, further private models are unnecessary. Otherwise the
bounded enumeration stops when its best finite, strictly improving candidate
reaches that same tolerance. The next ordinary sweep still certifies normal
complementarity and friction before success; this does not skip that sweep or
weaken any material, pass, rollback or position condition. A captured seven-car,
twenty-contact ordinary campaign query and forty rotated/reversed orderings
exercise the dense path. Its independent constitutive oracle includes the
documented 1e12 softness cap, with explicit valid/invalid checks below, at and
above the corresponding 1e-13 cone radius.

If mixed-contact warm branches still miss a complete root after the delay, one
cold constitutive model privately undoes every accumulated impulse. It
retains every contact and uses the existing analytic material Jacobian, fitted
nonnegative pressure directions, equation merit and sixteen-step refinement.
Its coupled tangent direction remains unprojected inside refinement; final cone
projection and full physical error still control acceptance. A cold candidate
must already meet the unchanged velocity tolerance and strictly improve the
outer error. A partial cold improvement is rejected so it cannot interrupt warm
progress. Rejection restores exact outer motion and impulses; the next ordinary
sweep remains required for certification. This adds one bounded base model,
without additional ordinary sweeps, a new matrix or relaxed material conditions.
The actual step-115420 query and all twenty-four orderings exercise this root.

After the existing warm/cold trials, one private warm model freezes the current
projected normal active set. Its released rows retain exact zero impulse targets;
the other rows retain their normal equations throughout refinement. Negative
full-step pressure predictions use the existing nonnegative candidate and
backtracking instead of forcing additional releases. Fitted equation merit
guides the private tangent fit; cone projection and complete physical tolerance
control outer acceptance. An incomplete improvement restores exact state.
Existing release trials preserve their earlier reduction contract. One base
model increases the bounded maximum to 260, with one matrix, 4,096 shared passes
and sixteen refinement/backtracking steps. The captured five-car/fourteen-contact
tick-223337 query and twenty-eight rotations/reversals check this route.

A world support may need higher pressure before it returns to the linear
friction branch. After the delay, a sliding support's local model uses the larger
of its current load and mean positive incident load as the private pressure
increment, followed by fixed-load friction equilibrium and private constitutive
refinement with fitted tangent directions and equation merit. The incident
scale also initializes an unloaded world support; without positive incident
loads, the trial is rejected. The selected support's
normal equation stays active inside the trial, including at zero pressure.
Provisional negative full-step predictions do not release that selected row;
candidate normal impulses remain clamped nonnegative and backtracking still
applies. Small-slip supports retain their existing pressure model. The
four-model-per-contact bound is unchanged: an unloaded support has no additional
release model. Every
final cone and physical residual is checked, and this alternate must meet the
unchanged velocity tolerance before replacing warm progress. Partial roots are
rejected with exact rollback. The actual five-car, eleven-contact step-108716
query and twenty-two rotations/reversals check this sliding-to-linear transition.
The three-world-contact tick-194353/tick-142893 queries and all six permutations
each check a loaded wall whose previous doubling seed stayed below an admissible stronger
pressure basin. Small-slip LOAD seeds preserve their current-load increment.
No model, matrix, refinement/pass bound or physical acceptance rule is added.

A sliding world support can also need to return to the linear friction branch.
After the delay, a world-only field compares a private refinement of the existing
linear world-friction direction. Its full fitted seed and up to sixteen steps
can pass through a larger intermediate residual before the active pressures
settle. Every candidate keeps normal refits, friction-cone projection, bounded
backtracking and exact rollback. The original physical law accepts only a finite
lower final residual. The extra world-only base model fits the bounded model
array. Its material directions and all physical acceptance checks are unchanged.

Nine arena-B regressions cover a five-contact cycle, simultaneous
sloped-ground/wall/flat-ground support, a ten-contact/eight-body chain, a
nine-contact/seven-body world-friction mode, a fifteen-contact/twelve-body chain
and a seven-contact/four-body fallback, plus an eighteen-contact/thirteen-body
pressure-sensitive support query, plus a ten-contact/seven-body normal-release
query and a seven-contact/six-body transition to sticking. A three-support
Circuit-2 championship query additionally checks a stronger projected correction
against a weaker accepted alternate direction. A four-world-support Circuit-2
query checks the shared low-speed law at the later tick-66,449 failure. A
two-body/seven-contact Circuit-2 query captures the tick-86,643 failure, with six
world supports and one car pair. A second two-body/seven-contact query captures
the tick-107,550 position-repair failure, with five nearly coincident wall
inequalities of different strength. A four-contact query captures the later
trajectory's tick-66,070 position failure with two differently tilted ground
planes, a wall and one car pair. All twenty-four orderings of those four equations
retain the same independent physical checks. An eight-contact query freezes
the tick-81,622 mixed-support velocity failure; sixteen rotations/reversals
retain all impulse, material, energy, clearance and clock checks. A five-contact
query captures tick 105,335, where the car-pair contact must leave the linear
branch; all 120 row permutations retain the same independent checks. Twenty
rotations/reversals of a four-body/ten-contact query captured at tick 105,932
exercise selective friction branches. A three-world-contact query captured at
tick 101,640 checks the positive pressure branch through all six row permutations.
A three-body/five-contact query captured at tick 93,231 checks a released world
support with two loaded car pairs through all 120 contact permutations.
A three-world-support query captured at tick 118,647 checks the sliding-to-linear
transition through all six row permutations. A six-body/eleven-contact query
captured at tick 128,036 checks a separating wall with the remaining ten supports
loaded. Twenty-two rotations/reversals retain the same independent checks.
A four-body/ten-contact query captured at tick 220,415 checks a dependent wall
patch and simultaneous releases through twenty rotations/reversals.
A six-body/fourteen-contact query captured at tick 233,358 checks two retained
wall supports with two released interior rows through twenty-eight rotations/
reversals. Independent analytic and finite-difference roots check the admissible
branch separately from production convergence. A three-world-contact query
captured at tick 83,450 after the stationary reverse retry correction checks a
released wall with one sliding and one linear road support. All six permutations
retain the independent material, impulse, energy, clearance and clock checks.
A two-body/five-contact query captured at tick 190,078 checks a released road/wall
branch whose private steps cross a physical-error ridge. All 120 permutations
retain those independent checks; the accepted response matches a separately
solved feasible material root. Final cone projection and physical acceptance
remain unchanged.
Fourteen
rotations/reversals of each seven-contact query
(twenty-eight total) check convergence without depending on captured row order. All
thirty-one frozen queries retain
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
`rewrite_contact_friction` adds ten analytic world and ten analytic two-body cases
across the linear/saturated transition and six
frozen original-data arena-9 rewrite queries with 15/7/22/10/19/11 contacts. Its
independent checks reconstruct linear/angular impulses from the published
responses, check the constitutive law in final velocity units, and retain normal
complementarity, friction cones, energy loss, clearance, orientation and clocks.
The released-normal fixture requires the bounded coordinate restart on all three
targets. The former world/car cycle converges without a restart under the shared
material law; its physical conditions remain checked.
The stored poses retain their captured values; they are rewrite simulation
states, not original executable trajectories. Passing these cases does not
establish complete natural arena behavior. A separate equal-mass, least-norm
position solve requires complementarity within 1e-9 world units, uses signed support gaps and
preserves shared motion without changing velocity, spin or physical clocks.
After 512 ordinary position sweeps, every 32 sweeps a bounded prediction may
transfer positive multipliers from weaker near-parallel constraints to a stronger
row with the same body partners. Matching uses the existing 1e-8 axis tolerance;
every original inequality remains in the query and full physical residual.
Offsets are rebuilt from all response columns, preserving least-norm stationarity
and equal/opposite pair translations. Exact coordinate projections apply only to
transfer recipients. A prediction must strictly reduce the finite full position
residual; otherwise offsets and multipliers are restored exactly. The 4,096-sweep
budget, 1e-9 position tolerance and ordinary 1.8 relaxation remain unchanged.
`rewrite_contact_group` also checks twenty-four independent analytic least-norm
cases with weaker duplicated world or pair rows and rotated/reversed orderings.
These verify translation, every inequality and unchanged motion/clocks separately
from the captured gameplay state. If the residual remains above tolerance after
that prediction, a bounded active-equation fit uses exact unit-multiplier
translations to assemble the coupled position Gram matrix. Unloaded satisfied
rows enforce zero pressure; active rows enforce their required gaps. A fit with
negative pressure releases that row and refits all remaining equations. Each
refit releases at least one row, so at most contact_count + 1 solves are needed.
Singular fits leave state unchanged. The final finite candidate must reduce the
full residual for every original row, or restore exact offsets and multipliers.
It reuses the velocity solver's checked Gaussian elimination and pivot floor;
there are no allocations or additional ordinary sweeps. Forty-eight analytic
tilted-plane cases provide known least-norm KKT solutions and inactive-plane
slack at three inclinations with rotated/reversed rows, independently checking
pressure release, centered pair translation and unchanged motion/clocks. Every body's remaining motion is swept again after correction.

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
1 MiB stack space for the bounded contact matrix and its callers.
`collision_math.h` shares the existing vector, rotation and
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
