#include "render/race_draw.h"

#include "game/accidents.h"
#include "game/race.h"
#include "physics/vehicle_collision.h"
#include "render/renderer.h"

#include <GL/softgl.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Five-bit rows, top first. Plain temporary race typography shared by both
 * targets; original menu fonts and complete front-end presentation are separate. */
enum {
    DD2_RACE_DRAW_WIDTH = 640,
    DD2_RACE_DRAW_HEIGHT = 480,
    DD2_RACE_DRAW_GLYPH_WIDTH = 5,
    DD2_RACE_DRAW_GLYPH_HEIGHT = 7,
    DD2_RACE_DRAW_PITCH = 6,
    DD2_RACE_DRAW_LINE = 18,
    DD2_RACE_DRAW_DIGITS = 10,
    DD2_RACE_DRAW_LETTERS = 26,
    DD2_RACE_DRAW_LETTER_BASE = 10,
    DD2_RACE_DRAW_SLASH = 36,
    DD2_RACE_DRAW_COLON = 37,
    DD2_RACE_DRAW_DOT = 38,
    DD2_RACE_DRAW_TIME_BYTES = 10,
    DD2_RACE_DRAW_TICKS_PER_SECOND = 200,
    DD2_RACE_DRAW_SECONDS_PER_MINUTE = 60,
    DD2_RACE_DRAW_MILLISECONDS_PER_TICK = 5,
    DD2_RACE_DRAW_MILLISECOND_HUNDREDS = 100,
    DD2_RACE_DRAW_MAX_TIME_TICKS = 1199999,
    DD2_RACE_DRAW_FIRST_ROW = 384,
    DD2_RACE_DRAW_GO_STEPS = 100,
    DD2_RACE_DRAW_NUMBER_BYTES = 11
};
static const float dd2_race_draw_panel_left = 20;
static const float dd2_race_draw_panel_bottom = 16;
static const float dd2_race_draw_panel_width = 600;
static const float dd2_race_draw_panel_height = 448;
static const float dd2_race_draw_text_left = 36;
static const float dd2_race_draw_title_bottom = 438;
static const float dd2_race_draw_mode_left = 410;
static const float dd2_race_draw_header_bottom = 408;
static const float dd2_race_draw_player_left = 510;
static const float dd2_race_draw_footer_bottom = 24;
static const float dd2_race_draw_position_left = 450;
static const float dd2_race_draw_position_bottom = 444;
static const float dd2_race_draw_position_width = 172;
static const float dd2_race_draw_position_height = 22;
static const float dd2_race_draw_pos_label = 458;
static const float dd2_race_draw_pos_digits = 506;
static const float dd2_race_draw_pos_slash = 542;
static const float dd2_race_draw_pos_count = 566;
static const float dd2_race_draw_light_panel_left = 244;
static const float dd2_race_draw_light_panel_bottom = 282;
static const float dd2_race_draw_light_panel_width = 152;
static const float dd2_race_draw_light_panel_height = 48;
static const float dd2_race_draw_light_left = 252;
static const float dd2_race_draw_light_bottom = 290;
static const float dd2_race_draw_light_width = 40;
static const float dd2_race_draw_light_height = 32;
static const float dd2_race_draw_cue_left = 305;
static const float dd2_race_draw_cue_bottom = 226;
static const float dd2_race_draw_cue_scale = 6;
static const float dd2_race_draw_go_left = 284;
static const float dd2_race_draw_end_left = 248;
static const float dd2_race_draw_end_bottom = 294;

