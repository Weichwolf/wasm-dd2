#include "game/accidents.h"
#include "game/laps.h"
#include "game/race.h"
#include "physics/damage.h"
#include "physics/vehicle_collision.h"
#include "render/race_draw.h"
#include "render/renderer.h"

#include <GL/softgl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_RACE_PIXEL_WIDTH = 640,
    DD2_RACE_PIXEL_HEIGHT = 480,
    DD2_RACE_PIXEL_CHANNELS = 4,
    DD2_RACE_PIXEL_FULL = 255,
    DD2_RACE_PIXEL_DIM = 51,
    DD2_RACE_PIXEL_LIGHT_X = 272,
    DD2_RACE_PIXEL_LIGHT_Y = 306,
    DD2_RACE_PIXEL_LIGHT_PITCH = 48,
    DD2_RACE_PIXEL_CUE_FIRST = 36,
    DD2_RACE_PIXEL_CUE_SECOND = 156,
    DD2_RACE_PIXEL_CUE_THIRD = 276,
    DD2_RACE_PIXEL_GO_X = 293,
    DD2_RACE_PIXEL_GO_Y = 329,
    DD2_RACE_PIXEL_PLAYER_X = 39,
    DD2_RACE_PIXEL_PLAYER_Y = 379,
    DD2_RACE_PIXEL_OTHER_X = 41,
    DD2_RACE_PIXEL_OTHER_Y = 397,
    DD2_RACE_PIXEL_POINTS = 980,
    DD2_TRIAL_PIXEL_ROWS = 7
};

