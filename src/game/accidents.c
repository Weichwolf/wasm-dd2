#include "game/accidents.h"

#include "physics/numeric.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

static const double dd2_accident_half_turn = 3.14159265358979323846;
static const double dd2_accident_full_turn = 6.28318530717958647693;
static const double dd2_accident_min_speed = 500;
static const double dd2_accident_min_impulse = 250;
static const double dd2_accident_angle_tolerance = 1e-10;
enum {
    DD2_ACCIDENT_QUARTER_POINTS = 10,
    DD2_ACCIDENT_HALF_POINTS = 25,
    DD2_ACCIDENT_FULL_POINTS = 50
};

static bool dd2_accident_observation_valid(const dd2_accident_observation *vehicle) {
    return dd2_numeric_finite(&vehicle->heading) &&
           fabs(vehicle->heading) <= dd2_accident_half_turn;
}

static bool dd2_accident_driver_valid(const dd2_accident_driver *drivers, unsigned slot,
                                      dd2_accident_frame frame) {
    const dd2_accident_driver *driver = &drivers[slot];
    const unsigned count = frame.count;
    if (driver->points > DD2_ACCIDENT_SCORE_LIMIT || driver->destructions >= count ||
        driver->remaining > DD2_ACCIDENT_WINDOW_STEPS || driver->quarters > 2 ||
        !dd2_numeric_finite(&driver->heading) || fabs(driver->heading) > dd2_accident_half_turn ||
        !dd2_numeric_finite(&driver->rotation) ||
        fabs(driver->rotation) >= dd2_accident_full_turn || driver->steps == UINT64_MAX) {
        return false;
    }
    if (driver->remaining == 0) {
        return driver->partner == DD2_VEHICLE_NO_PARTNER && driver->rotation == 0 &&
               driver->quarters == 0;
    }
    return !driver->retired && driver->partner < count && driver->partner != slot;
}

static bool dd2_accident_contact_valid(const dd2_vehicle_contact *contact, unsigned count,
                                       double previous_time) {
    if ((unsigned)contact->kind > (unsigned)DD2_VEHICLE_CONTACT_PAIR || contact->first >= count ||
        !dd2_numeric_finite(&contact->time) || contact->time < previous_time || contact->time > 1 ||
        !dd2_numeric_finite(&contact->normal_speed) || contact->normal_speed < 0 ||
        !dd2_numeric_finite(&contact->impulse) || contact->impulse < 0) {
        return false;
    }
    return contact->kind == DD2_VEHICLE_CONTACT_PAIR
               ? contact->second < count && contact->second != contact->first
               : contact->second == DD2_VEHICLE_NO_PARTNER;
}

static void dd2_accident_clear(dd2_accident_driver *driver) {
    driver->remaining = 0;
    driver->partner = DD2_VEHICLE_NO_PARTNER;
    driver->rotation = 0;
    driver->quarters = 0;
}

bool dd2_accidents_reset(dd2_accident_driver *drivers, const dd2_accident_observation *vehicles,
                         unsigned count) {
    if (drivers == NULL || vehicles == NULL || count == 0 || count > DD2_VEHICLE_FLEET_LIMIT) {
        return false;
    }
    for (unsigned slot = 0; slot < count; ++slot) {
        if (!dd2_accident_observation_valid(&vehicles[slot])) {
            return false;
        }
    }
    for (unsigned slot = 0; slot < count; ++slot) {
        drivers[slot] = (dd2_accident_driver){.partner = DD2_VEHICLE_NO_PARTNER,
                                              .heading = vehicles[slot].heading,
                                              .retired = vehicles[slot].retired};
    }
    return true;
}

static void dd2_accident_arm(dd2_accident_driver *victim, unsigned instigator) {
    if (victim->remaining == 0) {
        victim->partner = instigator;
        victim->remaining = DD2_ACCIDENT_WINDOW_STEPS;
    }
}

static void dd2_accident_points(dd2_accident_driver *driver, unsigned points) {
    const unsigned sum = driver->points + points;
    driver->points = sum > DD2_ACCIDENT_SCORE_LIMIT ? DD2_ACCIDENT_SCORE_LIMIT : sum;
}

