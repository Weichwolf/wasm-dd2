#ifndef DD2_TEST_ACCIDENT_TRACE_H
#define DD2_TEST_ACCIDENT_TRACE_H

#include "game/accidents.h"
#include "physics/damage.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct {
    const dd2_vehicle *vehicles;
    const dd2_vehicle_damage *damages;
    const dd2_accident_driver *scores;
    const dd2_vehicle_collision_report *contacts;
    unsigned count;
} dd2_test_accident_trace;

static inline void dd2_test_accident_write(dd2_test_accident_trace trace, unsigned step) {
    printf("{\"accident_step\":true,\"step\":%u,\"observations\":[", step);
    for (unsigned slot = 0; slot < trace.count; ++slot) {
        const dd2_vehicle_rotation rotation = trace.vehicles[slot].rotation;
        printf("%s[%.17g,%.17g,%.17g,%.17g,%d]", slot == 0 ? "" : ",", rotation.x, rotation.y,
               rotation.z, rotation.w, (int)trace.damages[slot].retired);
    }
    printf("],\"events\":[");
    bool first = true;
    for (unsigned event = 0; event < trace.contacts->count; ++event) {
        const dd2_vehicle_contact contact = trace.contacts->contacts[event];
        if (contact.kind == DD2_VEHICLE_CONTACT_PAIR) {
            printf("%s[%u,%u,%.17g,%.17g]", first ? "" : ",", contact.first, contact.second,
                   contact.normal_speed, contact.impulse);
            first = false;
        }
    }
    printf("],\"scores\":[");
    for (unsigned slot = 0; slot < trace.count; ++slot) {
        const dd2_accident_driver score = trace.scores[slot];
        printf("%s[%u,%u,%u,%u,%u,%.17g,%.17g,%d,%llu]", slot == 0 ? "" : ",", score.points,
               score.destructions, score.remaining, score.partner, score.quarters, score.heading,
               score.rotation, (int)score.retired, (unsigned long long)score.steps);
    }
    puts("]}");
}

#endif
