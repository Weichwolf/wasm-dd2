#include "render/driving_draw.h"

#include "assets/level.h"
#include "assets/track.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"
#include "render/damage_draw.h"
#include "render/mesh_draw.h"
#include "render/score_draw.h"

#include <GL/softgl.h>
#include <math.h>
#include <stddef.h>

static const double dd2_chase_distance = 1600;
static const double dd2_chase_height = 650;
static const double dd2_chase_target_distance = 400;
static const double dd2_chase_target_height = 100;
static const double dd2_chase_near = 30;
static const double dd2_chase_far = 150000;
static const double dd2_chase_half_fov_tangent = 0.57735026918962576451;
static const double dd2_draw_degrees = 57.2957795130823208768;
static const float dd2_draw_body_height = -60;
static const float dd2_draw_wheel_width = 152;
static const float dd2_draw_wheel_front = 291;
static const float dd2_draw_wheel_rear = -232;
static const float dd2_draw_wheel_height = -130;
static const float dd2_draw_wheel_scale = 60.0F / 67.0F;
static const double dd2_chase_axis_tolerance = 1e-6;

static void dd2_driving_camera(dd2_driving_view view) {
    dd2_vehicle_vector forward =
        dd2_vehicle_rotate(view.vehicle->rotation, (dd2_vehicle_vector){.z = 1});
    double length = hypot(forward.x, forward.z);
    if (length < dd2_chase_axis_tolerance) {
        forward = (dd2_vehicle_vector){.z = 1};
        length = 1;
    }
    forward.x /= length;
    forward.z /= length;
    const dd2_vehicle_vector eye = {
        .x = view.vehicle->position.x - (forward.x * dd2_chase_distance),
        .y = view.vehicle->position.y + dd2_chase_height,
        .z = view.vehicle->position.z - (forward.z * dd2_chase_distance)};
    const double distance = dd2_chase_distance + dd2_chase_target_distance;
    const double height = dd2_chase_target_height - dd2_chase_height;
    const double ray_length = hypot(distance, height);
    const dd2_vehicle_vector ray = {.x = forward.x * distance / ray_length,
                                    .y = height / ray_length,
                                    .z = forward.z * distance / ray_length};
    const dd2_vehicle_vector right = {.x = -forward.z, .z = forward.x};
    const dd2_vehicle_vector up_axis = {
        .x = -ray.y * forward.x, .y = distance / ray_length, .z = -ray.y * forward.z};
    const float matrix[] = {
        (float)right.x,
        (float)up_axis.x,
        (float)-ray.x,
        0,
        0,
        (float)up_axis.y,
        (float)-ray.y,
        0,
        (float)right.z,
        (float)up_axis.z,
        (float)-ray.z,
        0,
        (float)(-(right.x * eye.x) - (right.z * eye.z)),
        (float)(-(up_axis.x * eye.x) - (up_axis.y * eye.y) - (up_axis.z * eye.z)),
        (float)((ray.x * eye.x) + (ray.y * eye.y) + (ray.z * eye.z)),
        1};
    const double top = dd2_chase_near * dd2_chase_half_fov_tangent;
    const double aspect = (double)view.viewport.width / (double)view.viewport.height;
    glViewport(0, 0, view.viewport.width, view.viewport.height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-top * aspect, top * aspect, -top, top, dd2_chase_near, dd2_chase_far);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(matrix);
}

static void dd2_driving_pose(const dd2_vehicle *vehicle) {
    const dd2_vehicle_vector right =
        dd2_vehicle_rotate(vehicle->rotation, (dd2_vehicle_vector){.x = 1});
    const dd2_vehicle_vector up_axis =
        dd2_vehicle_rotate(vehicle->rotation, (dd2_vehicle_vector){.y = 1});
    const dd2_vehicle_vector forward =
        dd2_vehicle_rotate(vehicle->rotation, (dd2_vehicle_vector){.z = 1});
    const float matrix[] = {(float)right.x,
                            (float)right.y,
                            (float)right.z,
                            0,
                            (float)up_axis.x,
                            (float)up_axis.y,
                            (float)up_axis.z,
                            0,
                            (float)forward.x,
                            (float)forward.y,
                            (float)forward.z,
                            0,
                            (float)vehicle->position.x,
                            (float)vehicle->position.y,
                            (float)vehicle->position.z,
                            1};
    glMultMatrixf(matrix);
}

static bool dd2_driving_car(dd2_mesh_materials *materials, const dd2_track *track,
                            dd2_driving_view view) {
    glPushMatrix();
    dd2_driving_pose(view.vehicle);
    glPushMatrix();
    /* The source visual rig differs from the contact rig. Lower the body to fit
     * the suspended wheels; preserve the source visual wheel XZ placements. */
    glTranslatef(0, dd2_draw_body_height, 0);
    bool drawn =
        dd2_mesh_draw_damaged(materials, dd2_track_car(track), (dd2_track_vertex){0}, view.damage);
    glPopMatrix();
    for (unsigned wheel = 0; wheel < DD2_VEHICLE_WHEELS && drawn; ++wheel) {
        const bool front = (wheel & 1U) == 0;
        glPushMatrix();
        glTranslatef(wheel < 2 ? dd2_draw_wheel_width : -dd2_draw_wheel_width,
                     dd2_draw_wheel_height + (float)view.vehicle->wheels[wheel].compression,
                     front ? dd2_draw_wheel_front : dd2_draw_wheel_rear);
        if (front) {
            glRotatef((float)(view.vehicle->steering * dd2_draw_degrees), 0, 1, 0);
        }
        glRotatef((float)(view.wheel_roll * dd2_draw_degrees), 1, 0, 0);
        glScalef(dd2_draw_wheel_scale, dd2_draw_wheel_scale, dd2_draw_wheel_scale);
        drawn = dd2_mesh_draw(materials, dd2_track_wheel(track, wheel), (dd2_track_vertex){0});
        glPopMatrix();
    }
    glPopMatrix();
    return drawn;
}

bool dd2_driving_draw(dd2_mesh_materials *materials, const dd2_track *track,
                      dd2_driving_view view) {
    if (materials == NULL || track == NULL || view.vehicle == NULL || view.viewport.width <= 0 ||
        view.viewport.height <= 0 || view.opponent_count >= DD2_VEHICLE_FLEET_LIMIT ||
        (view.opponent_count != 0 && (view.opponents == NULL || view.opponent_rolls == NULL))) {
        return false;
    }
    dd2_driving_camera(view);
    bool drawn = dd2_scene_draw(materials, dd2_track_scene(track)) &&
                 dd2_driving_car(materials, track, view);
    for (unsigned index = 0; index < view.opponent_count && drawn; ++index) {
        drawn = dd2_driving_car(materials, track,
                                (dd2_driving_view){.vehicle = &view.opponents[index],
                                                   .damage = view.opponent_damage == NULL
                                                                 ? NULL
                                                                 : &view.opponent_damage[index],
                                                   .wheel_roll = view.opponent_rolls[index],
                                                   .viewport = view.viewport});
    }
    return drawn && (view.damage == NULL || dd2_damage_draw(view.damage, view.viewport)) &&
           (view.score == NULL || dd2_score_draw(view.score, view.viewport)) &&
           (view.lap == NULL || dd2_lap_draw(view.lap, view.required_laps, view.viewport));
}
