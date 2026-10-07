#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "game/course.h"
#include "game/laps.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_LAP_TEST_STRIPS = 8,
    DD2_LAP_TEST_RECORD = 64,
    DD2_LAP_TEST_VERTEX_BYTES = 96,
    DD2_LAP_TEST_SPLIT = 8,
    DD2_LAP_TEST_MERGE = 9,
    DD2_LAP_TEST_FIRST = 16,
    DD2_LAP_TEST_NEXT = 20,
    DD2_LAP_TEST_PREVIOUS = 24,
    DD2_LAP_TEST_BRANCH = 28,
    DD2_LAP_TEST_LENGTH = 5,
    DD2_LAP_TEST_FINISH = 4
};

static dd2_road *dd2_lap_test_road(bool merge) {
    uint8_t vertices[DD2_LAP_TEST_VERTEX_BYTES] = {0};
    uint8_t bytes[DD2_TEST_WORD_BYTES + (DD2_LAP_TEST_STRIPS * DD2_LAP_TEST_RECORD)] = {0};
    static const unsigned next[] = {1, 2, 3, 4, 0, 6, 7, 3};
    static const unsigned previous[] = {4, 0, 1, 2, 3, 1, 5, 6};
    dd2_test_write_le32(bytes, DD2_LAP_TEST_STRIPS);
    for (unsigned index = 0; index < DD2_LAP_TEST_STRIPS; ++index) {
        uint8_t *record = bytes + DD2_TEST_WORD_BYTES + ((size_t)index * DD2_LAP_TEST_RECORD);
        record[0] = 1;
        if (index == 1) {
            record[0] = DD2_LAP_TEST_SPLIT;
        }
        if (index == 3 && merge) {
            record[0] = DD2_LAP_TEST_MERGE;
        }
        record[1] = 2;
        dd2_test_write_le16(record + DD2_LAP_TEST_FIRST, 1);
        dd2_test_write_le32(record + DD2_LAP_TEST_NEXT, next[index] * DD2_LAP_TEST_RECORD);
        dd2_test_write_le32(record + DD2_LAP_TEST_PREVIOUS, previous[index] * DD2_LAP_TEST_RECORD);
        dd2_test_write_le32(record + DD2_LAP_TEST_BRANCH,
                            (index == 1 ? DD2_LAP_TEST_LENGTH : DD2_LAP_TEST_STRIPS - 1) *
                                DD2_LAP_TEST_RECORD);
    }
    dd2_level_data level = {.vertex_count = DD2_LAP_TEST_STRIPS};
    level.sections[DD2_LEVEL_ROAD_VERTICES] =
        (dd2_byte_view){.data = vertices, .size = sizeof(vertices)};
    level.sections[DD2_LEVEL_ROAD_STRIPS] = (dd2_byte_view){.data = bytes, .size = sizeof(bytes)};
    return dd2_road_create(&level, DD2_ROAD_RACING);
}

static uint32_t dd2_lap_test_cell(const dd2_road *road, unsigned source) {
    const dd2_road_strip *strips = dd2_road_strips(road);
    for (size_t index = 0; index < dd2_road_strip_count(road); ++index) {
        if (strips[index].source_offset == source * DD2_LAP_TEST_RECORD) {
            return strips[index].first_cell;
        }
    }
    return DD2_ROAD_NO_STRIP;
}

static bool dd2_lap_test_step(dd2_lap_driver *driver, const dd2_course *course, uint32_t cell) {
    return dd2_laps_step(driver, course, (dd2_lap_observation){.cells = &cell, .count = 1});
}

