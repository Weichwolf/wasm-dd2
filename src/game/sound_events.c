#include "game/sound_events.h"

#include "game/race.h"
#include "physics/numeric.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const double dd2_sound_contact_threshold = 300;
static const double dd2_sound_landing_threshold = 900;
static const double dd2_sound_full_impact = 1600;
static const double dd2_sound_distance = 12000;

static bool dd2_sound_observation_valid(dd2_sound_observation observation) {
    if (observation.count == 0 || observation.count > DD2_VEHICLE_FLEET_LIMIT ||
        !dd2_vehicle_valid(observation.listener) || observation.contacts == NULL ||
        observation.contacts->count > DD2_VEHICLE_REPORT_LIMIT ||
        (observation.contacts->count != 0 && observation.contacts->contacts == NULL) ||
        (observation.race != NULL && observation.race->rules.count != observation.count)) {
        return false;
    }
    for (unsigned index = 0; index < observation.contacts->count; ++index) {
        const dd2_vehicle_contact *contact = &observation.contacts->contacts[index];
        if (contact->kind < DD2_VEHICLE_CONTACT_GROUND ||
            contact->kind > DD2_VEHICLE_CONTACT_PAIR ||
            ((contact->kind == DD2_VEHICLE_CONTACT_PAIR) !=
             (contact->second != DD2_VEHICLE_NO_PARTNER)) ||
            contact->first == contact->second || contact->first >= observation.count ||
            (contact->second != DD2_VEHICLE_NO_PARTNER && contact->second >= observation.count) ||
            !dd2_numeric_finite(&contact->point.x) || !dd2_numeric_finite(&contact->point.y) ||
            !dd2_numeric_finite(&contact->point.z) || !dd2_numeric_finite(&contact->normal_speed) ||
            contact->normal_speed < 0 || !dd2_numeric_finite(&contact->impulse) ||
            contact->impulse < 0) {
            return false;
        }
    }
    return true;
}

static bool dd2_sound_append(dd2_sound_batch *batch, dd2_sound_event event) {
    if (batch->count == DD2_SOUND_EVENT_LIMIT) {
        return false;
    }
    batch->events[batch->count++] = event;
    return true;
}

static bool dd2_sound_countdown(dd2_sound_state *state, dd2_sound_batch *batch,
                                const dd2_race *race) {
    if (race == NULL) {
        return true;
    }
    const unsigned cue = dd2_race_countdown(race);
    if (cue != 0 && cue != state->countdown) {
        const dd2_sound_cue kind = cue == 3 ? DD2_SOUND_THREE : DD2_SOUND_ONE;
        if (!dd2_sound_append(batch, (dd2_sound_event){.cue = cue == 2 ? DD2_SOUND_TWO : kind,
                                                       .tick = state->ticks,
                                                       .gain = DD2_SOUND_GAIN_ONE})) {
            return false;
        }
    }
    state->countdown = cue;
    if (state->waiting_for_go && race->phase == DD2_RACE_RUNNING) {
        state->waiting_for_go = false;
        return dd2_sound_append(batch, (dd2_sound_event){.cue = DD2_SOUND_GO,
                                                         .tick = state->ticks,
                                                         .gain = DD2_SOUND_GAIN_ONE});
    }
    return true;
}

static unsigned dd2_sound_impact_gain(const dd2_vehicle_contact *contact,
                                      const dd2_vehicle *listener, double *pan) {
    const double threshold = contact->kind == DD2_VEHICLE_CONTACT_GROUND
                                 ? dd2_sound_landing_threshold
                                 : dd2_sound_contact_threshold;
    if (contact->normal_speed < threshold || contact->impulse == 0) {
        return 0;
    }
    const dd2_vehicle_vector relative = {.x = contact->point.x - listener->position.x,
                                         .y = contact->point.y - listener->position.y,
                                         .z = contact->point.z - listener->position.z};
    const double distance = hypot(hypot(relative.x, relative.z), relative.y);
    if (distance >= dd2_sound_distance) {
        return 0;
    }
    const dd2_vehicle_vector right =
        dd2_vehicle_rotate(listener->rotation, (dd2_vehicle_vector){.x = -1});
    *pan =
        distance == 0
            ? 0
            : ((relative.x * right.x) + (relative.y * right.y) + (relative.z * right.z)) / distance;
    const double strength = fmin(1, contact->normal_speed / dd2_sound_full_impact);
    return (unsigned)lround(strength * (1 - (distance / dd2_sound_distance)) *
                            (double)DD2_SOUND_GAIN_ONE);
}

static bool dd2_sound_impact(dd2_sound_state *state, dd2_sound_batch *batch,
                             dd2_sound_observation observation) {
    const dd2_vehicle_contact *selected = NULL;
    unsigned loudest = 0;
    double selected_pan = 0;
    for (unsigned index = 0; index < observation.contacts->count; ++index) {
        const dd2_vehicle_contact *contact = &observation.contacts->contacts[index];
        if (state->cooldown[contact->first][contact->second] != 0) {
            continue;
        }
        double pan = 0;
        const unsigned gain = dd2_sound_impact_gain(contact, observation.listener, &pan);
        if (gain > loudest) {
            selected = contact;
            loudest = gain;
            selected_pan = pan;
        }
    }
    if (selected == NULL) {
        return true;
    }
    const dd2_sound_event event = {
        .cue = DD2_SOUND_IMPACT,
        .tick = state->ticks,
        .gain = loudest,
        .pan = (int)lround(fmax(-1, fmin(1, selected_pan)) * (double)DD2_SOUND_GAIN_ONE)};
    if (!dd2_sound_append(batch, event)) {
        return false;
    }
    state->cooldown[selected->first][selected->second] = DD2_SOUND_COOLDOWN_STEPS;
    if (selected->second != DD2_VEHICLE_NO_PARTNER) {
        state->cooldown[selected->second][selected->first] = DD2_SOUND_COOLDOWN_STEPS;
    }
    return true;
}

bool dd2_sound_events_step(dd2_sound_state *state, dd2_sound_batch *batch,
                           dd2_sound_observation observation) {
    if (state == NULL || batch == NULL || batch->count > DD2_SOUND_EVENT_LIMIT ||
        state->ticks == UINT64_MAX || !dd2_sound_observation_valid(observation)) {
        return false;
    }
    dd2_sound_state next = *state;
    dd2_sound_batch events = *batch;
    ++next.ticks;
    for (unsigned slot = 0; slot < observation.count; ++slot) {
        for (unsigned partner = 0; partner <= DD2_VEHICLE_FLEET_LIMIT; ++partner) {
            if (next.cooldown[slot][partner] != 0) {
                --next.cooldown[slot][partner];
            }
        }
    }
    if (!dd2_sound_countdown(&next, &events, observation.race) ||
        !dd2_sound_impact(&next, &events, observation)) {
        return false;
    }
    *state = next;
    *batch = events;
    return true;
}
