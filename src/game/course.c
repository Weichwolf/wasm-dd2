#include "game/course.h"

#include "assets/road.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    DD2_COURSE_RACING_LEVELS = 7,
    DD2_COURSE_SPLIT = 8,
    DD2_COURSE_MERGE = 9,
    DD2_COURSE_MINIMUM_UNITS = 3,
    DD2_COURSE_FIXED_ONE = 65536,
    DD2_COURSE_FIXED_HALF = 32768
};

struct dd2_course {
    const dd2_road *road;
    uint32_t *numbers;
    size_t strips;
    uint32_t length;
    dd2_course_rules rules;
};

bool dd2_course_original_rules(unsigned level, dd2_course_rules *rules) {
    /* Init_Track_Strip_Numbers replaces the executable's nominal course length.
     * Only finish equivalents and default lap counts survive initialization. */
    static const dd2_course_rules originals[DD2_COURSE_RACING_LEVELS] = {
        {.finish = 267, .laps = 10}, {.finish = 560, .laps = 5}, {.finish = 340, .laps = 5},
        {.finish = 319, .laps = 5},  {.finish = 23, .laps = 8},  {.finish = 243, .laps = 7},
        {.finish = 449, .laps = 5}};
    if (level == 0 || level > DD2_COURSE_RACING_LEVELS || rules == NULL) {
        return false;
    }
    *rules = originals[level - 1];
    return true;
}

static bool dd2_course_path(const dd2_course *course, uint32_t start, uint32_t *length) {
    const dd2_road_strip *strips = dd2_road_strips(course->road);
    for (size_t visited = 0; visited < course->strips; ++visited) {
        if (start >= course->strips) {
            return false;
        }
        if (strips[start].kind == DD2_COURSE_MERGE) {
            *length = (uint32_t)visited;
            return true;
        }
        start = strips[start].next;
    }
    return false;
}

static bool dd2_course_assign(dd2_course *course, uint32_t *strip, uint32_t number) {
    if (*strip >= course->strips || course->numbers[*strip] != DD2_ROAD_NO_STRIP) {
        return false;
    }
    course->numbers[*strip] = number;
    *strip = dd2_road_strips(course->road)[*strip].next;
    return true;
}

static bool dd2_course_branch(dd2_course *course, uint32_t split, uint32_t *main) {
    uint32_t alternate = dd2_road_strips(course->road)[split].branch;
    uint32_t main_length = 0;
    uint32_t alternate_length = 0;
    if (!dd2_course_path(course, *main, &main_length) ||
        !dd2_course_path(course, alternate, &alternate_length)) {
        return false;
    }
    uint32_t *short_path = main_length < alternate_length ? main : &alternate;
    uint32_t *long_path = main_length < alternate_length ? &alternate : main;
    const uint32_t short_length = main_length < alternate_length ? main_length : alternate_length;
    const uint32_t long_length = main_length < alternate_length ? alternate_length : main_length;
    /* An empty arm needs no extra units. Both arms must then reach the same
     * merge immediately; otherwise no monotonic progress map can cover them. */
    if (short_length == 0) {
        return long_length == 0 && *main == alternate;
    }
    const uint64_t increment =
        (((uint64_t)long_length * DD2_COURSE_FIXED_ONE) + DD2_COURSE_FIXED_HALF) / short_length;
    uint64_t accumulator = DD2_COURSE_FIXED_HALF;
    uint64_t assigned = 0;
    for (uint32_t step = 0; step < short_length; ++step) {
        accumulator += increment;
        /* The source rounding can overshoot by one when the short arm divides
         * 32768 exactly. Bound the endpoint to its merge; never number a merge
         * as a path strip. Original courses do not reach this rounding edge. */
        const uint64_t rounded = accumulator / DD2_COURSE_FIXED_ONE;
        const uint64_t end = rounded < long_length ? rounded : long_length;
        for (; assigned < end; ++assigned) {
            if (!dd2_course_assign(course, long_path, course->length)) {
                return false;
            }
        }
        if (!dd2_course_assign(course, short_path, course->length)) {
            return false;
        }
        ++course->length;
    }
    return assigned == long_length && *main == alternate;
}

static bool dd2_course_number(dd2_course *course) {
    const dd2_road_strip *strips = dd2_road_strips(course->road);
    uint32_t current = 0;
    for (size_t visited = 0; visited < course->strips; ++visited) {
        const uint32_t previous = current;
        if (!dd2_course_assign(course, &current, course->length)) {
            return false;
        }
        ++course->length;
        if (strips[previous].kind == DD2_COURSE_SPLIT &&
            !dd2_course_branch(course, previous, &current)) {
            return false;
        }
        if (current == 0) {
            for (size_t strip = 0; strip < course->strips; ++strip) {
                if (course->numbers[strip] == DD2_ROAD_NO_STRIP) {
                    return false;
                }
            }
            /* Forward and reverse neighbors of the finish must differ. */
            return course->length >= DD2_COURSE_MINIMUM_UNITS &&
                   course->rules.finish < course->length;
        }
    }
    return false;
}

dd2_course *dd2_course_create(const dd2_road *road, dd2_course_rules rules) {
    const size_t count = dd2_road_strip_count(road);
    if (road == NULL || count == 0 || count > UINT16_MAX || rules.laps == 0 ||
        rules.laps > DD2_COURSE_LAP_LIMIT) {
        return NULL;
    }
    dd2_course *course = calloc(1, sizeof(*course));
    if (course == NULL) {
        return NULL;
    }
    course->road = road;
    course->strips = count;
    course->rules = rules;
    course->numbers = malloc(count * sizeof(*course->numbers));
    if (course->numbers == NULL) {
        dd2_course_destroy(course);
        return NULL;
    }
    for (size_t strip = 0; strip < count; ++strip) {
        course->numbers[strip] = DD2_ROAD_NO_STRIP;
    }
    if (!dd2_course_number(course)) {
        dd2_course_destroy(course);
        return NULL;
    }
    return course;
}

void dd2_course_destroy(dd2_course *course) {
    if (course != NULL) {
        free(course->numbers);
        free(course);
    }
}
uint32_t dd2_course_length(const dd2_course *course) {
    return course == NULL ? 0 : course->length;
}
unsigned dd2_course_laps(const dd2_course *course) {
    return course == NULL ? 0 : course->rules.laps;
}
uint32_t dd2_course_finish(const dd2_course *course) {
    return course == NULL ? DD2_ROAD_NO_STRIP : course->rules.finish;
}
size_t dd2_course_strip_count(const dd2_course *course) {
    return course == NULL ? 0 : course->strips;
}
const uint32_t *dd2_course_numbers(const dd2_course *course) {
    return course == NULL ? NULL : course->numbers;
}
uint32_t dd2_course_relative(const dd2_course *course, uint32_t cell) {
    if (course == NULL || cell >= dd2_road_cell_count(course->road)) {
        return DD2_ROAD_NO_STRIP;
    }
    const uint32_t strip = dd2_road_cells(course->road)[cell].strip;
    return (course->numbers[strip] + course->length - course->rules.finish) % course->length;
}
