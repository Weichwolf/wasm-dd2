#include "asset_fixture.h"
#include "assets/road.h"
#include "physics/body_surface.h"
#include "physics/road_contact.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"
#include "surface_fixture.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum {
    DD2_BODY_TEST_HALF_WIDTH = 186,
    DD2_BODY_TEST_HALF_HEIGHT = 130,
    DD2_BODY_TEST_HALF_LENGTH = 450,
    DD2_BODY_TEST_ELEVATED = 10000,
    DD2_BODY_TEST_GAP = 310,
    DD2_BODY_TEST_ESCAPE_STEPS = 1200,
    DD2_BODY_TEST_QUERY_X = 10,
    DD2_BODY_TEST_QUERY_Z = 20,
    DD2_BODY_TEST_QUERY_TOP = 200,
    DD2_BODY_TEST_VELOCITY_Z = 5,
    DD2_BODY_TEST_SPEED_X = 100,
    DD2_BODY_TEST_SPEED_Z = 200,
    DD2_BODY_TEST_GRAVITY = 2500,
    DD2_BODY_TEST_SPRING = 90,
    DD2_BODY_TEST_COMPRESSION = 10,
    DD2_BODY_TEST_RIDE = 190
};
enum { DD2_BODY_TEST_RADIUS = 60, DD2_BODY_TEST_TRAVEL = 70, DD2_BODY_TEST_BUMP = 15 };
static const double dd2_body_test_tie_spacing = 0.75;
static const double dd2_body_test_drag = 0.1;
static const double dd2_body_test_tolerance = 1e-9;
static const double dd2_body_test_eighth_turn = 0.78539816339744830962;

static void dd2_body_test_nonfinite(double *number, uint64_t representation) {
    unsigned char *target = (unsigned char *)number;
    const unsigned char *source = (const unsigned char *)&representation;
    for (size_t byte = 0; byte < sizeof(representation); ++byte) {
        target[byte] = source[byte];
    }
}

static bool dd2_body_test_near(double first, double second) {
    return fabs(first - second) <= dd2_body_test_tolerance;
}

