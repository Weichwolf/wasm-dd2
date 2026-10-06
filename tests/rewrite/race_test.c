#include "game/accidents.h"
#include "game/course.h"
#include "game/laps.h"
#include "game/race.h"
#include "game/recovery.h"
#include "physics/damage.h"
#include "physics/vehicle_collision.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_RACE_TEST_CARS = 4,
    DD2_RACE_TEST_LENGTH = 8,
    DD2_RACE_TEST_LAPS = 2,
    DD2_RACE_TEST_SECOND_POINTS = 75,
    DD2_RACE_TEST_FOURTH_POINTS = 40,
    DD2_RACE_TEST_ACCIDENT_POINTS = 80,
    DD2_RACE_TEST_SECOND_TOTAL = 105,
    DD2_RACE_TEST_THIRD_TOTAL = 90,
    DD2_RACE_TEST_WIN_POINTS = 100,
    DD2_RACE_TEST_THIRD_POINTS = 10,
    DD2_RACE_TEST_FIELD = 20
};
typedef struct {
    dd2_race race;
    dd2_lap_driver laps[DD2_RACE_TEST_CARS];
    dd2_vehicle_damage damage[DD2_RACE_TEST_CARS];
    dd2_accident_driver accidents[DD2_RACE_TEST_CARS];
    dd2_recovery_driver recovery[DD2_RACE_TEST_CARS];
} dd2_race_test;

static bool dd2_race_test_same(const dd2_race *first, const dd2_race *second) {
    if (first->rules.mode != second->rules.mode || first->rules.count != second->rules.count ||
        first->rules.length != second->rules.length || first->rules.laps != second->rules.laps ||
        first->phase != second->phase || first->end != second->end ||
        first->steps != second->steps || first->elapsed != second->elapsed ||
        first->coasting != second->coasting || first->finishers != second->finishers ||
        first->alive != second->alive) {
        return false;
    }
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
        const dd2_race_driver *left = &first->drivers[slot];
        const dd2_race_driver *right = &second->drivers[slot];
        if (left->finish_step != right->finish_step || left->retired_step != right->retired_step ||
            left->started_laps != right->started_laps ||
            left->current_lap_time != right->current_lap_time ||
            left->last_lap_time != right->last_lap_time ||
            left->best_lap_time != right->best_lap_time ||
            left->credited_laps != right->credited_laps || left->relative != right->relative ||
            left->place != right->place || left->finish_place != right->finish_place ||
            left->accident_points != right->accident_points ||
            left->finish_points != right->finish_points ||
            left->total_points != right->total_points || left->retired != right->retired ||
            first->order[slot] != second->order[slot] ||
            first->results[slot] != second->results[slot]) {
            return false;
        }
    }
    return true;
}

