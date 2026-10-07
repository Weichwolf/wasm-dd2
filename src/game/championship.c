#include "game/championship.h"

#include "game/course.h"
#include "game/league.h"
#include "game/race.h"

#include <stddef.h>
#include <stdint.h>

enum { DD2_CHAMPIONSHIP_STOCK_ROUNDS = 4, DD2_CHAMPIONSHIP_PROMOTION_CIRCUITS = 5 };

static unsigned dd2_championship_mode_rounds(dd2_race_mode mode) {
    if (mode == DD2_RACE_WRECKING) {
        return DD2_CHAMPIONSHIP_ROUNDS;
    }
    return mode == DD2_RACE_STOCKCAR ? DD2_CHAMPIONSHIP_STOCK_ROUNDS : 0;
}

unsigned dd2_championship_scheduled_track(dd2_race_mode mode, unsigned difficulty, unsigned round) {
    static const unsigned schedule[DD2_LEAGUE_DIVISIONS][DD2_CHAMPIONSHIP_ROUNDS] = {
        {1, 2, 5, 7, 10}, {2, 5, 7, 3, 8}, {1, 7, 3, 6, 9}, {2, 6, 3, 4, 11}};
    if (difficulty >= DD2_LEAGUE_DIVISIONS || round >= dd2_championship_mode_rounds(mode)) {
        return 0;
    }
    return schedule[difficulty][round];
}

bool dd2_championship_valid(const dd2_championship *championship) {
    if (championship == NULL || !dd2_league_valid(&championship->league) ||
        dd2_championship_mode_rounds(championship->mode) == 0 ||
        championship->phase < DD2_CHAMPIONSHIP_READY ||
        championship->phase > DD2_CHAMPIONSHIP_ABORTED || championship->history_count == 0 ||
        championship->history_count > DD2_CHAMPIONSHIP_HISTORY ||
        championship->circuits < DD2_CHAMPIONSHIP_INITIAL_CIRCUITS ||
        championship->circuits > DD2_CHAMPIONSHIP_CIRCUITS || championship->arenas == 0 ||
        championship->arenas > DD2_CHAMPIONSHIP_ARENAS) {
        return false;
    }
    const unsigned rounds = dd2_championship_mode_rounds(championship->mode);
    for (unsigned index = 0; index < championship->history_count; ++index) {
        const dd2_championship_season *season = &championship->history[index];
        if (season->difficulty >= DD2_LEAGUE_DIVISIONS || season->completed > rounds ||
            !dd2_league_valid(&season->standings) ||
            (index != 0 && (championship->history[index - 1].number == UINT64_MAX ||
                            season->number != championship->history[index - 1].number + 1))) {
            return false;
        }
    }
    const dd2_championship_season *current =
        &championship->history[championship->history_count - 1];
    const bool complete = current->completed == rounds;
    if (current->difficulty !=
            DD2_LEAGUE_DIVISIONS - 1 - championship->league.drivers[0].division ||
        (complete && current->outcome != dd2_league_standing(&championship->league, 0)) ||
        (!complete && current->outcome != DD2_LEAGUE_INVALID)) {
        return false;
    }
    if (championship->phase == DD2_CHAMPIONSHIP_ABORTED) {
        return true;
    }
    if (championship->phase == DD2_CHAMPIONSHIP_READY ||
        championship->phase == DD2_CHAMPIONSHIP_RACING ||
        championship->phase == DD2_CHAMPIONSHIP_ROUND_RESULTS) {
        return !complete &&
               (championship->phase != DD2_CHAMPIONSHIP_RACING || championship->ticket != 0) &&
               (championship->phase != DD2_CHAMPIONSHIP_ROUND_RESULTS || current->completed != 0);
    }
    return complete &&
           (championship->phase != DD2_CHAMPIONSHIP_CHAMPION ||
            current->outcome == DD2_LEAGUE_CHAMPION) &&
           (championship->phase != DD2_CHAMPIONSHIP_ELIMINATED ||
            current->outcome == DD2_LEAGUE_ELIMINATED);
}

bool dd2_championship_reset(dd2_championship *championship, dd2_race_mode mode) {
    if (championship == NULL || dd2_championship_mode_rounds(mode) == 0) {
        return false;
    }
    dd2_championship next = {.mode = mode,
                             .phase = DD2_CHAMPIONSHIP_READY,
                             .circuits = DD2_CHAMPIONSHIP_INITIAL_CIRCUITS,
                             .arenas = 1,
                             .history_count = 1};
    if (!dd2_league_reset(&next.league)) {
        return false;
    }
    next.history[0].standings = next.league;
    *championship = next;
    return true;
}