static const float dd2_race_draw_dim = 0.2F;
static const uint8_t dd2_race_draw_font[][DD2_RACE_DRAW_GLYPH_HEIGHT] = {
    {14, 17, 19, 21, 25, 17, 14}, {4, 12, 4, 4, 4, 4, 14},      {14, 17, 1, 2, 4, 8, 31},
    {30, 1, 1, 14, 1, 1, 30},     {2, 6, 10, 18, 31, 2, 2},     {31, 16, 16, 30, 1, 1, 30},
    {14, 16, 16, 30, 17, 17, 14}, {31, 1, 2, 4, 8, 8, 8},       {14, 17, 17, 14, 17, 17, 14},
    {14, 17, 17, 15, 1, 1, 14},   {14, 17, 17, 31, 17, 17, 17}, {30, 17, 17, 30, 17, 17, 30},
    {14, 17, 16, 16, 16, 17, 14}, {30, 17, 17, 17, 17, 17, 30}, {31, 16, 16, 30, 16, 16, 31},
    {31, 16, 16, 30, 16, 16, 16}, {14, 17, 16, 23, 17, 17, 15}, {17, 17, 17, 31, 17, 17, 17},
    {14, 4, 4, 4, 4, 4, 14},      {7, 2, 2, 2, 2, 18, 12},      {17, 18, 20, 24, 20, 18, 17},
    {16, 16, 16, 16, 16, 16, 31}, {17, 27, 21, 21, 17, 17, 17}, {17, 25, 21, 19, 17, 17, 17},
    {14, 17, 17, 17, 17, 17, 14}, {30, 17, 17, 30, 16, 16, 16}, {14, 17, 17, 17, 21, 18, 13},
    {30, 17, 17, 30, 20, 18, 17}, {15, 16, 16, 14, 1, 1, 30},   {31, 4, 4, 4, 4, 4, 4},
    {17, 17, 17, 17, 17, 17, 14}, {17, 17, 17, 17, 17, 10, 4},  {17, 17, 17, 21, 21, 21, 10},
    {17, 17, 10, 4, 10, 17, 17},  {17, 17, 10, 4, 4, 4, 4},     {31, 1, 2, 4, 8, 16, 31},
    {1, 1, 2, 4, 8, 16, 16},      {0, 4, 4, 0, 4, 4, 0},        {0, 0, 0, 0, 0, 4, 4}};

typedef struct {
    float x;
    float y;
    float scale;
} dd2_race_draw_pen;

static void dd2_race_draw_box(float left, float bottom, float width, float height) {
    glVertex2f(left, bottom);
    glVertex2f(left + width, bottom);
    glVertex2f(left, bottom + height);
    glVertex2f(left + width, bottom);
    glVertex2f(left + width, bottom + height);
    glVertex2f(left, bottom + height);
}

static void dd2_race_draw_text(const char *text, dd2_race_draw_pen pen) {
    for (size_t index = 0; text[index] != '\0'; ++index) {
        const unsigned char character = (unsigned char)text[index];
        unsigned glyph = DD2_RACE_DRAW_DOT + 1;
        if (character >= '0' && character <= '9') {
            glyph = (unsigned)(character - '0');
        } else if (character >= 'A' && character <= 'Z') {
            glyph = DD2_RACE_DRAW_LETTER_BASE + (unsigned)(character - 'A');
        } else if (character == '/') {
            glyph = DD2_RACE_DRAW_SLASH;
        } else if (character == ':') {
            glyph = DD2_RACE_DRAW_COLON;
        } else if (character == '.') {
            glyph = DD2_RACE_DRAW_DOT;
        }
        if (glyph <= DD2_RACE_DRAW_DOT) {
            for (unsigned row = 0; row < DD2_RACE_DRAW_GLYPH_HEIGHT; ++row) {
                for (unsigned column = 0; column < DD2_RACE_DRAW_GLYPH_WIDTH; ++column) {
                    if ((dd2_race_draw_font[glyph][row] &
                         (1U << (DD2_RACE_DRAW_GLYPH_WIDTH - column - 1))) != 0) {
                        dd2_race_draw_box(
                            pen.x + ((float)column * pen.scale),
                            pen.y + ((float)(DD2_RACE_DRAW_GLYPH_HEIGHT - row - 1) * pen.scale),
                            pen.scale, pen.scale);
                    }
                }
            }
        }
        pen.x += (float)DD2_RACE_DRAW_PITCH * pen.scale;
    }
}

static void dd2_race_draw_number(unsigned number, dd2_race_draw_pen pen) {
    char text[DD2_RACE_DRAW_NUMBER_BYTES] = {0};
    unsigned count = 0;
    do {
        text[count++] = (char)('0' + (number % DD2_RACE_DRAW_DIGITS));
        number /= DD2_RACE_DRAW_DIGITS;
    } while (number != 0 && count < DD2_RACE_DRAW_NUMBER_BYTES - 1);
    for (unsigned index = 0; index < count / 2; ++index) {
        const char digit = text[index];
        text[index] = text[count - index - 1];
        text[count - index - 1] = digit;
    }
    dd2_race_draw_text(text, pen);
}

