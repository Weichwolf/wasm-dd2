#include "render/track_draw.h"

#include "assets/bounds.h"
#include "assets/car.h"
#include "assets/car_class.h"
#include "assets/level.h"
#include "assets/model.h"
#include "assets/road.h"
#include "assets/track.h"
#include "assets/world.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"
#include "render/camera.h"
#include "render/driving_draw.h"
#include "render/frustum.h"
#include "render/mesh_draw.h"
#include "render/model_draw.h"
#include "render/renderer.h"
#include "render/world_draw.h"

#include <GL/softgl.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

enum {
    DD2_TRACK_DRAW_BODIES = DD2_CAR_LIVERIES * DD2_TRACK_BODY_LODS,
    DD2_TRACK_DRAW_MODELS = DD2_TRACK_DRAW_BODIES + 2,
    DD2_TRACK_DRAW_AXES = 3
};
static const float dd2_track_draw_body_height = -60.0F / (float)DD2_ROAD_UNITS_PER_METER;
static const float dd2_track_draw_wheel_width = 152.0F / (float)DD2_ROAD_UNITS_PER_METER;
static const float dd2_track_draw_wheel_front = 291.0F / (float)DD2_ROAD_UNITS_PER_METER;
static const float dd2_track_draw_wheel_rear = -232.0F / (float)DD2_ROAD_UNITS_PER_METER;
static const float dd2_track_draw_wheel_height = -130.0F / (float)DD2_ROAD_UNITS_PER_METER;
static const float dd2_track_draw_wheel_scale = 60.0F / 67.0F;
static const double dd2_track_draw_degrees = 57.2957795130823208768;
static const double dd2_track_draw_close_distance = 30;
static const double dd2_track_draw_medium_distance = 90;

struct dd2_track_draw {
    const dd2_track *track;
    dd2_mesh_materials *reference;
    dd2_model_texture_cache *textures;
    dd2_world_draw *world;
    dd2_model_draw *models[DD2_TRACK_DRAW_MODELS];
    dd2_bounds bounds[DD2_TRACK_DRAW_MODELS];
    dd2_track_draw_stats stats;
};

static const dd2_model *dd2_track_draw_source(const dd2_track *track, unsigned slot) {
    if (slot < DD2_TRACK_DRAW_BODIES) {
        return dd2_track_prepared_car(track, slot / DD2_TRACK_BODY_LODS,
                                      (dd2_track_model_kind)(slot % DD2_TRACK_BODY_LODS));
    }
    return dd2_track_prepared_model(track, slot == DD2_TRACK_DRAW_BODIES
                                               ? DD2_TRACK_MODEL_WHEEL_PRIMARY
                                               : DD2_TRACK_MODEL_WHEEL_SECONDARY);
}

void dd2_track_draw_destroy(dd2_track_draw *draw) {
    if (draw != NULL) {
        for (size_t index = 0; index < DD2_TRACK_DRAW_MODELS; ++index) {
            dd2_model_draw_destroy(draw->models[index]);
        }
        dd2_world_draw_destroy(draw->world);
        dd2_model_texture_cache_destroy(draw->textures);
        dd2_mesh_materials_destroy(draw->reference);
        free(draw);
    }
}

dd2_track_draw *dd2_track_draw_create(const dd2_track *track, dd2_model_image_loader loader,
                                      void *user) {
    if (track == NULL) {
        return NULL;
    }
    dd2_track_draw *draw = calloc(1, sizeof(*draw));
    if (draw == NULL) {
        return NULL;
    }
    draw->track = track;
    const dd2_world *world = dd2_track_world(track);
    if (world == NULL) {
        draw->reference =
            dd2_mesh_materials_create(dd2_track_level(track), dd2_track_textures(track));
        if (draw->reference == NULL) {
            dd2_track_draw_destroy(draw);
            return NULL;
        }
        return draw;
    }
    draw->textures = dd2_model_texture_cache_create(loader, user);
    draw->world = dd2_world_draw_create_shared(world, draw->textures);
    if (draw->world == NULL) {
        dd2_track_draw_destroy(draw);
        return NULL;
    }
    for (unsigned kind = 0; kind < DD2_TRACK_DRAW_MODELS; ++kind) {
        const dd2_model *model = dd2_track_draw_source(track, kind);
        const dd2_model_vertex *vertices = dd2_model_vertices(model);
        if (vertices == NULL) {
            dd2_track_draw_destroy(draw);
            return NULL;
        }
        for (size_t vertex = 0; vertex < dd2_model_vertex_count(model); ++vertex) {
            for (size_t axis = 0; axis < DD2_TRACK_DRAW_AXES; ++axis) {
                const float value = vertices[vertex].position[axis];
                if (vertex == 0 || value < draw->bounds[kind].minimum[axis]) {
                    draw->bounds[kind].minimum[axis] = value;
                }
                if (vertex == 0 || value > draw->bounds[kind].maximum[axis]) {
                    draw->bounds[kind].maximum[axis] = value;
                }
            }
        }
    }
    return draw;
}