static bool dd2_body_test_geometry(void) {
    dd2_vehicle vehicles[4] = {0};
    for (unsigned body = 0; body < 4; ++body) {
        if (!dd2_vehicle_reset(
                &vehicles[body],
                (dd2_vehicle_spawn){.position = {.x = body == 3 ? DD2_BODY_TEST_ELEVATED : 0}})) {
            return false;
        }
    }
    vehicles[0].velocity = (dd2_vehicle_vector){.x = 3, .y = 4, .z = DD2_BODY_TEST_VELOCITY_Z};
    vehicles[0].angular_velocity.y = 2;
    dd2_body_surface field = {0};
    dd2_body_surface_contact hit = {0};
    dd2_surface_query query = {.point = {.x = DD2_BODY_TEST_QUERY_X, .z = DD2_BODY_TEST_QUERY_Z},
                               .min_height = 0,
                               .max_height = DD2_BODY_TEST_QUERY_TOP};
    bool valid = dd2_body_surface_prepare(vehicles, 4, &field) &&
                 dd2_body_surface_sample(&field, &query, 3, &hit) && hit.body == 0 &&
                 hit.point.y == (double)DD2_BODY_TEST_HALF_HEIGHT && hit.normal.y == 1 &&
                 hit.velocity.x == 3 + (2 * DD2_BODY_TEST_QUERY_Z) && hit.velocity.y == 4 &&
                 hit.velocity.z == DD2_BODY_TEST_VELOCITY_Z - (2 * DD2_BODY_TEST_QUERY_X);
    query.point.x = DD2_BODY_TEST_HALF_WIDTH;
    valid = valid && dd2_body_surface_sample(&field, &query, 3, &hit);
    ++query.point.x;
    valid = valid && !dd2_body_surface_sample(&field, &query, 3, &hit);
    query.point = (dd2_road_point){.z = DD2_BODY_TEST_HALF_LENGTH + 1};
    valid = valid && !dd2_body_surface_sample(&field, &query, 3, &hit);
    query.point = (dd2_road_point){0};
    query.max_height = DD2_BODY_TEST_HALF_HEIGHT - 1;
    valid = valid && !dd2_body_surface_sample(&field, &query, 3, &hit);
    query.max_height = DD2_BODY_TEST_HALF_HEIGHT;
    query.min_height = DD2_BODY_TEST_HALF_HEIGHT;
    valid = valid && dd2_body_surface_sample(&field, &query, 3, &hit);
    query.min_height = 0;
    query.max_height = DD2_BODY_TEST_QUERY_TOP;
    vehicles[1].position.y = DD2_ROAD_EDGE_TOLERANCE * dd2_body_test_tie_spacing;
    vehicles[2].position.y = DD2_ROAD_EDGE_TOLERANCE * (2 * dd2_body_test_tie_spacing);
    valid = valid && dd2_body_surface_prepare(vehicles, 4, &field) &&
            dd2_body_surface_sample(&field, &query, 3, &hit) && hit.body == 1;
    vehicles[0].rotation = (dd2_vehicle_rotation){.z = sin(dd2_body_test_eighth_turn / 2),
                                                  .w = cos(dd2_body_test_eighth_turn / 2)};
    vehicles[1].position.x = DD2_BODY_TEST_ELEVATED;
    vehicles[2].position.x = DD2_BODY_TEST_ELEVATED;
    valid = valid && dd2_body_surface_prepare(vehicles, 4, &field) &&
            dd2_body_surface_sample(&field, &query, 3, &hit) && hit.body == 0 &&
            dd2_body_test_near(hit.point.y, (double)DD2_BODY_TEST_HALF_HEIGHT /
                                                cos(dd2_body_test_eighth_turn)) &&
            dd2_body_test_near(hit.normal.x, -sin(dd2_body_test_eighth_turn)) &&
            dd2_body_test_near(hit.normal.y, cos(dd2_body_test_eighth_turn));
    valid = valid && !dd2_body_surface_sample(&field, &query, 0, &hit);
    dd2_body_test_nonfinite(&query.point.x, UINT64_C(0x7ff8000000000001));
    valid = valid && !dd2_body_surface_sample(&field, &query, 3, &hit);
    dd2_body_test_nonfinite(&vehicles[3].velocity.x, UINT64_C(0x7ff0000000000000));
    valid = valid && !dd2_body_surface_prepare(vehicles, 4, &field) && field.count == 0;
    return valid;
}

