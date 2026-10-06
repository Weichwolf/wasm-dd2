#include "game/laps.h"

#include "assets/road.h"
#include "game/course.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool dd2_laps_reset(dd2_lap_driver *driver, const dd2_course *course, uint32_t cell) {
    const uint32_t relative = dd2_course_relative(course, cell);
    if (driver == NULL || relative == DD2_ROAD_NO_STRIP) {
        return false;
    }
    *driver = (dd2_lap_driver){.cell = cell, .relative = relative, .checkpoint = relative};
    return true;
}

static void dd2_laps_cross(dd2_lap_driver *driver, const dd2_course *course, uint32_t relative) {
    const uint32_t last = dd2_course_length(course) - 1;
    if (relative == 0) {
        if (driver->started_laps == driver->credited_laps && driver->checkpoint == last) {
            driver->checkpoint = 0;
            if (driver->started_laps != 0) {
                driver->last_lap = driver->steps - driver->lap_start;
                if (driver->best_lap == 0 || driver->last_lap < driver->best_lap) {
                    driver->best_lap = driver->last_lap;
                }
            }
            driver->lap_start = driver->steps;
            ++driver->started_laps;
            driver->credited_laps = driver->started_laps;
            if (driver->credited_laps == dd2_course_laps(course) + 1) {
                driver->finished = true;
                driver->finish_step = driver->steps;
            }
        } else if (driver->credited_laps != driver->started_laps && driver->relative == last) {
            driver->credited_laps = driver->started_laps;
        }
    } else {
        if (relative == driver->checkpoint + 1) {
            driver->checkpoint = relative;
        }
        if (driver->relative == 0 && relative == last && driver->started_laps != 0 &&
            driver->started_laps == driver->credited_laps) {
            --driver->credited_laps;
        }
    }
    driver->relative = relative;
}

static bool dd2_laps_valid(const dd2_lap_driver *driver, const dd2_course *course) {
    return driver->steps != UINT64_MAX && driver->lap_start <= driver->steps &&
           driver->last_lap <= driver->steps && driver->best_lap <= driver->steps &&
           driver->finish_step <= driver->steps && driver->checkpoint < dd2_course_length(course) &&
           driver->relative < dd2_course_length(course) &&
           driver->relative == dd2_course_relative(course, driver->cell) &&
           driver->started_laps <= dd2_course_laps(course) + 1 &&
           driver->credited_laps <= driver->started_laps &&
           driver->started_laps - driver->credited_laps <= 1 &&
           (driver->finished == (driver->credited_laps == dd2_course_laps(course) + 1)) &&
           (driver->finished == (driver->finish_step != 0));
}

bool dd2_laps_step(dd2_lap_driver *driver, const dd2_course *course,
                   dd2_lap_observation observation) {
    if (driver == NULL || course == NULL || observation.count > DD2_LAP_TRACE_LIMIT ||
        (observation.count != 0 && observation.cells == NULL) ||
        (driver->retired && !observation.retired) || !dd2_laps_valid(driver, course)) {
        return false;
    }
    for (size_t index = 0; index < observation.count; ++index) {
        if (observation.cells[index] != DD2_ROAD_NO_STRIP &&
            dd2_course_relative(course, observation.cells[index]) == DD2_ROAD_NO_STRIP) {
            return false;
        }
    }
    dd2_lap_driver next = *driver;
    ++next.steps;
    next.retired = observation.retired;
    for (size_t index = 0; index < observation.count && !next.finished && !next.retired; ++index) {
        const uint32_t cell = observation.cells[index];
        if (cell == DD2_ROAD_NO_STRIP) {
            continue;
        }
        dd2_laps_cross(&next, course, dd2_course_relative(course, cell));
        next.cell = cell;
    }
    *driver = next;
    return true;
}

unsigned dd2_laps_completed(const dd2_lap_driver *driver) {
    return driver == NULL || driver->credited_laps == 0 ? 0 : driver->credited_laps - 1;
}
