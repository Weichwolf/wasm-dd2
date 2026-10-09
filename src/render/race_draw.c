#include "render/race_draw.h"

#include "game/accidents.h"
#include "game/championship.h"
#include "game/configuration.h"
#include "game/drivers.h"
#include "game/league.h"
#include "game/profile_menu.h"
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
    DD2_RACE_DRAW_SYMBOL_BASE = 39,
    DD2_RACE_DRAW_SYMBOLS = 29,
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

/* Printable ASCII punctuation for profile names; lower-case letters retain the
 * existing upper-case presentation while stored identity keeps its exact case. */
static const char dd2_race_draw_symbols[] = "!\"#$%&'()*+,-;<=>?@[\\]^_`{|}~";
static const uint8_t dd2_race_draw_symbol_font[DD2_RACE_DRAW_SYMBOLS][DD2_RACE_DRAW_GLYPH_HEIGHT] =
    {{4, 4, 4, 4, 4, 0, 4},        {10, 10, 10, 0, 0, 0, 0}, {10, 31, 10, 10, 31, 10, 0},
     {4, 15, 20, 14, 5, 30, 4},    {17, 2, 4, 8, 17, 0, 0},  {12, 18, 20, 8, 21, 18, 13},
     {4, 4, 8, 0, 0, 0, 0},        {2, 4, 8, 8, 8, 4, 2},    {8, 4, 2, 2, 2, 4, 8},
     {0, 21, 14, 31, 14, 21, 0},   {0, 4, 4, 31, 4, 4, 0},   {0, 0, 0, 0, 4, 4, 8},
     {0, 0, 0, 31, 0, 0, 0},       {0, 4, 4, 0, 4, 4, 8},    {2, 4, 8, 16, 8, 4, 2},
     {0, 0, 31, 0, 31, 0, 0},      {8, 4, 2, 1, 2, 4, 8},    {14, 17, 1, 2, 4, 0, 4},
     {14, 17, 23, 21, 23, 16, 14}, {14, 8, 8, 8, 8, 8, 14},  {16, 8, 4, 2, 1, 0, 0},
     {14, 2, 2, 2, 2, 2, 14},      {4, 10, 17, 0, 0, 0, 0},  {0, 0, 0, 0, 0, 0, 31},
     {8, 4, 2, 0, 0, 0, 0},        {2, 4, 4, 8, 4, 4, 2},    {4, 4, 4, 4, 4, 4, 4},
     {8, 4, 4, 2, 4, 4, 8},        {0, 0, 9, 22, 0, 0, 0}};
_Static_assert(sizeof(dd2_race_draw_symbols) == DD2_RACE_DRAW_SYMBOLS + 1,
               "ASCII punctuation extent");

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

static unsigned dd2_race_draw_glyph(unsigned char character) {
    if (character >= 'a' && character <= 'z') {
        character = (unsigned char)(character - ('a' - 'A'));
    }
    if (character >= '0' && character <= '9') {
        return (unsigned)(character - '0');
    }
    if (character >= 'A' && character <= 'Z') {
        return DD2_RACE_DRAW_LETTER_BASE + (unsigned)(character - 'A');
    }
    switch (character) {
    case '/':
        return DD2_RACE_DRAW_SLASH;
    case ':':
        return DD2_RACE_DRAW_COLON;
    case '.':
        return DD2_RACE_DRAW_DOT;
    default:
        for (unsigned index = 0; index < DD2_RACE_DRAW_SYMBOLS; ++index) {
            if (character == (unsigned char)dd2_race_draw_symbols[index]) {
                return DD2_RACE_DRAW_SYMBOL_BASE + index;
            }
        }
        return DD2_RACE_DRAW_SYMBOL_BASE + DD2_RACE_DRAW_SYMBOLS;
    }
}