static bool dd2_body_test_forces(const dd2_road *road, const dd2_road_surface *surface) {
    dd2_vehicle vehicles[2] = {0};
    for (unsigned body = 0; body < 2; ++body) {
        if (!dd2_vehicle_reset(
                &vehicles[body],
                (dd2_vehicle_spawn){.position = {.y = (double)DD2_BODY_TEST_ELEVATED +
                                                      (body * DD2_BODY_TEST_GAP)}})) {
            return false;
        }
        vehicles[body].velocity =
            (dd2_vehicle_vector){.x = DD2_BODY_TEST_SPEED_X, .z = DD2_BODY_TEST_SPEED_Z};
    }
    dd2_vehicle initial[2];
    for (unsigned body = 0; body < 2; ++body) {
        initial[body] = vehicles[body];
    }
    dd2_vehicle_control controls[2] = {0};
    dd2_vehicle_field_step step = {
        .road = road, .surface = surface, .controls = controls, .count = 2};
    const double load = DD2_BODY_TEST_SPRING * DD2_BODY_TEST_COMPRESSION;
    const double normal_force = (double)DD2_VEHICLE_WHEELS * load;
    const double retained_speed = 1 - (dd2_body_test_drag * DD2_VEHICLE_STEP_SECONDS);
    bool valid =
        dd2_vehicle_step_field(vehicles, &step) &&
        dd2_body_test_near(vehicles[0].velocity.y,
                           (-DD2_BODY_TEST_GRAVITY - normal_force) * DD2_VEHICLE_STEP_SECONDS) &&
        dd2_body_test_near(vehicles[1].velocity.y,
                           (-DD2_BODY_TEST_GRAVITY + normal_force) * DD2_VEHICLE_STEP_SECONDS);
    for (unsigned wheel = 0; wheel < DD2_VEHICLE_WHEELS; ++wheel) {
        valid = valid && vehicles[1].wheels[wheel].support == DD2_VEHICLE_WHEEL_BODY &&
                vehicles[1].wheels[wheel].body == 0 && vehicles[1].wheels[wheel].grounded &&
                vehicles[1].wheels[wheel].contact.cell == DD2_ROAD_NO_STRIP &&
                dd2_body_test_near(vehicles[1].wheels[wheel].load, load) &&
                !vehicles[0].wheels[wheel].grounded;
    }
    for (unsigned body = 0; body < 2; ++body) {
        valid = valid &&
                dd2_body_test_near(vehicles[body].velocity.x,
                                   (double)DD2_BODY_TEST_SPEED_X * retained_speed) &&
                dd2_body_test_near(vehicles[body].velocity.z,
                                   (double)DD2_BODY_TEST_SPEED_Z * retained_speed) &&
                vehicles[body].steps == 1;
    }
    controls[1].throttle = 1;
    for (unsigned body = 0; body < 2; ++body) {
        vehicles[body] = initial[body];
    }
    valid = valid && dd2_vehicle_step_field(vehicles, &step) &&
            dd2_body_test_near(vehicles[0].velocity.z + vehicles[1].velocity.z,
                               2 * (double)DD2_BODY_TEST_SPEED_Z * retained_speed) &&
            vehicles[1].velocity.z > initial[1].velocity.z &&
            vehicles[0].velocity.z < initial[0].velocity.z;
    const dd2_vehicle expected[2] = {vehicles[0], vehicles[1]};
    vehicles[0] = initial[1];
    vehicles[1] = initial[0];
    controls[0] = controls[1];
    controls[1] = (dd2_vehicle_control){0};
    valid = valid && dd2_vehicle_step_field(vehicles, &step) &&
            dd2_body_test_near(vehicles[0].velocity.z, expected[1].velocity.z) &&
            dd2_body_test_near(vehicles[1].velocity.z, expected[0].velocity.z) &&
            dd2_body_test_near(vehicles[0].angular_velocity.x, expected[1].angular_velocity.x);
    unsigned char before[sizeof(vehicles)];
    const unsigned char *bytes = (const unsigned char *)vehicles;
    for (size_t index = 0; index < sizeof(before); ++index) {
        before[index] = bytes[index];
    }
    dd2_body_test_nonfinite(&controls[1].throttle, UINT64_C(0x7ff8000000000001));
    valid = valid && !dd2_vehicle_step_field(vehicles, &step);
    for (size_t index = 0; index < sizeof(before); ++index) {
        valid = valid && before[index] == bytes[index];
    }
    return valid;
}

static bool dd2_body_test_mixed_ties(const dd2_road *road, const dd2_road_surface *surface) {
    dd2_vehicle vehicles[3] = {0};
    for (unsigned body = 0; body < 3; ++body) {
        const double height =
            body == 2
                ? (double)DD2_BODY_TEST_RIDE
                : -(double)DD2_BODY_TEST_HALF_HEIGHT +
                      ((double)(body + 1) * dd2_body_test_tie_spacing * DD2_ROAD_EDGE_TOLERANCE);
        if (!dd2_vehicle_reset(&vehicles[body], (dd2_vehicle_spawn){.position = {.y = height}})) {
            return false;
        }
    }
    const dd2_vehicle_control controls[3] = {0};
    if (!dd2_vehicle_step_field(
            vehicles, &(dd2_vehicle_field_step){
                          .road = road, .surface = surface, .controls = controls, .count = 3})) {
        return false;
    }
    for (unsigned wheel = 0; wheel < DD2_VEHICLE_WHEELS; ++wheel) {
        if (vehicles[2].wheels[wheel].support != DD2_VEHICLE_WHEEL_BODY ||
            vehicles[2].wheels[wheel].body != 0) {
            return false;
        }
    }
    return true;
}