static void dd2_race_draw_time(uint64_t ticks, dd2_race_draw_pen pen) {
    const unsigned bounded =
        ticks > DD2_RACE_DRAW_MAX_TIME_TICKS ? DD2_RACE_DRAW_MAX_TIME_TICKS : (unsigned)ticks;
    const unsigned seconds = bounded / DD2_RACE_DRAW_TICKS_PER_SECOND;
    const unsigned minutes = seconds / DD2_RACE_DRAW_SECONDS_PER_MINUTE;
    const unsigned remainder = seconds % DD2_RACE_DRAW_SECONDS_PER_MINUTE;
    const unsigned milliseconds =
        (bounded % DD2_RACE_DRAW_TICKS_PER_SECOND) * DD2_RACE_DRAW_MILLISECONDS_PER_TICK;
    const char text[DD2_RACE_DRAW_TIME_BYTES] = {
        (char)('0' + (minutes / DD2_RACE_DRAW_DIGITS)),
        (char)('0' + (minutes % DD2_RACE_DRAW_DIGITS)),
        ':',
        (char)('0' + (remainder / DD2_RACE_DRAW_DIGITS)),
        (char)('0' + (remainder % DD2_RACE_DRAW_DIGITS)),
        '.',
        (char)('0' + (milliseconds / DD2_RACE_DRAW_MILLISECOND_HUNDREDS)),
        (char)('0' + ((milliseconds / DD2_RACE_DRAW_DIGITS) % DD2_RACE_DRAW_DIGITS)),
        (char)('0' + (milliseconds % DD2_RACE_DRAW_DIGITS)),
        '\0'};
    dd2_race_draw_text(text, pen);
}

static void dd2_race_draw_trial(const dd2_race *race, float scale, bool results) {
    const dd2_race_driver *driver = &race->drivers[0];
    static const float left = 20;
    static const float bottom = 364;
    static const float width = 268;
    static const float height = 102;
    static const float labels_left = 28;
    static const float times_left = 132;
    static const float first_time = 448;
    static const float result_offset = 108;
    static const float time_line = 24;
    const float offset = results ? result_offset : 0;
    if (!results) {
        glColor3f(0, 0, 0);
        dd2_race_draw_box(left * scale, bottom * scale, width * scale, height * scale);
    }
    glColor3f(1, 1, 0);
    const char *labels[] = {"CURRENT", "LAST", "BEST"};
    const uint64_t times[] = {driver->current_lap_time, driver->last_lap_time,
                              driver->best_lap_time};
    for (unsigned row = 0; row < sizeof(times) / sizeof(times[0]); ++row) {
        const float y_pos = (first_time - offset - ((float)row * time_line)) * scale;
        dd2_race_draw_text(
            labels[row],
            (dd2_race_draw_pen){.x = labels_left * scale, .y = y_pos, .scale = 2 * scale});
        dd2_race_draw_time(
            times[row],
            (dd2_race_draw_pen){.x = times_left * scale, .y = y_pos, .scale = 2 * scale});
    }
    const float y_pos = (first_time - offset - (3 * time_line)) * scale;
    dd2_race_draw_text(
        results ? "LAPS" : "LAP",
        (dd2_race_draw_pen){.x = labels_left * scale, .y = y_pos, .scale = 2 * scale});
    unsigned laps = driver->started_laps == 0 ? 1 : driver->started_laps;
    if (results) {
        laps = driver->credited_laps == 0 ? 0 : driver->credited_laps - 1;
    }
    dd2_race_draw_number(
        laps, (dd2_race_draw_pen){.x = times_left * scale, .y = y_pos, .scale = 2 * scale});
}