unsigned dd2_championship_round_count(const dd2_championship *championship) {
    return dd2_championship_valid(championship) ? dd2_championship_mode_rounds(championship->mode)
                                                : 0;
}

const dd2_championship_season *dd2_championship_current(const dd2_championship *championship) {
    return dd2_championship_valid(championship)
               ? &championship->history[championship->history_count - 1]
               : NULL;
}

unsigned dd2_championship_track(const dd2_championship *championship) {
    const dd2_championship_season *current = dd2_championship_current(championship);
    return current == NULL ? 0
                           : dd2_championship_scheduled_track(
                                 championship->mode, current->difficulty, current->completed);
}

static bool dd2_championship_rules_valid(const dd2_championship *championship,
                                         dd2_race_rules rules) {
    if (rules.mode != championship->mode || rules.count != DD2_LEAGUE_DRIVERS) {
        return false;
    }
    const unsigned track = dd2_championship_track(championship);
    if (track > DD2_CHAMPIONSHIP_CIRCUITS) {
        return rules.mode == DD2_RACE_WRECKING && rules.length == 0 && rules.laps == 0;
    }
    dd2_course_rules original = {0};
    return dd2_course_original_rules(track, &original) && rules.length >= 3 &&
           rules.length <= UINT16_MAX && rules.laps == original.laps;
}

bool dd2_championship_begin(dd2_championship *championship, dd2_race_rules rules,
                            uint64_t *ticket) {
    if (!dd2_championship_valid(championship) || ticket == NULL ||
        championship->phase != DD2_CHAMPIONSHIP_READY || championship->ticket == UINT64_MAX ||
        !dd2_championship_rules_valid(championship, rules)) {
        return false;
    }
    championship->active_rules = rules;
    championship->phase = DD2_CHAMPIONSHIP_RACING;
    *ticket = ++championship->ticket;
    return true;
}

static bool dd2_championship_scores_valid(const dd2_race *race) {
    uint32_t occupied = 0;
    for (unsigned index = 0; index < DD2_LEAGUE_DRIVERS; ++index) {
        const dd2_race_driver driver = race->drivers[index];
        const unsigned result = race->results[index];
        const unsigned ordered = race->order[index];
        const unsigned finish =
            race->rules.length == 0 ? 0 : dd2_race_finish_points(race->rules.mode, driver.place);
        const unsigned points =
            finish + (race->rules.mode == DD2_RACE_WRECKING ? driver.accident_points : 0);
        if (ordered >= DD2_LEAGUE_DRIVERS || race->drivers[ordered].place != index + 1 ||
            result >= DD2_LEAGUE_DRIVERS || (occupied & (UINT32_C(1) << result)) != 0 ||
            driver.place == 0 || driver.place > DD2_LEAGUE_DRIVERS ||
            driver.accident_points > DD2_LEAGUE_RACE_POINT_LIMIT ||
            driver.finish_points != finish ||
            driver.total_points !=
                (points < DD2_LEAGUE_RACE_POINT_LIMIT ? points : DD2_LEAGUE_RACE_POINT_LIMIT)) {
            return false;
        }
        if (index != 0) {
            const dd2_race_driver previous = race->drivers[race->results[index - 1]];
            const dd2_race_driver current = race->drivers[result];
            if (previous.total_points < current.total_points ||
                (previous.total_points == current.total_points && previous.place > current.place)) {
                return false;
            }
        }
        occupied |= UINT32_C(1) << result;
    }
    return true;
}

static bool dd2_championship_result_valid(const dd2_championship *championship,
                                          const dd2_race *race) {
    const dd2_race_rules rules = championship->active_rules;
    return race != NULL && race->phase == DD2_RACE_RESULTS &&
           race->end >= DD2_RACE_PLAYER_FINISHED && race->end <= DD2_RACE_LAST_SURVIVOR &&
           race->coasting == DD2_RACE_COAST_STEPS && race->elapsed > DD2_RACE_COAST_STEPS &&
           race->finishers <= DD2_LEAGUE_DRIVERS && race->alive <= DD2_LEAGUE_DRIVERS &&
           race->elapsed <= UINT64_MAX - DD2_RACE_START_STEPS &&
           race->steps == DD2_RACE_START_STEPS + race->elapsed && race->rules.mode == rules.mode &&
           race->rules.count == rules.count && race->rules.length == rules.length &&
           race->rules.laps == rules.laps &&
           (race->end != DD2_RACE_PLAYER_FINISHED || race->drivers[0].finish_place != 0) &&
           (race->end != DD2_RACE_PLAYER_RETIRED || race->drivers[0].retired) &&
           (race->end != DD2_RACE_LAST_SURVIVOR || (rules.length == 0 && race->alive < 2)) &&
           dd2_championship_scores_valid(race);
}

