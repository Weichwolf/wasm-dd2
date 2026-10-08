#include "physics/car_contact.h"
#include "physics/vehicle.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { DD2_QUERY_FIELDS = 20, DD2_QUERY_SCENES = 16 };
static const double dd2_query_tolerance = 1e-8;
static const double dd2_query_height = 1000;
static const double dd2_query_margin = 0.0002;
static const double dd2_query_far = 5000;
static const double dd2_query_scene_base = 300;
static const double dd2_query_scene_spacing = 100;
static const double dd2_query_scene_height = 80;
static const double dd2_query_scene_yaw = 50;
static const double dd2_query_scene_shift = 50;
static const double dd2_query_invalid_margin = 505;
enum { DD2_QUERY_COLUMNS = 5, DD2_QUERY_YAW_DIVISOR = 7 };

static bool dd2_query_contact_same(dd2_car_contact first, dd2_car_contact second) {
    return fabs(first.time - second.time) < dd2_query_tolerance &&
           fabs(first.penetration - second.penetration) < dd2_query_tolerance &&
           fabs(first.point.x - second.point.x) < dd2_query_tolerance &&
           fabs(first.point.y - second.point.y) < dd2_query_tolerance &&
           fabs(first.point.z - second.point.z) < dd2_query_tolerance &&
           fabs(first.normal.x - second.normal.x) < dd2_query_tolerance &&
           fabs(first.normal.y - second.normal.y) < dd2_query_tolerance &&
           fabs(first.normal.z - second.normal.z) < dd2_query_tolerance &&
           first.unresolved == second.unresolved;
}

static bool dd2_query_analytical(void) {
    dd2_vehicle vehicles[DD2_QUERY_FIELDS] = {0};
    for (unsigned slot = 0; slot < DD2_QUERY_FIELDS; ++slot) {
        if (!dd2_vehicle_reset(&vehicles[slot],
                               (dd2_vehicle_spawn){.position = {.y = dd2_query_height}})) {
            return false;
        }
    }
    const dd2_car_fleet_motion sweep = {
        .start = vehicles, .end = vehicles, .count = DD2_QUERY_FIELDS};
    const dd2_car_fleet_neighborhood near = {
        .vehicles = vehicles, .count = DD2_QUERY_FIELDS, .margin = dd2_query_margin};
    dd2_car_pair_contacts swept;
    dd2_car_pair_contacts neighbors;
    if (!dd2_car_contacts_sweep(&sweep, &swept) || !dd2_car_contacts_proximity(&near, &neighbors) ||
        swept.count != DD2_CAR_PAIR_LIMIT || neighbors.count != DD2_CAR_PAIR_LIMIT) {
        return false;
    }
    unsigned index = 0;
    for (unsigned first = 0; first < DD2_QUERY_FIELDS; ++first) {
        for (unsigned second = first + 1; second < DD2_QUERY_FIELDS; ++second) {
            const dd2_car_pair_contact pair = swept.pairs[index];
            const dd2_car_contact expected = {
                .penetration = 260, .normal = {.y = -1}, .point = {.y = dd2_query_height}};
            if (pair.first != first || pair.second != second ||
                !dd2_query_contact_same(pair.contact, expected) ||
                neighbors.pairs[index].first != first || neighbors.pairs[index].second != second ||
                !dd2_query_contact_same(neighbors.pairs[index].contact, expected)) {
                return false;
            }
            ++index;
        }
    }
    /* Reuse caller-owned result storage after changing every body. A cache may
     * not retain the preceding coincident field or publish stale pair records. */
    for (unsigned slot = 0; slot < DD2_QUERY_FIELDS; ++slot) {
        vehicles[slot].position.x = (double)slot * dd2_query_far;
    }
    return dd2_car_contacts_sweep(&sweep, &swept) && swept.count == 0 &&
           dd2_car_contacts_proximity(&near, &neighbors) && neighbors.count == 0;
}

static bool dd2_query_compare(const dd2_vehicle *start, const dd2_vehicle *end, unsigned count) {
    dd2_car_pair_contacts swept;
    dd2_car_pair_contacts neighbors;
    if (!dd2_car_contacts_sweep(&(dd2_car_fleet_motion){start, end, count}, &swept) ||
        !dd2_car_contacts_proximity(&(dd2_car_fleet_neighborhood){start, count, dd2_query_margin},
                                    &neighbors)) {
        return false;
    }
    unsigned swept_index = 0;
    unsigned near_index = 0;
    for (unsigned first = 0; first < count; ++first) {
        for (unsigned second = first + 1; second < count; ++second) {
            dd2_car_contact contact = {0};
            if (dd2_car_contact_sweep(&start[first], &end[first], &start[second], &end[second],
                                      &contact)) {
                if (swept_index >= swept.count || swept.pairs[swept_index].first != first ||
                    swept.pairs[swept_index].second != second ||
                    !dd2_query_contact_same(contact, swept.pairs[swept_index].contact)) {
                    return false;
                }
                ++swept_index;
            }
            if (dd2_car_contact_proximity(
                    &(dd2_car_neighborhood){&start[first], &start[second], dd2_query_margin},
                    &contact)) {
                if (near_index >= neighbors.count || neighbors.pairs[near_index].first != first ||
                    neighbors.pairs[near_index].second != second ||
                    !dd2_query_contact_same(contact, neighbors.pairs[near_index].contact)) {
                    return false;
                }
                ++near_index;
            }
        }
    }
    return swept_index == swept.count && near_index == neighbors.count;
}

