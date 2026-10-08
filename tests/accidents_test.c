#include "game/accidents.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_ACCIDENT_TEST_CARS = 3,
    DD2_ACCIDENT_TEST_CAP_SPINS = 20,
    DD2_ACCIDENT_TEST_QUARTER_POINTS = 10,
    DD2_ACCIDENT_TEST_HALF_POINTS = 25,
    DD2_ACCIDENT_TEST_FULL_POINTS = 50,
    DD2_ACCIDENT_TEST_WRAP_DEGREES = 179
};
static const double dd2_accident_test_quarter = 1.57079632679489661923;
static const double dd2_accident_test_speed = 6000;
static const double dd2_accident_test_impulse = 3250;
static const double dd2_accident_test_degrees = 0.01745329251994329577;

typedef struct {
    dd2_accident_driver drivers[DD2_ACCIDENT_TEST_CARS];
    dd2_accident_observation vehicles[DD2_ACCIDENT_TEST_CARS];
    dd2_vehicle_contact events[2];
    dd2_vehicle_collision_report contacts;
} dd2_accident_test;

static bool dd2_accident_test_step(dd2_accident_test *test) {
    return dd2_accidents_step(test->drivers, (dd2_accident_frame){.contacts = &test->contacts,
                                                                  .vehicles = test->vehicles,
                                                                  .count = DD2_ACCIDENT_TEST_CARS});
}

static bool dd2_accident_test_reset(dd2_accident_test *test) {
    *test = (dd2_accident_test){0};
    return dd2_accidents_reset(test->drivers, test->vehicles, DD2_ACCIDENT_TEST_CARS);
}

static void dd2_accident_test_contact(dd2_accident_test *test, unsigned second) {
    test->contacts = (dd2_vehicle_collision_report){.contacts = test->events, .count = 1};
    test->events[0] = (dd2_vehicle_contact){.kind = DD2_VEHICLE_CONTACT_PAIR,
                                            .first = 0,
                                            .second = second,
                                            .normal_speed = dd2_accident_test_speed,
                                            .impulse = dd2_accident_test_impulse};
}

static bool dd2_accident_test_expire(dd2_accident_test *test) {
    test->contacts = (dd2_vehicle_collision_report){0};
    for (unsigned step = 0; step < DD2_ACCIDENT_WINDOW_STEPS; ++step) {
        if (!dd2_accident_test_step(test)) {
            return false;
        }
    }
    return test->drivers[0].remaining == 0 && test->drivers[1].remaining == 0;
}

typedef struct {
    unsigned quarters;
    int direction;
    unsigned points;
} dd2_accident_test_spin;

static bool dd2_accident_test_spins(dd2_accident_test_spin spin) {
    dd2_accident_test test = {0};
    if (!dd2_accident_test_reset(&test)) {
        return false;
    }
    dd2_accident_test_contact(&test, 1);
    if (!dd2_accident_test_step(&test)) {
        return false;
    }
    test.contacts = (dd2_vehicle_collision_report){0};
    for (unsigned quarter = 1; quarter <= spin.quarters; ++quarter) {
        double heading = (double)spin.direction * (double)quarter * dd2_accident_test_quarter;
        if (heading > 2 * dd2_accident_test_quarter) {
            heading -= 4 * dd2_accident_test_quarter;
        } else if (heading < -2 * dd2_accident_test_quarter) {
            heading += 4 * dd2_accident_test_quarter;
        }
        test.vehicles[1].heading = heading;
        if (!dd2_accident_test_step(&test)) {
            return false;
        }
    }
    if (spin.quarters == 4) {
        return test.drivers[0].points == spin.points && test.drivers[1].remaining == 0;
    }
    return test.drivers[0].points == 0 && dd2_accident_test_expire(&test) &&
           test.drivers[0].points == spin.points;
}