static bool dd2_lap_test_rules(const dd2_road *road, const dd2_course *course) {
    uint32_t cells[DD2_LAP_TEST_STRIPS] = {0};
    for (unsigned index = 0; index < DD2_LAP_TEST_STRIPS; ++index) {
        cells[index] = dd2_lap_test_cell(road, index);
    }
    dd2_lap_driver driver = {0};
    bool passed = dd2_laps_reset(&driver, course, cells[3]) && driver.relative == 4 &&
                  driver.checkpoint == 4 && dd2_lap_test_step(&driver, course, cells[4]) &&
                  driver.started_laps == 1 && dd2_laps_completed(&driver) == 0 &&
                  driver.lap_start == 1 && driver.best_lap == 0;
    /* Reverse/re-cross the finish twice: credited lap drops/restores; no new
     * lap, timer reset or record. Unsupported samples grant nothing. */
    for (unsigned attempt = 0; attempt < 2 && passed; ++attempt) {
        passed = dd2_lap_test_step(&driver, course, cells[3]) && driver.credited_laps == 0 &&
                 dd2_lap_test_step(&driver, course, cells[4]) && driver.credited_laps == 1 &&
                 driver.started_laps == 1 && driver.lap_start == 1;
    }
    passed = passed && dd2_lap_test_step(&driver, course, DD2_ROAD_NO_STRIP) &&
             driver.relative == 0 && driver.checkpoint == 0;
    /* A skipped checkpoint blocks the next finish. Returning to collect the
     * missing units in order repairs progress, as in Calc_Track_Positions. */
    passed = passed && dd2_lap_test_step(&driver, course, cells[0]) &&
             dd2_lap_test_step(&driver, course, cells[2]) && driver.checkpoint == 1 &&
             dd2_lap_test_step(&driver, course, cells[3]) &&
             dd2_lap_test_step(&driver, course, cells[4]) && driver.started_laps == 1;
    const uint32_t route[] = {cells[1], cells[5], cells[6], cells[7], cells[3], cells[4]};
    for (size_t index = 0; index < sizeof(route) / sizeof(route[0]) && passed; ++index) {
        passed = dd2_lap_test_step(&driver, course, route[index]);
    }
    passed = passed && driver.started_laps == 2 && driver.credited_laps == 2 &&
             dd2_laps_completed(&driver) == 1 && driver.last_lap == driver.steps - 1 &&
             driver.best_lap == driver.last_lap && !driver.finished;
    /* Several geometry samples in one fixed step share a tick, never manufacture
     * time. Complete a second lap through the shorter arm. */
    const uint32_t short_route[] = {cells[0], cells[1], cells[2], cells[3], cells[4]};
    passed = passed &&
             dd2_laps_step(
                 &driver, course,
                 (dd2_lap_observation){.cells = short_route,
                                       .count = sizeof(short_route) / sizeof(short_route[0])}) &&
             driver.finished && dd2_laps_completed(&driver) == 2 && driver.last_lap == 1 &&
             driver.best_lap == 1 && driver.finish_step == driver.steps;
    const dd2_lap_driver finished = driver;
    passed = passed && dd2_lap_test_step(&driver, course, cells[3]) &&
             driver.started_laps == finished.started_laps &&
             driver.finish_step == finished.finish_step && driver.cell == finished.cell &&
             driver.steps == finished.steps + 1;
    passed =
        passed && dd2_laps_reset(&driver, course, cells[3]) && driver.steps == 0 &&
        !driver.finished && driver.best_lap == 0 &&
        dd2_laps_step(&driver, course,
                      (dd2_lap_observation){.cells = &cells[4], .count = 1, .retired = true}) &&
        driver.retired && driver.started_laps == 0 && driver.cell == cells[3] &&
        !dd2_lap_test_step(&driver, course, cells[4]);
    return passed;
}