static bool dd2_query_scenes(void) {
    for (unsigned scene = 0; scene < DD2_QUERY_SCENES; ++scene) {
        dd2_vehicle start[DD2_QUERY_FIELDS] = {0};
        dd2_vehicle end[DD2_QUERY_FIELDS] = {0};
        for (unsigned slot = 0; slot < DD2_QUERY_FIELDS; ++slot) {
            const double gap = dd2_query_scene_base + ((double)scene * dd2_query_scene_spacing);
            const dd2_vehicle_spawn spawn = {
                .position = {.x = (double)(slot % DD2_QUERY_COLUMNS) * gap,
                             .y = dd2_query_height + ((double)(slot % 3) * dd2_query_scene_height),
                             .z = floor((double)slot / (double)DD2_QUERY_COLUMNS) * gap},
                .yaw = (double)slot / (double)DD2_QUERY_YAW_DIVISOR};
            if (!dd2_vehicle_reset(&start[slot], spawn) ||
                !dd2_vehicle_reset(&end[slot],
                                   (dd2_vehicle_spawn){
                                       .position = spawn.position,
                                       .yaw = spawn.yaw + ((double)scene / dd2_query_scene_yaw)})) {
                return false;
            }
            end[slot].position.x +=
                (slot % 2 == 0 ? 1 : -1) * ((double)scene * dd2_query_scene_shift);
            end[slot].position.z -= (double)(slot * scene);
        }
        unsigned char saved_start[sizeof(start)];
        unsigned char saved_end[sizeof(end)];
        const unsigned char *start_bytes = (const unsigned char *)start;
        const unsigned char *end_bytes = (const unsigned char *)end;
        for (size_t byte = 0; byte < sizeof(start); ++byte) {
            saved_start[byte] = start_bytes[byte];
            saved_end[byte] = end_bytes[byte];
        }
        if (!dd2_query_compare(start, end, DD2_QUERY_FIELDS)) {
            return false;
        }
        for (size_t byte = 0; byte < sizeof(start); ++byte) {
            if (saved_start[byte] != start_bytes[byte] || saved_end[byte] != end_bytes[byte]) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_query_rejection(void) {
    dd2_vehicle vehicles[DD2_QUERY_FIELDS] = {0};
    for (unsigned slot = 0; slot < DD2_QUERY_FIELDS; ++slot) {
        if (!dd2_vehicle_reset(
                &vehicles[slot],
                (dd2_vehicle_spawn){.position = {.x = (double)slot * dd2_query_far}})) {
            return false;
        }
    }
    dd2_car_fleet_motion sweep = {vehicles, vehicles, DD2_QUERY_FIELDS};
    dd2_car_fleet_neighborhood near = {vehicles, DD2_QUERY_FIELDS, dd2_query_margin};
    dd2_car_pair_contacts result = {.count = DD2_CAR_PAIR_LIMIT};
    if (dd2_car_contacts_sweep(NULL, &result) || result.count != 0 ||
        dd2_car_contacts_proximity(NULL, &result) || result.count != 0 ||
        dd2_car_contacts_sweep(&sweep, NULL) || dd2_car_contacts_proximity(&near, NULL)) {
        return false;
    }
    sweep.count = 0;
    near.count = DD2_QUERY_FIELDS + 1;
    if (dd2_car_contacts_sweep(&sweep, &result) || result.count != 0 ||
        dd2_car_contacts_proximity(&near, &result) || result.count != 0) {
        return false;
    }
    sweep.count = DD2_QUERY_FIELDS;
    near.count = DD2_QUERY_FIELDS;
    /* Invalid far-away bodies still invalidate the complete batch, although
     * a distant pair could otherwise fail its sphere or SAT test immediately. */
    const uint64_t invalid[] = {UINT64_C(0x7ff8000000000001), UINT64_C(0x7ff0000000000000)};
    for (unsigned bits = 0; bits < sizeof(invalid) / sizeof(invalid[0]); ++bits) {
        unsigned char *destination = (unsigned char *)&vehicles[DD2_QUERY_FIELDS - 1].velocity.x;
        const unsigned char *source = (const unsigned char *)&invalid[bits];
        for (size_t byte = 0; byte < sizeof(double); ++byte) {
            destination[byte] = source[byte];
        }
        if (dd2_car_contacts_sweep(&sweep, &result) || result.count != 0 ||
            dd2_car_contacts_proximity(&near, &result) || result.count != 0) {
            return false;
        }
    }
    vehicles[DD2_QUERY_FIELDS - 1].velocity.x = 0;
    near.margin = -1;
    if (dd2_car_contacts_proximity(&near, &result) || result.count != 0) {
        return false;
    }
    near.margin = dd2_query_invalid_margin;
    if (dd2_car_contacts_proximity(&near, &result) || result.count != 0) {
        return false;
    }
    sweep.count = 1;
    near.count = 1;
    near.margin = dd2_query_margin;
    return dd2_car_contacts_sweep(&sweep, &result) && result.count == 0 &&
           dd2_car_contacts_proximity(&near, &result) && result.count == 0;
}

int main(void) {
    if (!dd2_query_analytical() || !dd2_query_scenes() || !dd2_query_rejection()) {
        (void)fputs("Fleet pair preparation contract failed\n", stderr);
        return EXIT_FAILURE;
    }
    puts("Fleet pair preparation contract passed");
    return EXIT_SUCCESS;
}