static bool dd2_accident_test_latch(void) {
    dd2_accident_test test = {0};
    if (!dd2_accident_test_reset(&test)) {
        return false;
    }
    dd2_accident_test_contact(&test, 1);
    if (!dd2_accident_test_step(&test)) {
        return false;
    }
    const unsigned remaining = test.drivers[0].remaining;
    dd2_accident_test_contact(&test, 2);
    if (!dd2_accident_test_step(&test) || test.drivers[0].partner != 1 ||
        test.drivers[0].remaining != remaining - 1 || test.drivers[2].partner != 0) {
        return false;
    }
    test.contacts = (dd2_vehicle_collision_report){0};
    test.vehicles[1].heading = dd2_accident_test_quarter;
    if (!dd2_accident_test_step(&test)) {
        return false;
    }
    test.vehicles[1].heading = 0;
    if (!dd2_accident_test_step(&test) || !dd2_accident_test_expire(&test) ||
        test.drivers[0].points != DD2_ACCIDENT_TEST_QUARTER_POINTS) {
        return false;
    }
    test.vehicles[1].retired = true;
    return dd2_accident_test_step(&test) &&
           test.drivers[0].points == DD2_ACCIDENT_TEST_QUARTER_POINTS &&
           test.drivers[0].destructions == 0;
}

static bool dd2_accident_test_retirement(bool instigator_retired) {
    dd2_accident_test test = {0};
    if (!dd2_accident_test_reset(&test)) {
        return false;
    }
    dd2_accident_test_contact(&test, 1);
    if (!dd2_accident_test_step(&test)) {
        return false;
    }
    test.contacts = (dd2_vehicle_collision_report){0};
    test.vehicles[1].retired = true;
    test.vehicles[0].retired = instigator_retired;
    if (!dd2_accident_test_step(&test)) {
        return false;
    }
    const unsigned expected = instigator_retired ? 0 : DD2_ACCIDENT_TEST_HALF_POINTS;
    if (test.drivers[0].points != expected ||
        test.drivers[0].destructions != (unsigned)!instigator_retired ||
        test.drivers[0].remaining != 0 || test.drivers[1].remaining != 0) {
        return false;
    }
    dd2_accident_test_contact(&test, 1);
    return dd2_accident_test_step(&test) && test.drivers[0].points == expected &&
           test.drivers[0].remaining == 0 && test.drivers[1].remaining == 0;
}

static bool dd2_accident_test_wrap(void) {
    dd2_accident_test test = {0};
    if (!dd2_accident_test_reset(&test)) {
        return false;
    }
    const double almost_half = (double)DD2_ACCIDENT_TEST_WRAP_DEGREES * dd2_accident_test_degrees;
    test.vehicles[1].heading = almost_half;
    if (!dd2_accidents_reset(test.drivers, test.vehicles, DD2_ACCIDENT_TEST_CARS)) {
        return false;
    }
    dd2_accident_test_contact(&test, 1);
    if (!dd2_accident_test_step(&test)) {
        return false;
    }
    test.contacts = (dd2_vehicle_collision_report){0};
    test.vehicles[1].heading = -almost_half;
    if (!dd2_accident_test_step(&test)) {
        return false;
    }
    const double tolerance = 1e-10;
    return fabs(test.drivers[1].rotation - (2 * dd2_accident_test_degrees)) < tolerance &&
           dd2_accident_test_expire(&test) && test.drivers[0].points == 0;
}

static bool dd2_accident_test_cap(void) {
    dd2_accident_test test = {0};
    if (!dd2_accident_test_reset(&test)) {
        return false;
    }
    for (unsigned spin = 0; spin < DD2_ACCIDENT_TEST_CAP_SPINS; ++spin) {
        dd2_accident_test_contact(&test, 1);
        if (!dd2_accident_test_step(&test)) {
            return false;
        }
        test.contacts = (dd2_vehicle_collision_report){0};
        for (unsigned quarter = 1; quarter <= 4; ++quarter) {
            test.vehicles[1].heading = (double)(quarter <= 2 ? (int)quarter : (int)quarter - 4) *
                                       dd2_accident_test_quarter;
            if (!dd2_accident_test_step(&test)) {
                return false;
            }
        }
    }
    return test.drivers[0].points == DD2_ACCIDENT_SCORE_LIMIT && test.drivers[0].destructions == 0;
}

