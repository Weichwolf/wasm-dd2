#include "game/race.h"
#include "game/sound_events.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_SOUND_TEST_FIRST = 36,
    DD2_SOUND_TEST_SECOND = 156,
    DD2_SOUND_TEST_THIRD = 276,
    DD2_SOUND_TEST_SPEED = 1600,
    DD2_SOUND_TEST_DISTANCE = 6000,
    DD2_SOUND_TEST_FAR = 24000
};

static bool dd2_sound_test_countdown(dd2_vehicle *listener) {
    dd2_sound_state state = {.waiting_for_go = true};
    dd2_sound_batch batch = {0};
    const dd2_vehicle_collision_report report = {0};
    dd2_race race = {.rules = {.count = DD2_VEHICLE_FLEET_LIMIT}, .phase = DD2_RACE_COUNTDOWN};
    const unsigned ticks[] = {DD2_SOUND_TEST_FIRST, DD2_SOUND_TEST_SECOND, DD2_SOUND_TEST_THIRD,
                              DD2_RACE_START_STEPS};
    const dd2_sound_cue cues[] = {DD2_SOUND_THREE, DD2_SOUND_TWO, DD2_SOUND_ONE, DD2_SOUND_GO};
    unsigned emitted = 0;
    for (unsigned tick = 1; tick <= DD2_RACE_START_STEPS + 2; ++tick) {
        race.steps = tick;
        race.phase = tick >= DD2_RACE_START_STEPS ? DD2_RACE_RUNNING : DD2_RACE_COUNTDOWN;
        if (!dd2_sound_events_step(
                &state, &batch,
                (dd2_sound_observation){listener, &report, &race, DD2_VEHICLE_FLEET_LIMIT})) {
            return false;
        }
        if (batch.count != emitted) {
            if (emitted >= 4 || batch.count != emitted + 1 ||
                batch.events[emitted].tick != ticks[emitted] ||
                batch.events[emitted].cue != cues[emitted]) {
                return false;
            }
            ++emitted;
        }
    }
    return emitted == 4 && !state.waiting_for_go;
}

static bool dd2_sound_test_impacts(dd2_vehicle *listener) {
    dd2_sound_state state = {0};
    dd2_sound_batch batch = {0};
    dd2_vehicle_collision_report report = {.count = 2};
    report.contacts[0] = (dd2_vehicle_contact){.kind = DD2_VEHICLE_CONTACT_PAIR,
                                               .first = 0,
                                               .second = 1,
                                               .normal_speed = DD2_SOUND_TEST_SPEED,
                                               .impulse = 1,
                                               .point = {.x = -DD2_SOUND_TEST_DISTANCE}};
    report.contacts[1] = (dd2_vehicle_contact){.kind = DD2_VEHICLE_CONTACT_BARRIER,
                                               .first = 2,
                                               .second = DD2_VEHICLE_NO_PARTNER,
                                               .normal_speed = DD2_SOUND_TEST_SPEED,
                                               .impulse = 1,
                                               .point = {.x = DD2_SOUND_TEST_FAR}};
    const dd2_sound_observation observation = {listener, &report, NULL, DD2_VEHICLE_FLEET_LIMIT};
    if (!dd2_sound_events_step(&state, &batch, observation) || batch.count != 1 ||
        batch.events[0].gain != DD2_SOUND_GAIN_ONE / 2 ||
        batch.events[0].pan != DD2_SOUND_GAIN_ONE) {
        return false;
    }
    /* An impulse in an earlier substep remains after a final empty substep. */
    report.count = 0;
    if (!dd2_sound_events_step(&state, &batch, observation) || batch.count != 1 ||
        batch.events[0].tick != 1) {
        return false;
    }
    report.count = 1;
    report.contacts[0].first = 1;
    report.contacts[0].second = 0;
    for (unsigned tick = 3; tick <= DD2_SOUND_COOLDOWN_STEPS; ++tick) {
        if (!dd2_sound_events_step(&state, &batch, observation) || batch.count != 1) {
            return false;
        }
    }
    if (!dd2_sound_events_step(&state, &batch, observation) || batch.count != 2 ||
        batch.events[1].tick != DD2_SOUND_COOLDOWN_STEPS + 1) {
        return false;
    }
    state = (dd2_sound_state){0};
    batch = (dd2_sound_batch){0};
    report.contacts[0].impulse = 0;
    if (!dd2_sound_events_step(&state, &batch, observation) || batch.count != 0) {
        return false;
    }
    report.contacts[0].impulse = 1;
    report.contacts[0].kind = DD2_VEHICLE_CONTACT_GROUND;
    report.contacts[0].second = DD2_VEHICLE_NO_PARTNER;
    report.contacts[0].normal_speed = 1;
    return dd2_sound_events_step(&state, &batch, observation) && batch.count == 0;
}

static bool dd2_sound_test_rollback(dd2_vehicle *listener) {
    dd2_sound_state state = {.ticks = 2, .waiting_for_go = true};
    dd2_sound_batch batch = {.count = DD2_SOUND_EVENT_LIMIT};
    dd2_race race = {
        .rules = {.count = 1}, .steps = DD2_SOUND_TEST_FIRST, .phase = DD2_RACE_COUNTDOWN};
    dd2_vehicle_collision_report report = {0};
    dd2_sound_observation observation = {listener, &report, &race, 1};
    if (dd2_sound_events_step(&state, &batch, observation) || state.ticks != 2 ||
        state.countdown != 0 || batch.count != DD2_SOUND_EVENT_LIMIT) {
        return false;
    }
    batch.count = 0;
    report.count = 1;
    report.contacts[0].first = DD2_VEHICLE_FLEET_LIMIT;
    if (dd2_sound_events_step(&state, &batch, observation) || state.ticks != 2 ||
        batch.count != 0) {
        return false;
    }
    report.count = 0;
    state.ticks = UINT64_MAX;
    return !dd2_sound_events_step(&state, &batch, observation) && state.ticks == UINT64_MAX;
}

int main(void) {
    dd2_vehicle listener = {0};
    const bool valid = dd2_vehicle_reset(&listener, (dd2_vehicle_spawn){0}) &&
                       dd2_sound_test_countdown(&listener) && dd2_sound_test_impacts(&listener) &&
                       dd2_sound_test_rollback(&listener);
    printf("Fixed-step sound events: %s\n", valid ? "PASS" : "FAIL");
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
