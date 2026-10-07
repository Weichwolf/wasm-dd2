#include "game/league.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_LEAGUE_EXPORT_POINTS = 1000,
    DD2_LEAGUE_EXPORT_STRIDE = 31,
    DD2_LEAGUE_EXPORT_RANK_POINTS = 100
};
typedef struct {
    unsigned mode;
    unsigned rotation;
    unsigned stride;
    unsigned shape;
    unsigned history;
    unsigned unlocks;
} dd2_league_export_case;

static bool dd2_league_export(dd2_league_export_case description) {
    dd2_league league = {0};
    for (unsigned index = 0; index < DD2_LEAGUE_DRIVERS; ++index) {
        const unsigned place =
            (description.rotation + (index * description.stride)) % DD2_LEAGUE_DRIVERS;
        const unsigned rank = place % DD2_LEAGUE_DIVISION_SIZE;
        unsigned points = 0;
        if (description.shape == 1) {
            points = (DD2_LEAGUE_DIVISION_SIZE - 1 - rank) * DD2_LEAGUE_EXPORT_RANK_POINTS;
        } else if (description.shape == 2) {
            points = DD2_LEAGUE_EXPORT_POINTS - (index * DD2_LEAGUE_EXPORT_STRIDE);
        }
        league.drivers[index] = (dd2_league_driver){
            .points = points, .division = place / DD2_LEAGUE_DIVISION_SIZE, .rank = rank};
    }
    const dd2_league_outcome standing = dd2_league_standing(&league, 0);
    bool valid = false;
    switch (description.mode) {
    case 0:
        valid = dd2_league_transfer(&league);
        break;
    case 1:
        valid = dd2_league_clear_points(&league);
        break;
    case 2:
        valid = dd2_league_sort(&league);
        break;
    case 3:
        valid = standing == DD2_LEAGUE_CHAMPION || standing == DD2_LEAGUE_ELIMINATED ||
                (dd2_league_transfer(&league) && dd2_league_clear_points(&league));
        break;
    case 4:
        valid = dd2_league_reset(&league);
        break;
    default:
        break;
    }
    if (!valid || !dd2_league_valid(&league)) {
        return false;
    }
    printf("{\"case\":[%u,%u,%u,%u,%u,%u],\"standing\":%u,\"drivers\":[", description.mode,
           description.rotation, description.stride, description.shape, description.history,
           description.unlocks, (unsigned)standing);
    for (unsigned index = 0; index < DD2_LEAGUE_DRIVERS; ++index) {
        const dd2_league_driver driver = league.drivers[index];
        printf("%s[%u,%u,%u]", index == 0 ? "" : ",", driver.points, driver.division, driver.rank);
    }
    puts("]}");
    return true;
}

enum {
    DD2_LEAGUE_EXPORT_HISTORY = 5,
    DD2_LEAGUE_EXPORT_CASES = 6240,
    DD2_LEAGUE_EXPORT_BASIC_CASES = 480
};
static dd2_league_export_case dd2_league_export_description(unsigned index) {
    static const unsigned strides[] = {1, 3, 7, 9, 11, 13, 17, 19};
    const unsigned stride_count = sizeof(strides) / sizeof(strides[0]);
    dd2_league_export_case description = {.mode = index < 3 * DD2_LEAGUE_EXPORT_BASIC_CASES
                                                      ? index / DD2_LEAGUE_EXPORT_BASIC_CASES
                                                      : 3};
    unsigned part = index - (description.mode * DD2_LEAGUE_EXPORT_BASIC_CASES);
    if (description.mode == 3) {
        description.unlocks = part % 2;
        part /= 2;
        description.history = part % DD2_LEAGUE_EXPORT_HISTORY;
        part /= DD2_LEAGUE_EXPORT_HISTORY;
    }
    description.shape = part % 3;
    part /= 3;
    description.stride = strides[part % stride_count];
    description.rotation = part / stride_count;
    return description;
}

int main(void) {
    for (unsigned index = 0; index < DD2_LEAGUE_EXPORT_CASES; ++index) {
        if (!dd2_league_export(dd2_league_export_description(index))) {
            return EXIT_FAILURE;
        }
    }
    return dd2_league_export((dd2_league_export_case){.mode = 4, .stride = 1}) ? EXIT_SUCCESS
                                                                               : EXIT_FAILURE;
}
