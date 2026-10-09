#ifndef DD2_GAME_APPLICATION_H
#define DD2_GAME_APPLICATION_H

#include "assets/save_card.h"
#include "assets/track.h"
#include "game/championship.h"
#include "game/driving.h"
#include "render/renderer.h"

/* Sole main-thread application. Open owns the archive, window, audio and game
 * resources; a second open is rejected. Run also owns the event loop. Close
 * cancels that owned browser loop before releasing its callback context. */
int dd2_application_open(const char *path, int level);
/* Close polls completed storage first and returns zero while it is still
 * pending. Retain/poll the application and retry; no pending owner is freed. */
int dd2_application_close(void);
int dd2_application_run(const char *path, int level);
/* Hosts with their own Native event loop may pump the same real SDL frame used
 * by run. Returns 1 while running, 0 on requested close, -1 on failure. Storage
 * still requires the usual explicit close/pending lifetime handling. */
int dd2_application_poll_frame(void);
/* Shared analog/keyboard control path. Bounded frame input advances actual game
 * state and audio; pause holds it. The caller suspends on lost focus. Present
 * renders the current state without advancing it. No score/state injection. */
int dd2_application_advance(dd2_driving_frame frame);
int dd2_application_present(void);
/* Borrowed read-only views expire on selection, restart, continuation or close. */
const dd2_driving *dd2_application_driving_view(void);
const dd2_track *dd2_application_track_view(void);
const dd2_championship *dd2_application_championship_view(void);
/* Bottom-first RGBA owned by the renderer, valid until next present/close. */
typedef struct {
    const uint8_t *pixels;
    dd2_render_options viewport;
} dd2_application_image;
dd2_application_image dd2_application_image_view(void);

/* Main-thread UI bridge shared with the browser page. Selection is transactional:
 * failure preserves the previous track/materials/camera. No original addresses
 * or register state are used. A stopped/uninitialized application returns zero. */
int dd2_application_select_level(int number);
/* Select only outside driving/championships, dialogs and pending storage work.
 * Prepare the complete field before publishing. Selection survives track/mode
 * changes and championship rounds, but saved class restoration is separate. */
int dd2_application_select_car(int car_class);
int dd2_application_current_car(void);                /* -1 when closed. */
unsigned dd2_application_car_rating(unsigned rating); /* Acceleration, speed, grip: 0..2. */
int dd2_application_show_car(int car);
int dd2_application_current_level(void);
int dd2_application_current_view(void);
/* Native: load beside Dirinfo from Redbook/trackNN.cdda. Browser: load the
 * staged /Music.cdda local file. Failure preserves the current playing track.
 * Phase -1 means no device/application; otherwise dd2_music_phase. */
int dd2_application_load_music(unsigned track);
int dd2_application_music_phase(void);
unsigned dd2_application_music_track(void);
unsigned dd2_application_music_frame(void);
unsigned dd2_application_music_fraction(void);
unsigned dd2_application_music_rate(void);
int dd2_application_set_music_playing(int playing);
int dd2_application_set_music_gain(unsigned gain);
int dd2_application_set_effects_gain(unsigned gain);
unsigned dd2_application_music_gain(void);
unsigned dd2_application_effects_gain(void);
/* Explicit dedicated Native directory / browser database, never original
 * asset paths. Open/save/delete/reload begin requests; poll confirms completion.
 * A preference load consumes the selected physical payload, validates it and
 * applies both audio gains together, retaining all source profile fields for
 * subsequent edits. Other selections, records and saved-game restoration still
 * need their own consumers; GAME/REPLAY are rejected by this audio action. */