static dd2_race_observation dd2_race_test_observe(const dd2_race_test *test) {
    return (dd2_race_observation){.laps = test->laps,
                                  .damage = test->damage,
                                  .accidents = test->accidents,
                                  .recovery = test->recovery,
                                  .count = DD2_RACE_TEST_CARS};
}
static bool dd2_race_test_reset(dd2_race_test *test, dd2_race_mode mode, bool arena) {
    *test = (dd2_race_test){0};
    for (unsigned slot = 0; slot < DD2_RACE_TEST_CARS; ++slot) {
        test->laps[slot].relative = DD2_RACE_TEST_LENGTH - slot - 1;
    }
    return dd2_race_reset(&test->race,
                          (dd2_race_rules){.mode = mode,
                                           .count = DD2_RACE_TEST_CARS,
                                           .length = arena ? 0 : DD2_RACE_TEST_LENGTH,
                                           .laps = arena ? 0 : DD2_RACE_TEST_LAPS},
                          dd2_race_test_observe(test));
}
static bool dd2_race_test_start(dd2_race_test *test) {
    const unsigned cues[] = {36, 156, 276};
    for (unsigned tick = 1; tick <= DD2_RACE_START_STEPS; ++tick) {
        if (!dd2_race_step(&test->race, dd2_race_test_observe(test)) || test->race.elapsed != 0) {
            return false;
        }
        unsigned expected = 0;
        for (unsigned cue = 0; cue < 3; ++cue) {
            if (tick >= cues[cue] && tick < DD2_RACE_START_STEPS) {
                expected = 3 - cue;
            }
        }
        if (dd2_race_countdown(&test->race) != expected || test->race.steps != tick ||
            test->race.phase !=
                (tick == DD2_RACE_START_STEPS ? DD2_RACE_RUNNING : DD2_RACE_COUNTDOWN)) {
            return false;
        }
    }
    return true;
}
static bool dd2_race_test_tick(dd2_race_test *test) {
    for (unsigned slot = 0; slot < DD2_RACE_TEST_CARS; ++slot) {
        ++test->laps[slot].steps;
    }
    return dd2_race_step(&test->race, dd2_race_test_observe(test));
}
static void dd2_race_test_finish(dd2_race_test *test, unsigned slot) {
    dd2_lap_driver *lap = &test->laps[slot];
    lap->started_laps = DD2_RACE_TEST_LAPS + 1;
    lap->credited_laps = DD2_RACE_TEST_LAPS + 1;
    lap->relative = 0;
    lap->finished = true;
    lap->finish_step = lap->steps + 1;
}
static void dd2_race_test_retire(dd2_race_test *test, unsigned slot) {
    test->damage[slot].retired = true;
    test->accidents[slot].retired = true;
    test->laps[slot].retired = true;
}
static bool dd2_race_test_finish_order(void) {
    dd2_race_test test = {0};
    if (!dd2_race_test_reset(&test, DD2_RACE_STOCKCAR, false) || !dd2_race_test_start(&test)) {
        return false;
    }
    /* A completed lap outranks a larger cell number; ordinary ties use slot ID.
     * Reverse crossings may reduce credited laps without ending the race. */
    test.laps[2].started_laps = 1;
    test.laps[2].credited_laps = 1;
    test.laps[2].relative = 0;
    if (!dd2_race_test_tick(&test) || test.race.order[0] != 2 || test.race.drivers[2].place != 1) {
        return false;
    }
    test.laps[2].credited_laps = 0;
    test.laps[2].relative = DD2_RACE_TEST_LENGTH - 1;
    if (!dd2_race_test_tick(&test) || test.race.order[0] != 0 || test.race.order[1] != 2) {
        return false;
    }
    dd2_race_test_finish(&test, 3);
    if (!dd2_race_test_tick(&test) || test.race.finishers != 1 ||
        test.race.drivers[3].finish_place != 1 || test.race.phase != DD2_RACE_RUNNING) {
        return false;
    }
    /* Simultaneous finishes use the prior placing, not array iteration order. */
    dd2_race_test_finish(&test, 2);
    dd2_race_test_finish(&test, 0);
    if (!dd2_race_test_tick(&test) || test.race.phase != DD2_RACE_COASTING ||
        test.race.end != DD2_RACE_PLAYER_FINISHED || test.race.drivers[0].finish_place != 2 ||
        test.race.drivers[2].finish_place != 3 || test.race.drivers[3].finish_place != 1) {
        return false;
    }
    test.accidents[0].points = DD2_ACCIDENT_SCORE_LIMIT;
    for (unsigned tick = 0; tick < DD2_RACE_COAST_STEPS; ++tick) {
        if (tick == 1) {
            dd2_race_test_finish(&test, 1);
        }
        if (!dd2_race_test_tick(&test) ||
            test.race.phase !=
                (tick + 1 == DD2_RACE_COAST_STEPS ? DD2_RACE_RESULTS : DD2_RACE_COASTING)) {
            return false;
        }
    }
    const dd2_race finished = test.race;
    return test.race.drivers[0].total_points == DD2_RACE_TEST_SECOND_POINTS &&
           test.race.drivers[1].total_points == DD2_RACE_TEST_FOURTH_POINTS &&
           test.race.results[0] == 3 && test.race.results[1] == 0 && test.race.results[2] == 2 &&
           test.race.results[3] == 1 && dd2_race_test_tick(&test) &&
           dd2_race_test_same(&finished, &test.race) && !dd2_race_withdraw(&test.race);
}
static bool dd2_race_test_scores(void) {
    dd2_race_test test = {0};
    if (!dd2_race_test_reset(&test, DD2_RACE_WRECKING, false) || !dd2_race_test_start(&test)) {
        return false;
    }
    test.accidents[0].points = DD2_ACCIDENT_SCORE_LIMIT;
    test.accidents[1].points = DD2_RACE_TEST_ACCIDENT_POINTS;
    test.accidents[2].points = DD2_RACE_TEST_ACCIDENT_POINTS;
    dd2_race_test_retire(&test, 0);
    if (!dd2_race_test_tick(&test) || test.race.end != DD2_RACE_PLAYER_RETIRED ||
        test.race.phase != DD2_RACE_COASTING || !dd2_race_withdraw(&test.race)) {
        return false;
    }
    return test.race.phase == DD2_RACE_RESULTS && test.race.end == DD2_RACE_PLAYER_RETIRED &&
           test.race.results[0] == 0 && test.race.results[1] == 1 && test.race.results[2] == 2 &&
           test.race.drivers[0].total_points == DD2_ACCIDENT_SCORE_LIMIT &&
           test.race.drivers[1].total_points == DD2_RACE_TEST_SECOND_TOTAL &&
           test.race.drivers[2].total_points == DD2_RACE_TEST_THIRD_TOTAL;
}
static bool dd2_race_test_arena(void) {
    dd2_race_test test = {0};
    if (!dd2_race_test_reset(&test, DD2_RACE_WRECKING, true) || !dd2_race_test_start(&test)) {
        return false;
    }
    dd2_race_test_retire(&test, 1);
    if (!dd2_race_test_tick(&test) || test.race.alive != 3 || test.race.phase != DD2_RACE_RUNNING) {
        return false;
    }
    dd2_race_test_retire(&test, 2);
    dd2_race_test_retire(&test, 3);
    test.accidents[3].points = DD2_RACE_TEST_ACCIDENT_POINTS;
    if (!dd2_race_test_tick(&test) || test.race.alive != 1 ||
        test.race.end != DD2_RACE_LAST_SURVIVOR || test.race.order[0] != 0 ||
        test.race.order[1] != 2 || test.race.order[2] != 3 || test.race.order[3] != 1 ||
        !dd2_race_withdraw(&test.race) || test.race.results[0] != 3 ||
        test.race.drivers[0].total_points != 0) {
        return false;
    }
    if (!dd2_race_test_reset(&test, DD2_RACE_WRECKING, true) || !dd2_race_test_start(&test)) {
        return false;
    }
    for (unsigned slot = 0; slot < DD2_RACE_TEST_CARS; ++slot) {
        dd2_race_test_retire(&test, slot);
    }
    return dd2_race_test_tick(&test) && test.race.alive == 0 &&
           test.race.end == DD2_RACE_PLAYER_RETIRED;
}
static bool dd2_race_test_rejection(void) {
    dd2_race_test test = {0};
    if (!dd2_race_test_reset(&test, DD2_RACE_WRECKING, false) || !dd2_race_test_start(&test)) {
        return false;
    }
    const dd2_race initial = test.race;
    dd2_race_observation bad = dd2_race_test_observe(&test);
    bad.count = DD2_RACE_TEST_CARS + 1;
    if (dd2_race_step(&test.race, bad) || !dd2_race_test_same(&initial, &test.race)) {
        return false;
    }
    test.laps[1].relative = DD2_RACE_TEST_LENGTH;
    if (dd2_race_step(&test.race, dd2_race_test_observe(&test)) ||
        !dd2_race_test_same(&initial, &test.race)) {
        return false;
    }
    test.laps[1].relative = 0;
    test.accidents[1].points = DD2_ACCIDENT_SCORE_LIMIT + 1;
    if (dd2_race_step(&test.race, dd2_race_test_observe(&test)) ||
        !dd2_race_test_same(&initial, &test.race)) {
        return false;
    }
    test.accidents[1].points = 0;
    test.race.steps = UINT64_MAX;
    if (dd2_race_step(&test.race, dd2_race_test_observe(&test)) || test.race.steps != UINT64_MAX) {
        return false;
    }
    test.race = initial;
    dd2_race_rules rules = test.race.rules;
    rules.count = 0;
    if (dd2_race_reset(&test.race, rules, dd2_race_test_observe(&test)) ||
        !dd2_race_test_same(&initial, &test.race)) {
        return false;
    }
    dd2_race_test_retire(&test, 1);
    if (!dd2_race_test_tick(&test)) {
        return false;
    }
    const dd2_race retired = test.race;
    test.damage[1].retired = false;
    test.accidents[1].retired = false;
    test.laps[1].retired = false;
    return !dd2_race_step(&test.race, dd2_race_test_observe(&test)) &&
           dd2_race_test_same(&retired, &test.race);
}
static bool dd2_race_test_withdrawal(void) {
    dd2_race_test test = {0};
    return dd2_race_test_reset(&test, DD2_RACE_STOCKCAR, false) && dd2_race_withdraw(&test.race) &&
           test.race.end == DD2_RACE_WITHDRAWN && test.race.phase == DD2_RACE_RESULTS &&
           test.race.finishers == 0 && test.race.drivers[0].finish_place == 0 &&
           dd2_race_test_reset(&test, DD2_RACE_STOCKCAR, false) &&
           test.race.phase == DD2_RACE_COUNTDOWN && test.race.steps == 0 &&
           test.race.drivers[0].total_points == 0;
}
static bool dd2_race_test_trial(void) {
    dd2_race race = {0};
    dd2_lap_driver lap = {0};
    dd2_vehicle_damage damage = {0};
    dd2_accident_driver accidents = {0};
    const dd2_race_observation observation = {
        .laps = &lap, .damage = &damage, .accidents = &accidents, .count = 1};
    const dd2_race_rules rules = {
        .mode = DD2_RACE_TIME_TRIAL, .count = 1, .length = DD2_RACE_TEST_LENGTH};
    if (!dd2_race_reset(&race, rules, observation)) {
        return false;
    }
    for (unsigned tick = 0; tick < DD2_RACE_START_STEPS; ++tick) {
        if (!dd2_race_step(&race, observation)) {
            return false;
        }
    }
    /* Continuous laps cannot trigger finite finish awards or LAST_SURVIVOR. */
    for (unsigned tick = 1; tick <= DD2_COURSE_LAP_LIMIT + 1; ++tick) {
        lap.steps = tick;
        lap.started_laps = tick;
        lap.credited_laps = tick;
        lap.last_lap = tick == 1 ? 0 : 1;
        lap.best_lap = lap.last_lap;
        lap.lap_start = tick;
        if (!dd2_race_step(&race, observation) || race.phase != DD2_RACE_RUNNING ||
            race.end != DD2_RACE_NO_END || race.finishers != 0 ||
            race.drivers[0].best_lap_time != lap.best_lap) {
            return false;
        }
    }
    ++lap.steps;
    if (!dd2_race_step(&race, observation) || race.drivers[0].current_lap_time != 1) {
        return false;
    }
    const dd2_race before = race;
    lap.finished = true;
    if (dd2_race_step(&race, observation) || !dd2_race_test_same(&race, &before)) {
        return false;
    }
    lap.finished = false;
    dd2_race_rules bad = rules;
    bad.count = 2;
    if (dd2_race_reset(&race, bad, observation) || !dd2_race_test_same(&race, &before)) {
        return false;
    }
    bad = rules;
    bad.length = 0;
    if (dd2_race_reset(&race, bad, observation) || !dd2_race_test_same(&race, &before)) {
        return false;
    }
    bad = rules;
    bad.laps = 1;
    if (dd2_race_reset(&race, bad, observation) || !dd2_race_test_same(&race, &before) ||
        !dd2_race_withdraw(&race) || race.drivers[0].total_points != 0 ||
        race.drivers[0].finish_place != 0 || race.drivers[0].best_lap_time != 1) {
        return false;
    }
    /* A retiring session freezes current timing throughout the coasting phase. */
    race = before;
    damage.retired = true;
    accidents.retired = true;
    lap.retired = true;
    for (unsigned tick = 0; tick <= DD2_RACE_COAST_STEPS; ++tick) {
        ++lap.steps;
        if (!dd2_race_step(&race, observation) || race.drivers[0].current_lap_time != 1) {
            return false;
        }
    }
    return race.phase == DD2_RACE_RESULTS && race.end == DD2_RACE_PLAYER_RETIRED &&
           race.drivers[0].last_lap_time == 1 && race.drivers[0].total_points == 0;
}