static void dd2_race_draw_text(const char *text, dd2_race_draw_pen pen) {
    for (size_t index = 0; text[index] != '\0'; ++index) {
        const unsigned glyph = dd2_race_draw_glyph((unsigned char)text[index]);
        if (glyph < DD2_RACE_DRAW_SYMBOL_BASE + DD2_RACE_DRAW_SYMBOLS) {
            const uint8_t *rows =
                glyph < DD2_RACE_DRAW_SYMBOL_BASE
                    ? dd2_race_draw_font[glyph]
                    : dd2_race_draw_symbol_font[glyph - DD2_RACE_DRAW_SYMBOL_BASE];
            for (unsigned row = 0; row < DD2_RACE_DRAW_GLYPH_HEIGHT; ++row) {
                for (unsigned column = 0; column < DD2_RACE_DRAW_GLYPH_WIDTH; ++column) {
                    if ((rows[row] & (1U << (DD2_RACE_DRAW_GLYPH_WIDTH - column - 1))) != 0) {
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

static void dd2_race_draw_survival(const dd2_race *race, float scale, bool results) {
    static const float left = 20;
    static const float bottom = 416;
    static const float width = 300;
    static const float height = 50;
    static const float labels_left = 28;
    static const float values_left = 144;
    static const float first_line = 448;
    static const float result_offset = 108;
    static const float line_height = 24;
    const float offset = results ? result_offset : 0;
    if (!results) {
        glColor3f(0, 0, 0);
        dd2_race_draw_box(left * scale, bottom * scale, width * scale, height * scale);
    }
    glColor3f(1, 1, 0);
    dd2_race_draw_text("SURVIVAL", (dd2_race_draw_pen){.x = labels_left * scale,
                                                       .y = (first_line - offset) * scale,
                                                       .scale = 2 * scale});
    dd2_race_draw_time(race->survival, (dd2_race_draw_pen){.x = values_left * scale,
                                                           .y = (first_line - offset) * scale,
                                                           .scale = 2 * scale});
    dd2_race_draw_text("ALIVE",
                       (dd2_race_draw_pen){.x = labels_left * scale,
                                           .y = (first_line - offset - line_height) * scale,
                                           .scale = 2 * scale});
    dd2_race_draw_number(race->alive,
                         (dd2_race_draw_pen){.x = values_left * scale,
                                             .y = (first_line - offset - line_height) * scale,
                                             .scale = 2 * scale});
}

static void dd2_race_draw_table(const dd2_race *race, float scale) {
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
            dd2_race_draw_number(
                numbers[column],
                (dd2_race_draw_pen){.x = columns[column] * scale, .y = bottom, .scale = 2 * scale});
        }
        const char *status = driver->retired ? "OUT" : "DNF";
        if (driver->finish_place != 0) {
            status = "FIN";
        }
        dd2_race_draw_text(
            status, (dd2_race_draw_pen){.x = columns[4] * scale, .y = bottom, .scale = 2 * scale});
        if (slot == 0) {
            dd2_race_draw_text("YOU", (dd2_race_draw_pen){.x = dd2_race_draw_player_left * scale,
                                                          .y = bottom,
                                                          .scale = 2 * scale});
        }
    }
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
    } else if (race->rules.mode == DD2_RACE_TOTAL_DESTRUCTION) {
        mode = "TOTAL DESTRUCTION";
    }
    dd2_race_draw_text(mode, (dd2_race_draw_pen){.x = dd2_race_draw_mode_left * scale,
                                                 .y = dd2_race_draw_title_bottom * scale,
                                                 .scale = 2 * scale});
    if (race->rules.mode == DD2_RACE_TIME_TRIAL) {
        dd2_race_draw_trial(race, scale, true);
    } else if (race->rules.mode == DD2_RACE_TOTAL_DESTRUCTION) {
        dd2_race_draw_survival(race, scale, true);
    } else {
        dd2_race_draw_table(race, scale);
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
    } else if (race->rules.mode == DD2_RACE_TOTAL_DESTRUCTION) {
        dd2_race_draw_survival(race, scale, false);
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

static const float dd2_champ_draw_left = 32;
static const float dd2_champ_draw_label_bottom = 6;
static const float dd2_champ_draw_label_height = 20;
static const float dd2_champ_draw_season_left = 30;
static const float dd2_champ_draw_round_left = 176;
static const float dd2_champ_draw_division_left = 298;
static const float dd2_champ_draw_points_left = 440;
static const float dd2_champ_draw_number_offset = 60;
static const float dd2_champ_draw_group_width = 296;
static const float dd2_champ_draw_group_height = 142;
static const float dd2_champ_draw_first_group_bottom = 362;
static const float dd2_champ_draw_name_offset = 18;
static const float dd2_champ_draw_points_offset = 228;
static const float dd2_champ_draw_hint_bottom = 44;
static const float dd2_champ_draw_outcome_bottom = 94;
static const float dd2_champ_draw_white = 0.88F;
static const float dd2_champ_draw_background_red = 0.04F;
static const float dd2_champ_draw_background_green = 0.07F;
static const float dd2_champ_draw_background_blue = 0.12F;

static void dd2_champ_draw_metadata(const dd2_championship *state, float scale, float bottom) {
    const dd2_championship_season *season = dd2_championship_current(state);
    const unsigned number =
        (unsigned)(season->number < UINT32_MAX ? season->number + 1 : UINT32_MAX);
    const unsigned round = season->completed + (unsigned)(state->phase == DD2_CHAMPIONSHIP_RACING);
    glColor3f(dd2_champ_draw_white, dd2_champ_draw_white, dd2_champ_draw_white);
    dd2_race_draw_text(
        "SEASON",
        (dd2_race_draw_pen){.x = dd2_champ_draw_season_left * scale, .y = bottom, .scale = scale});
    dd2_race_draw_number(
        number, (dd2_race_draw_pen){
                    .x = (dd2_champ_draw_season_left + dd2_champ_draw_number_offset) * scale,
                    .y = bottom,
                    .scale = scale});
    dd2_race_draw_text(
        "RACE",
        (dd2_race_draw_pen){.x = dd2_champ_draw_round_left * scale, .y = bottom, .scale = scale});
    dd2_race_draw_number(
        round,
        (dd2_race_draw_pen){.x = (dd2_champ_draw_round_left + dd2_champ_draw_number_offset) * scale,
                            .y = bottom,
                            .scale = scale});
    dd2_race_draw_text("DIVISION", (dd2_race_draw_pen){.x = dd2_champ_draw_division_left * scale,
                                                       .y = bottom,
                                                       .scale = scale});
    dd2_race_draw_number(
        state->league.drivers[0].division + 1,
        (dd2_race_draw_pen){.x = (dd2_champ_draw_division_left + dd2_champ_draw_number_offset) *
                                 scale,
                            .y = bottom,
                            .scale = scale});
    dd2_race_draw_text(
        "POINTS",
        (dd2_race_draw_pen){.x = dd2_champ_draw_points_left * scale, .y = bottom, .scale = scale});
    dd2_race_draw_number(
        state->league.drivers[0].points,
        (dd2_race_draw_pen){.x =
                                (dd2_champ_draw_points_left + dd2_champ_draw_number_offset) * scale,
                            .y = bottom,
                            .scale = scale});
}

static void dd2_champ_draw_standings(const dd2_championship *state, const char *human,
                                     float scale) {
    for (unsigned division = 0; division < DD2_LEAGUE_DIVISIONS; ++division) {
        const unsigned group_row = division / 2;
        const float left =
            (dd2_champ_draw_left + ((float)(division % 2) * dd2_champ_draw_group_width)) * scale;
        const float bottom =
            (dd2_champ_draw_first_group_bottom - ((float)group_row * dd2_champ_draw_group_height)) *
            scale;
        glColor3f(dd2_champ_draw_white, dd2_champ_draw_white, dd2_champ_draw_white);
        dd2_race_draw_text("DIVISION", (dd2_race_draw_pen){.x = left, .y = bottom, .scale = scale});
        dd2_race_draw_number(division + 1,
                             (dd2_race_draw_pen){.x = left + (dd2_champ_draw_number_offset * scale),
                                                 .y = bottom,
                                                 .scale = scale});
    }
    for (unsigned driver = 0; driver < DD2_LEAGUE_DRIVERS; ++driver) {
        const dd2_league_driver standing = state->league.drivers[driver];
        const unsigned group_row = standing.division / 2;
        const float left =
            (dd2_champ_draw_left + ((float)(standing.division % 2) * dd2_champ_draw_group_width)) *
            scale;
        const float bottom =
            (dd2_champ_draw_first_group_bottom - ((float)group_row * dd2_champ_draw_group_height) -
             ((float)(standing.rank + 1) * (float)DD2_RACE_DRAW_LINE)) *
            scale;
        if (driver == 0) {
            glColor3f(1, 1, 0);
        } else {
            glColor3f(dd2_champ_draw_white, dd2_champ_draw_white, dd2_champ_draw_white);
        }
        dd2_race_draw_number(standing.rank + 1,
                             (dd2_race_draw_pen){.x = left, .y = bottom, .scale = scale});
        dd2_race_draw_text(driver == 0 ? human : dd2_driver_name(driver),
                           (dd2_race_draw_pen){.x = left + (dd2_champ_draw_name_offset * scale),
                                               .y = bottom,
                                               .scale = scale});
        dd2_race_draw_number(standing.points,
                             (dd2_race_draw_pen){.x = left + (dd2_champ_draw_points_offset * scale),
                                                 .y = bottom,
                                                 .scale = scale});
    }
}

static const char *dd2_champ_draw_outcome(dd2_league_outcome outcome) {
    switch (outcome) {
    case DD2_LEAGUE_PROMOTED:
        return "PROMOTED";
    case DD2_LEAGUE_STAYS:
        return "DIVISION RETAINED";
    case DD2_LEAGUE_RELEGATED:
        return "RELEGATED";
    case DD2_LEAGUE_CHAMPION:
        return "CHAMPION";
    case DD2_LEAGUE_ELIMINATED:
        return "ELIMINATED";
    default:
        return "ROUND COMPLETE";
    }
}

bool dd2_championship_draw_named(const dd2_championship *championship, const char *human,
                                 dd2_render_options viewport) {
    if (!dd2_championship_valid(championship) || !dd2_configuration_player_valid(human) ||
        viewport.width <= 0 || viewport.height <= 0) {
        return false;
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
    glColor3f(dd2_champ_draw_background_red, dd2_champ_draw_background_green,
              dd2_champ_draw_background_blue);
    const bool racing = championship->phase == DD2_CHAMPIONSHIP_RACING;
    if (racing) {
        dd2_race_draw_box(0, 0, (float)viewport.width, dd2_champ_draw_label_height * scale);
        dd2_champ_draw_metadata(championship, scale, dd2_champ_draw_label_bottom * scale);
    } else {
        dd2_race_draw_box(dd2_race_draw_panel_left * scale, dd2_race_draw_panel_bottom * scale,
                          dd2_race_draw_panel_width * scale, dd2_race_draw_panel_height * scale);
        glColor3f(dd2_champ_draw_white, dd2_champ_draw_white, dd2_champ_draw_white);
        dd2_race_draw_text(championship->mode == DD2_RACE_STOCKCAR ? "STOCKCAR CHAMPIONSHIP"
                                                                   : "WRECKING CHAMPIONSHIP",
                           (dd2_race_draw_pen){.x = dd2_champ_draw_left * scale,
                                               .y = dd2_race_draw_title_bottom * scale,
                                               .scale = scale});
        dd2_champ_draw_metadata(championship, scale, dd2_race_draw_header_bottom * scale);
        dd2_champ_draw_standings(championship, human, scale);
        glColor3f(1, 1, 0);
        dd2_race_draw_text(dd2_champ_draw_outcome(dd2_championship_current(championship)->outcome),
                           (dd2_race_draw_pen){.x = dd2_champ_draw_left * scale,
                                               .y = dd2_champ_draw_outcome_bottom * scale,
                                               .scale = scale});
        const bool terminal = championship->phase == DD2_CHAMPIONSHIP_CHAMPION ||
                              championship->phase == DD2_CHAMPIONSHIP_ELIMINATED;
        dd2_race_draw_text(terminal ? "ENTER OR ESC TO LEAVE" : "ENTER TO CONTINUE   ESC TO LEAVE",
                           (dd2_race_draw_pen){.x = dd2_champ_draw_left * scale,
                                               .y = dd2_champ_draw_hint_bottom * scale,
                                               .scale = scale});
    }
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    return glGetError() == GL_NO_ERROR;
}

bool dd2_championship_draw(const dd2_championship *championship, dd2_render_options viewport) {
    return dd2_championship_draw_named(championship, dd2_driver_name(0), viewport);
}

static const char *dd2_profile_draw_title(dd2_profile_phase phase) {
    switch (phase) {
    case DD2_PROFILE_PLAYER_NAME:
        return "PLAYER NAME";
    case DD2_PROFILE_OPEN_LOAD:
    case DD2_PROFILE_LOAD_SELECT:
        return "RESTORE AUDIO AND PLAYER";
    case DD2_PROFILE_MESSAGE:
        return "AUDIO AND PLAYER STATUS";
    default:
        return "SAVE AUDIO AND PLAYER";
    }
}
static const char *dd2_profile_draw_help(dd2_profile_phase phase) {
    switch (phase) {
    case DD2_PROFILE_PLAYER_NAME:
    case DD2_PROFILE_SAVE_NAME:
        return "EIGHT ASCII CHARACTERS. BACKSPACE EDITS.";
    case DD2_PROFILE_CONFIRM:
        return "ENTER REPLACES THE SELECTED ENTRY.";
    case DD2_PROFILE_SAVE_SELECT:
    case DD2_PROFILE_LOAD_SELECT:
        return "LEFT/RIGHT CHOOSE AN ENTRY.";
    default:
        return "";
    }
}
static const char *dd2_profile_draw_footer(dd2_profile_phase phase) {
    switch (phase) {
    case DD2_PROFILE_WRITING:
    case DD2_PROFILE_OPEN_SAVE:
    case DD2_PROFILE_OPEN_LOAD:
        return "PLEASE WAIT FOR COMPLETION.";
    case DD2_PROFILE_MESSAGE:
        return "ENTER OR ESCAPE RETURNS.";
    default:
        return "ENTER CONFIRMS. ESCAPE CANCELS.";
    }
}
static const float dd2_profile_draw_left = 32;
static const float dd2_profile_draw_bottom = 160;
static const float dd2_profile_draw_width = 576;
static const float dd2_profile_draw_height = 176;
static const float dd2_profile_draw_text_left = 48;
static const float dd2_profile_draw_title_bottom = 306;
static const float dd2_profile_draw_entry_bottom = 270;
static const float dd2_profile_draw_number_left = 132;
static const float dd2_profile_draw_entry_left = 190;
static const float dd2_profile_draw_help_bottom = 220;
static const float dd2_profile_draw_footer_bottom = 180;
bool dd2_profile_draw(const dd2_profile_menu *menu, const char *selected,
                      dd2_render_options viewport) {
    if (menu == NULL || viewport.width <= 0 || viewport.height <= 0) {
        return false;
    }
    if (menu->phase == DD2_PROFILE_CLOSED) {
        return true;
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
    glColor3f(0, 0, 0);
    dd2_race_draw_box(dd2_profile_draw_left * scale, dd2_profile_draw_bottom * scale,
                      dd2_profile_draw_width * scale, dd2_profile_draw_height * scale);
    glColor3f(1, 1, 1);
    const char *title = dd2_profile_draw_title(menu->phase);
    dd2_race_draw_text(title, (dd2_race_draw_pen){.x = dd2_profile_draw_text_left * scale,
                                                  .y = dd2_profile_draw_title_bottom * scale,
                                                  .scale = 2 * scale});
    if (dd2_profile_menu_editing(menu) || menu->phase == DD2_PROFILE_CONFIRM) {
        glColor3f(1, 1, 0);
        dd2_race_draw_text(menu->draft,
                           (dd2_race_draw_pen){.x = dd2_profile_draw_text_left * scale,
                                               .y = dd2_profile_draw_entry_bottom * scale,
                                               .scale = 3 * scale});
        glColor3f(1, 1, 1);
    }
    if (menu->phase == DD2_PROFILE_SAVE_SELECT || menu->phase == DD2_PROFILE_LOAD_SELECT) {
        dd2_race_draw_text("ENTRY", (dd2_race_draw_pen){.x = dd2_profile_draw_text_left * scale,
                                                        .y = dd2_profile_draw_entry_bottom * scale,
                                                        .scale = 2 * scale});
        dd2_race_draw_number(menu->logical + 1,
                             (dd2_race_draw_pen){.x = dd2_profile_draw_number_left * scale,
                                                 .y = dd2_profile_draw_entry_bottom * scale,
                                                 .scale = 2 * scale});
        dd2_race_draw_text(selected == NULL ? "EMPTY" : selected,
                           (dd2_race_draw_pen){.x = dd2_profile_draw_entry_left * scale,
                                               .y = dd2_profile_draw_entry_bottom * scale,
                                               .scale = 2 * scale});
    }
    const char *help = dd2_profile_draw_help(menu->phase);
    dd2_race_draw_text(menu->message == NULL ? help : menu->message,
                       (dd2_race_draw_pen){.x = dd2_profile_draw_text_left * scale,
                                           .y = dd2_profile_draw_help_bottom * scale,
                                           .scale = 2 * scale});
    dd2_race_draw_text(dd2_profile_draw_footer(menu->phase),
                       (dd2_race_draw_pen){.x = dd2_profile_draw_text_left * scale,
                                           .y = dd2_profile_draw_footer_bottom * scale,
                                           .scale = 2 * scale});
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    return glGetError() == GL_NO_ERROR;
}
