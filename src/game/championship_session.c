#include "game/championship_session.h"

#include "assets/archive.h"
#include "assets/track.h"
#include "game/championship.h"
#include "game/driving.h"
#include "game/league.h"
#include "game/race.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    dd2_track *track;
    dd2_driving *driving;
    unsigned level;
} dd2_championship_field;
struct dd2_championship_session {
    const dd2_archive *archive;
    dd2_championship state;
    dd2_championship_field field;
};
struct dd2_championship_transition {
    const dd2_championship_session *owner;
    uint64_t ticket;
    dd2_championship_phase phase;
    dd2_championship next;
    dd2_championship_field field;
    bool owns_field;
};

static void dd2_championship_field_destroy(dd2_championship_field field) {
    dd2_driving_destroy(field.driving);
    dd2_track_destroy(field.track);
}

static bool dd2_championship_field_prepare(const dd2_archive *archive, dd2_championship *state,
                                           dd2_championship_field *output) {
    unsigned slots[DD2_LEAGUE_DRIVERS] = {0};
    const unsigned track = dd2_championship_track(state);
    dd2_championship_field field = {.track = dd2_track_create(archive, track), .level = track};
    if (field.track != NULL && dd2_league_grid(&state->league, slots)) {
        field.driving = dd2_driving_create_grid(dd2_track_road(field.track), track, slots);
    }
    uint64_t ticket = 0;
    if (field.driving == NULL || !dd2_driving_set_race(field.driving, true, state->mode) ||
        !dd2_championship_begin(state, dd2_driving_race(field.driving)->rules, &ticket)) {
        dd2_championship_field_destroy(field);
        return false;
    }
    *output = field;
    return true;
}

dd2_championship_session *dd2_championship_session_create(const dd2_archive *archive,
                                                          dd2_race_mode mode) {
    if (archive == NULL || (mode != DD2_RACE_WRECKING && mode != DD2_RACE_STOCKCAR)) {
        return NULL;
    }
    dd2_championship_session *session = calloc(1, sizeof(*session));
    if (session == NULL) {
        return NULL;
    }
    session->archive = archive;
    if (!dd2_championship_reset(&session->state, mode) ||
        !dd2_championship_field_prepare(archive, &session->state, &session->field)) {
        dd2_championship_session_destroy(session);
        return NULL;
    }
    return session;
}

void dd2_championship_session_destroy(dd2_championship_session *session) {
    if (session != NULL) {
        dd2_championship_field_destroy(session->field);
        free(session);
    }
}

bool dd2_championship_session_advance(dd2_championship_session *session, dd2_driving_frame frame) {
    if (session == NULL || session->state.phase == DD2_CHAMPIONSHIP_ABORTED) {
        return false;
    }
    if (session->state.phase != DD2_CHAMPIONSHIP_RACING) {
        return dd2_driving_advance(session->field.driving, frame);
    }
    if (!dd2_driving_advance(session->field.driving, frame)) {
        return false;
    }
    const dd2_race *race = dd2_driving_race(session->field.driving);
    return race->phase != DD2_RACE_RESULTS ||
           dd2_championship_finish(&session->state, session->state.ticket, race);
}

bool dd2_championship_session_continue(dd2_championship_session *session) {
    dd2_championship_transition *transition = dd2_championship_session_prepare(session);
    if (transition == NULL) {
        return false;
    }
    if (!dd2_championship_session_commit(session, transition)) {
        dd2_championship_transition_destroy(transition);
        return false;
    }
    return true;
}

static dd2_championship_transition *dd2_championship_prepare(dd2_championship_session *session,
                                                             bool restart) {
    if (session == NULL) {
        return NULL;
    }
    const bool continuing = session->state.phase == DD2_CHAMPIONSHIP_ROUND_RESULTS ||
                            session->state.phase == DD2_CHAMPIONSHIP_SEASON_RESULTS;
    if ((restart && session->state.phase != DD2_CHAMPIONSHIP_RACING) || (!restart && !continuing)) {
        return NULL;
    }
    dd2_championship_transition *transition = calloc(1, sizeof(*transition));
    if (transition == NULL) {
        return NULL;
    }
    transition->owner = session;
    transition->ticket = session->state.ticket;
    transition->phase = session->state.phase;
    transition->next = session->state;
    const bool ready = restart ? dd2_championship_restart(&transition->next)
                               : dd2_championship_continue(&transition->next);
    if (!ready) {
        dd2_championship_transition_destroy(transition);
        return NULL;
    }
    if (transition->next.phase == DD2_CHAMPIONSHIP_READY) {
        if (!dd2_championship_field_prepare(session->archive, &transition->next,
                                            &transition->field)) {
            dd2_championship_transition_destroy(transition);
            return NULL;
        }
        transition->owns_field = true;
    } else {
        transition->field = session->field;
    }
    return transition;
}

dd2_championship_transition *dd2_championship_session_prepare(dd2_championship_session *session) {
    return dd2_championship_prepare(session, false);
}
dd2_championship_transition *
dd2_championship_session_prepare_restart(dd2_championship_session *session) {
    return dd2_championship_prepare(session, true);
}

void dd2_championship_transition_destroy(dd2_championship_transition *transition) {
    if (transition != NULL) {
        if (transition->owns_field) {
            dd2_championship_field_destroy(transition->field);
        }
        free(transition);
    }
}

bool dd2_championship_transition_current(const dd2_championship_session *session,
                                         const dd2_championship_transition *transition) {
    return session != NULL && transition != NULL && transition->owner == session &&
           transition->ticket == session->state.ticket && transition->phase == session->state.phase;
}

bool dd2_championship_session_commit(dd2_championship_session *session,
                                     dd2_championship_transition *transition) {
    if (!dd2_championship_transition_current(session, transition)) {
        return false;
    }
    if (transition->owns_field) {
        dd2_championship_field_destroy(session->field);
        session->field = transition->field;
        transition->owns_field = false;
    }
    session->state = transition->next;
    dd2_championship_transition_destroy(transition);
    return true;
}

const dd2_championship *
dd2_championship_transition_state(const dd2_championship_transition *transition) {
    return transition == NULL ? NULL : &transition->next;
}
const dd2_track *dd2_championship_transition_track(const dd2_championship_transition *transition) {
    return transition == NULL ? NULL : transition->field.track;
}
const dd2_driving *
dd2_championship_transition_driving(const dd2_championship_transition *transition) {
    return transition == NULL ? NULL : transition->field.driving;
}
unsigned dd2_championship_transition_level(const dd2_championship_transition *transition) {
    return transition == NULL ? 0 : transition->field.level;
}

bool dd2_championship_session_abort(dd2_championship_session *session) {
    if (session == NULL || !dd2_championship_abort(&session->state)) {
        return false;
    }
    dd2_driving_suspend(session->field.driving);
    return true;
}

void dd2_championship_session_suspend(dd2_championship_session *session) {
    if (session != NULL) {
        dd2_driving_suspend(session->field.driving);
    }
}

const dd2_championship *dd2_championship_session_state(const dd2_championship_session *session) {
    return session == NULL ? NULL : &session->state;
}

const dd2_track *dd2_championship_session_track(const dd2_championship_session *session) {
    return session == NULL ? NULL : session->field.track;
}

const dd2_driving *dd2_championship_session_driving(const dd2_championship_session *session) {
    return session == NULL ? NULL : session->field.driving;
}
unsigned dd2_championship_session_level(const dd2_championship_session *session) {
    return session == NULL ? 0 : session->field.level;
}