dd2_track_draw_stats dd2_track_draw_statistics(const dd2_track_draw *draw) {
    return draw == NULL ? (dd2_track_draw_stats){0} : draw->stats;
}

bool dd2_track_draw_fit(const dd2_track *track, dd2_camera *camera, bool car) {
    if (dd2_track_world(track) != NULL) {
        return car ? dd2_camera_fit_model(camera,
                                          dd2_track_prepared_model(track, DD2_TRACK_MODEL_CLOSE))
                   : dd2_camera_fit_world(camera, dd2_track_world(track));
    }
    return car ? dd2_camera_fit_mesh(camera, dd2_track_car(track))
               : dd2_camera_fit_scene(camera, dd2_track_scene(track));
}

static bool dd2_track_draw_model(dd2_track_draw *draw, unsigned kind,
                                 dd2_model_draw_options options) {
    float projection[DD2_FRUSTUM_MATRIX] = {0};
    float modelview[DD2_FRUSTUM_MATRIX] = {0};
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
    const dd2_frustum frustum = dd2_frustum_create(projection, modelview);
    if (!dd2_frustum_visible(&frustum, &draw->bounds[kind])) {
        ++draw->stats.culled_models;
        return true;
    }
    dd2_model_draw **model = &draw->models[kind];
    if (*model == NULL) {
        *model =
            dd2_model_draw_create_shared(dd2_track_draw_source(draw->track, kind), draw->textures);
    }
    if (*model == NULL || !dd2_model_draw_frame(*model, options)) {
        return false;
    }
    const dd2_model_draw_stats stats = dd2_model_draw_statistics(*model);
    ++draw->stats.vehicle_models;
    draw->stats.vehicle_triangles += stats.triangles;
    draw->stats.vehicle_batches += stats.batches;
    if (kind < DD2_TRACK_DRAW_BODIES) {
        ++draw->stats.body_lods[kind % DD2_TRACK_BODY_LODS];
    }
    return true;
}

static void dd2_track_draw_pose(const dd2_vehicle *vehicle) {
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
                            (float)(vehicle->position.x / (double)DD2_ROAD_UNITS_PER_METER),
                            (float)(vehicle->position.y / (double)DD2_ROAD_UNITS_PER_METER),
                            (float)(vehicle->position.z / (double)DD2_ROAD_UNITS_PER_METER),
                            1};
    glMultMatrixf(matrix);
}

static bool dd2_track_draw_vehicle(dd2_track_draw *draw, dd2_driving_view view,
                                   dd2_vehicle_vector eye) {
    const dd2_vehicle_vector delta = {
        .x = eye.x - (view.vehicle->position.x / (double)DD2_ROAD_UNITS_PER_METER),
        .y = eye.y - (view.vehicle->position.y / (double)DD2_ROAD_UNITS_PER_METER),
        .z = eye.z - (view.vehicle->position.z / (double)DD2_ROAD_UNITS_PER_METER)};
    const double distance = sqrt((delta.x * delta.x) + (delta.y * delta.y) + (delta.z * delta.z));
    dd2_track_model_kind kind = DD2_TRACK_MODEL_CLOSE;
    if (view.driver != 0 && distance > dd2_track_draw_medium_distance) {
        kind = DD2_TRACK_MODEL_DISTANT;
    } else if (view.driver != 0 && distance > dd2_track_draw_close_distance) {
        kind = DD2_TRACK_MODEL_MEDIUM;
    }
    const dd2_vehicle_rotation inverse = {.x = -view.vehicle->rotation.x,
                                          .y = -view.vehicle->rotation.y,
                                          .z = -view.vehicle->rotation.z,
                                          .w = view.vehicle->rotation.w};
    const dd2_vehicle_vector local_eye = dd2_vehicle_rotate(inverse, delta);
    dd2_model_draw_options options = {.double_sided = true,
                                      .cutout_textures = true,
                                      .eye = {(float)local_eye.x,
                                              (float)local_eye.y - dd2_track_draw_body_height,
                                              (float)local_eye.z}};
    glPushMatrix();
    dd2_track_draw_pose(view.vehicle);
    glPushMatrix();
    glTranslatef(0, dd2_track_draw_body_height, 0);
    const unsigned livery = dd2_car_livery_index(view.driver, view.car_class);
    bool passed =
        dd2_track_draw_model(draw, (livery * DD2_TRACK_BODY_LODS) + (unsigned)kind, options);
    glPopMatrix();
    for (unsigned wheel = 0; wheel < DD2_VEHICLE_WHEELS && passed; ++wheel) {
        const bool front = (wheel & 1U) == 0;
        const float position[] = {
            wheel < 2 ? dd2_track_draw_wheel_width : -dd2_track_draw_wheel_width,
            dd2_track_draw_wheel_height +
                (float)(view.vehicle->wheels[wheel].compression / (double)DD2_ROAD_UNITS_PER_METER),
            front ? dd2_track_draw_wheel_front : dd2_track_draw_wheel_rear};
        /* Invert translation, steering, roll and scale for model-local sorting. */
        const double steer = front ? view.vehicle->steering : 0;
        const double eye_x = local_eye.x - (double)position[0];
        const double eye_y = local_eye.y - (double)position[1];
        const double eye_z = local_eye.z - (double)position[2];
        const double turned_x = (cos(steer) * eye_x) - (sin(steer) * eye_z);
        const double turned_z = (sin(steer) * eye_x) + (cos(steer) * eye_z);
        options.eye[0] = (float)(turned_x / (double)dd2_track_draw_wheel_scale);
        options.eye[1] =
            (float)(((cos(view.wheel_roll) * eye_y) + (sin(view.wheel_roll) * turned_z)) /
                    (double)dd2_track_draw_wheel_scale);
        options.eye[2] =
            (float)((-(sin(view.wheel_roll) * eye_y) + (cos(view.wheel_roll) * turned_z)) /
                    (double)dd2_track_draw_wheel_scale);
        glPushMatrix();
        glTranslatef(position[0], position[1], position[2]);
        if (front) {
            glRotatef((float)(view.vehicle->steering * dd2_track_draw_degrees), 0, 1, 0);
        }
        glRotatef((float)(view.wheel_roll * dd2_track_draw_degrees), 1, 0, 0);
        glScalef(dd2_track_draw_wheel_scale, dd2_track_draw_wheel_scale,
                 dd2_track_draw_wheel_scale);
        passed = dd2_track_draw_model(draw, DD2_TRACK_DRAW_BODIES + (wheel & 1U), options);
        glPopMatrix();
    }
    glPopMatrix();
    return passed;
}