bool dd2_championship_finish(dd2_championship *championship, uint64_t ticket,
                             const dd2_race *race) {
    if (!dd2_championship_valid(championship) || championship->phase != DD2_CHAMPIONSHIP_RACING ||
        ticket != championship->ticket || !dd2_championship_result_valid(championship, race)) {
        return false;
    }
    unsigned points[DD2_LEAGUE_DRIVERS] = {0};
    for (unsigned index = 0; index < DD2_LEAGUE_DRIVERS; ++index) {
        points[index] = race->drivers[index].total_points;
    }
    dd2_league league = championship->league;
    if (!dd2_league_add_points(&league, points)) {
        return false;
    }
    dd2_championship_season *current = &championship->history[championship->history_count - 1];
    current->rounds[current->completed] = *race;
    ++current->completed;
    current->standings = league;
    championship->league = league;
    championship->phase = DD2_CHAMPIONSHIP_ROUND_RESULTS;
    if (current->completed == dd2_championship_mode_rounds(championship->mode)) {
        current->outcome = dd2_league_standing(&league, 0);
        championship->phase = DD2_CHAMPIONSHIP_SEASON_RESULTS;
    }
    return true;
}

static bool dd2_championship_next_season(dd2_championship *championship) {
    const dd2_championship_season *current =
        &championship->history[championship->history_count - 1];
    const uint64_t number = current->number;
    if (number == UINT64_MAX) {
        return false;
    }
    dd2_league league = championship->league;
    if (!dd2_league_transfer(&league) || !dd2_league_clear_points(&league)) {
        return false;
    }
    if (current->outcome == DD2_LEAGUE_PROMOTED) {
        const unsigned circuits = current->difficulty + DD2_CHAMPIONSHIP_PROMOTION_CIRCUITS;
        const unsigned arenas = current->difficulty + 2;
        championship->circuits =
            championship->circuits > circuits ? championship->circuits : circuits;
        championship->arenas = championship->arenas > arenas ? championship->arenas : arenas;
    }
    if (championship->history_count == DD2_CHAMPIONSHIP_HISTORY) {
        for (unsigned index = 1; index < DD2_CHAMPIONSHIP_HISTORY; ++index) {
            championship->history[index - 1] = championship->history[index];
        }
    } else {
        ++championship->history_count;
    }
    championship->league = league;
    championship->history[championship->history_count - 1] = (dd2_championship_season){
        .number = number + 1,
        .difficulty = DD2_LEAGUE_DIVISIONS - 1 - league.drivers[0].division,
        .standings = league};
    championship->active_rules = (dd2_race_rules){0};
    championship->phase = DD2_CHAMPIONSHIP_READY;
    return true;
}

bool dd2_championship_continue(dd2_championship *championship) {
    if (!dd2_championship_valid(championship)) {
        return false;
    }
    if (championship->phase == DD2_CHAMPIONSHIP_ROUND_RESULTS) {
        championship->phase = DD2_CHAMPIONSHIP_READY;
        return true;
    }
    if (championship->phase != DD2_CHAMPIONSHIP_SEASON_RESULTS) {
        return false;
    }
    const dd2_league_outcome outcome = dd2_league_standing(&championship->league, 0);
    if (outcome == DD2_LEAGUE_CHAMPION || outcome == DD2_LEAGUE_ELIMINATED) {
        championship->phase = outcome == DD2_LEAGUE_CHAMPION ? DD2_CHAMPIONSHIP_CHAMPION
                                                             : DD2_CHAMPIONSHIP_ELIMINATED;
        return true;
    }
    return dd2_championship_next_season(championship);
}

bool dd2_championship_abort(dd2_championship *championship) {
    if (!dd2_championship_valid(championship) || championship->phase >= DD2_CHAMPIONSHIP_CHAMPION) {
        return false;
    }
    championship->phase = DD2_CHAMPIONSHIP_ABORTED;
    return true;
}

bool dd2_championship_restart(dd2_championship *championship) {
    if (!dd2_championship_valid(championship) || championship->phase != DD2_CHAMPIONSHIP_RACING ||
        championship->ticket == UINT64_MAX) {
        return false;
    }
    championship->phase = DD2_CHAMPIONSHIP_READY;
    championship->active_rules = (dd2_race_rules){0};
    return true;
}