static bool dd2_race_pixel_color(dd2_renderer *renderer, unsigned xpos, unsigned ypos,
                                 const uint8_t *color) {
    const uint8_t *pixels = dd2_renderer_pixels(renderer);
    const size_t offset = (((size_t)ypos * DD2_RACE_PIXEL_WIDTH) + xpos) * DD2_RACE_PIXEL_CHANNELS;
    return pixels != NULL && pixels[offset] == color[0] && pixels[offset + 1] == color[1] &&
           pixels[offset + 2] == color[2];
}
static bool dd2_race_pixel_draw(const dd2_race *race) {
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return dd2_race_draw(
        race, (dd2_render_options){.width = DD2_RACE_PIXEL_WIDTH, .height = DD2_RACE_PIXEL_HEIGHT});
}
static bool dd2_race_pixel_lights(dd2_renderer *renderer, const dd2_race *race) {
    unsigned lit = 0;
    const unsigned cues[] = {DD2_RACE_PIXEL_CUE_FIRST, DD2_RACE_PIXEL_CUE_SECOND,
                             DD2_RACE_PIXEL_CUE_THIRD};
    for (unsigned cue = 0; cue < 3; ++cue) {
        if (race->phase == DD2_RACE_COUNTDOWN && race->steps >= cues[cue]) {
            ++lit;
        }
    }
    if (!dd2_race_pixel_draw(race)) {
        return false;
    }
    for (unsigned light = 0; light < 3; ++light) {
        const uint8_t red[] = {(uint8_t)(light < lit ? DD2_RACE_PIXEL_FULL : DD2_RACE_PIXEL_DIM), 0,
                               0};
        if (!dd2_race_pixel_color(renderer,
                                  DD2_RACE_PIXEL_LIGHT_X + (light * DD2_RACE_PIXEL_LIGHT_PITCH),
                                  DD2_RACE_PIXEL_LIGHT_Y, red)) {
            return false;
        }
    }
    return true;
}
typedef struct {
    unsigned left;
    unsigned bottom;
} dd2_trial_pixel_origin;
static bool dd2_race_pixel_glyph(dd2_renderer *renderer, const uint8_t *chart,
                                 dd2_trial_pixel_origin origin) {
    enum { DD2_TRIAL_PIXEL_COLUMNS = 5 };
    for (unsigned row = 0; row < DD2_TRIAL_PIXEL_ROWS; ++row) {
        for (unsigned column = 0; column < DD2_TRIAL_PIXEL_COLUMNS; ++column) {
            const bool lit = (chart[row] & (1U << (DD2_TRIAL_PIXEL_COLUMNS - column - 1))) != 0;
            const uint8_t color[] = {(uint8_t)(lit ? DD2_RACE_PIXEL_FULL : 0),
                                     (uint8_t)(lit ? DD2_RACE_PIXEL_FULL : 0), 0};
            const unsigned xpos = origin.left + (column * 2) + 1;
            const unsigned ypos = origin.bottom + ((DD2_TRIAL_PIXEL_ROWS - row - 1) * 2) + 1;
            if (!dd2_race_pixel_color(renderer, xpos, ypos, color)) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_race_pixel_trial_chart(dd2_renderer *renderer,
                                       const uint8_t chart[][DD2_TRIAL_PIXEL_ROWS],
                                       unsigned result) {
    enum {
        DD2_TRIAL_PIXEL_GLYPHS = 9,
        DD2_TRIAL_PIXEL_PITCH = 12,
        DD2_TRIAL_PIXEL_LEFT = 132,
        DD2_TRIAL_PIXEL_TOP = 448,
        DD2_TRIAL_PIXEL_LINE = 24,
        DD2_TRIAL_PIXEL_OFFSET = 108
    };
    for (unsigned line = 0; line < 3; ++line) {
        for (unsigned glyph = 0; glyph < DD2_TRIAL_PIXEL_GLYPHS; ++glyph) {
            const unsigned xpos = DD2_TRIAL_PIXEL_LEFT + (glyph * DD2_TRIAL_PIXEL_PITCH);
            const unsigned ypos = DD2_TRIAL_PIXEL_TOP - (line * DD2_TRIAL_PIXEL_LINE) -
                                  (result * DD2_TRIAL_PIXEL_OFFSET);
            if (!dd2_race_pixel_glyph(renderer, chart[glyph],
                                      (dd2_trial_pixel_origin){.left = xpos, .bottom = ypos})) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_race_pixel_trial(dd2_renderer *renderer) {
    enum { DD2_TRIAL_PIXEL_TICKS = 247 };
    /* Independent charts for 00:01.235 and saturated 99:59.995. Check all empty
     * pixels too, and both the live HUD and frozen result HUD. */
    static const uint8_t chart[][DD2_TRIAL_PIXEL_ROWS] = {
        {14, 17, 19, 21, 25, 17, 14}, {14, 17, 19, 21, 25, 17, 14}, {0, 4, 4, 0, 4, 4, 0},
        {14, 17, 19, 21, 25, 17, 14}, {4, 12, 4, 4, 4, 4, 14},      {0, 0, 0, 0, 0, 4, 4},
        {14, 17, 1, 2, 4, 8, 31},     {30, 1, 1, 14, 1, 1, 30},     {31, 16, 16, 30, 1, 1, 30}};
    static const uint8_t capped[][DD2_TRIAL_PIXEL_ROWS] = {
        {14, 17, 17, 15, 1, 1, 14}, {14, 17, 17, 15, 1, 1, 14}, {0, 4, 4, 0, 4, 4, 0},
        {31, 16, 16, 30, 1, 1, 30}, {14, 17, 17, 15, 1, 1, 14}, {0, 0, 0, 0, 0, 4, 4},
        {14, 17, 17, 15, 1, 1, 14}, {14, 17, 17, 15, 1, 1, 14}, {31, 16, 16, 30, 1, 1, 30}};
    dd2_race race = {.rules = {.mode = DD2_RACE_TIME_TRIAL, .count = 1}};
    for (unsigned maximum = 0; maximum < 2; ++maximum) {
        const uint64_t ticks = maximum == 0 ? DD2_TRIAL_PIXEL_TICKS : UINT64_MAX;
        race.drivers[0] = (dd2_race_driver){
            .current_lap_time = ticks, .last_lap_time = ticks, .best_lap_time = ticks};
        for (unsigned result = 0; result < 2; ++result) {
            race.phase = result == 0 ? DD2_RACE_RUNNING : DD2_RACE_RESULTS;
            if (!dd2_race_pixel_draw(&race) ||
                !dd2_race_pixel_trial_chart(renderer, maximum == 0 ? chart : capped, result)) {
                return false;
            }
        }
    }
    return true;
}

int main(void) {
    dd2_renderer *renderer = dd2_renderer_create(
        &(dd2_render_options){.width = DD2_RACE_PIXEL_WIDTH, .height = DD2_RACE_PIXEL_HEIGHT});
    if (renderer == NULL) {
        return EXIT_FAILURE;
    }
    dd2_lap_driver laps[DD2_VEHICLE_FLEET_LIMIT] = {0};
    const dd2_vehicle_damage damage[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_accident_driver accidents[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
        laps[slot].relative = DD2_VEHICLE_FLEET_LIMIT - slot;
    }
    const dd2_race_observation observation = {
        .laps = laps, .damage = damage, .accidents = accidents, .count = DD2_VEHICLE_FLEET_LIMIT};
    dd2_race race = {0};
    bool passed = dd2_race_reset(&race,
                                 (dd2_race_rules){.mode = DD2_RACE_WRECKING,
                                                  .count = DD2_VEHICLE_FLEET_LIMIT,
                                                  .length = DD2_VEHICLE_FLEET_LIMIT + 1,
                                                  .laps = 1},
                                 observation) &&
                  dd2_race_pixel_lights(renderer, &race);
    for (unsigned tick = 1; tick <= DD2_RACE_START_STEPS && passed; ++tick) {
        passed = dd2_race_step(&race, observation);
        if (tick == DD2_RACE_PIXEL_CUE_FIRST || tick == DD2_RACE_PIXEL_CUE_SECOND ||
            tick == DD2_RACE_PIXEL_CUE_THIRD) {
            passed = passed && dd2_race_pixel_lights(renderer, &race);
        }
    }
    const uint8_t green[] = {0, DD2_RACE_PIXEL_FULL, 0};
    passed = passed && dd2_race_pixel_draw(&race) &&
             dd2_race_pixel_color(renderer, DD2_RACE_PIXEL_GO_X, DD2_RACE_PIXEL_GO_Y, green);
    accidents[1].points = DD2_RACE_PIXEL_POINTS;
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
        laps[slot].steps = 1;
    }
    const uint8_t yellow[] = {DD2_RACE_PIXEL_FULL, DD2_RACE_PIXEL_FULL, 0};
    const uint8_t white[] = {DD2_RACE_PIXEL_FULL, DD2_RACE_PIXEL_FULL, DD2_RACE_PIXEL_FULL};
    passed =
        passed && dd2_race_step(&race, observation) && dd2_race_withdraw(&race) &&
        race.results[0] == 1 && race.results[1] == 0 &&
        race.drivers[1].total_points == DD2_ACCIDENT_SCORE_LIMIT && dd2_race_pixel_draw(&race) &&
        dd2_race_pixel_color(renderer, DD2_RACE_PIXEL_PLAYER_X, DD2_RACE_PIXEL_PLAYER_Y, yellow) &&
        dd2_race_pixel_color(renderer, DD2_RACE_PIXEL_OTHER_X, DD2_RACE_PIXEL_OTHER_Y, white);
    race.results[0] = DD2_VEHICLE_FLEET_LIMIT;
    passed = passed && !dd2_race_pixel_draw(&race) && dd2_race_pixel_trial(renderer);
    dd2_renderer_destroy(renderer);
    puts(passed ? "race lights/GO/result highlight rendering: PASS" : "race pixel checks: FAIL");
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