bool dd2_track_draw_inspect(dd2_track_draw *draw, const dd2_camera *camera, bool car,
                            dd2_render_options viewport, dd2_car_class car_class) {
    const unsigned livery = dd2_car_livery_index(0, car_class);
    if (draw == NULL || camera == NULL || viewport.width <= 0 || viewport.height <= 0 ||
        livery >= DD2_CAR_LIVERIES) {
        return false;
    }
    draw->stats = (dd2_track_draw_stats){0};
    dd2_camera_apply(camera, viewport);
    if (draw->reference != NULL) {
        return car ? dd2_mesh_draw_car(draw->reference, dd2_track_car(draw->track),
                                       (dd2_track_vertex){0}, NULL,
                                       dd2_track_car_livery(draw->track, 0, car_class))
                   : dd2_scene_draw(draw->reference, dd2_track_scene(draw->track));
    }
    const dd2_model_draw_options options = {.double_sided = true, .cutout_textures = true};
    const bool passed = car ? dd2_track_draw_model(draw, livery * DD2_TRACK_BODY_LODS, options)
                            : dd2_world_draw_frame(draw->world, options, &draw->stats.world);
    draw->stats.uploaded_textures = dd2_model_texture_cache_count(draw->textures);
    return passed;
}

bool dd2_track_draw_driving(dd2_track_draw *draw, dd2_driving_view view) {
    if (draw == NULL || view.vehicle == NULL || !dd2_vehicle_valid(view.vehicle) ||
        view.viewport.width <= 0 || view.viewport.height <= 0 ||
        dd2_car_livery_index(view.driver, view.car_class) >= DD2_CAR_LIVERIES ||
        view.opponent_count >= DD2_VEHICLE_FLEET_LIMIT ||
        (view.opponent_count != 0 && (view.opponents == NULL || view.opponent_rolls == NULL))) {
        return false;
    }
    for (unsigned index = 0; index < view.opponent_count; ++index) {
        if (!dd2_vehicle_valid(&view.opponents[index])) {
            return false;
        }
    }
    draw->stats = (dd2_track_draw_stats){0};
    if (draw->reference != NULL) {
        return dd2_driving_draw(draw->reference, draw->track, view);
    }
    const dd2_vehicle_vector eye = dd2_driving_camera_apply(view, (double)DD2_ROAD_UNITS_PER_METER);
    bool passed = dd2_world_draw_frame(
                      draw->world,
                      (dd2_model_draw_options){.double_sided = true,
                                               .cutout_textures = true,
                                               .eye = {(float)eye.x, (float)eye.y, (float)eye.z}},
                      &draw->stats.world) &&
                  dd2_track_draw_vehicle(draw, view, eye);
    for (unsigned index = 0; index < view.opponent_count && passed; ++index) {
        passed =
            dd2_track_draw_vehicle(draw,
                                   (dd2_driving_view){.vehicle = &view.opponents[index],
                                                      .driver = index + 1,
                                                      .wheel_roll = view.opponent_rolls[index]},
                                   eye);
    }
    draw->stats.uploaded_textures = dd2_model_texture_cache_count(draw->textures);
    return passed && dd2_driving_overlay(view);
}
