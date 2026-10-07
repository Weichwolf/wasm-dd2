# Single-player championship ownership

`championship.h` owns value state: stable-ID league standings, original round
schedules, copied race results, unlock counts, phase and the five-season history
window. `championship_session.h` owns the decoded scheduled track and its twenty
physical cars. The session borrows an immutable archive and its bytes until
its destruction. Its const track/driving views support rendering without exposing
score, lap, damage or progression injection.

The original single-player loop initializes the bottom division and schedules
four Stockcar rounds or five Wrecking rounds. Difficulty zero selects
`1, 2, 5, 7, A`; subsequent schedules are `2, 5, 7, 3, 8`, `1, 7, 3, 6, 9`
and `2, 6, 3, 4, B`. Stockcar omits the fifth arena. Difficulty follows the human
league division, independently of the monotonically increasing season number.
The schedule table and placement-point tables are independently decoded from
the original executable by the verifier. The frozen initialization,
`Championship`, `Do_End_Of_Season_Stuff` and history routines supply the transition
reference; production game code contains no original addresses.

A session starts the scheduled track with the league's copied driver-to-physical
slot permutation. Human driver zero remains zero. Actual driving rules supply
course length and original default lap count. A monotonically increasing ticket
binds the current round to its result consumption. Only naturally frozen race
results after full coasting can update the league. Wrong rules, unfinished or
withdrawn results, stale/duplicate tickets and invalid scores fail without
scoring. Driver score caps and placement bonuses come from the existing race
module; no post-race NPC randomness or progress boost is manufactured.

Round results hold simulation and points until explicit continuation. Normal
bounded frame/control validation still applies, and the next frame clears the
previous sound-event batch. The session prepares the entire next track, assigned
field and countdown before replacing owned state. A failed load/allocation leaves
the old results and track/driving views intact. Cancellation preserves completed
rounds and never awards points for the unfinished round.

After the last round, final standings/outcome remain visible before continuation.
Promotion unlocks at least `old difficulty + 5` circuits and `old difficulty + 2`
arenas, starting from four circuits and one arena; relegation never removes
unlocks. Continuing seasons transfer adjacent division leaders/tailenders,
clear points and append a fresh history record. Five records include the current
partial season; advancing beyond that drops the oldest completed record.
Champion/elimination ends progression and retains final scores and the last
physical field. Counter overflow is rejected without wrapping.

`make rewrite-championship-verify` runs strict LLVM19 and both builds, synthetic
rule fixtures and original-data physical session checks on Native, Node/WASM and
fully instrumented ASan/UBSan. Rule fixtures use synthetic race observations;
they cover all division outcomes, history rollover, capped scoring, result copies,
stale/duplicate/malformed result rejection, cancellation and overflow. They do
not establish physically completed seasons. The physical exporter uses only the
existing AI's ordinary player controls on the first scheduled ten-lap circuit,
with real physics, opponents, damage, checkpoints and race ownership. It checks
natural result consumption for all twenty drivers, repeated result-screen frames,
next-track/grid preparation, suspended partial time and unscored cancellation.
A copied archive missing the next track tests preparation rollback; provisioned
original data is never edited. Receipts identify source/binary hashes and label
these scopes explicitly. No per-frame original-physics parity is claimed.

This is a consuming gameplay controller, still awaiting application/menu
integration. Renderer material caches borrow the current track's level/textures
and must be released before that track is destroyed. Application integration
must prepare its new materials/camera together with the next field, using a
prepared transition or a callback before replacement. Calling simple session
continuation underneath the application's old material cache is unsafe. Complete physical campaigns on every schedule/division, original
names and configurable human identity, multiplayer, championship displays,
profile-wide unlock retention, replay/records and compatible save/load remain
under work items 0004, 0003 and 0008. Initial unlock counts are session-owned until
a persistent profile/front end supplies that ownership. This module does not
establish complete championship or full-game acceptance.
