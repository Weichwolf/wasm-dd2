#include "render/camera.h"

#include "assets/bounds.h"
#include "assets/level.h"
#include "assets/mesh.h"
#include "assets/model.h"
#include "assets/scene.h"
#include "assets/world.h"
#include "render/renderer.h"

#include <GL/softgl.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>

static const float dd2_camera_initial_pitch = 55.0F;
static const float dd2_camera_initial_yaw = 30.0F;
static const float dd2_camera_padding = 1.2F;
static const float dd2_camera_turn_rate = 90.0F;
static const float dd2_camera_min_pitch = -85.0F;
static const float dd2_camera_max_pitch = 85.0F;
static const float dd2_camera_min_zoom = 0.05F;
static const float dd2_camera_max_zoom = 20.0F;
static const float dd2_camera_full_turn = 360.0F;

typedef struct {
    float min[3];
    float max[3];
    bool valid;
} dd2_camera_bounds;

static void dd2_camera_mesh_bounds(dd2_camera_bounds *bounds, const dd2_mesh *mesh,
                                   dd2_track_vertex origin) {
    const dd2_mesh_vector *vertices = dd2_mesh_vertices(mesh);
    for (size_t index = 0; index < dd2_mesh_vertex_count(mesh); ++index) {
        const float values[] = {(float)origin.x + (float)vertices[index].x,
                                (float)origin.y + (float)vertices[index].y,
                                (float)origin.z + (float)vertices[index].z};
        for (size_t axis = 0; axis < 3; ++axis) {
            if (!bounds->valid || values[axis] < bounds->min[axis]) {
                bounds->min[axis] = values[axis];
            }
            if (!bounds->valid || values[axis] > bounds->max[axis]) {
                bounds->max[axis] = values[axis];
            }
        }
        bounds->valid = true;
    }
}

static bool dd2_camera_fit(dd2_camera *camera, dd2_camera_bounds bounds) {
    if (camera == NULL || !bounds.valid) {
        return false;
    }
    *camera = (dd2_camera){
        .radius = 1, .pitch = dd2_camera_initial_pitch, .yaw = dd2_camera_initial_yaw, .zoom = 1};
    for (size_t axis = 0; axis < 3; ++axis) {
        const float span = bounds.max[axis] - bounds.min[axis];
        if (span > camera->radius) {
            camera->radius = span;
        }
        camera->center[axis] = (bounds.min[axis] + bounds.max[axis]) / 2;
    }
    camera->radius *= dd2_camera_padding / 2;
    return true;
}

bool dd2_camera_fit_mesh(dd2_camera *camera, const dd2_mesh *mesh) {
    dd2_camera_bounds bounds = {0};
    dd2_camera_mesh_bounds(&bounds, mesh, (dd2_track_vertex){0});
    return dd2_camera_fit(camera, bounds);
}

bool dd2_camera_fit_scene(dd2_camera *camera, const dd2_scene *scene) {
    dd2_camera_bounds bounds = {0};
    const dd2_scene_object *objects = dd2_scene_objects(scene);
    for (size_t index = 0; index < dd2_scene_object_count(scene); ++index) {
        dd2_camera_mesh_bounds(&bounds, objects[index].mesh, objects[index].origin);
    }
    return dd2_camera_fit(camera, bounds);
}

bool dd2_camera_fit_model(dd2_camera *camera, const dd2_model *model) {
    dd2_camera_bounds bounds = {0};
    const dd2_model_vertex *vertices = dd2_model_vertices(model);
    for (size_t index = 0; index < dd2_model_vertex_count(model); ++index) {
        for (size_t axis = 0; axis < 3; ++axis) {
            const float value = vertices[index].position[axis];
            if (!bounds.valid || value < bounds.min[axis]) {
                bounds.min[axis] = value;
            }
            if (!bounds.valid || value > bounds.max[axis]) {
                bounds.max[axis] = value;
            }
        }
        bounds.valid = true;
    }
    return dd2_camera_fit(camera, bounds);
}

bool dd2_camera_fit_world(dd2_camera *camera, const dd2_world *world) {
    dd2_camera_bounds bounds = {0};
    const dd2_world_instance *instances = dd2_world_instances(world);
    const dd2_world_resource *resources = dd2_world_resources(world);
    for (size_t index = 0; index < dd2_world_instance_count(world); ++index) {
        const dd2_world_instance *instance = &instances[index];
        const dd2_bounds local = resources[instance->resource].bounds;
        for (size_t axis = 0; axis < 3; ++axis) {
            const float low = local.minimum[axis] + instance->position[axis];
            const float high = local.maximum[axis] + instance->position[axis];
            if (!bounds.valid || low < bounds.min[axis]) {
                bounds.min[axis] = low;
            }
            if (!bounds.valid || high > bounds.max[axis]) {
                bounds.max[axis] = high;
            }
        }
        bounds.valid = true;
    }
    return dd2_camera_fit(camera, bounds);
}

bool dd2_camera_step(dd2_camera *camera, dd2_camera_motion motion, float seconds) {
    if (camera == NULL || seconds <= 0 || seconds > 1 || camera->radius <= 0) {
        return false;
    }
    if (motion.yaw == 0 && motion.pitch == 0 && motion.zoom == 0 && motion.pan_x == 0 &&
        motion.pan_y == 0) {
        return false;
    }
    camera->yaw =
        fmodf(camera->yaw + (motion.yaw * seconds * dd2_camera_turn_rate), dd2_camera_full_turn);
    camera->pitch = fminf(dd2_camera_max_pitch,
                          fmaxf(dd2_camera_min_pitch,
                                camera->pitch + (motion.pitch * seconds * dd2_camera_turn_rate)));
    camera->zoom = fminf(dd2_camera_max_zoom,
                         fmaxf(dd2_camera_min_zoom, camera->zoom * expf(motion.zoom * seconds)));
    const float distance = camera->radius * camera->zoom * seconds;
    camera->pan_x += motion.pan_x * distance;
    camera->pan_y += motion.pan_y * distance;
    return true;
}

void dd2_camera_apply(const dd2_camera *camera, dd2_render_options viewport) {
    if (camera == NULL || viewport.width <= 0 || viewport.height <= 0 || camera->radius <= 0) {
        return;
    }
    const float radius = camera->radius * camera->zoom;
    const float aspect = (float)viewport.width / (float)viewport.height;
    glViewport(0, 0, viewport.width, viewport.height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-radius * aspect, radius * aspect, -radius, radius, -camera->radius * 4,
            camera->radius * 4);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(camera->pan_x, camera->pan_y, 0);
    glRotatef(camera->pitch, 1, 0, 0);
    glRotatef(camera->yaw, 0, 1, 0);
    glTranslatef(-camera->center[0], -camera->center[1], -camera->center[2]);
}
