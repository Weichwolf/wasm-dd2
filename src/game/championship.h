#ifndef DD2_GAME_CHAMPIONSHIP_H
#define DD2_GAME_CHAMPIONSHIP_H

#include "game/league.h"
#include "game/race.h"

#include <stdbool.h>
#include <stdint.h>

enum {
    DD2_CHAMPIONSHIP_ROUNDS = 5,
    DD2_CHAMPIONSHIP_HISTORY = 5,
    DD2_CHAMPIONSHIP_CIRCUITS = 7,
    DD2_CHAMPIONSHIP_ARENAS = 4,
    DD2_CHAMPIONSHIP_INITIAL_CIRCUITS = 4
};
typedef enum {
    DD2_CHAMPIONSHIP_READY,
    DD2_CHAMPIONSHIP_RACING,
    DD2_CHAMPIONSHIP_ROUND_RESULTS,
    DD2_CHAMPIONSHIP_SEASON_RESULTS,
    DD2_CHAMPIONSHIP_CHAMPION,
    DD2_CHAMPIONSHIP_ELIMINATED,
    DD2_CHAMPIONSHIP_ABORTED
} dd2_championship_phase;
typedef struct {
    uint64_t number;
    unsigned difficulty;
    unsigned completed;
    dd2_league standings;
    dd2_league_outcome outcome;
    dd2_race rounds[DD2_CHAMPIONSHIP_ROUNDS];
} dd2_championship_season;
typedef struct {
    dd2_league league;
    dd2_race_mode mode;
    dd2_championship_phase phase;
    unsigned circuits;
    unsigned arenas;
    unsigned history_count;
    /* Oldest first, including the current partial season. Copies own all data. */
    dd2_championship_season history[DD2_CHAMPIONSHIP_HISTORY];
    uint64_t ticket;
    dd2_race_rules active_rules;
} dd2_championship;

/* Single-player progression. Initialize through reset; callers must not edit
 * fields. No allocation, randomness or borrowed pointers. Mutations reject
 * invalid input/overflow without changing state or outputs. */
bool dd2_championship_reset(dd2_championship *championship, dd2_race_mode mode);
bool dd2_championship_valid(const dd2_championship *championship);
unsigned dd2_championship_round_count(const dd2_championship *championship);
/* Original schedule, indexed by difficulty (zero is the bottom division).
 * Stockcar uses the first four rounds; Wrecking includes the fifth arena. */
unsigned dd2_championship_scheduled_track(dd2_race_mode mode, unsigned difficulty, unsigned round);
unsigned dd2_championship_track(const dd2_championship *championship);
const dd2_championship_season *dd2_championship_current(const dd2_championship *championship);

/* Begin with the actual driving owner's course length/default lap rules.
 * Output must not overlap championship. A nonzero ticket identifies this round
 * until abort or exactly one successful finish. Finish expects the immutable
 * race owner's naturally completed results, never provisional withdrawal. */
bool dd2_championship_begin(dd2_championship *championship, dd2_race_rules rules, uint64_t *ticket);
bool dd2_championship_finish(dd2_championship *championship, uint64_t ticket, const dd2_race *race);
/* Explicit result-screen continuation. Season transfer/reset/history advance
 * occurs here, after preserving final standings and all completed results. */
bool dd2_championship_continue(dd2_championship *championship);
/* Restart an unfinished round without consuming it. The next begin issues a
 * new ticket; completed rounds and season standings remain intact. */
bool dd2_championship_restart(dd2_championship *championship);
/* Leave without scoring an unfinished race. Completed rounds remain recorded. */
bool dd2_championship_abort(dd2_championship *championship);

#endif
