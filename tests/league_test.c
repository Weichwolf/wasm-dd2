#include "game/league.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_LEAGUE_TEST_PLAYER_POINTS = 200,
    DD2_LEAGUE_TEST_RIVAL_POINTS = 300,
    DD2_LEAGUE_TEST_RIVAL = 6,
    DD2_LEAGUE_TEST_GRID_SENTINEL = 100
};

static bool dd2_league_test_same(const dd2_league *first, const dd2_league *second) {
    for (unsigned index = 0; index < DD2_LEAGUE_DRIVERS; ++index) {
        if (first->drivers[index].points != second->drivers[index].points ||
            first->drivers[index].division != second->drivers[index].division ||
            first->drivers[index].rank != second->drivers[index].rank) {
            return false;
        }
    }
    return true;
}

static bool dd2_league_test_points(void) {
    dd2_league league = {0};
    unsigned points[DD2_LEAGUE_DRIVERS] = {0};
    points[0] = DD2_LEAGUE_TEST_PLAYER_POINTS;
    points[DD2_LEAGUE_TEST_RIVAL] = DD2_LEAGUE_TEST_RIVAL_POINTS;
    if (!dd2_league_reset(&league) || !dd2_league_add_points(&league, points) ||
        league.drivers[0].points != DD2_LEAGUE_TEST_PLAYER_POINTS || league.drivers[0].rank != 1 ||
        league.drivers[DD2_LEAGUE_TEST_RIVAL].rank != 0 ||
        dd2_league_standing(&league, 0) != DD2_LEAGUE_STAYS) {
        return false;
    }
    const dd2_league saved = league;
    points[DD2_LEAGUE_DRIVERS - 1] = DD2_LEAGUE_RACE_POINT_LIMIT + 1;
    if (dd2_league_add_points(&league, points) || !dd2_league_test_same(&league, &saved)) {
        return false;
    }
    league.drivers[0].points = INT16_MAX - 1;
    const dd2_league overflow = league;
    points[DD2_LEAGUE_DRIVERS - 1] = 0;
    points[0] = 2;
    return !dd2_league_add_points(&league, points) && dd2_league_test_same(&league, &overflow) &&
           dd2_league_clear_points(&league) && league.drivers[0].points == 0 &&
           league.drivers[0].rank == 0;
}

static bool dd2_league_test_invalid(dd2_league league) {
    const dd2_league saved = league;
    unsigned grid[DD2_LEAGUE_DRIVERS];
    unsigned points[DD2_LEAGUE_DRIVERS] = {0};
    for (unsigned index = 0; index < DD2_LEAGUE_DRIVERS; ++index) {
        grid[index] = DD2_LEAGUE_TEST_GRID_SENTINEL;
    }
    if (dd2_league_valid(&league) || dd2_league_sort(&league) || dd2_league_clear_points(&league) ||
        dd2_league_transfer(&league) || dd2_league_grid(&league, grid) ||
        dd2_league_add_points(&league, points) ||
        dd2_league_standing(&league, 0) != DD2_LEAGUE_INVALID ||
        !dd2_league_test_same(&league, &saved)) {
        return false;
    }
    for (unsigned index = 0; index < DD2_LEAGUE_DRIVERS; ++index) {
        if (grid[index] != DD2_LEAGUE_TEST_GRID_SENTINEL) {
            return false;
        }
    }
    return true;
}

static bool dd2_league_test_rejection(void) {
    dd2_league league = {0};
    if (!dd2_league_reset(&league)) {
        return false;
    }
    dd2_league invalid = league;
    invalid.drivers[0].division = DD2_LEAGUE_DIVISIONS;
    if (!dd2_league_test_invalid(invalid)) {
        return false;
    }
    invalid = league;
    invalid.drivers[0].rank = DD2_LEAGUE_DIVISION_SIZE;
    if (!dd2_league_test_invalid(invalid)) {
        return false;
    }
    invalid = league;
    invalid.drivers[0].points = (unsigned)INT16_MAX + 1;
    if (!dd2_league_test_invalid(invalid)) {
        return false;
    }
    invalid = league;
    invalid.drivers[0] = invalid.drivers[1];
    return dd2_league_test_invalid(invalid) && !dd2_league_reset(NULL) && !dd2_league_valid(NULL) &&
           !dd2_league_sort(NULL) && !dd2_league_clear_points(NULL) && !dd2_league_transfer(NULL) &&
           !dd2_league_grid(NULL, NULL) && dd2_league_standing(NULL, 0) == DD2_LEAGUE_INVALID &&
           !dd2_league_add_points(&league, NULL) && !dd2_league_grid(&league, NULL) &&
           dd2_league_standing(&league, DD2_LEAGUE_DRIVERS) == DD2_LEAGUE_INVALID;
}

static bool dd2_league_test_grid(void) {
    /* Original initial field listed by physical slot, independently of the
     * implementation's division/rank-to-slot expression. */
    static const unsigned initial[] = {0,  6, 15, 12, 9, 17, 5, 18, 13, 19,
                                       11, 2, 8,  14, 4, 10, 1, 16, 3,  7};
    dd2_league league = {0};
    unsigned grid[DD2_LEAGUE_DRIVERS] = {0};
    if (!dd2_league_reset(&league) || !dd2_league_grid(&league, grid)) {
        return false;
    }
    for (unsigned slot = 0; slot < DD2_LEAGUE_DRIVERS; ++slot) {
        if (grid[initial[slot]] != slot) {
            return false;
        }
    }
    return true;
}

int main(void) {
    const bool valid =
        dd2_league_test_points() && dd2_league_test_rejection() && dd2_league_test_grid();
    puts(valid ? "Typed league points/rollback: PASS" : "Typed league validation: FAIL");
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
