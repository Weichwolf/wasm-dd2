#include "render/score_draw.h"

#include "game/accidents.h"
#include "game/course.h"
#include "game/laps.h"
#include "physics/vehicle_collision.h"
#include "render/renderer.h"

#include <GL/softgl.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

enum {
    DD2_SCORE_WIDTH = 320,
    DD2_SCORE_HEIGHT = 240,
    DD2_SCORE_GLYPH_WIDTH = 3,
    DD2_SCORE_GLYPH_HEIGHT = 5,
    DD2_SCORE_GLYPH_PITCH = 4,
    DD2_SCORE_DIGIT_BASE = 10,
    DD2_SCORE_HUNDREDS = 100,
    DD2_SCORE_GLYPH_P = 10,
    DD2_SCORE_GLYPH_T = 11,
    DD2_SCORE_GLYPH_S = 12,
    DD2_SCORE_GLYPH_K = 13,
    DD2_SCORE_GLYPH_O = 14,
    DD2_SCORE_GLYPH_L = 15,
    DD2_SCORE_GLYPH_A = 16,
    DD2_SCORE_GLYPH_SLASH = 17,
    DD2_SCORE_GLYPH_F = 18,
    DD2_SCORE_GLYPH_I = 19,
    DD2_SCORE_GLYPH_N = 20,
    DD2_SCORE_GLYPHS = 21,
    DD2_SCORE_BAR = 7,
    DD2_SCORE_EDGES = 5,
    DD2_SCORE_LEFT_MIDDLE = 6,
    DD2_SCORE_LEFT = 4,
    DD2_SCORE_MIDDLE = 2,
    DD2_SCORE_RIGHT = 1
};
static const float dd2_score_left = 44;
static const float dd2_score_top_line = 23;
static const float dd2_score_bottom_line = 12;
static const float dd2_score_digits_offset = 16;
static const float dd2_score_backdrop_left = 42;
static const float dd2_score_backdrop_bottom = 8;
static const float dd2_score_backdrop_width = 32;
static const float dd2_score_backdrop_height = 24;

/* Compact 3x5 glyphs for the numeric driving HUD. Full menu typography belongs
 * to the asset/font renderer; these labels remain readable at native scale. */
static const uint8_t dd2_score_glyphs[DD2_SCORE_GLYPHS][DD2_SCORE_GLYPH_HEIGHT] = {
    {DD2_SCORE_BAR, DD2_SCORE_EDGES, DD2_SCORE_EDGES, DD2_SCORE_EDGES, DD2_SCORE_BAR},
    {DD2_SCORE_MIDDLE, DD2_SCORE_LEFT_MIDDLE, DD2_SCORE_MIDDLE, DD2_SCORE_MIDDLE, DD2_SCORE_BAR},
    {DD2_SCORE_BAR, DD2_SCORE_RIGHT, DD2_SCORE_BAR, DD2_SCORE_LEFT, DD2_SCORE_BAR},
    {DD2_SCORE_BAR, DD2_SCORE_RIGHT, DD2_SCORE_BAR, DD2_SCORE_RIGHT, DD2_SCORE_BAR},
    {DD2_SCORE_EDGES, DD2_SCORE_EDGES, DD2_SCORE_BAR, DD2_SCORE_RIGHT, DD2_SCORE_RIGHT},
    {DD2_SCORE_BAR, DD2_SCORE_LEFT, DD2_SCORE_BAR, DD2_SCORE_RIGHT, DD2_SCORE_BAR},
    {DD2_SCORE_BAR, DD2_SCORE_LEFT, DD2_SCORE_BAR, DD2_SCORE_EDGES, DD2_SCORE_BAR},
    {DD2_SCORE_BAR, DD2_SCORE_RIGHT, DD2_SCORE_MIDDLE, DD2_SCORE_MIDDLE, DD2_SCORE_MIDDLE},
    {DD2_SCORE_BAR, DD2_SCORE_EDGES, DD2_SCORE_BAR, DD2_SCORE_EDGES, DD2_SCORE_BAR},
    {DD2_SCORE_BAR, DD2_SCORE_EDGES, DD2_SCORE_BAR, DD2_SCORE_RIGHT, DD2_SCORE_BAR},
    {DD2_SCORE_LEFT_MIDDLE, DD2_SCORE_EDGES, DD2_SCORE_LEFT_MIDDLE, DD2_SCORE_LEFT, DD2_SCORE_LEFT},
    {DD2_SCORE_BAR, DD2_SCORE_MIDDLE, DD2_SCORE_MIDDLE, DD2_SCORE_MIDDLE, DD2_SCORE_MIDDLE},
    {DD2_SCORE_BAR, DD2_SCORE_LEFT, DD2_SCORE_BAR, DD2_SCORE_RIGHT, DD2_SCORE_BAR},
    {DD2_SCORE_EDGES, DD2_SCORE_EDGES, DD2_SCORE_LEFT_MIDDLE, DD2_SCORE_EDGES, DD2_SCORE_EDGES},
    {DD2_SCORE_BAR, DD2_SCORE_EDGES, DD2_SCORE_EDGES, DD2_SCORE_EDGES, DD2_SCORE_BAR},
    {DD2_SCORE_LEFT, DD2_SCORE_LEFT, DD2_SCORE_LEFT, DD2_SCORE_LEFT, DD2_SCORE_BAR},
    {DD2_SCORE_MIDDLE, DD2_SCORE_EDGES, DD2_SCORE_BAR, DD2_SCORE_EDGES, DD2_SCORE_EDGES},
    {DD2_SCORE_RIGHT, DD2_SCORE_RIGHT, DD2_SCORE_MIDDLE, DD2_SCORE_LEFT, DD2_SCORE_LEFT},
    {DD2_SCORE_BAR, DD2_SCORE_LEFT, DD2_SCORE_LEFT_MIDDLE, DD2_SCORE_LEFT, DD2_SCORE_LEFT},
    {DD2_SCORE_BAR, DD2_SCORE_MIDDLE, DD2_SCORE_MIDDLE, DD2_SCORE_MIDDLE, DD2_SCORE_BAR},
    {DD2_SCORE_EDGES, DD2_SCORE_BAR, DD2_SCORE_BAR, DD2_SCORE_BAR, DD2_SCORE_EDGES}};

