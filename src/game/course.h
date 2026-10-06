#ifndef DD2_GAME_COURSE_H
#define DD2_GAME_COURSE_H

#include "assets/road.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { DD2_COURSE_LAP_LIMIT = 99 };
typedef struct dd2_course dd2_course;
typedef struct {
    uint32_t finish;
    unsigned laps;
} dd2_course_rules;

/* Original finish equivalents and default lap counts for racing levels 1..7.
 * Arenas have no lap course. Invalid requests leave rules unchanged. */
bool dd2_course_original_rules(unsigned level, dd2_course_rules *rules);
/* Owns a progress number per strip, borrowing road until destruction. At each
 * main-route split, the shorter path supplies one progress unit per strip;
 * the longer path shares units using the original rounded 16.16 distribution.
 * Nested split nodes on those paths are numbered as ordinary strips. Main-loop
 * IDs (starting grids) and raw source numbers remain separate and immutable.
 * Unclosed branches, repeated assignments, missing strips, fewer than three
 * progress units (ambiguous finish neighbors) and invalid rules fail. */
dd2_course *dd2_course_create(const dd2_road *road, dd2_course_rules rules);
void dd2_course_destroy(dd2_course *course);
uint32_t dd2_course_length(const dd2_course *course);
unsigned dd2_course_laps(const dd2_course *course);
uint32_t dd2_course_finish(const dd2_course *course);
size_t dd2_course_strip_count(const dd2_course *course);
const uint32_t *dd2_course_numbers(const dd2_course *course);
/* Finish-relative progress for a road cell, or DD2_ROAD_NO_STRIP for invalid
 * cells/course. Repeated numbers along longer branches are intentional. */
uint32_t dd2_course_relative(const dd2_course *course, uint32_t cell);

#endif
