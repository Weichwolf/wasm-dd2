#include "game/accidents.h"
#include "game/championship.h"
#include "game/course.h"
#include "game/drivers.h"
#include "game/laps.h"
#include "game/league.h"
#include "game/race.h"
#include "physics/damage.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_CHAMP_TEST_LENGTH = 10000,
    DD2_CHAMP_TEST_SECOND_SCORE = 200,
    DD2_CHAMP_TEST_WINNER_SCORE = 300,
    DD2_CHAMP_TEST_RIVAL_SCORE = 100,
    DD2_CHAMP_TEST_FIRST_UNLOCK = 5,
    DD2_CHAMP_TEST_STOCK_SECOND = 75
};
typedef struct {
    unsigned human_place;
    unsigned winner;
} dd2_champ_test_placing;
typedef struct {
    dd2_race race;
    dd2_lap_driver laps[DD2_LEAGUE_DRIVERS];
    dd2_vehicle_damage damage[DD2_LEAGUE_DRIVERS];
    dd2_accident_driver accidents[DD2_LEAGUE_DRIVERS];
} dd2_champ_test_round;

static dd2_race_observation dd2_champ_test_observation(const dd2_champ_test_round *round) {
    return (dd2_race_observation){.laps = round->laps,
                                  .damage = round->damage,
                                  .accidents = round->accidents,
                                  .count = DD2_LEAGUE_DRIVERS};
}

static dd2_race_rules dd2_champ_test_rules(const dd2_championship *championship) {
    const unsigned track = dd2_championship_track(championship);
    dd2_course_rules course = {0};
    const bool circuit = dd2_course_original_rules(track, &course);
    return (dd2_race_rules){.mode = championship->mode,
                            .count = DD2_LEAGUE_DRIVERS,
                            .length = circuit ? DD2_CHAMP_TEST_LENGTH : 0,
                            .laps = circuit ? course.laps : 0};
}

static unsigned dd2_champ_test_accident_points(unsigned driver, dd2_champ_test_placing placing) {
    if (driver == 0 && placing.human_place == 1) {
        return DD2_LEAGUE_RACE_POINT_LIMIT;
    }
    if (driver == 0 && placing.human_place == 2) {
        return DD2_CHAMP_TEST_SECOND_SCORE;
    }
    if (driver != 0 && placing.human_place == DD2_LEAGUE_DRIVERS) {
        return DD2_CHAMP_TEST_RIVAL_SCORE;
    }
    return driver != 0 && driver == placing.winner ? DD2_CHAMP_TEST_WINNER_SCORE : 0;
}

static void dd2_champ_test_retire(dd2_champ_test_round *round, dd2_champ_test_placing placing) {
    unsigned next_place = placing.winner == 0 ? 1 : 2;
    for (unsigned driver = 0; driver < DD2_LEAGUE_DRIVERS; ++driver) {
        unsigned place = placing.human_place;
        if (driver != 0) {
            if (driver == placing.winner) {
                place = 1;
            } else {
                if (next_place == placing.human_place) {
                    ++next_place;
                }
                place = next_place++;
            }
        }
        round->laps[driver] =
            (dd2_lap_driver){.steps = 1, .started_laps = 1, .relative = DD2_LEAGUE_DRIVERS - place};
        if (round->race.rules.mode == DD2_RACE_WRECKING) {
            round->accidents[driver].points = dd2_champ_test_accident_points(driver, placing);
        }
    }
    round->damage[0].retired = true;
    round->accidents[0].retired = true;
    round->laps[0].retired = true;
}

/* Synthetic observations exercise real race scoring/coasting and progression
 * rules. They do not represent physically driven races or seasons. */
