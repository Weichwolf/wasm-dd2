#include "asset_fixture.h"
#include "assets/car_class.h"
#include "assets/road.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "surface_fixture.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_CLASS_TEST_SETTLE = 600,
    DD2_CLASS_TEST_DRIVE = 600,
    DD2_CLASS_TEST_HEIGHT = 190,
    DD2_CLASS_TEST_SLIDE = 10000
};
static const double dd2_class_test_stop = 0.1;
typedef struct {
    dd2_surface_test_fixture fixture;
    dd2_road *road;
    dd2_road_surface *surface;
} dd2_class_test_track;

static bool dd2_class_test_track_init(dd2_class_test_track *track) {
    dd2_surface_test_fixture_init(&track->fixture, 1);
    for (unsigned corner = 0; corner < DD2_SURFACE_TEST_VERTICES; ++corner) {
        uint8_t *vertex =
            track->fixture.vertices + ((size_t)corner * DD2_SURFACE_TEST_VERTEX_BYTES);
        const int32_t xpos =
            ((int32_t)(corner % DD2_SURFACE_TEST_ROW_VERTICES) - 1) * DD2_SURFACE_TEST_LONG_SIDE;
        const int32_t zpos = corner < DD2_SURFACE_TEST_ROW_VERTICES ? -DD2_SURFACE_TEST_LONG_SIDE
                                                                    : DD2_SURFACE_TEST_LONG_SIDE;
        dd2_test_write_le32(vertex, (uint32_t)xpos);
        dd2_test_write_le32(vertex + DD2_SURFACE_TEST_Z_OFFSET, (uint32_t)zpos);
    }
    track->road = dd2_road_create(&track->fixture.level, DD2_ROAD_RACING);
    track->surface = dd2_road_surface_create(track->road);
    return track->surface != NULL;
}

static bool dd2_class_test_run(dd2_vehicle *vehicle, const dd2_class_test_track *track,
                               dd2_car_class car_class, dd2_vehicle_control control,
                               unsigned steps) {
    for (unsigned step = 0; step < steps; ++step) {
        if (!dd2_vehicle_step_class(vehicle, track->road, track->surface, control, car_class)) {
            return false;
        }
    }
    return true;
}

static bool dd2_class_test_vector(dd2_vehicle_vector first, dd2_vehicle_vector second) {
    return first.x == second.x && first.y == second.y && first.z == second.z;
}

static bool dd2_class_test_unchanged(const dd2_vehicle *actual, const dd2_vehicle *before) {
    bool valid =
        dd2_class_test_vector(actual->position, before->position) &&
        dd2_class_test_vector(actual->velocity, before->velocity) &&
        dd2_class_test_vector(actual->angular_velocity, before->angular_velocity) &&
        actual->rotation.x == before->rotation.x && actual->rotation.y == before->rotation.y &&
        actual->rotation.z == before->rotation.z && actual->rotation.w == before->rotation.w &&
        actual->steering == before->steering && actual->steps == before->steps;
    for (unsigned index = 0; index < DD2_VEHICLE_WHEELS; ++index) {
        const dd2_vehicle_wheel *wheel = &actual->wheels[index];
        const dd2_vehicle_wheel *old = &before->wheels[index];
        valid = valid && dd2_class_test_vector(wheel->mount, old->mount) &&
                dd2_class_test_vector(wheel->center, old->center) &&
                dd2_class_test_vector(wheel->point, old->point) &&
                wheel->compression == old->compression && wheel->load == old->load &&
                wheel->support == old->support && wheel->body == old->body &&
                wheel->grounded == old->grounded && wheel->contact.cell == old->contact.cell &&
                wheel->contact.triangle == old->contact.triangle &&
                wheel->contact.height == old->contact.height;
        for (unsigned axis = 0; axis < 3; ++axis) {
            valid = valid && wheel->contact.normal[axis] == old->contact.normal[axis];
        }
    }
    return valid;
}

static bool dd2_class_test_invalid(const dd2_class_test_track *track, dd2_vehicle rest) {
    dd2_vehicle field[2] = {rest, rest};
    field[1].position.x = DD2_CLASS_TEST_SLIDE;
    dd2_vehicle before[2] = {field[0], field[1]};
    const dd2_vehicle_control controls[2] = {{.throttle = 1}, {.throttle = 1}};
    const dd2_car_class classes[2] = {DD2_CAR_PRO, DD2_CAR_CLASSES};
    const dd2_vehicle_field_step step = {.road = track->road,
                                         .surface = track->surface,
                                         .controls = controls,
                                         .classes = classes,
                                         .count = 2};
    return dd2_car_class_handling(DD2_CAR_CLASSES) == NULL &&
           !dd2_vehicle_step_class(&field[0], track->road, track->surface, controls[0],
                                   DD2_CAR_CLASSES) &&
           !dd2_vehicle_step_field(field, &step) &&
           dd2_class_test_unchanged(&field[0], &before[0]) &&
           dd2_class_test_unchanged(&field[1], &before[1]);
}

