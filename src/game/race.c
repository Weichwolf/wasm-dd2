#include "game/race.h"

#include "game/accidents.h"
#include "game/course.h"
#include "game/laps.h"
#include "physics/damage.h"
#include "physics/vehicle_collision.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    DD2_RACE_FIRST_CUE = 36,
    DD2_RACE_SECOND_CUE = 156,
    DD2_RACE_THIRD_CUE = 276,
    DD2_RACE_STOCK_WIN = 100,
    DD2_RACE_STOCK_SECOND = 75,
    DD2_RACE_STOCK_THIRD = 50,
    DD2_RACE_STOCK_FOURTH = 40,
    DD2_RACE_STOCK_FIFTH = 35,
    DD2_RACE_STOCK_SIXTH = 30,
    DD2_RACE_STOCK_SEVENTH = 25,
    DD2_RACE_STOCK_EIGHTH = 20,
    DD2_RACE_STOCK_NINTH = 15,
    DD2_RACE_STOCK_TENTH = 10,
    DD2_RACE_WRECK_WIN = 50,
    DD2_RACE_WRECK_SECOND = 25,
    DD2_RACE_WRECK_THIRD = 10
};

unsigned dd2_race_finish_points(dd2_race_mode mode, unsigned place) {
    static const unsigned stock[DD2_VEHICLE_FLEET_LIMIT] = {DD2_RACE_STOCK_WIN,
                                                            DD2_RACE_STOCK_SECOND,
                                                            DD2_RACE_STOCK_THIRD,
                                                            DD2_RACE_STOCK_FOURTH,
                                                            DD2_RACE_STOCK_FIFTH,
                                                            DD2_RACE_STOCK_SIXTH,
                                                            DD2_RACE_STOCK_SEVENTH,
                                                            DD2_RACE_STOCK_EIGHTH,
                                                            DD2_RACE_STOCK_NINTH,
                                                            DD2_RACE_STOCK_TENTH,
                                                            9,
                                                            8,
                                                            7,
                                                            6,
                                                            5,
                                                            4,
                                                            3,
                                                            2,
                                                            1,
                                                            0};
    static const unsigned wreck[DD2_VEHICLE_FLEET_LIMIT] = {
        DD2_RACE_WRECK_WIN, DD2_RACE_WRECK_SECOND, DD2_RACE_WRECK_THIRD};
    if (place == 0 || place > DD2_VEHICLE_FLEET_LIMIT ||
        (mode != DD2_RACE_STOCKCAR && mode != DD2_RACE_WRECKING)) {
        return 0;
    }
    return mode == DD2_RACE_STOCKCAR ? stock[place - 1] : wreck[place - 1];
}

static bool dd2_race_rules_valid(dd2_race_rules rules) {
    if (rules.count == 0 || rules.count > DD2_VEHICLE_FLEET_LIMIT) {
        return false;
    }
    if (rules.mode == DD2_RACE_TIME_TRIAL) {
        return rules.count == 1 && rules.length >= 3 && rules.length <= UINT16_MAX &&
               rules.laps == 0;
    }
    return (rules.mode == DD2_RACE_STOCKCAR || rules.mode == DD2_RACE_WRECKING) &&
           ((rules.length >= 3 && rules.length <= UINT16_MAX && rules.laps != 0 &&
             rules.laps <= DD2_COURSE_LAP_LIMIT) ||
            (rules.length == 0 && rules.laps == 0 && rules.mode == DD2_RACE_WRECKING &&
             rules.count >= 2));
}

static bool dd2_race_observation_valid(const dd2_race *race, dd2_race_observation observation) {
    if (observation.count != race->rules.count || observation.damage == NULL ||
        observation.accidents == NULL || (race->rules.length != 0 && observation.laps == NULL)) {
        return false;
    }
    for (unsigned slot = 0; slot < observation.count; ++slot) {
        const dd2_race_driver *driver = &race->drivers[slot];
        if ((driver->retired && !observation.damage[slot].retired) ||
            observation.accidents[slot].points > DD2_ACCIDENT_SCORE_LIMIT ||
            observation.accidents[slot].retired != observation.damage[slot].retired) {
            return false;
        }
        if (race->rules.length == 0) {
            continue;
        }
        const dd2_lap_driver *lap = &observation.laps[slot];
        const bool continuous = race->rules.mode == DD2_RACE_TIME_TRIAL;
        if (lap->relative >= race->rules.length ||
            (!continuous && lap->credited_laps > race->rules.laps + 1) ||
            lap->credited_laps > lap->started_laps || lap->started_laps - lap->credited_laps > 1 ||
            (!continuous && lap->started_laps > race->rules.laps + 1) ||
            lap->finished != (!continuous && lap->credited_laps == race->rules.laps + 1) ||
            lap->lap_start > lap->steps || lap->last_lap > lap->steps ||
            lap->best_lap > lap->steps || lap->finished != (lap->finish_step != 0) ||
            lap->finish_step > lap->steps || lap->retired != observation.damage[slot].retired ||
            (driver->finish_place != 0 && driver->finish_step != lap->finish_step)) {
            return false;
        }
    }
    return true;
}