static void dd2_accident_spin(dd2_accident_driver *drivers, dd2_accident_frame frame,
                              unsigned slot) {
    dd2_accident_driver *victim = &drivers[slot];
    dd2_accident_driver *instigator = &drivers[victim->partner];
    const double heading = frame.vehicles[slot].heading;
    double delta = heading - victim->heading;
    if (delta > dd2_accident_half_turn) {
        delta -= dd2_accident_full_turn;
    } else if (delta < -dd2_accident_half_turn) {
        delta += dd2_accident_full_turn;
    }
    victim->rotation += delta;
    const double angle = fabs(victim->rotation) + dd2_accident_angle_tolerance;
    if (angle >= dd2_accident_half_turn / 2 && victim->quarters == 0) {
        victim->quarters = 1;
    }
    if (angle >= dd2_accident_half_turn) {
        victim->quarters = 2;
    }
    --victim->remaining;
    if (angle >= dd2_accident_full_turn) {
        dd2_accident_points(instigator, DD2_ACCIDENT_FULL_POINTS);
        dd2_accident_clear(victim);
    } else if (victim->remaining == 0) {
        unsigned points = 0;
        if (victim->quarters == 2) {
            points = DD2_ACCIDENT_HALF_POINTS;
        } else if (victim->quarters == 1) {
            points = DD2_ACCIDENT_QUARTER_POINTS;
        }
        dd2_accident_points(instigator, points);
        dd2_accident_clear(victim);
    }
}

static void dd2_accident_update(dd2_accident_driver *drivers, dd2_accident_frame frame,
                                unsigned slot) {
    dd2_accident_driver *victim = &drivers[slot];
    if (victim->remaining != 0) {
        dd2_accident_driver *instigator = &drivers[victim->partner];
        if (frame.vehicles[victim->partner].retired) {
            dd2_accident_clear(victim);
        } else if (frame.vehicles[slot].retired) {
            dd2_accident_points(instigator, DD2_ACCIDENT_HALF_POINTS);
            if (instigator->destructions < frame.count - 1) {
                ++instigator->destructions;
            }
            dd2_accident_clear(victim);
        } else {
            dd2_accident_spin(drivers, frame, slot);
        }
    }
    victim->heading = frame.vehicles[slot].heading;
    victim->retired = frame.vehicles[slot].retired;
    ++victim->steps;
}

bool dd2_accidents_step(dd2_accident_driver *drivers, dd2_accident_frame frame) {
    if (drivers == NULL || frame.contacts == NULL || frame.vehicles == NULL || frame.count == 0 ||
        frame.count > DD2_VEHICLE_FLEET_LIMIT || frame.contacts->count > DD2_VEHICLE_REPORT_LIMIT ||
        (frame.contacts->count != 0 && frame.contacts->contacts == NULL)) {
        return false;
    }
    for (unsigned slot = 0; slot < frame.count; ++slot) {
        if (!dd2_accident_driver_valid(drivers, slot, frame) ||
            !dd2_accident_observation_valid(&frame.vehicles[slot]) ||
            (drivers[slot].retired && !frame.vehicles[slot].retired)) {
            return false;
        }
    }
    double previous_time = 0;
    for (unsigned event = 0; event < frame.contacts->count; ++event) {
        const dd2_vehicle_contact *contact = &frame.contacts->contacts[event];
        if (!dd2_accident_contact_valid(contact, frame.count, previous_time)) {
            return false;
        }
        previous_time = contact->time;
    }
    for (unsigned event = 0; event < frame.contacts->count; ++event) {
        const dd2_vehicle_contact *contact = &frame.contacts->contacts[event];
        if (contact->kind == DD2_VEHICLE_CONTACT_PAIR &&
            contact->normal_speed > dd2_accident_min_speed &&
            contact->impulse > dd2_accident_min_impulse && !drivers[contact->first].retired &&
            !drivers[contact->second].retired) {
            dd2_accident_arm(&drivers[contact->first], contact->second);
            dd2_accident_arm(&drivers[contact->second], contact->first);
        }
    }
    for (unsigned slot = 0; slot < frame.count; ++slot) {
        dd2_accident_update(drivers, frame, slot);
    }
    return true;
}
