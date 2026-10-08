#include "physics/damage.h"

#include "physics/numeric.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

static const double dd2_damage_min_speed = 500;
static const double dd2_damage_min_impulse = 250;
static const double dd2_damage_crush_impulse = 6000;
static const double dd2_damage_half_width = 186;
static const double dd2_damage_zone_distance = 300;
static const double dd2_damage_power_loss = 0.4;
static const double dd2_damage_width_crush = 0.2;
static const double dd2_damage_height_crush = 0.35;
static const double dd2_damage_length_crush = 0.45;
static const double dd2_damage_limit_tolerance = 1e-12;

bool dd2_damage_valid(const dd2_vehicle_damage *damage) {
    if (damage == NULL) {
        return false;
    }
    for (unsigned region = 0; region < DD2_DAMAGE_REGIONS; ++region) {
        if (!dd2_numeric_finite(&damage->regions[region]) || damage->regions[region] < 0 ||
            damage->regions[region] > 1) {
            return false;
        }
    }
    return true;
}

static bool dd2_damage_vector_valid(const dd2_vehicle_vector *point) {
    return dd2_numeric_finite(&point->x) && dd2_numeric_finite(&point->y) &&
           dd2_numeric_finite(&point->z);
}

static bool dd2_damage_contact_valid(const dd2_vehicle_contact *contact, unsigned count,
                                     double previous_time) {
    if (contact->first >= count || (unsigned)contact->kind > (unsigned)DD2_VEHICLE_CONTACT_PAIR ||
        !dd2_numeric_finite(&contact->time) || contact->time < previous_time || contact->time > 1 ||
        !dd2_numeric_finite(&contact->normal_speed) || contact->normal_speed < 0 ||
        !dd2_numeric_finite(&contact->impulse) || contact->impulse < 0 ||
        !dd2_damage_vector_valid(&contact->point) || !dd2_damage_vector_valid(&contact->normal) ||
        !dd2_damage_vector_valid(&contact->local_points[0]) ||
        !dd2_damage_vector_valid(&contact->local_points[1])) {
        return false;
    }
    return contact->kind == DD2_VEHICLE_CONTACT_PAIR
               ? contact->second < count && contact->second != contact->first
               : contact->second == DD2_VEHICLE_NO_PARTNER;
}

static void dd2_damage_weights(dd2_vehicle_vector point, double *weights) {
    const double right =
        fmax(0, fmin(1, (point.x + dd2_damage_half_width) / (2 * dd2_damage_half_width)));
    const double front = fmax(0, fmin(1, point.z / dd2_damage_zone_distance));
    const double rear = fmax(0, fmin(1, -point.z / dd2_damage_zone_distance));
    const double longitudinal[] = {front, 1 - front - rear, rear};
    for (unsigned region = 0; region < DD2_DAMAGE_REGIONS; ++region) {
        weights[region] = longitudinal[region / 2] * ((region & 1U) != 0 ? right : 1 - right);
    }
}

static void dd2_damage_contact_load(const dd2_vehicle_contact *contact,
                                    double loads[][DD2_DAMAGE_REGIONS]) {
    if (contact->normal_speed <= dd2_damage_min_speed ||
        contact->impulse <= dd2_damage_min_impulse) {
        return;
    }
    const double crush =
        fmin(1, (contact->impulse - dd2_damage_min_impulse) / dd2_damage_crush_impulse);
    const unsigned count = contact->kind == DD2_VEHICLE_CONTACT_PAIR ? 2 : 1;
    const unsigned bodies[] = {contact->first, contact->second};
    for (unsigned body = 0; body < count; ++body) {
        double weights[DD2_DAMAGE_REGIONS] = {0};
        dd2_damage_weights(contact->local_points[body], weights);
        for (unsigned region = 0; region < DD2_DAMAGE_REGIONS; ++region) {
            loads[bodies[body]][region] =
                fmax(loads[bodies[body]][region], crush * weights[region]);
        }
    }
}

bool dd2_damage_step(dd2_vehicle_damage *damage, dd2_damage_frame frame) {
    if (damage == NULL || frame.contacts == NULL || frame.count == 0 ||
        frame.count > DD2_VEHICLE_FLEET_LIMIT || frame.contacts->count > DD2_VEHICLE_REPORT_LIMIT ||
        (frame.contacts->count != 0 && frame.contacts->contacts == NULL)) {
        return false;
    }
    for (unsigned body = 0; body < frame.count; ++body) {
        if (!dd2_damage_valid(&damage[body]) || damage[body].steps == UINT64_MAX) {
            return false;
        }
    }
    double loads[DD2_VEHICLE_FLEET_LIMIT][DD2_DAMAGE_REGIONS] = {0};
    double time = 0;
    for (unsigned event = 0; event < frame.contacts->count; ++event) {
        const dd2_vehicle_contact *contact = &frame.contacts->contacts[event];
        if (!dd2_damage_contact_valid(contact, frame.count, time)) {
            return false;
        }
        time = contact->time;
        dd2_damage_contact_load(contact, loads);
    }
    for (unsigned body = 0; body < frame.count; ++body) {
        for (unsigned region = 0; region < DD2_DAMAGE_REGIONS; ++region) {
            const double accumulated = damage[body].regions[region] + loads[body][region];
            /* Fast-math may leave a mathematically exhausted region one ulp
             * below one. Snap the saturation boundary across targets. */
            damage[body].regions[region] =
                accumulated >= 1 - dd2_damage_limit_tolerance ? 1 : accumulated;
        }
        damage[body].retired = damage[body].retired ||
                               damage[body].regions[DD2_DAMAGE_FRONT_LEFT] == 1 ||
                               damage[body].regions[DD2_DAMAGE_FRONT_RIGHT] == 1;
        ++damage[body].steps;
    }
    return true;
}

double dd2_damage_health(const dd2_vehicle_damage *damage) {
    return damage->retired ? 0
                           : 1 - fmax(damage->regions[DD2_DAMAGE_FRONT_LEFT],
                                      damage->regions[DD2_DAMAGE_FRONT_RIGHT]);
}

dd2_vehicle_control dd2_damage_control(const dd2_vehicle_damage *damage,
                                       dd2_vehicle_control control) {
    const double health = dd2_damage_health(damage);
    if (health == 0) {
        return (dd2_vehicle_control){.brake = 1};
    }
    control.throttle *= 1 - ((1 - health) * dd2_damage_power_loss);
    return control;
}

dd2_vehicle_vector dd2_damage_deform(const dd2_vehicle_damage *damage, dd2_vehicle_vector point) {
    double weights[DD2_DAMAGE_REGIONS] = {0};
    dd2_damage_weights(point, weights);
    double crush = 0;
    for (unsigned region = 0; region < DD2_DAMAGE_REGIONS; ++region) {
        crush += damage->regions[region] * weights[region];
    }
    return (dd2_vehicle_vector){.x = point.x * (1 - (crush * dd2_damage_width_crush)),
                                .y = point.y * (1 - (crush * dd2_damage_height_crush)),
                                .z = point.z * (1 - (crush * dd2_damage_length_crush))};
}