static bool dd2_champ_test_result(dd2_race *output, dd2_race_rules rules,
                                  dd2_champ_test_placing placing) {
    dd2_champ_test_round round = {0};
    if (!dd2_race_reset(&round.race, rules, dd2_champ_test_observation(&round))) {
        return false;
    }
    for (unsigned step = 0; step < DD2_RACE_START_STEPS; ++step) {
        if (!dd2_race_step(&round.race, dd2_champ_test_observation(&round))) {
            return false;
        }
    }
    dd2_champ_test_retire(&round, placing);
    for (unsigned step = 0; step <= DD2_RACE_COAST_STEPS; ++step) {
        if (!dd2_race_step(&round.race, dd2_champ_test_observation(&round))) {
            return false;
        }
    }
    *output = round.race;
    return output->phase == DD2_RACE_RESULTS && output->end == DD2_RACE_PLAYER_RETIRED;
}

static unsigned dd2_champ_test_rival(const dd2_championship *championship) {
    for (unsigned driver = 1; driver < DD2_LEAGUE_DRIVERS; ++driver) {
        if (championship->league.drivers[driver].division ==
            championship->league.drivers[0].division) {
            return driver;
        }
    }
    return DD2_LEAGUE_DRIVERS;
}

static bool dd2_champ_test_round_run(dd2_championship *championship, unsigned human_place,
                                     unsigned winner) {
    uint64_t ticket = 0;
    dd2_race race = {0};
    const dd2_race_rules rules = dd2_champ_test_rules(championship);
    const unsigned previous = championship->league.drivers[0].points;
    if (!dd2_champ_test_result(
            &race, rules, (dd2_champ_test_placing){.human_place = human_place, .winner = winner}) ||
        !dd2_championship_begin(championship, rules, &ticket) ||
        !dd2_championship_finish(championship, ticket, &race) ||
        !dd2_championship_valid(championship) ||
        championship->league.drivers[0].points != previous + race.drivers[0].total_points ||
        dd2_championship_finish(championship, ticket, &race)) {
        return false;
    }
    const dd2_championship_season *current = dd2_championship_current(championship);
    race.drivers[0].total_points = 0;
    return current != NULL && current->completed != 0 &&
           current->rounds[current->completed - 1].drivers[0].total_points ==
               championship->league.drivers[0].points - previous;
}

static bool dd2_champ_test_season(dd2_championship *championship, unsigned human_place) {
    const unsigned winner = human_place == 1 ? 0 : dd2_champ_test_rival(championship);
    const unsigned rounds = dd2_championship_round_count(championship);
    for (unsigned round = 0; round < rounds; ++round) {
        if (!dd2_champ_test_round_run(championship, human_place, winner) ||
            (round + 1 < rounds && !dd2_championship_continue(championship))) {
            return false;
        }
    }
    return championship->phase == DD2_CHAMPIONSHIP_SEASON_RESULTS &&
           dd2_championship_track(championship) == 0;
}

static bool dd2_champ_test_progression(dd2_race_mode mode) {
    dd2_championship championship = {0};
    if (!dd2_championship_reset(&championship, mode) || championship.circuits != 4 ||
        championship.arenas != 1 || championship.league.drivers[0].division != 3) {
        return false;
    }
    for (unsigned difficulty = 0; difficulty < DD2_LEAGUE_DIVISIONS; ++difficulty) {
        if (dd2_championship_current(&championship)->difficulty != difficulty ||
            !dd2_champ_test_season(&championship, 1)) {
            return false;
        }
        const dd2_championship_season final = *dd2_championship_current(&championship);
        const dd2_league_outcome expected =
            difficulty == 3 ? DD2_LEAGUE_CHAMPION : DD2_LEAGUE_PROMOTED;
        if (final.outcome != expected ||
            final.completed != dd2_championship_round_count(&championship) ||
            !dd2_championship_continue(&championship)) {
            return false;
        }
        if (difficulty < 3 &&
            (championship.circuits != difficulty + DD2_CHAMP_TEST_FIRST_UNLOCK ||
             championship.arenas != difficulty + 2 || championship.league.drivers[0].points != 0 ||
             championship.league.drivers[0].division != 2 - difficulty ||
             championship.history[difficulty].standings.drivers[0].points !=
                 final.standings.drivers[0].points)) {
            return false;
        }
    }
    return championship.phase == DD2_CHAMPIONSHIP_CHAMPION &&
           !dd2_championship_continue(&championship) && !dd2_championship_abort(&championship) &&
           dd2_championship_valid(&championship) &&
           championship.circuits == DD2_CHAMPIONSHIP_CIRCUITS && championship.arenas == 4;
}

