#include "render/damage_draw.h"

#include "physics/damage.h"
#include "physics/vehicle.h"
#include "render/renderer.h"

#include <GL/softgl.h>
#include <math.h>

enum {
    DD2_DAMAGE_HUD_MARGIN = 8,
    DD2_DAMAGE_HUD_WIDTH = 30,
    DD2_DAMAGE_HUD_HEIGHT = 44,
    DD2_DAMAGE_HUD_CELL = 9,
    DD2_DAMAGE_HUD_PITCH = 11,
    DD2_DAMAGE_HUD_COLUMNS = 2,
    DD2_DAMAGE_HUD_ROWS = 3,
    DD2_DAMAGE_HUD_BASE_WIDTH = 320,
    DD2_DAMAGE_HUD_BASE_HEIGHT = 240
};
static const double dd2_damage_hud_color_floor = 0.15;
static const double dd2_damage_hud_blue = 0.12;
static const double dd2_damage_hud_color_middle = 0.5;
static const float dd2_damage_hud_cell_bottom = 9;
static const float dd2_damage_hud_arrow_top = 42;
static const float dd2_damage_hud_arrow_bottom = 38;
static const float dd2_damage_hud_bar_width = 20;

typedef struct {
    float left;
    float bottom;
    float width;
    float height;
} dd2_damage_hud_rect;

static void dd2_damage_hud_rectangle(dd2_damage_hud_rect rect, dd2_vehicle_vector color) {
    glColor3f((float)color.x, (float)color.y, (float)color.z);
    glBegin(GL_TRIANGLES);
    glVertex2f(rect.left, rect.bottom);
    glVertex2f(rect.left + rect.width, rect.bottom);
    glVertex2f(rect.left, rect.bottom + rect.height);
    glVertex2f(rect.left + rect.width, rect.bottom);
    glVertex2f(rect.left + rect.width, rect.bottom + rect.height);
    glVertex2f(rect.left, rect.bottom + rect.height);
    glEnd();
}

static dd2_vehicle_vector dd2_damage_hud_color(double crush) {
    return (dd2_vehicle_vector){.x = fmin(1, dd2_damage_hud_color_floor + (2 * crush)),
                                .y = fmax(dd2_damage_hud_color_floor,
                                          1 - (2 * fmax(0, crush - dd2_damage_hud_color_middle))),
                                .z = dd2_damage_hud_blue};
}

static void dd2_damage_hud_icon(const dd2_vehicle_damage *damage, float scale) {
    const float margin = (float)DD2_DAMAGE_HUD_MARGIN;
    dd2_damage_hud_rectangle((dd2_damage_hud_rect){.left = margin * scale,
                                                   .bottom = margin * scale,
                                                   .width = (float)DD2_DAMAGE_HUD_WIDTH * scale,
                                                   .height = (float)DD2_DAMAGE_HUD_HEIGHT * scale},
                             (dd2_vehicle_vector){0});
    for (unsigned region = 0; region < DD2_DAMAGE_REGIONS; ++region) {
        const unsigned row = region / DD2_DAMAGE_HUD_COLUMNS;
        const unsigned column = region % DD2_DAMAGE_HUD_COLUMNS;
        dd2_damage_hud_rectangle(
            (dd2_damage_hud_rect){
                .left = (margin + 4 + ((float)column * (float)DD2_DAMAGE_HUD_PITCH)) * scale,
                .bottom = (margin + dd2_damage_hud_cell_bottom +
                           ((float)(DD2_DAMAGE_HUD_ROWS - 1 - row) * (float)DD2_DAMAGE_HUD_PITCH)) *
                          scale,
                .width = (float)DD2_DAMAGE_HUD_CELL * scale,
                .height = (float)DD2_DAMAGE_HUD_CELL * scale},
            dd2_damage_hud_color(damage->regions[region]));
    }
    glColor3f(1, 1, 1);
    glBegin(GL_TRIANGLES);
    glVertex2f((margin + (float)DD2_DAMAGE_HUD_WIDTH / 2) * scale,
               (margin + dd2_damage_hud_arrow_top) * scale);
    glVertex2f((margin + 4) * scale, (margin + dd2_damage_hud_arrow_bottom) * scale);
    glVertex2f((margin + (float)DD2_DAMAGE_HUD_WIDTH - 4) * scale,
               (margin + dd2_damage_hud_arrow_bottom) * scale);
    glEnd();
    dd2_damage_hud_rectangle((dd2_damage_hud_rect){.left = (margin + 4) * scale,
                                                   .bottom = (margin + 3) * scale,
                                                   .width = dd2_damage_hud_bar_width * scale,
                                                   .height = 3 * scale},
                             (dd2_vehicle_vector){.x = dd2_damage_hud_color_floor,
                                                  .y = dd2_damage_hud_color_floor,
                                                  .z = dd2_damage_hud_color_floor});
    const double health = dd2_damage_health(damage);
    dd2_damage_hud_rectangle(
        (dd2_damage_hud_rect){.left = (margin + 4) * scale,
                              .bottom = (margin + 3) * scale,
                              .width = dd2_damage_hud_bar_width * (float)health * scale,
                              .height = 3 * scale},
        dd2_damage_hud_color(1 - health));
}

bool dd2_damage_draw(const dd2_vehicle_damage *damage, dd2_render_options viewport) {
    if (!dd2_damage_valid(damage) || viewport.width <= 0 || viewport.height <= 0) {
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
    const float scale = fminf(2, fminf((float)viewport.width / (float)DD2_DAMAGE_HUD_BASE_WIDTH,
                                       (float)viewport.height / (float)DD2_DAMAGE_HUD_BASE_HEIGHT));
    dd2_damage_hud_icon(damage, scale);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    return glGetError() == GL_NO_ERROR;
}