static bool dd2_body_test_nose_contact(void) {
    /* Explicit core geometry from the matched WASM B pre-overturn observation.
     * Body 16's rear tire samples body 17's steep nose vertically, but reaching
     * that face would require displacement -264.224 along its suspension. */
    static const dd2_vehicle snapshot[2] = {
        {.position = {-12335.587504844556, 398.6875836924443, 6988.853028756409},
         .rotation = {-0.024352795362671903, 0.1911559803489319, -0.0654686915763054,
                      0.9790710816677966}},
        {.position = {-12680.282907298306, 287.70120847725565, 6143.482248399294},
         .rotation = {-0.11026663427013762, 0.19651907886709527, -0.03657688113950241,
                      0.9735931659446214}}};
    static const dd2_body_wheel_query wheel_query = {
        .mount = {-12341.782931680438, 303.5011706041003, 6507.607599219986},
        .axis = {0.11888664041040044, 0.9902415835626208, -0.0727156992355924},
        .radius = (double)DD2_BODY_TEST_RADIUS,
        .min_displacement = -(double)DD2_BODY_TEST_BUMP,
        .max_displacement = (double)DD2_BODY_TEST_TRAVEL};
    const dd2_surface_query vertical_query = {
        .point = {.x = wheel_query.mount.x - ((double)DD2_BODY_TEST_TRAVEL * wheel_query.axis.x),
                  .z = wheel_query.mount.z - ((double)DD2_BODY_TEST_TRAVEL * wheel_query.axis.z)},
        .min_height = wheel_query.mount.y - ((double)DD2_BODY_TEST_TRAVEL * wheel_query.axis.y) -
                      (double)DD2_BODY_TEST_RADIUS,
        .max_height =
            wheel_query.mount.y - (double)DD2_BODY_TEST_RADIUS + (double)DD2_BODY_TEST_BUMP};
    dd2_body_surface field = {0};
    dd2_body_surface_contact hit = {0};
    return dd2_body_surface_prepare(snapshot, 2, &field) &&
           dd2_body_surface_sample(&field, &vertical_query, 0, &hit) && hit.body == 1 &&
           !dd2_body_surface_wheel_sample(&field, &wheel_query, 0, &hit);
}

static bool dd2_body_test_banked_struts(const dd2_road *road, const dd2_road_surface *surface) {
    dd2_vehicle vehicles[2] = {0};
    const double root = sqrt(1.0 / 2);
    for (unsigned body = 0; body < 2; ++body) {
        if (!dd2_vehicle_reset(
                &vehicles[body],
                (dd2_vehicle_spawn){
                    .position = {.x = -(double)(body * DD2_BODY_TEST_GAP) * root,
                                 .y = (double)DD2_BODY_TEST_ELEVATED +
                                      ((double)(body * DD2_BODY_TEST_GAP) * root)}})) {
            return false;
        }
        vehicles[body].rotation = (dd2_vehicle_rotation){.z = sin(dd2_body_test_eighth_turn / 2),
                                                         .w = cos(dd2_body_test_eighth_turn / 2)};
    }
    const dd2_vehicle_control controls[2] = {0};
    bool valid = dd2_vehicle_step_field(
        vehicles, &(dd2_vehicle_field_step){
                      .road = road, .surface = surface, .controls = controls, .count = 2});
    for (unsigned wheel = 0; wheel < DD2_VEHICLE_WHEELS; ++wheel) {
        const dd2_vehicle_wheel *hit = &vehicles[1].wheels[wheel];
        valid = valid && hit->support == DD2_VEHICLE_WHEEL_BODY && hit->body == 0 &&
                dd2_body_test_near(hit->compression, DD2_BODY_TEST_COMPRESSION) &&
                dd2_body_test_near(hit->load, DD2_BODY_TEST_SPRING * DD2_BODY_TEST_COMPRESSION) &&
                dd2_body_test_near(hit->center.x - hit->point.x,
                                   (double)DD2_BODY_TEST_RADIUS * hit->contact.normal[0]) &&
                dd2_body_test_near(hit->center.y - hit->point.y,
                                   (double)DD2_BODY_TEST_RADIUS * hit->contact.normal[1]) &&
                dd2_body_test_near(hit->center.z - hit->point.z,
                                   (double)DD2_BODY_TEST_RADIUS * hit->contact.normal[2]);
    }
    const double load = DD2_BODY_TEST_SPRING * DD2_BODY_TEST_COMPRESSION;
    const double force = (double)DD2_VEHICLE_WHEELS * load * root;
    return valid && dd2_body_test_near(vehicles[1].velocity.x, -force * DD2_VEHICLE_STEP_SECONDS) &&
           dd2_body_test_near(vehicles[0].velocity.x, force * DD2_VEHICLE_STEP_SECONDS) &&
           dd2_body_test_near(vehicles[1].velocity.y,
                              (-DD2_BODY_TEST_GRAVITY + force) * DD2_VEHICLE_STEP_SECONDS) &&
           dd2_body_test_near(vehicles[0].velocity.y,
                              (-DD2_BODY_TEST_GRAVITY - force) * DD2_VEHICLE_STEP_SECONDS);
}