static bool dd2_race_test_temporary_overturn(void) {
    dd2_race_test test = {0};
    if (!dd2_race_test_reset(&test, DD2_RACE_WRECKING, true) || !dd2_race_test_start(&test)) {
        return false;
    }
    test.recovery[0] = (dd2_recovery_driver){.rest_steps = 1, .overturned = true};
    if (!dd2_race_test_tick(&test) || test.race.alive != DD2_RACE_TEST_CARS - 1 ||
        test.race.phase != DD2_RACE_RUNNING || test.race.drivers[0].retired ||
        test.race.drivers[0].retired_step != 0) {
        return false;
    }
    test.recovery[0] = (dd2_recovery_driver){.recoveries = 1};
    if (!dd2_race_test_tick(&test) || test.race.alive != DD2_RACE_TEST_CARS) {
        return false;
    }
    for (unsigned slot = 1; slot < DD2_RACE_TEST_CARS; ++slot) {
        test.recovery[slot] = (dd2_recovery_driver){.rest_steps = 1, .overturned = true};
    }
    if (!dd2_race_test_tick(&test) || test.race.end != DD2_RACE_LAST_SURVIVOR ||
        test.race.alive != 1 || test.race.phase != DD2_RACE_COASTING) {
        return false;
    }
    for (unsigned slot = 0; slot < DD2_RACE_TEST_CARS; ++slot) {
        if (test.race.drivers[slot].retired || test.race.drivers[slot].retired_step != 0) {
            return false;
        }
        test.recovery[slot] = (dd2_recovery_driver){0};
    }
    return dd2_race_test_tick(&test) && test.race.alive == DD2_RACE_TEST_CARS &&
           test.race.end == DD2_RACE_LAST_SURVIVOR;
}

int main(void) {
    const bool passed =
        dd2_race_test_temporary_overturn() && dd2_race_test_trial() &&
        dd2_race_test_finish_order() && dd2_race_test_scores() && dd2_race_test_arena() &&
        dd2_race_test_rejection() && dd2_race_test_withdrawal() && dd2_race_countdown(NULL) == 0 &&
        !dd2_race_step(NULL, (dd2_race_observation){0}) && !dd2_race_withdraw(NULL) &&
        dd2_race_finish_points(DD2_RACE_STOCKCAR, 1) == DD2_RACE_TEST_WIN_POINTS &&
        dd2_race_finish_points(DD2_RACE_WRECKING, 3) == DD2_RACE_TEST_THIRD_POINTS &&
        dd2_race_finish_points(DD2_RACE_STOCKCAR, DD2_RACE_TEST_FIELD) == 0 &&
        dd2_race_finish_points(DD2_RACE_STOCKCAR, 0) == 0;
    puts(passed ? "race phases, finish order, scoring and rollback: PASS"
                : "race validation: FAIL");
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