static bool dd2_race_ahead(const dd2_race *race, unsigned first, unsigned second) {
    const dd2_race_driver *left = &race->drivers[first];
    const dd2_race_driver *right = &race->drivers[second];
    if (left->finish_place != 0 || right->finish_place != 0) {
        return right->finish_place == 0 ||
               (left->finish_place != 0 && left->finish_place < right->finish_place);
    }
    if (race->rules.length == 0) {
        if (left->retired != right->retired) {
            return !left->retired;
        }
        if (left->retired_step != right->retired_step) {
            return left->retired_step > right->retired_step;
        }
    } else {
        if (left->credited_laps != right->credited_laps) {
            return left->credited_laps > right->credited_laps;
        }
        if (left->relative != right->relative) {
            return left->relative > right->relative;
        }
    }
    return first < second;
}

static void dd2_race_order(dd2_race *race) {
    for (unsigned slot = 0; slot < race->rules.count; ++slot) {
        race->order[slot] = slot;
        unsigned index = slot;
        while (index != 0 && dd2_race_ahead(race, slot, race->order[index - 1])) {
            race->order[index] = race->order[index - 1];
            --index;
        }
        race->order[index] = slot;
    }
    for (unsigned index = 0; index < race->rules.count; ++index) {
        race->drivers[race->order[index]].place = index + 1;
    }
}

static void dd2_race_finishes(dd2_race *race) {
    for (;;) {
        unsigned selected = race->rules.count;
        for (unsigned slot = 0; slot < race->rules.count; ++slot) {
            const dd2_race_driver *driver = &race->drivers[slot];
            if (driver->finish_step == 0 || driver->finish_place != 0) {
                continue;
            }
            if (selected == race->rules.count ||
                driver->finish_step < race->drivers[selected].finish_step ||
                (driver->finish_step == race->drivers[selected].finish_step &&
                 driver->place < race->drivers[selected].place)) {
                selected = slot;
            }
        }
        if (selected == race->rules.count) {
            return;
        }
        race->drivers[selected].finish_place = ++race->finishers;
    }
}

static void dd2_race_observe(dd2_race *race, dd2_race_observation observation) {
    race->alive = 0;
    for (unsigned slot = 0; slot < race->rules.count; ++slot) {
        dd2_race_driver *driver = &race->drivers[slot];
        driver->accident_points = observation.accidents[slot].points;
        if (!driver->retired && observation.damage[slot].retired) {
            driver->retired_step = race->elapsed;
        }
        if (race->rules.length != 0) {
            const dd2_lap_driver *lap = &observation.laps[slot];
            if (!driver->retired && !observation.damage[slot].retired) {
                driver->current_lap_time =
                    lap->finished ? lap->last_lap : lap->steps - lap->lap_start;
                if (lap->started_laps == 0) {
                    driver->current_lap_time = 0;
                }
            }
            driver->last_lap_time = lap->last_lap;
            driver->best_lap_time = lap->best_lap;
            driver->started_laps = lap->started_laps;
        }
        driver->retired = observation.damage[slot].retired;
        race->alive += (unsigned)!driver->retired;
        if (race->rules.length != 0) {
            driver->credited_laps = observation.laps[slot].credited_laps;
            driver->relative = observation.laps[slot].relative;
            driver->finish_step = observation.laps[slot].finish_step;
        }
    }
    dd2_race_finishes(race);
    dd2_race_order(race);
}