static bool dd2_champ_test_history(void) {
    dd2_championship championship = {0};
    if (!dd2_championship_reset(&championship, DD2_RACE_STOCKCAR)) {
        return false;
    }
    enum { DD2_CHAMP_TEST_SEASONS = 8 };
    for (unsigned season = 0; season < DD2_CHAMP_TEST_SEASONS; ++season) {
        if (!dd2_champ_test_season(&championship, 2) ||
            dd2_championship_current(&championship)->outcome != DD2_LEAGUE_STAYS ||
            !dd2_championship_continue(&championship)) {
            return false;
        }
        const unsigned expected_count =
            season + 2 < DD2_CHAMPIONSHIP_HISTORY ? season + 2 : DD2_CHAMPIONSHIP_HISTORY;
        if (championship.history_count != expected_count ||
            championship.history[0].number != season + 2 - expected_count ||
            dd2_championship_current(&championship)->number != season + 1 ||
            dd2_championship_current(&championship)->completed != 0) {
            return false;
        }
        for (unsigned index = 0; index + 1 < expected_count; ++index) {
            if (championship.history[index].completed != 4 ||
                championship.history[index].outcome != DD2_LEAGUE_STAYS ||
                championship.history[index].standings.drivers[0].points !=
                    DD2_CHAMP_TEST_WINNER_SCORE ||
                championship.history[index].rounds[0].drivers[0].total_points !=
                    DD2_CHAMP_TEST_STOCK_SECOND) {
                return false;
            }
        }
    }
    return championship.league.drivers[0].division == 3 && championship.circuits == 4 &&
           championship.arenas == 1;
}

static bool dd2_champ_test_losses(void) {
    dd2_championship championship = {0};
    if (!dd2_championship_reset(&championship, DD2_RACE_WRECKING) ||
        !dd2_champ_test_season(&championship, 1) || !dd2_championship_continue(&championship) ||
        !dd2_champ_test_season(&championship, DD2_LEAGUE_DRIVERS) ||
        dd2_championship_current(&championship)->outcome != DD2_LEAGUE_RELEGATED ||
        !dd2_championship_continue(&championship) || championship.league.drivers[0].division != 3 ||
        championship.circuits != DD2_CHAMP_TEST_FIRST_UNLOCK || championship.arenas != 2 ||
        !dd2_champ_test_season(&championship, DD2_LEAGUE_DRIVERS) ||
        dd2_championship_current(&championship)->outcome != DD2_LEAGUE_ELIMINATED ||
        !dd2_championship_continue(&championship)) {
        return false;
    }
    return championship.phase == DD2_CHAMPIONSHIP_ELIMINATED &&
           dd2_championship_valid(&championship) && !dd2_championship_continue(&championship) &&
           !dd2_championship_abort(&championship);
}

static bool dd2_champ_test_rejected(dd2_championship *championship, uint64_t ticket,
                                    dd2_race race) {
    const unsigned points = championship->league.drivers[0].points;
    const unsigned completed = dd2_championship_current(championship)->completed;
    return !dd2_championship_finish(championship, ticket, &race) &&
           championship->phase == DD2_CHAMPIONSHIP_RACING &&
           championship->league.drivers[0].points == points &&
           dd2_championship_current(championship)->completed == completed;
}