static bool dd2_lap_test_rejection(const dd2_road *road, const dd2_course *course) {
    const uint32_t initial = dd2_lap_test_cell(road, 3);
    const uint32_t invalid = UINT32_MAX - 1;
    const uint32_t finish = dd2_lap_test_cell(road, 4);
    const uint32_t mixed[] = {finish, invalid};
    dd2_lap_driver driver = {0};
    bool passed = dd2_laps_reset(&driver, course, initial);
    const dd2_lap_observation bad[] = {{.count = 1},
                                       {.cells = &invalid, .count = 1},
                                       {.cells = mixed, .count = 2},
                                       {.cells = &finish, .count = DD2_LAP_TRACE_LIMIT + 1}};
    for (size_t index = 0; index < sizeof(bad) / sizeof(bad[0]) && passed; ++index) {
        passed = !dd2_laps_step(&driver, course, bad[index]) && driver.steps == 0 &&
                 driver.cell == initial && driver.started_laps == 0;
    }
    /* Reject corrupted saved/public state before processing valid contacts. */
    const dd2_lap_driver corrupt[] = {
        {.cell = invalid, .relative = DD2_ROAD_NO_STRIP},
        {.cell = initial, .relative = 4, .checkpoint = 4, .lap_start = 1},
        {.cell = initial, .relative = 4, .checkpoint = 4, .started_laps = DD2_COURSE_LAP_LIMIT},
        {.cell = initial, .relative = 4, .checkpoint = 4, .credited_laps = 1},
        {.cell = initial, .relative = 4, .checkpoint = 4, .finished = true}};
    for (size_t index = 0; index < sizeof(corrupt) / sizeof(corrupt[0]) && passed; ++index) {
        driver = corrupt[index];
        passed = !dd2_lap_test_step(&driver, course, finish) && driver.steps == 0 &&
                 driver.cell == corrupt[index].cell && driver.relative == corrupt[index].relative &&
                 driver.started_laps == corrupt[index].started_laps &&
                 driver.credited_laps == corrupt[index].credited_laps &&
                 driver.finished == corrupt[index].finished;
    }
    passed = passed && dd2_laps_reset(&driver, course, initial);
    driver.steps = UINT64_MAX;
    passed = passed && !dd2_lap_test_step(&driver, course, finish) && driver.steps == UINT64_MAX;
    passed = passed && !dd2_laps_reset(&driver, course, invalid) && driver.steps == UINT64_MAX;
    dd2_course_rules rules = {0};
    passed =
        passed && !dd2_course_original_rules(0, &rules) &&
        !dd2_course_original_rules(DD2_LAP_TEST_STRIPS, &rules) &&
        !dd2_course_original_rules(1, NULL) &&
        dd2_course_create(road, (dd2_course_rules){.laps = 1, .finish = DD2_LAP_TEST_LENGTH}) ==
            NULL &&
        dd2_course_create(road, (dd2_course_rules){.laps = DD2_COURSE_LAP_LIMIT + 1}) == NULL &&
        dd2_course_create(NULL, (dd2_course_rules){.laps = 1}) == NULL &&
        !dd2_laps_reset(NULL, course, initial) && !dd2_laps_reset(&driver, NULL, initial) &&
        !dd2_lap_test_step(NULL, course, finish) && !dd2_lap_test_step(&driver, NULL, finish) &&
        dd2_laps_completed(NULL) == 0 && dd2_course_length(NULL) == 0 &&
        dd2_course_numbers(NULL) == NULL && dd2_course_relative(NULL, 0) == DD2_ROAD_NO_STRIP;
    dd2_course_destroy(NULL);
    return passed;
}

static bool dd2_lap_test_continuous(const dd2_road *road) {
    dd2_course *course = dd2_course_create(road, (dd2_course_rules){.finish = DD2_LAP_TEST_FINISH});
    dd2_lap_driver driver = {0};
    bool passed = course != NULL && dd2_course_laps(course) == 0 &&
                  dd2_laps_reset(&driver, course, dd2_lap_test_cell(road, 3));
    const uint32_t finish = dd2_lap_test_cell(road, 4);
    const uint32_t route[] = {dd2_lap_test_cell(road, 0), dd2_lap_test_cell(road, 1),
                              dd2_lap_test_cell(road, 2), dd2_lap_test_cell(road, 3), finish};
    passed = passed && dd2_lap_test_step(&driver, course, finish);
    /* Beyond both source default limits and the configurable finite limit. */
    for (unsigned lap = 1; lap <= DD2_COURSE_LAP_LIMIT + 1 && passed; ++lap) {
        for (size_t cell = 0; cell < sizeof(route) / sizeof(route[0]) && passed; ++cell) {
            passed = dd2_lap_test_step(&driver, course, route[cell]);
        }
        passed = passed && dd2_laps_completed(&driver) == lap && !driver.finished &&
                 driver.finish_step == 0 && driver.last_lap == DD2_LAP_TEST_LENGTH &&
                 driver.best_lap == DD2_LAP_TEST_LENGTH;
    }
    /* Overflow on the second crossing within one trace must roll back the first
     * crossing, all timing, and the fixed-step clock as well. */
    driver.started_laps = UINT_MAX - 1;
    driver.credited_laps = UINT_MAX - 1;
    const uint32_t twice[] = {route[0], route[1], route[2], route[3], finish,
                              route[0], route[1], route[2], route[3], finish};
    const dd2_lap_driver before = driver;
    passed = passed &&
             !dd2_laps_step(&driver, course,
                            (dd2_lap_observation){.cells = twice,
                                                  .count = sizeof(twice) / sizeof(twice[0])}) &&
             driver.started_laps == before.started_laps && driver.steps == before.steps &&
             driver.cell == before.cell && driver.checkpoint == before.checkpoint &&
             driver.lap_start == before.lap_start && driver.last_lap == before.last_lap &&
             driver.best_lap == before.best_lap && !driver.finished;
    dd2_course_destroy(course);
    return passed;
}

