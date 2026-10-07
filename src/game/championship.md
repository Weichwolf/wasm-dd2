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

The shared application now owns this session alongside its retained practice
track. Native C/N and the browser's mode selector enter Wrecking/Stockcar
championships. Enter continues round/season results; R restarts only an unfinished
round, and Escape/F7 leaves without awarding unfinished points. The browser also
provides restart, next-race and leave buttons. Track selection is locked during a
scheduled campaign. Const queries, audio events and driving presentation use the
active owned field. Results show all four divisions, actual ranks/points, the
nineteen original NPC names and the default human label PLAYER. Configurable human
identity remains profile work.

Prepared transitions expose candidate track/driving/state views while retaining
the old field. The application prepares materials/camera against that candidate,
releases the old borrowed material cache, then commits without allocating. Failure
or explicit candidate destruction preserves the original field and results.
Tickets reject stale competing transitions; restart retains completed rounds and
season points, recreates the assigned grid/countdown and issues a new ticket.
Loaded field metadata remains separate from the next scheduled track on results.
Candidate transitions must be destroyed before their owner. All calls remain on
the application's main thread.

`make rewrite-championship-application-verify` checks actual shared application
lifetime/presentation plus production Native/browser input. The application
exporter supplies ordinary AI analog controls through the same bounded advance
path as the keyboard. First scheduled natural ten-lap results, all twenty league
scores, frozen results, next-field rendering, restart, missing-next-track rollback,
unscored exit and close/reopen are checked on Native, Chromium/WASM and ASan/UBSan.
X11/canvas presentation must match the actual application framebuffer; an
independent glyph oracle checks all twenty visible names, ranks and points.
These are real first-round sessions, not physical full-season or original-frame
comparisons. The reusable application library separates its entry-point wrapper
and offers bounded analog input, const views and presentation without advancing
or injecting game state.

Complete physical campaigns on every schedule/division, full original menus,
configurable identity, multiplayer, profile-wide unlock retention, replay/records
and compatible save/load remain under 0004, 0003 and 0008. Work item 0025 remains
active for full season/outcome application flows and reusable lifecycle audits.
Initial unlock counts remain session-owned until a persistent profile/front end
supplies that ownership. This module does not establish complete championship or
full-game acceptance.