static bool dd2_champ_test_rejections(void) {
    dd2_championship championship = {0};
    if (!dd2_championship_reset(&championship, DD2_RACE_WRECKING)) {
        return false;
    }
    dd2_race_rules rules = dd2_champ_test_rules(&championship);
    uint64_t ticket = UINT64_MAX;
    const dd2_race_rules wrong = {.mode = DD2_RACE_WRECKING, .count = DD2_LEAGUE_DRIVERS};
    if (dd2_championship_begin(&championship, wrong, &ticket) || ticket != UINT64_MAX ||
        championship.phase != DD2_CHAMPIONSHIP_READY ||
        !dd2_championship_begin(&championship, rules, &ticket) || ticket != 1 ||
        dd2_championship_begin(&championship, rules, &ticket)) {
        return false;
    }
    dd2_race race = {0};
    if (!dd2_champ_test_result(&race, rules, (dd2_champ_test_placing){.human_place = 1}) ||
        !dd2_champ_test_rejected(&championship, ticket + 1, race)) {
        return false;
    }
    dd2_race invalid = race;
    invalid.coasting = 0;
    if (!dd2_champ_test_rejected(&championship, ticket, invalid)) {
        return false;
    }
    invalid = race;
    invalid.end = DD2_RACE_WITHDRAWN;
    if (!dd2_champ_test_rejected(&championship, ticket, invalid)) {
        return false;
    }
    invalid = race;
    invalid.rules.laps += 1;
    if (!dd2_champ_test_rejected(&championship, ticket, invalid)) {
        return false;
    }
    invalid = race;
    invalid.results[1] = invalid.results[0];
    if (!dd2_champ_test_rejected(&championship, ticket, invalid)) {
        return false;
    }
    invalid = race;
    invalid.drivers[0].total_points = DD2_LEAGUE_RACE_POINT_LIMIT + 1;
    if (!dd2_champ_test_rejected(&championship, ticket, invalid) ||
        !dd2_championship_finish(&championship, ticket, &race) ||
        championship.league.drivers[0].points != DD2_LEAGUE_RACE_POINT_LIMIT ||
        !dd2_championship_continue(&championship)) {
        return false;
    }
    rules = dd2_champ_test_rules(&championship);
    if (!dd2_champ_test_result(&race, rules, (dd2_champ_test_placing){.human_place = 1}) ||
        !dd2_championship_begin(&championship, rules, &ticket) || ticket != 2 ||
        !dd2_champ_test_rejected(&championship, 1, race) ||
        !dd2_championship_abort(&championship)) {
        return false;
    }
    return championship.phase == DD2_CHAMPIONSHIP_ABORTED &&
           championship.league.drivers[0].points == DD2_LEAGUE_RACE_POINT_LIMIT &&
           dd2_championship_current(&championship)->completed == 1 &&
           !dd2_championship_finish(&championship, ticket, &race);
}

static bool dd2_champ_test_overflow(void) {
    dd2_championship championship = {0};
    if (!dd2_championship_reset(&championship, DD2_RACE_STOCKCAR)) {
        return false;
    }
    championship.ticket = UINT64_MAX; /* Deliberate boundary fixture. */
    uint64_t ticket = 0;
    if (dd2_championship_begin(&championship, dd2_champ_test_rules(&championship), &ticket) ||
        championship.phase != DD2_CHAMPIONSHIP_READY || ticket != 0) {
        return false;
    }
    championship.ticket = 0;
    if (!dd2_champ_test_season(&championship, 2)) {
        return false;
    }
    championship.history[0].number = UINT64_MAX; /* Deliberate boundary fixture. */
    if (dd2_championship_continue(&championship) || championship.history_count != 1 ||
        championship.phase != DD2_CHAMPIONSHIP_SEASON_RESULTS ||
        championship.history[0].number != UINT64_MAX) {
        return false;
    }
    return !dd2_championship_reset(NULL, DD2_RACE_STOCKCAR) &&
           !dd2_championship_reset(&championship, DD2_RACE_TIME_TRIAL) &&
           !dd2_championship_reset(&championship, DD2_RACE_TOTAL_DESTRUCTION) &&
           !dd2_championship_valid(NULL) && dd2_championship_current(NULL) == NULL &&
           dd2_championship_track(NULL) == 0 && !dd2_championship_continue(NULL) &&
           !dd2_championship_abort(NULL) &&
           !dd2_championship_begin(NULL, (dd2_race_rules){0}, NULL) &&
           !dd2_championship_finish(NULL, 0, NULL);
}

