# Supported overturns and recovery

`recovery.h` owns one small counter/flag record per physical vehicle. The driving
owner observes fully resolved body motion once per 5 ms integration step, after
lap and accident attribution. A recovery is a discontinuous placement; it cannot
credit a lap, manufacture an impact/KO, repair damage or advance a vehicle clock.
Countdown, pause, suspended input and published results freeze this state. Reset
clears it. The complete field is copied and published transactionally, including
counter-overflow rejection. Road, surface and world are borrowed immutable data.

The functional reference is `FUN_00440ab0` and `Car_Landed` in `re_out/dd2.c`.
The original waits 100 continuous 20 ms rest updates (two seconds). A living
player can then be righted; an opponent additionally needs an XZ distance greater
than 8192 source world units from the player. That threshold follows from the
original squared-distance division by 4096 and comparison against 0x4000. The
rewrite saturates its 400-step counter and retries a nearby/blocked opponent on
later steps, avoiding the original exact-deadline opportunity being lost forever.
An engine-retired wreck is never repaired or righted by recovery.

Rest classification uses rewrite tuning: body up Y below 0.2, angular speed at
most 0.5 rad/s, a source body corner within two Y units of an upward road triangle,
and road-normal speed at most 50 world units/s. Airborne/spinning cars are not
unavailable merely because their roof is down. Any interrupted rest clears the
counter. The eight contact corners are shared with the physical world solver.
When corners touch different banks, recovery selects the road normal most
aligned with the downward-facing roof, retaining source-corner order for equal
alignment. A first corner on the opposite bank can extrapolate a center height
outside the real road's local window and prevent every later recovery attempt.
The selection still requires actual corner contact and bounded normal speed;
the landing window, deadline and NPC distance rule are unchanged.

Landing preserves XZ position and horizontal heading, queries the actual road triangle under the vehicle center
within a local height window and aligns body up to its normal. Placement keeps
the flat 190-unit ride height and accounts for the wheel model's vertical
60-unit radius on slopes: center height = road + 130 / normal Y + 60. Using
190 / normal Y instead leaves all tires above a banked road. A one-millionth
unit placement skin avoids an exact-touch query missing support through rounding. The regular world solver corrects static overlaps with zero velocity,
so righting creates no impact impulse. It then retains only road-tangent incoming
velocity, clears angular velocity, preserves steering/step count, and refreshes
actual four-wheel geometry without integrating another tick. Placement without
any real tire support is postponed. Pair response occurs in the next ordinary
coupled physics step; dense-field contact convergence remains separate work.

The race observer counts a temporarily resting overturned car as unavailable for
an arena ending, matching `GetRacePositions`. This status does not set permanent
engine retirement, a retirement timestamp or destruction credit. A living player
still participates in timing; clearing the temporary status restores availability.
An already triggered coasting ending remains latched.

`make rewrite-recovery-verify` checks roof/side/nose rest deadlines, upper/lower
bridge support, airborne/spinning rejection, NPC distance boundaries and later
retry, permanent wrecks, frame rollback and the no-extra-tick invariant. It also
checks an independently constructed two-ramp valley where the first touching
corner lies on the opposing bank, while the roof rests on the center bank.
The old selection fails this case; the corrected selection rights the car with
real tire support at the same deadline. The verifier additionally
physically drops a partially damaged player car roof-down at all twenty source
grid positions on all eleven levels. Native, Node/WASM and ASan/UBSan separately
settle, right and accelerate it. An independent reader checks the original grid,
body corners, supported plane and every grounded wheel's triangle/height/normal.
The report retains source/binary/data identities; successful raw logs are deleted.
These controlled single-car cases do not prove natural arena completion or
original handling parity. Off-road rescue and full game coverage remain pending.