static bool dd2_body_test_escape(const dd2_road *road, const dd2_road_surface *surface) {
    dd2_vehicle vehicles[2] = {0};
    const double height =
        (double)DD2_BODY_TEST_RIDE - ((double)DD2_BODY_TEST_GRAVITY / (4 * DD2_BODY_TEST_SPRING));
    bool valid = dd2_vehicle_reset(&vehicles[0], (dd2_vehicle_spawn){.position = {.y = height}}) &&
                 dd2_vehicle_reset(
                     &vehicles[1],
                     (dd2_vehicle_spawn){
                         .position = {.y = height + (double)DD2_BODY_TEST_HALF_HEIGHT + height}});
    const dd2_vehicle_control controls[2] = {{.brake = 1}, {.throttle = 1}};
    unsigned supported = 0;
    for (unsigned tick = 0; valid && tick < DD2_BODY_TEST_ESCAPE_STEPS; ++tick) {
        dd2_vehicle previous[2] = {vehicles[0], vehicles[1]};
        valid = dd2_vehicle_step_field(vehicles, &(dd2_vehicle_field_step){.road = road,
                                                                           .surface = surface,
                                                                           .controls = controls,
                                                                           .count = 2}) &&
                dd2_vehicle_collide_fleet(vehicles, previous, 2, surface, NULL, NULL, NULL);
        for (unsigned wheel = 0; wheel < DD2_VEHICLE_WHEELS; ++wheel) {
            supported += (unsigned)(vehicles[1].wheels[wheel].support == DD2_VEHICLE_WHEEL_ROAD);
        }
    }
    printf("Body-supported drive: valid=%d z=%.17g road_wheel_steps=%u\n", (int)valid,
           vehicles[1].position.z, supported);
    return valid && vehicles[1].position.z > (double)DD2_SURFACE_TEST_SIDE &&
           supported > DD2_VEHICLE_WHEELS;
}

int main(void) {
    dd2_surface_test_fixture fixture;
    dd2_surface_test_fixture_init(&fixture, 1);
    for (unsigned vertex = 0; vertex < DD2_SURFACE_TEST_VERTICES; ++vertex) {
        uint8_t *record = fixture.vertices + ((size_t)vertex * DD2_SURFACE_TEST_VERTEX_BYTES);
        const int32_t xpos =
            ((int32_t)(vertex % DD2_SURFACE_TEST_ROW_VERTICES) - 1) * DD2_SURFACE_TEST_LONG_SIDE;
        const int32_t zpos = vertex < DD2_SURFACE_TEST_ROW_VERTICES ? -DD2_SURFACE_TEST_LONG_SIDE
                                                                    : DD2_SURFACE_TEST_LONG_SIDE;
        dd2_test_write_le32(record, (uint32_t)xpos);
        dd2_test_write_le32(record + DD2_SURFACE_TEST_Z_OFFSET, (uint32_t)zpos);
    }
    dd2_road *road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
    dd2_road_surface *surface = dd2_road_surface_create(road);
    const bool geometry = dd2_body_test_geometry();
    const bool forces = road != NULL && surface != NULL && dd2_body_test_forces(road, surface);
    const bool mixed = road != NULL && surface != NULL && dd2_body_test_mixed_ties(road, surface);
    const bool banked =
        road != NULL && surface != NULL && dd2_body_test_banked_struts(road, surface);
    const bool nose = dd2_body_test_nose_contact();
    const bool struts = banked && nose;
    const bool escape = road != NULL && surface != NULL && dd2_body_test_escape(road, surface);
    dd2_road_surface_destroy(surface);
    dd2_road_destroy(road);
    printf("Body wheel support: geometry=%d forces=%d escape=%d\n", (int)geometry, (int)forces,
           (int)escape);
    printf("Mixed road/body global ties: valid=%d\n", (int)mixed);
    printf("Finite body suspension geometry: valid=%d banked=%d nose=%d\n", (int)struts,
           (int)banked, (int)nose);
    return geometry && forces && mixed && struts && escape ? 0 : 1;
}