int dd2_application_saves_open(const char *location);
/* The final receipt survives application close until the next open. */
int dd2_application_saves_poll(void);
int dd2_application_saves_phase(void);
unsigned dd2_application_saves_count(void);
const dd2_save_card *dd2_application_saves_view(void);
const char *dd2_application_save_name(unsigned logical);
int dd2_application_save_kind(unsigned logical);
int dd2_application_save_preferences(unsigned logical, const char *name);
int dd2_application_load_preferences(unsigned logical);
int dd2_application_delete_save(unsigned logical);
int dd2_application_reload_saves(void);
/* Full identity/audio actions consume player zero's source name, preserving
 * eleven-character legacy names; new edits accept eight printable ASCII chars.
 * Other configuration/game fields remain retained and unapplied. Active
 * championships keep their human identity until exit. Audio-only load remains
 * independent. A save captures the current identity, not dormant imported text. */
const char *dd2_application_player_name(void);
const char *dd2_application_driver_name(unsigned driver);
int dd2_application_set_player_name(const char *name);
int dd2_application_save_profile(unsigned logical, const char *name);
int dd2_application_load_profile(unsigned logical);
/* Read-only keyboard frontend observations. No state injection. */
int dd2_application_profile_phase(void);
const char *dd2_application_profile_draft(void);
unsigned dd2_application_profile_slot(void);
typedef enum {
    DD2_EFFECT_QUERY_SAMPLE,
    DD2_EFFECT_QUERY_PLAYING,
    DD2_EFFECT_QUERY_FRAME,
    DD2_EFFECT_QUERY_FRACTION,
    DD2_EFFECT_QUERY_FREQUENCY,
    DD2_EFFECT_QUERY_GAIN,
    DD2_EFFECT_QUERY_PAN,
    DD2_EFFECT_QUERY_LOOP,
    DD2_EFFECT_QUERY_COUNT
} dd2_effect_query;
/* Read-only voice state for the UI/verification; invalid channel/field is -1. */
/* Flattened UI query: channel * DD2_EFFECT_QUERY_COUNT + field. */
int dd2_application_effect_voice(unsigned query);
unsigned dd2_application_sound_cues(unsigned cue);
unsigned dd2_application_collision_count(void);
unsigned dd2_application_pair_collision_count(void);
unsigned dd2_application_vehicle_count(void);
double dd2_application_engine_health(void);
double dd2_application_region_damage(unsigned region);
unsigned dd2_application_accident_points(void);
unsigned dd2_application_destructions(void);
unsigned dd2_application_accident_windows(void);
unsigned dd2_application_current_lap(void);
unsigned dd2_application_required_laps(void);
unsigned dd2_application_completed_laps(void);
unsigned dd2_application_lap_steps(void);
unsigned dd2_application_last_lap_steps(void);
unsigned dd2_application_best_lap_steps(void);
int dd2_application_laps_finished(void);
void dd2_application_reset_camera(void);
void dd2_application_release_input(void);
void dd2_application_resume_input(void);
int dd2_application_set_driving(int enabled);
int dd2_application_set_paused(int paused);
int dd2_application_is_paused(void);
/* Actual single-player scheduled championships. Entry/continuation/restart
 * prepare renderer resources before replacing owned fields. Exit restores the
 * previous practice track without scoring an unfinished round. */
int dd2_application_start_championship(int mode);
int dd2_application_continue_championship(void);
int dd2_application_restart_championship(void);
int dd2_application_exit_championship(void);
int dd2_application_championship_phase(void);
unsigned dd2_application_championship_points(unsigned driver);
unsigned dd2_application_championship_division(void);
unsigned dd2_application_championship_round(void);
unsigned dd2_application_championship_season(void);

/* Start a fresh race: 0 Wrecking (including arenas), 1 Stockcar, 2 Time Trial (circuits),
 * 3 Total Destruction (arenas).
 * Phase -1 means no race; otherwise dd2_race_phase. Results remain visible
 * until reset, view change or another race/track selection. */
int dd2_application_start_race(int mode);
int dd2_application_withdraw_race(void);
int dd2_application_race_phase(void);
unsigned dd2_application_race_steps(void);
unsigned dd2_application_race_place(void);
unsigned dd2_application_race_points(void);
unsigned dd2_application_survival_steps(void);
unsigned dd2_application_race_alive(void);

#endif