static bool dd2_accident_test_support(void) {
    dd2_accident_test test = {0};
    if (!dd2_accident_test_reset(&test)) {
        return false;
    }
    dd2_accident_test_contact(&test, 1);
    test.events[0].impulse = 0;
    if (!dd2_accident_test_step(&test) || test.drivers[0].remaining != 0) {
        return false;
    }
    test.events[0].kind = DD2_VEHICLE_CONTACT_BARRIER;
    test.events[0].second = DD2_VEHICLE_NO_PARTNER;
    test.events[0].impulse = dd2_accident_test_impulse;
    return dd2_accident_test_step(&test) && test.drivers[0].remaining == 0 &&
           test.drivers[0].points == 0;
}

static bool dd2_accident_test_same(const dd2_accident_driver *first,
                                   const dd2_accident_driver *second) {
    for (unsigned slot = 0; slot < DD2_ACCIDENT_TEST_CARS; ++slot) {
        const dd2_accident_driver left = first[slot];
        const dd2_accident_driver right = second[slot];
        if (left.points != right.points || left.destructions != right.destructions ||
            left.partner != right.partner || left.remaining != right.remaining ||
            left.quarters != right.quarters || left.heading != right.heading ||
            left.rotation != right.rotation || left.steps != right.steps ||
            left.retired != right.retired) {
            return false;
        }
    }
    return true;
}

static bool dd2_accident_test_invalid(void) {
    dd2_accident_test test = {0};
    if (!dd2_accident_test_reset(&test)) {
        return false;
    }
    const union {
        uint64_t bits;
        double number;
    } invalid = {.bits = UINT64_C(0x7ff8000000000001)};
    dd2_accident_test_contact(&test, 1);
    test.contacts.count = 2;
    test.events[1] = test.events[0];
    test.events[1].impulse = invalid.number;
    dd2_accident_driver before[DD2_ACCIDENT_TEST_CARS] = {0};
    for (unsigned slot = 0; slot < DD2_ACCIDENT_TEST_CARS; ++slot) {
        before[slot] = test.drivers[slot];
    }
    if (dd2_accident_test_step(&test) || !dd2_accident_test_same(before, test.drivers)) {
        return false;
    }
    test.contacts.count = 1;
    test.drivers[2].steps = UINT64_MAX;
    for (unsigned slot = 0; slot < DD2_ACCIDENT_TEST_CARS; ++slot) {
        before[slot] = test.drivers[slot];
    }
    if (dd2_accident_test_step(&test) || !dd2_accident_test_same(before, test.drivers)) {
        return false;
    }
    test.vehicles[2].heading = invalid.number;
    return !dd2_accidents_reset(test.drivers, test.vehicles, DD2_ACCIDENT_TEST_CARS) &&
           dd2_accident_test_same(before, test.drivers);
}

int main(void) {
    const dd2_accident_test_spin spins[] = {
        {.quarters = 1, .direction = 1, .points = DD2_ACCIDENT_TEST_QUARTER_POINTS},
        {.quarters = 1, .direction = -1, .points = DD2_ACCIDENT_TEST_QUARTER_POINTS},
        {.quarters = 2, .direction = 1, .points = DD2_ACCIDENT_TEST_HALF_POINTS},
        {.quarters = 2, .direction = -1, .points = DD2_ACCIDENT_TEST_HALF_POINTS},
        {.quarters = 4, .direction = 1, .points = DD2_ACCIDENT_TEST_FULL_POINTS},
        {.quarters = 4, .direction = -1, .points = DD2_ACCIDENT_TEST_FULL_POINTS}};
    bool pass = true;
    for (unsigned spin = 0; spin < sizeof(spins) / sizeof(spins[0]); ++spin) {
        pass = dd2_accident_test_spins(spins[spin]) && pass;
    }
    pass = pass && dd2_accident_test_latch() && dd2_accident_test_retirement(false) &&
           dd2_accident_test_retirement(true) && dd2_accident_test_wrap() &&
           dd2_accident_test_cap() && dd2_accident_test_support() && dd2_accident_test_invalid();
    puts(pass ? "accident attribution/scoring: PASS" : "accident attribution/scoring: FAIL");
    return pass ? EXIT_SUCCESS : EXIT_FAILURE;
}