static void dd2_race_draw_results(const dd2_race *race, float scale) {
    glColor3f(0, 0, 0);
    dd2_race_draw_box(dd2_race_draw_panel_left * scale, dd2_race_draw_panel_bottom * scale,
                      dd2_race_draw_panel_width * scale, dd2_race_draw_panel_height * scale);
    glColor3f(1, 1, 1);
    const char *title = "RESULTS";
    switch (race->end) {
    case DD2_RACE_PLAYER_FINISHED:
        title = "FINISH";
        break;
    case DD2_RACE_PLAYER_RETIRED:
        title = "RETIRED";
        break;
    case DD2_RACE_LAST_SURVIVOR:
        title = "LAST CAR";
        break;
    case DD2_RACE_WITHDRAWN:
        title = "WITHDRAWN";
        break;
    case DD2_RACE_NO_END:
        break;
    }
    dd2_race_draw_text(title, (dd2_race_draw_pen){.x = dd2_race_draw_text_left * scale,
                                                  .y = dd2_race_draw_title_bottom * scale,
                                                  .scale = 2 * scale});
    const char *mode = "WRECKING";
    if (race->rules.mode == DD2_RACE_STOCKCAR) {
        mode = "STOCKCAR";
    } else if (race->rules.mode == DD2_RACE_TIME_TRIAL) {
        mode = "TIME TRIAL";
    }
    dd2_race_draw_text(mode, (dd2_race_draw_pen){.x = dd2_race_draw_mode_left * scale,
                                                 .y = dd2_race_draw_title_bottom * scale,
                                                 .scale = 2 * scale});
    if (race->rules.mode == DD2_RACE_TIME_TRIAL) {
        dd2_race_draw_trial(race, scale, true);
    } else {
        const char *labels[] = {"POS", "CAR", "PLACE", "PTS", "STATUS"};
        const float columns[] = {36, 116, 218, 308, 412};
        for (size_t column = 0; column < sizeof(columns) / sizeof(columns[0]); ++column) {
            dd2_race_draw_text(labels[column],
                               (dd2_race_draw_pen){.x = columns[column] * scale,
                                                   .y = dd2_race_draw_header_bottom * scale,
                                                   .scale = 2 * scale});
        }
        for (unsigned row = 0; row < race->rules.count; ++row) {
            const unsigned slot = race->results[row];
            const dd2_race_driver *driver = &race->drivers[slot];
            const float bottom =
                ((float)DD2_RACE_DRAW_FIRST_ROW - ((float)row * (float)DD2_RACE_DRAW_LINE)) * scale;
            if (slot == 0) {
                glColor3f(1, 1, 0);
            } else {
                glColor3f(1, 1, 1);
            }
            const unsigned numbers[] = {row + 1, slot + 1, driver->place, driver->total_points};
            for (size_t column = 0; column < sizeof(numbers) / sizeof(numbers[0]); ++column) {
                dd2_race_draw_number(numbers[column],
                                     (dd2_race_draw_pen){.x = columns[column] * scale,
                                                         .y = bottom,
                                                         .scale = 2 * scale});
            }
            const char *status = driver->retired ? "OUT" : "DNF";
            if (driver->finish_place != 0) {
                status = "FIN";
            }
            dd2_race_draw_text(
                status,
                (dd2_race_draw_pen){.x = columns[4] * scale, .y = bottom, .scale = 2 * scale});
            if (slot == 0) {
                dd2_race_draw_text("YOU",
                                   (dd2_race_draw_pen){.x = dd2_race_draw_player_left * scale,
                                                       .y = bottom,
                                                       .scale = 2 * scale});
            }
        }
    }
    glColor3f(1, 1, 1);
    dd2_race_draw_text("R RESTART   ENTER VIEW",
                       (dd2_race_draw_pen){.x = dd2_race_draw_text_left * scale,
                                           .y = dd2_race_draw_footer_bottom * scale,
                                           .scale = 2 * scale});
}

