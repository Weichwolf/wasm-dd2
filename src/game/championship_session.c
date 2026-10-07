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
} dd2_championship_field;
struct dd2_championship_session {
    const dd2_archive *archive;
    dd2_championship state;
    dd2_championship_field field;
};

static void dd2_championship_field_destroy(dd2_championship_field field) {
    dd2_driving_destroy(field.driving);
    dd2_track_destroy(field.track);
}

static bool dd2_championship_field_prepare(const dd2_archive *archive, dd2_championship *state,
                                           dd2_championship_field *output) {
    unsigned slots[DD2_LEAGUE_DRIVERS] = {0};
    const unsigned track = dd2_championship_track(state);
    dd2_championship_field field = {.track = dd2_track_create(archive, track)};
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
    if (session == NULL) {
        return false;
    }
    dd2_championship next = session->state;
    if (!dd2_championship_continue(&next)) {
        return false;
    }
    dd2_championship_field field = {0};
    if (next.phase == DD2_CHAMPIONSHIP_READY) {
        if (!dd2_championship_field_prepare(session->archive, &next, &field)) {
            return false;
        }
        dd2_championship_field_destroy(session->field);
        session->field = field;
    }
    session->state = next;
    return true;
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