typedef struct {
    float x;
    float y;
    float scale;
} dd2_score_pen;

static void dd2_score_glyph(unsigned glyph, dd2_score_pen pen) {
    for (unsigned row = 0; row < DD2_SCORE_GLYPH_HEIGHT; ++row) {
        for (unsigned column = 0; column < DD2_SCORE_GLYPH_WIDTH; ++column) {
            if ((dd2_score_glyphs[glyph][row] & (1U << (DD2_SCORE_GLYPH_WIDTH - column - 1))) ==
                0) {
                continue;
            }
            const float left = pen.x + ((float)column * pen.scale);
            const float bottom = pen.y + ((float)(DD2_SCORE_GLYPH_HEIGHT - row - 1) * pen.scale);
            glVertex2f(left, bottom);
            glVertex2f(left + pen.scale, bottom);
            glVertex2f(left, bottom + pen.scale);
            glVertex2f(left + pen.scale, bottom);
            glVertex2f(left + pen.scale, bottom + pen.scale);
            glVertex2f(left, bottom + pen.scale);
        }
    }
}

static void dd2_score_run(const unsigned *glyphs, size_t count, dd2_score_pen pen) {
    for (size_t index = 0; index < count; ++index) {
        dd2_score_glyph(glyphs[index], pen);
        pen.x += (float)DD2_SCORE_GLYPH_PITCH * pen.scale;
    }
}