static bool dd2_champ_test_restart(void) {
    dd2_championship championship = {0};
    if (!dd2_championship_reset(&championship, DD2_RACE_WRECKING) ||
        dd2_championship_restart(&championship)) {
        return false;
    }
    const dd2_race_rules rules = dd2_champ_test_rules(&championship);
    uint64_t ticket = 0;
    dd2_race race = {0};
    if (!dd2_championship_begin(&championship, rules, &ticket) ||
        !dd2_champ_test_result(&race, rules, (dd2_champ_test_placing){.human_place = 1}) ||
        !dd2_championship_restart(&championship) || championship.phase != DD2_CHAMPIONSHIP_READY ||
        championship.league.drivers[0].points != 0 ||
        dd2_championship_current(&championship)->completed != 0 ||
        !dd2_championship_begin(&championship, rules, &ticket) || ticket != 2 ||
        dd2_championship_finish(&championship, 1, &race) ||
        !dd2_championship_finish(&championship, ticket, &race) ||
        dd2_championship_restart(&championship)) {
        return false;
    }
    return championship.league.drivers[0].points == DD2_LEAGUE_RACE_POINT_LIMIT &&
           dd2_championship_current(&championship)->completed == 1 &&
           !dd2_championship_restart(NULL);
}

static bool dd2_champ_test_schedule(void) {
    const dd2_race_mode modes[] = {DD2_RACE_WRECKING, DD2_RACE_STOCKCAR};
    for (unsigned mode = 0; mode < sizeof(modes) / sizeof(modes[0]); ++mode) {
        const unsigned rounds = modes[mode] == DD2_RACE_WRECKING ? DD2_CHAMPIONSHIP_ROUNDS : 4;
        for (unsigned difficulty = 0; difficulty < DD2_LEAGUE_DIVISIONS; ++difficulty) {
            printf("{\"kind\":\"schedule\",\"mode\":%u,\"difficulty\":%u,\"tracks\":[",
                   (unsigned)modes[mode], difficulty);
            for (unsigned round = 0; round < rounds; ++round) {
                printf("%s%u", round == 0 ? "" : ",",
                       dd2_championship_scheduled_track(modes[mode], difficulty, round));
            }
            puts("]}");
        }
        if (dd2_championship_scheduled_track(modes[mode], DD2_LEAGUE_DIVISIONS, 0) != 0 ||
            dd2_championship_scheduled_track(modes[mode], 0, rounds) != 0) {
            return false;
        }
    }
    return dd2_championship_scheduled_track(DD2_RACE_TIME_TRIAL, 0, 0) == 0;
}

int main(void) {
    for (unsigned driver = 0; driver < DD2_LEAGUE_DRIVERS; ++driver) {
        printf("{\"kind\":\"name\",\"driver\":%u,\"name\":\"%s\"}\n", driver,
               dd2_driver_name(driver));
    }
    const bool names = dd2_driver_name(DD2_LEAGUE_DRIVERS) == NULL;
    const bool restart = dd2_champ_test_restart();
    const bool schedule = dd2_champ_test_schedule();
    const bool stock = dd2_champ_test_progression(DD2_RACE_STOCKCAR);
    const bool wreck = dd2_champ_test_progression(DD2_RACE_WRECKING);
    const bool history = dd2_champ_test_history();
    const bool losses = dd2_champ_test_losses();
    const bool rejection = dd2_champ_test_rejections();
    const bool overflow = dd2_champ_test_overflow();
    printf("Championship restart: %s\n", restart ? "PASS" : "FAIL");
    printf("Synthetic championship rules: stock=%u wreck=%u history=%u losses=%u rejection=%u "
           "overflow=%u\n",
           (unsigned)stock, (unsigned)wreck, (unsigned)history, (unsigned)losses,
           (unsigned)rejection, (unsigned)overflow);
    return names && restart && schedule && stock && wreck && history && losses && rejection &&
                   overflow
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
