#include "game/league.h"

#include <stddef.h>
#include <stdint.h>

bool dd2_league_valid(const dd2_league *league) {
    if (league == NULL) {
        return false;
    }
    uint32_t occupied = 0;
    for (unsigned index = 0; index < DD2_LEAGUE_DRIVERS; ++index) {
        const dd2_league_driver driver = league->drivers[index];
        if (driver.division >= DD2_LEAGUE_DIVISIONS || driver.rank >= DD2_LEAGUE_DIVISION_SIZE ||
            driver.points > INT16_MAX) {
            return false;
        }
        const unsigned place = (driver.division * DD2_LEAGUE_DIVISION_SIZE) + driver.rank;
        const uint32_t bit = UINT32_C(1) << place;
        if ((occupied & bit) != 0) {
            return false;
        }
        occupied |= bit;
    }
    return true;
}

bool dd2_league_reset(dd2_league *league) {
    if (league == NULL) {
        return false;
    }
    /* Original initial grid order, independently retained by stable driver ID. */
    static const unsigned initial[DD2_LEAGUE_DRIVERS] = {0,  6, 15, 12, 9, 17, 5, 18, 13, 19,
                                                         11, 2, 8,  14, 4, 10, 1, 16, 3,  7};
    dd2_league next = {0};
    for (unsigned slot = 0; slot < DD2_LEAGUE_DRIVERS; ++slot) {
        next.drivers[initial[slot]] = (dd2_league_driver){
            .division = DD2_LEAGUE_DIVISIONS - 1 - (slot / DD2_LEAGUE_DIVISION_SIZE),
            .rank = DD2_LEAGUE_DIVISION_SIZE - 1 - (slot % DD2_LEAGUE_DIVISION_SIZE)};
    }
    *league = next;
    return true;
}

bool dd2_league_sort(dd2_league *league) {
    if (!dd2_league_valid(league)) {
        return false;
    }
    for (unsigned index = 0; index < DD2_LEAGUE_DRIVERS; ++index) {
        const dd2_league_driver driver = league->drivers[index];
        unsigned rank = 0;
        for (unsigned other = 0; other < DD2_LEAGUE_DRIVERS; ++other) {
            const dd2_league_driver rival = league->drivers[other];
            rank += (unsigned)(rival.division == driver.division &&
                               (rival.points > driver.points ||
                                (rival.points == driver.points && other < index)));
        }
        league->drivers[index].rank = rank;
    }
    return true;
}

bool dd2_league_clear_points(dd2_league *league) {
    if (!dd2_league_valid(league)) {
        return false;
    }
    for (unsigned index = 0; index < DD2_LEAGUE_DRIVERS; ++index) {
        league->drivers[index].points = 0;
    }
    return dd2_league_sort(league);
}

bool dd2_league_add_points(dd2_league *league, const unsigned *points) {
    if (!dd2_league_valid(league) || points == NULL) {
        return false;
    }
    dd2_league next = *league;
    for (unsigned index = 0; index < DD2_LEAGUE_DRIVERS; ++index) {
        if (points[index] > DD2_LEAGUE_RACE_POINT_LIMIT ||
            points[index] > (unsigned)INT16_MAX - next.drivers[index].points) {
            return false;
        }
        next.drivers[index].points += points[index];
    }
    if (!dd2_league_sort(&next)) {
        return false;
    }
    *league = next;
    return true;
}

bool dd2_league_transfer(dd2_league *league) {
    if (!dd2_league_valid(league)) {
        return false;
    }
    unsigned order[DD2_LEAGUE_DRIVERS] = {0};
    for (unsigned index = 0; index < DD2_LEAGUE_DRIVERS; ++index) {
        const dd2_league_driver driver = league->drivers[index];
        order[(driver.division * DD2_LEAGUE_DIVISION_SIZE) + driver.rank] = index;
    }
    dd2_league next = *league;
    for (unsigned division = 0; division + 1 < DD2_LEAGUE_DIVISIONS; ++division) {
        const unsigned lower = (division + 1) * DD2_LEAGUE_DIVISION_SIZE;
        const unsigned promoted = order[lower];
        const unsigned relegated = order[lower - 1];
        next.drivers[promoted].division = division;
        next.drivers[promoted].rank = DD2_LEAGUE_DIVISION_SIZE - 1;
        next.drivers[relegated].division = division + 1;
        next.drivers[relegated].rank = 0;
    }
    *league = next;
    return true;
}

dd2_league_outcome dd2_league_standing(const dd2_league *league, unsigned player) {
    if (!dd2_league_valid(league) || player >= DD2_LEAGUE_DRIVERS) {
        return DD2_LEAGUE_INVALID;
    }
    const dd2_league_driver driver = league->drivers[player];
    if (driver.rank == 0) {
        return driver.division == 0 ? DD2_LEAGUE_CHAMPION : DD2_LEAGUE_PROMOTED;
    }
    if (driver.rank + 1 == DD2_LEAGUE_DIVISION_SIZE) {
        return driver.division + 1 == DD2_LEAGUE_DIVISIONS ? DD2_LEAGUE_ELIMINATED
                                                           : DD2_LEAGUE_RELEGATED;
    }
    return DD2_LEAGUE_STAYS;
}

bool dd2_league_grid(const dd2_league *league, unsigned *slot_for_driver) {
    if (!dd2_league_valid(league) || slot_for_driver == NULL) {
        return false;
    }
    for (unsigned index = 0; index < DD2_LEAGUE_DRIVERS; ++index) {
        const dd2_league_driver driver = league->drivers[index];
        slot_for_driver[index] =
            DD2_LEAGUE_DRIVERS - 1 - ((driver.division * DD2_LEAGUE_DIVISION_SIZE) + driver.rank);
    }
    return true;
}
