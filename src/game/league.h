#ifndef DD2_GAME_LEAGUE_H
#define DD2_GAME_LEAGUE_H

#include <stdbool.h>

enum {
    DD2_LEAGUE_DIVISIONS = 4,
    DD2_LEAGUE_DIVISION_SIZE = 5,
    DD2_LEAGUE_DRIVERS = DD2_LEAGUE_DIVISIONS * DD2_LEAGUE_DIVISION_SIZE,
    DD2_LEAGUE_RACE_POINT_LIMIT = 999
};

typedef struct {
    unsigned points;
    unsigned division; /* Zero is the highest division. */
    unsigned rank;     /* Zero is first within the division. */
} dd2_league_driver;
typedef struct {
    /* Stable driver IDs: zero is the first human, independently of grid order. */
    dd2_league_driver drivers[DD2_LEAGUE_DRIVERS];
} dd2_league;
typedef enum {
    DD2_LEAGUE_INVALID,
    DD2_LEAGUE_PROMOTED,
    DD2_LEAGUE_STAYS,
    DD2_LEAGUE_RELEGATED,
    DD2_LEAGUE_CHAMPION,
    DD2_LEAGUE_ELIMINATED
} dd2_league_outcome;

/* Source initial four-by-five standings, including player zero at the back of
 * division three. No pointers retained, allocation, global image or randomness.
 * Scores are nonnegative and fit the original signed 16-bit ranking fields.
 * Invalid input preserves the complete league/output. Sort ties favor the lower
 * stable driver ID. */
bool dd2_league_reset(dd2_league *league);
bool dd2_league_valid(const dd2_league *league);
bool dd2_league_sort(dd2_league *league);
bool dd2_league_clear_points(dd2_league *league);
bool dd2_league_add_points(dd2_league *league, const unsigned *points);
/* One top/bottom driver swaps across each adjacent division boundary.
 * Standings must already describe the final ranks; points are retained here.
 * Next-season ownership calls clear_points after the transfers. */
bool dd2_league_transfer(dd2_league *league);
dd2_league_outcome dd2_league_standing(const dd2_league *league, unsigned player);
/* Map stable driver ID to the original physical grid slot, with the overall
 * leader in slot 19 and the bottom driver in slot 0. Output holds 20 entries
 * and must not overlap the league. */
bool dd2_league_grid(const dd2_league *league, unsigned *slot_for_driver);

#endif
