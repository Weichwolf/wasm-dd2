# AI

Own opponent behavior and driving decisions using typed game/track observations.
Pass randomness and simulation timing explicitly so driving scenarios can be
reproduced independently of rendering.

`path.c` provides allocation-free forward guidance through the decoded source
road graph. Normalized lane positions interpolate lane centers across changing
road widths; partial edge cells use their active triangle centroid. Horizontal
arc length and projection provide a lookahead point, segment direction, width
and maximum encountered curvature. Source next-links preserve alternate-branch
starts and main-loop closure. This is path geometry, not lap/checkpoint progress
or a policy for selecting an alternate route. Degenerate paths and invalid or
nonfinite queries clear their output, including under fast-math.

`driver.c` makes one decision per 5 ms simulation step from the simultaneous
pre-step field, keeping cell/lane, target, stuck/reverse timers and decision count
in a typed state. Road-height sampling distinguishes stacked levels. Racing
uses speed-dependent lookahead, curvature braking, yaw damping and traffic
headway. Passing compares both candidate lane corridors before choosing a shift;
the planned corridor supplies the speed constraint. Arenas initially target the
opposite starter slot, then periodically choose nearby cars with a forward
preference and velocity lead. Total Destruction instead explicitly keeps player
slot zero as the target; its periodic nearest-car selection is bypassed and
inverted braking retains that same target. Prolonged stalls and low-speed backward targets
trigger timed reverse/steering maneuvers. The driver also checks accepted horizontal
positions: it must move at least 120 world units from a retained anchor within
300 decisions (1.5 seconds). This detects contact-limited travel even when the
velocity still exceeds the 80-unit/s low-speed threshold. Small oscillations do
not reset the position timer. Reverse, inverted braking and missing guidance
restart this observation window; an expired reverse gets a fresh forward attempt.
The low-speed counter counts only forward decisions and clears during reverse;
otherwise a stationary reverse
would fill that counter and immediately rearm itself on expiry. A zero-velocity
regression requires two complete 1.5-second forward/reverse cycles, separately
from the accepted-position cases that advertise substantial velocity.
Position history belongs to the driver and resets with the rest of its state.
An inverted car brakes; tire traction, body support and collision impulses remain
owned by physics.

Source behavior references are `AI_Com_Server`, `Determine_AI`,
`Recommended_Acceleration`, `InitialiseAI` and `Obstacle_Ahead` in the reference
reconstruction. The new controller uses world units/seconds and explicit typed
observations, rather than the original global byte-command interpreter. Default
speed caps are 3200 on roads and 2000 in arenas; curvature load 650, 900-unit
wheelbase, 0.6-second lookahead contribution and 1.5-second reverse maneuvers are
rewrite tuning. All observations/driver state are validated before a decision;
failure preserves the state and clears control. The caller must keep matching
road/surface geometry alive. No pointers or allocations are retained.

`make rewrite-ai-verify` checks source-linked path queries from every original
racing strip, three lateral positions and three lookahead lengths against an
independent archive/image reader, on Native, Node/WASM and ASan/UBSan. Sixty-second
twenty-car scenarios on all eleven levels require sustained movement, at least
95% tire-supported samples, finite state, exact decision cadence and full reset.
Synthetic tests exercise an analytic right-angle path, whole-loop wrap, straight
control, zero-speed and position-limited stalls, slow creep, oscillation,
sustained travel, reverse expiry and transactional nonfinite rejection.
Frame-partition, pause/reset and real window/browser presentation checks include
the driving AI.
Field encounters can diverge across targets due to floating-point-sensitive
traffic choices; the original-data test checks each target's behavior, not exact
cross-target race trajectories or replay determinism.

Opponent personality/handicaps, difficulty, damage-aware tactics, pit behavior,
deliberate branch choice, off-road rescue and complete race
rules remain pending. The game owner now rights supported resting overturned
cars after two seconds; distant opponents additionally use the source 8192-unit
boundary. See `src/game/recovery.md`. This establishes driving/pursuit, not complete original AI
parity or a complete race.