static bool dd2_class_test_motion(const dd2_class_test_track *track, dd2_vehicle rest,
                                  dd2_car_class car_class) {
    dd2_vehicle vehicle = rest;
    if (!dd2_class_test_run(&vehicle, track, car_class, (dd2_vehicle_control){.throttle = 1},
                            DD2_CLASS_TEST_DRIVE) ||
        vehicle.velocity.z <= 0 ||
        !dd2_class_test_run(&vehicle, track, car_class, (dd2_vehicle_control){.brake = 1},
                            DD2_CLASS_TEST_DRIVE) ||
        fabs(vehicle.velocity.z) > dd2_class_test_stop ||
        !dd2_class_test_run(&vehicle, track, car_class, (dd2_vehicle_control){.throttle = -1},
                            DD2_CLASS_TEST_DRIVE) ||
        vehicle.velocity.z >= 0) {
        return false;
    }
    const dd2_car_handling *handling = dd2_car_class_handling(car_class);
    return printf("{\"class\":%u,\"ratings\":[%u,%u,%u],\"traction\":%u,\"rear_grip\":%u,"
                  "\"coasting\":[%u,%u],\"negative_drive\":[%u,%u]}\n",
                  (unsigned)car_class, handling->ratings.acceleration, handling->ratings.top_speed,
                  handling->ratings.grip, handling->traction, handling->rear_grip,
                  handling->coasting.front, handling->coasting.rear, handling->negative_drive.front,
                  handling->negative_drive.rear) > 0;
}

static bool dd2_class_test_differences(const dd2_class_test_track *track, dd2_vehicle rest) {
    double acceleration[DD2_CAR_CLASSES] = {0};
    double slide[DD2_CAR_CLASSES] = {0};
    double coast[DD2_CAR_CLASSES] = {0};
    double reverse[DD2_CAR_CLASSES] = {0};
    for (unsigned index = 0; index < DD2_CAR_CLASSES; ++index) {
        const dd2_car_class car_class = (dd2_car_class)index;
        dd2_vehicle vehicle = rest;
        if (!dd2_class_test_run(&vehicle, track, car_class, (dd2_vehicle_control){.throttle = 1},
                                1)) {
            return false;
        }
        acceleration[index] = vehicle.velocity.z;
        vehicle = rest;
        vehicle.velocity.x = DD2_CLASS_TEST_SLIDE;
        if (!dd2_class_test_run(&vehicle, track, car_class, (dd2_vehicle_control){.throttle = 1},
                                1)) {
            return false;
        }
        slide[index] = vehicle.velocity.x;
        vehicle = rest;
        vehicle.velocity.x = 1;
        if (!dd2_class_test_run(&vehicle, track, car_class, (dd2_vehicle_control){0}, 1)) {
            return false;
        }
        coast[index] = vehicle.angular_velocity.y;
        vehicle = rest;
        vehicle.velocity.x = 1;
        if (!dd2_class_test_run(&vehicle, track, car_class, (dd2_vehicle_control){.throttle = -1},
                                1)) {
            return false;
        }
        reverse[index] = vehicle.angular_velocity.y;
    }
    return acceleration[0] < acceleration[1] && acceleration[1] < acceleration[2] &&
           slide[0] < slide[1] && slide[1] < slide[2] && coast[0] < coast[1] &&
           coast[1] < coast[2] && reverse[2] < reverse[1] && reverse[1] < reverse[0];
}

int main(void) {
    dd2_class_test_track track = {0};
    dd2_vehicle rest = {0};
    bool valid =
        dd2_class_test_track_init(&track) &&
        dd2_vehicle_reset(&rest, (dd2_vehicle_spawn){.position = {.y = DD2_CLASS_TEST_HEIGHT}}) &&
        dd2_class_test_run(&rest, &track, DD2_CAR_ROOKIE, (dd2_vehicle_control){0},
                           DD2_CLASS_TEST_SETTLE);
    valid =
        valid && dd2_class_test_invalid(&track, rest) && dd2_class_test_differences(&track, rest);
    for (unsigned index = 0; valid && index < DD2_CAR_CLASSES; ++index) {
        valid = dd2_class_test_motion(&track, rest, (dd2_car_class)index);
    }
    dd2_road_surface_destroy(track.surface);
    dd2_road_destroy(track.road);
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