static bool dd2_lap_test_short_course(void) {
    uint8_t vertices[DD2_LAP_TEST_VERTEX_BYTES] = {0};
    uint8_t bytes[DD2_TEST_WORD_BYTES + (3 * DD2_LAP_TEST_RECORD)] = {0};
    dd2_level_data level = {.vertex_count = DD2_LAP_TEST_STRIPS};
    level.sections[DD2_LEVEL_ROAD_VERTICES] =
        (dd2_byte_view){.data = vertices, .size = sizeof(vertices)};
    bool passed = true;
    for (unsigned count = 1; count <= 3 && passed; ++count) {
        dd2_test_write_le32(bytes, count);
        for (unsigned index = 0; index < count; ++index) {
            uint8_t *record = bytes + DD2_TEST_WORD_BYTES + ((size_t)index * DD2_LAP_TEST_RECORD);
            record[0] = 1;
            record[1] = 2;
            dd2_test_write_le16(record + DD2_LAP_TEST_FIRST, 1);
            dd2_test_write_le32(record + DD2_LAP_TEST_NEXT,
                                ((index + 1) % count) * DD2_LAP_TEST_RECORD);
            dd2_test_write_le32(record + DD2_LAP_TEST_PREVIOUS,
                                ((index + count - 1) % count) * DD2_LAP_TEST_RECORD);
        }
        level.sections[DD2_LEVEL_ROAD_STRIPS] = (dd2_byte_view){
            .data = bytes, .size = DD2_TEST_WORD_BYTES + ((size_t)count * DD2_LAP_TEST_RECORD)};
        dd2_road *road = dd2_road_create(&level, DD2_ROAD_RACING);
        dd2_course *course = dd2_course_create(road, (dd2_course_rules){.laps = 1});
        passed = road != NULL && ((course != NULL) == (count == 3));
        dd2_course_destroy(course);
        dd2_road_destroy(road);
    }
    return passed;
}

int main(void) {
    dd2_road *road = dd2_lap_test_road(true);
    dd2_course *course =
        dd2_course_create(road, (dd2_course_rules){.finish = DD2_LAP_TEST_FINISH, .laps = 2});
    bool passed = road != NULL && course != NULL &&
                  dd2_course_length(course) == DD2_LAP_TEST_LENGTH &&
                  dd2_course_strip_count(course) == DD2_LAP_TEST_STRIPS &&
                  dd2_lap_test_rules(road, course) && dd2_lap_test_rejection(road, course) &&
                  dd2_lap_test_continuous(road) && dd2_lap_test_short_course();
    dd2_course_destroy(course);
    dd2_road_destroy(road);
    road = dd2_lap_test_road(false);
    course = dd2_course_create(road, (dd2_course_rules){.laps = 1});
    passed = passed && road != NULL && course == NULL;
    dd2_course_destroy(course);
    dd2_road_destroy(road);
    puts(passed ? "course equivalence/lap rules: PASS" : "course equivalence/lap rules: FAIL");
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
