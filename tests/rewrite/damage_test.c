#include "physics/damage.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { DD2_DAMAGE_TEST_BODIES = 2, DD2_DAMAGE_TEST_SUPPORT_STEPS = 1000 };
static const double dd2_damage_test_tolerance = 1e-10;
static const double dd2_damage_test_speed = 6000;
static const double dd2_damage_test_impulse = 3250;
static const double dd2_damage_test_width = 186;
static const double dd2_damage_test_length = 450;
static const double dd2_damage_test_height = 100;
static const double dd2_damage_test_crushed_width = 148.8;
static const double dd2_damage_test_crushed_height = 65;
static const double dd2_damage_test_crushed_length = 247.5;

static dd2_vehicle_collision_report dd2_damage_test_report(void) {
    dd2_vehicle_collision_report report = {.count = 1};
    report.contacts[0] = (dd2_vehicle_contact){
        .first = 0,
        .second = 1,
        .kind = DD2_VEHICLE_CONTACT_PAIR,
        .normal = {.z = -1},
        .normal_speed = dd2_damage_test_speed,
        .impulse = dd2_damage_test_impulse,
        .local_points = {{.x = -dd2_damage_test_width, .z = dd2_damage_test_length},
                         {.x = dd2_damage_test_width, .z = -dd2_damage_test_length}}};
    return report;
}

static bool dd2_damage_test_impacts(void) {
    dd2_vehicle_damage damage[DD2_DAMAGE_TEST_BODIES] = {0};
    dd2_vehicle_collision_report report = dd2_damage_test_report();
    /* Repeated solver contacts must not double the regional load of this step. */
    report.count = 2;
    report.contacts[1] = report.contacts[0];
    const dd2_damage_frame frame = {.contacts = &report, .count = DD2_DAMAGE_TEST_BODIES};
    if (!dd2_damage_step(damage, frame) ||
        fabs(damage[0].regions[DD2_DAMAGE_FRONT_LEFT] - (1.0 / 2)) > dd2_damage_test_tolerance ||
        fabs(damage[1].regions[DD2_DAMAGE_REAR_RIGHT] - (1.0 / 2)) > dd2_damage_test_tolerance ||
        dd2_damage_health(&damage[0]) != (1.0 / 2) || dd2_damage_health(&damage[1]) != 1 ||
        damage[0].retired || damage[1].retired || damage[0].steps != 1) {
        printf("First crush %.17g rear %.17g health %.17g steps=%llu\n", damage[0].regions[0],
               damage[1].regions[DD2_DAMAGE_REAR_RIGHT], dd2_damage_health(&damage[0]),
               (unsigned long long)damage[0].steps);
        return false;
    }
    const dd2_vehicle_control slowed =
        dd2_damage_control(&damage[0], (dd2_vehicle_control){.throttle = 1});
    const double expected_power = 0.8;
    if (fabs(slowed.throttle - expected_power) > dd2_damage_test_tolerance ||
        !dd2_damage_step(damage, frame) || !damage[0].retired || damage[1].retired ||
        dd2_damage_health(&damage[0]) != 0 || damage[1].regions[DD2_DAMAGE_REAR_RIGHT] != 1) {
        printf("Second crush %.17g rear %.17g power %.17g retired=%d\n", damage[0].regions[0],
               damage[1].regions[DD2_DAMAGE_REAR_RIGHT], slowed.throttle, (int)damage[0].retired);
        return false;
    }
    const dd2_vehicle_control stopped =
        dd2_damage_control(&damage[0], (dd2_vehicle_control){.throttle = 1, .steer = 1});
    return stopped.throttle == 0 && stopped.brake == 1 && stopped.steer == 0;
}

static bool dd2_damage_test_support(void) {
    dd2_vehicle_damage damage[DD2_DAMAGE_TEST_BODIES] = {0};
    dd2_vehicle_collision_report report = dd2_damage_test_report();
    report.contacts[0].normal_speed = 0;
    report.contacts[0].impulse = 0;
    for (unsigned step = 0; step < DD2_DAMAGE_TEST_SUPPORT_STEPS; ++step) {
        if (!dd2_damage_step(
                damage, (dd2_damage_frame){.contacts = &report, .count = DD2_DAMAGE_TEST_BODIES}) ||
            dd2_damage_health(&damage[0]) != 1 || dd2_damage_health(&damage[1]) != 1) {
            return false;
        }
    }
    return damage[0].steps == DD2_DAMAGE_TEST_SUPPORT_STEPS &&
           damage[1].steps == DD2_DAMAGE_TEST_SUPPORT_STEPS;
}

static bool dd2_damage_test_invalid(void) {
    dd2_vehicle_damage damage[DD2_DAMAGE_TEST_BODIES] = {0};
    dd2_vehicle_collision_report report = dd2_damage_test_report();
    report.count = 2;
    report.contacts[1] = report.contacts[0];
    const union {
        uint64_t bits;
        double number;
    } invalid = {.bits = UINT64_C(0x7ff8000000000001)};
    report.contacts[1].impulse = invalid.number;
    if (dd2_damage_step(damage,
                        (dd2_damage_frame){.contacts = &report, .count = DD2_DAMAGE_TEST_BODIES}) ||
        damage[0].steps != 0 || damage[0].regions[DD2_DAMAGE_FRONT_LEFT] != 0) {
        return false;
    }
    report.count = 1;
    damage[1].steps = UINT64_MAX;
    return !dd2_damage_step(
               damage, (dd2_damage_frame){.contacts = &report, .count = DD2_DAMAGE_TEST_BODIES}) &&
           damage[0].steps == 0 && damage[0].regions[DD2_DAMAGE_FRONT_LEFT] == 0 &&
           !dd2_damage_step(damage, (dd2_damage_frame){.contacts = &report, .count = 0});
}

static bool dd2_damage_test_shape(void) {
    dd2_vehicle_damage damage = {0};
    const dd2_vehicle_vector point = {
        .x = -dd2_damage_test_width, .y = dd2_damage_test_height, .z = dd2_damage_test_length};
    const dd2_vehicle_vector intact = dd2_damage_deform(&damage, point);
    if (intact.x != point.x || intact.y != point.y || intact.z != point.z) {
        return false;
    }
    damage.regions[DD2_DAMAGE_FRONT_LEFT] = 1;
    const dd2_vehicle_vector crushed = dd2_damage_deform(&damage, point);
    const dd2_vehicle_vector rear = {.x = point.x, .y = point.y, .z = -point.z};
    const dd2_vehicle_vector unchanged = dd2_damage_deform(&damage, rear);
    return fabs(crushed.x + dd2_damage_test_crushed_width) < dd2_damage_test_tolerance &&
           fabs(crushed.y - dd2_damage_test_crushed_height) < dd2_damage_test_tolerance &&
           fabs(crushed.z - dd2_damage_test_crushed_length) < dd2_damage_test_tolerance &&
           unchanged.x == rear.x && unchanged.y == rear.y && unchanged.z == rear.z;
}

int main(void) {
    const bool impacts = dd2_damage_test_impacts();
    const bool support = dd2_damage_test_support();
    const bool invalid = dd2_damage_test_invalid();
    const bool shape = dd2_damage_test_shape();
    const bool pass = impacts && support && invalid && shape;
    if (!pass) {
        printf("Damage checks impacts=%d support=%d invalid=%d shape=%d\n", (int)impacts,
               (int)support, (int)invalid, (int)shape);
    }
    puts(pass ? "regional damage: PASS" : "regional damage: FAIL");
    return pass ? EXIT_SUCCESS : EXIT_FAILURE;
}
