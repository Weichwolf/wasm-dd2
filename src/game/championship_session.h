#ifndef DD2_GAME_CHAMPIONSHIP_SESSION_H
#define DD2_GAME_CHAMPIONSHIP_SESSION_H

#include "assets/archive.h"
#include "assets/track.h"
#include "game/championship.h"
#include "game/driving.h"

typedef struct dd2_championship_session dd2_championship_session;
typedef struct dd2_championship_transition dd2_championship_transition;

/* Owns progression, decoded track and physical field. The immutable archive
 * and its bytes must outlive the session. No result/state injection API.
 * Creation starts the first scheduled countdown with the league's assigned
 * physical slots, keeping human driver zero stable. */
dd2_championship_session *dd2_championship_session_create(const dd2_archive *archive,
                                                          dd2_race_mode mode);
void dd2_championship_session_destroy(dd2_championship_session *session);
/* Advances real driving and consumes naturally frozen results once. Further
 * result-screen frames hold all simulation and scores until continuation,
 * validate bounded driving input and clear the previous sound-event batch. */
bool dd2_championship_session_advance(dd2_championship_session *session, dd2_driving_frame frame);
/* Prepares the complete next track/grid/countdown before replacing any owned
 * state. Failed asset loading/allocation preserves the current results screen.
 * Champion/elimination retains the last field for presentation. */
bool dd2_championship_session_continue(dd2_championship_session *session);
/* Own a candidate continuation without changing the session. The session must
 * outlive this transition. Candidate views let the main-thread renderer prepare
 * materials/camera before releasing its old borrowed resources. No driving or
 * progression mutations may intervene between the final current check and
 * commit. A successful commit consumes the transition without allocations;
 * rejected commits retain it for explicit destruction. */
dd2_championship_transition *dd2_championship_session_prepare(dd2_championship_session *session);
/* Prepare an unscored restart of the active round, with the same assigned grid.
 * Results already consumed into the league cannot be restarted. */
dd2_championship_transition *
dd2_championship_session_prepare_restart(dd2_championship_session *session);
void dd2_championship_transition_destroy(dd2_championship_transition *transition);
bool dd2_championship_transition_current(const dd2_championship_session *session,
                                         const dd2_championship_transition *transition);
bool dd2_championship_session_commit(dd2_championship_session *session,
                                     dd2_championship_transition *transition);
const dd2_championship *
dd2_championship_transition_state(const dd2_championship_transition *transition);
const dd2_track *dd2_championship_transition_track(const dd2_championship_transition *transition);
const dd2_driving *
dd2_championship_transition_driving(const dd2_championship_transition *transition);
unsigned dd2_championship_transition_level(const dd2_championship_transition *transition);
bool dd2_championship_session_abort(dd2_championship_session *session);
void dd2_championship_session_suspend(dd2_championship_session *session);
/* Views remain owned by session; a successful continuation invalidates track
 * and driving views. Destruction invalidates every view. */
const dd2_championship *dd2_championship_session_state(const dd2_championship_session *session);
const dd2_track *dd2_championship_session_track(const dd2_championship_session *session);
const dd2_driving *dd2_championship_session_driving(const dd2_championship_session *session);
/* Loaded field number, including result screens; progression's next scheduled
 * track may already differ after result consumption. */
unsigned dd2_championship_session_level(const dd2_championship_session *session);

#endif