static void dd2_score_lines(const dd2_accident_driver *score, float scale) {
    const unsigned label[] = {DD2_SCORE_GLYPH_P, DD2_SCORE_GLYPH_T, DD2_SCORE_GLYPH_S};
    const unsigned points[] = {score->points / DD2_SCORE_HUNDREDS,
                               (score->points / DD2_SCORE_DIGIT_BASE) % DD2_SCORE_DIGIT_BASE,
                               score->points % DD2_SCORE_DIGIT_BASE};
    const unsigned knockouts[] = {DD2_SCORE_GLYPH_K, DD2_SCORE_GLYPH_O};
    const unsigned destructions[] = {score->destructions / DD2_SCORE_DIGIT_BASE,
                                     score->destructions % DD2_SCORE_DIGIT_BASE};
    const float left = dd2_score_backdrop_left * scale;
    const float right = (dd2_score_backdrop_left + dd2_score_backdrop_width) * scale;
    const float bottom = dd2_score_backdrop_bottom * scale;
    const float top = (dd2_score_backdrop_bottom + dd2_score_backdrop_height) * scale;
    glColor3f(0, 0, 0);
    glBegin(GL_TRIANGLES);
    glVertex2f(left, bottom);
    glVertex2f(right, bottom);
    glVertex2f(left, top);
    glVertex2f(right, bottom);
    glVertex2f(right, top);
    glVertex2f(left, top);
    glColor3f(1, 1, 1);
    dd2_score_run(label, sizeof(label) / sizeof(label[0]),
                  (dd2_score_pen){.x = dd2_score_left * scale,
                                  .y = dd2_score_top_line * scale,
                                  .scale = scale});
    dd2_score_run(points, sizeof(points) / sizeof(points[0]),
                  (dd2_score_pen){.x = (dd2_score_left + dd2_score_digits_offset) * scale,
                                  .y = dd2_score_top_line * scale,
                                  .scale = scale});
    dd2_score_run(knockouts, sizeof(knockouts) / sizeof(knockouts[0]),
                  (dd2_score_pen){.x = dd2_score_left * scale,
                                  .y = dd2_score_bottom_line * scale,
                                  .scale = scale});
    dd2_score_run(destructions, sizeof(destructions) / sizeof(destructions[0]),
                  (dd2_score_pen){.x = (dd2_score_left + dd2_score_digits_offset) * scale,
                                  .y = dd2_score_bottom_line * scale,
                                  .scale = scale});
    glEnd();
}

bool dd2_score_draw(const dd2_accident_driver *score, dd2_render_options viewport) {
    if (score == NULL || score->points > DD2_ACCIDENT_SCORE_LIMIT ||
        score->destructions >= DD2_VEHICLE_FLEET_LIMIT || viewport.width <= 0 ||
        viewport.height <= 0) {
        return false;
    }
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
    const float scale = fminf(2, fminf((float)viewport.width / (float)DD2_SCORE_WIDTH,
                                       (float)viewport.height / (float)DD2_SCORE_HEIGHT));
    dd2_score_lines(score, scale);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    return glGetError() == GL_NO_ERROR;
}

bool dd2_lap_draw(const dd2_lap_driver *lap, unsigned required, dd2_render_options viewport) {
    if (lap == NULL || required == 0 || required > DD2_COURSE_LAP_LIMIT ||
        lap->started_laps > required + 1 || viewport.width <= 0 || viewport.height <= 0) {
        return false;
    }
    unsigned current = lap->started_laps;
    if (current == 0) {
        current = 1;
    }
    if (current > required) {
        current = required;
    }
    const unsigned glyphs[] = {DD2_SCORE_GLYPH_L,
                               DD2_SCORE_GLYPH_A,
                               DD2_SCORE_GLYPH_P,
                               current / DD2_SCORE_DIGIT_BASE,
                               current % DD2_SCORE_DIGIT_BASE,
                               DD2_SCORE_GLYPH_SLASH,
                               required / DD2_SCORE_DIGIT_BASE,
                               required % DD2_SCORE_DIGIT_BASE};
    const unsigned finish[] = {DD2_SCORE_GLYPH_F, DD2_SCORE_GLYPH_I, DD2_SCORE_GLYPH_N};
    const float scale = fminf(2, fminf((float)viewport.width / (float)DD2_SCORE_WIDTH,
                                       (float)viewport.height / (float)DD2_SCORE_HEIGHT));
    const float left = 8 * scale;
    const float top = (float)viewport.height - (8 * scale);
    const size_t glyph_count = sizeof(glyphs) / sizeof(glyphs[0]);
    const float right = left + ((float)DD2_SCORE_GLYPH_PITCH * (float)glyph_count * scale);
    const float bottom = top - ((DD2_SCORE_GLYPH_HEIGHT + 4) * scale);
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
    glColor3f(0, 0, 0);
    glBegin(GL_TRIANGLES);
    glVertex2f(left - scale, bottom - scale);
    glVertex2f(right + scale, bottom - scale);
    glVertex2f(left - scale, top + scale);
    glVertex2f(right + scale, bottom - scale);
    glVertex2f(right + scale, top + scale);
    glVertex2f(left - scale, top + scale);
    glColor3f(1, 1, 1);
    dd2_score_run(lap->finished ? finish : glyphs,
                  lap->finished ? sizeof(finish) / sizeof(finish[0])
                                : sizeof(glyphs) / sizeof(glyphs[0]),
                  (dd2_score_pen){.x = left, .y = bottom + (2 * scale), .scale = scale});
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    return glGetError() == GL_NO_ERROR;
}