static void dd2_race_results(dd2_race *race) {
    for (unsigned slot = 0; slot < race->rules.count; ++slot) {
        dd2_race_driver *driver = &race->drivers[slot];
        /* Arena scores measure accidents/survival; circuit finish bonuses do
         * not award 50 points merely for ending an arena session voluntarily. */
        driver->finish_points =
            race->rules.length == 0 ? 0 : dd2_race_finish_points(race->rules.mode, driver->place);
        const unsigned points =
            driver->finish_points +
            (race->rules.mode == DD2_RACE_WRECKING ? driver->accident_points : 0);
        driver->total_points =
            points < DD2_ACCIDENT_SCORE_LIMIT ? points : DD2_ACCIDENT_SCORE_LIMIT;
        unsigned index = slot;
        while (index != 0) {
            const dd2_race_driver *previous = &race->drivers[race->results[index - 1]];
            if (driver->total_points < previous->total_points ||
                (driver->total_points == previous->total_points &&
                 driver->place > previous->place)) {
                break;
            }
            race->results[index] = race->results[index - 1];
            --index;
        }
        race->results[index] = slot;
    }
    race->phase = DD2_RACE_RESULTS;
}

bool dd2_race_reset(dd2_race *race, dd2_race_rules rules, dd2_race_observation grid) {
    if (race == NULL || !dd2_race_rules_valid(rules)) {
        return false;
    }
    dd2_race next = {.rules = rules, .phase = DD2_RACE_COUNTDOWN};
    if (!dd2_race_observation_valid(&next, grid)) {
        return false;
    }
    for (unsigned slot = 0; slot < rules.count; ++slot) {
        if (grid.damage[slot].retired || grid.accidents[slot].points != 0 ||
            (rules.length != 0 &&
             (grid.laps[slot].steps != 0 || grid.laps[slot].started_laps != 0))) {
            return false;
        }
    }
    dd2_race_observe(&next, grid);
    *race = next;
    return true;
}

bool dd2_race_step(dd2_race *race, dd2_race_observation observation) {
    if (race == NULL || !dd2_race_rules_valid(race->rules) || race->phase < DD2_RACE_COUNTDOWN ||
        race->phase > DD2_RACE_RESULTS || race->end < DD2_RACE_NO_END ||
        race->end > DD2_RACE_WITHDRAWN || race->steps == UINT64_MAX ||
        race->elapsed == UINT64_MAX || race->finishers > race->rules.count ||
        race->coasting > DD2_RACE_COAST_STEPS || !dd2_race_observation_valid(race, observation)) {
        return false;
    }
    if (race->phase == DD2_RACE_RESULTS) {
        return true;
    }
    dd2_race next = *race;
    ++next.steps;
    if (next.phase == DD2_RACE_COUNTDOWN) {
        if (next.steps == DD2_RACE_START_STEPS) {
            next.phase = DD2_RACE_RUNNING;
        }
    } else {
        ++next.elapsed;
        dd2_race_observe(&next, observation);
        if (next.phase == DD2_RACE_RUNNING) {
            if (next.drivers[0].finish_place != 0) {
                next.end = DD2_RACE_PLAYER_FINISHED;
            } else if (next.drivers[0].retired) {
                next.end = DD2_RACE_PLAYER_RETIRED;
            } else if (next.rules.length == 0 && next.alive < 2) {
                next.end = DD2_RACE_LAST_SURVIVOR;
            }
            if (next.end != DD2_RACE_NO_END) {
                next.phase = DD2_RACE_COASTING;
            }
        } else if (++next.coasting == DD2_RACE_COAST_STEPS) {
            dd2_race_results(&next);
        }
    }
    *race = next;
    return true;
}

bool dd2_race_withdraw(dd2_race *race) {
    if (race == NULL || !dd2_race_rules_valid(race->rules) ||
        (race->phase != DD2_RACE_RUNNING && race->phase != DD2_RACE_COUNTDOWN &&
         race->phase != DD2_RACE_COASTING)) {
        return false;
    }
    if (race->end == DD2_RACE_NO_END) {
        race->end = DD2_RACE_WITHDRAWN;
    }
    dd2_race_results(race);
    return true;
}

unsigned dd2_race_countdown(const dd2_race *race) {
    if (race == NULL || race->phase != DD2_RACE_COUNTDOWN || race->steps < DD2_RACE_FIRST_CUE) {
        return 0;
    }
    if (race->steps < DD2_RACE_SECOND_CUE) {
        return 3;
    }
    return race->steps < DD2_RACE_THIRD_CUE ? 2U : 1U;
}