static void dd2_race_draw_active(const dd2_race *race, float scale) {
    if (race->rules.mode == DD2_RACE_TIME_TRIAL) {
        dd2_race_draw_trial(race, scale, false);
    } else {
        glColor3f(0, 0, 0);
        dd2_race_draw_box(
            dd2_race_draw_position_left * scale, dd2_race_draw_position_bottom * scale,
            dd2_race_draw_position_width * scale, dd2_race_draw_position_height * scale);
        glColor3f(1, 1, 1);
        dd2_race_draw_text("POS", (dd2_race_draw_pen){.x = dd2_race_draw_pos_label * scale,
                                                      .y = dd2_race_draw_panel_height * scale,
                                                      .scale = 2 * scale});
        dd2_race_draw_number(race->drivers[0].place,
                             (dd2_race_draw_pen){.x = dd2_race_draw_pos_digits * scale,
                                                 .y = dd2_race_draw_panel_height * scale,
                                                 .scale = 2 * scale});
        dd2_race_draw_text("/", (dd2_race_draw_pen){.x = dd2_race_draw_pos_slash * scale,
                                                    .y = dd2_race_draw_panel_height * scale,
                                                    .scale = 2 * scale});
        dd2_race_draw_number(race->rules.count,
                             (dd2_race_draw_pen){.x = dd2_race_draw_pos_count * scale,
                                                 .y = dd2_race_draw_panel_height * scale,
                                                 .scale = 2 * scale});
    }
    if (race->phase == DD2_RACE_COUNTDOWN) {
        const unsigned cue = dd2_race_countdown(race);
        glColor3f(0, 0, 0);
        dd2_race_draw_box(
            dd2_race_draw_light_panel_left * scale, dd2_race_draw_light_panel_bottom * scale,
            dd2_race_draw_light_panel_width * scale, dd2_race_draw_light_panel_height * scale);
        for (unsigned light = 0; light < 3; ++light) {
            glColor3f(cue != 0 && light < 4 - cue ? 1 : dd2_race_draw_dim, 0, 0);
            dd2_race_draw_box(
                (dd2_race_draw_light_left + ((float)light * dd2_race_draw_light_panel_height)) *
                    scale,
                dd2_race_draw_light_bottom * scale, dd2_race_draw_light_width * scale,
                dd2_race_draw_light_height * scale);
        }
        if (cue != 0) {
            glColor3f(1, 1, 1);
            dd2_race_draw_number(cue,
                                 (dd2_race_draw_pen){.x = dd2_race_draw_cue_left * scale,
                                                     .y = dd2_race_draw_cue_bottom * scale,
                                                     .scale = dd2_race_draw_cue_scale * scale});
        }
    } else if (race->phase == DD2_RACE_RUNNING && race->elapsed < DD2_RACE_DRAW_GO_STEPS) {
        glColor3f(0, 1, 0);
        dd2_race_draw_text("GO", (dd2_race_draw_pen){.x = dd2_race_draw_go_left * scale,
                                                     .y = dd2_race_draw_light_bottom * scale,
                                                     .scale = dd2_race_draw_cue_scale * scale});
    } else if (race->phase == DD2_RACE_COASTING) {
        glColor3f(0, 0, 0);
        dd2_race_draw_box(
            dd2_race_draw_light_panel_left * scale, dd2_race_draw_light_panel_bottom * scale,
            dd2_race_draw_light_panel_width * scale, dd2_race_draw_light_panel_height * scale);
        glColor3f(1, 1, 1);
        dd2_race_draw_text(race->end == DD2_RACE_PLAYER_RETIRED ? "OUT" : "FINISH",
                           (dd2_race_draw_pen){.x = dd2_race_draw_end_left * scale,
                                               .y = dd2_race_draw_end_bottom * scale,
                                               .scale = 4 * scale});
    }
}

bool dd2_race_draw(const dd2_race *race, dd2_render_options viewport) {
    if (race == NULL || race->rules.count == 0 || race->rules.count > DD2_VEHICLE_FLEET_LIMIT ||
        race->phase < DD2_RACE_COUNTDOWN || race->phase > DD2_RACE_RESULTS || viewport.width <= 0 ||
        viewport.height <= 0) {
        return false;
    }
    for (unsigned row = 0; row < race->rules.count; ++row) {
        if (race->phase == DD2_RACE_RESULTS &&
            (race->results[row] >= race->rules.count ||
             race->drivers[race->results[row]].total_points > DD2_ACCIDENT_SCORE_LIMIT)) {
            return false;
        }
    }
    const float scale = fminf((float)viewport.width / (float)DD2_RACE_DRAW_WIDTH,
                              (float)viewport.height / (float)DD2_RACE_DRAW_HEIGHT);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, viewport.width, 0, viewport.height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glBegin(GL_TRIANGLES);
    if (race->phase == DD2_RACE_RESULTS) {
        dd2_race_draw_results(race, scale);
    } else {
        dd2_race_draw_active(race, scale);
    }
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    return glGetError() == GL_NO_ERROR;
}
