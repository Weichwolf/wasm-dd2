#include "game/application.h"

#include "assets/archive.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/save_card.h"
#include "assets/save_profile.h"
#include "assets/track.h"
#include "audio/effects.h"
#include "audio/mixer.h"
#include "game/audio.h"
#include "game/championship.h"
#include "game/championship_session.h"
#include "game/configuration.h"
#include "game/course.h"
#include "game/drivers.h"
#include "game/driving.h"
#include "game/laps.h"
#include "game/league.h"
#include "game/profile_menu.h"
#include "game/race.h"
#include "game/sound_events.h"
#include "physics/damage.h"
#include "physics/vehicle.h"
#include "platform/file.h"
#include "platform/save_location.h"
#include "platform/save_store.h"
#include "platform/window.h"
#include "render/camera.h"
#include "render/driving_draw.h"
#include "render/mesh_draw.h"
#include "render/race_draw.h"
#include "render/renderer.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

enum {
    DD2_APP_WIDTH = 640,
    DD2_APP_HEIGHT = 480,
    DD2_APP_RACING_LEVELS = 7,
    DD2_APP_CHAMP_WRECK_VIEW = 7,
    DD2_APP_CHAMP_STOCK_VIEW = 8
};
static const float dd2_app_wheel_seconds = 0.15F;

typedef struct {
    dd2_file file;
    dd2_archive *archive;
    dd2_track *track;
    dd2_renderer *renderer;
    dd2_mesh_materials *materials;
    dd2_window *window;
    dd2_camera camera;
    dd2_driving *driving;
    dd2_championship_session *championship;
    int practice_level;
    dd2_game_audio *audio;
    dd2_configuration configuration;
    char player_name[DD2_SAVE_PROFILE_PLAYER_NAME];
    dd2_profile_menu profile_menu;
    dd2_save_store *saves;
    char save_names[DD2_SAVE_CARD_SLOTS][DD2_SAVE_CARD_NAME_LIMIT + 1];
    bool drive;
    bool paused;
    int level;
    bool car;
    bool running;
    bool dirty;
    bool world_drawn;
    bool failed;
    bool main_loop;
} dd2_application;

/* Sole active application. The callback retains its typed ownership context;
 * exports address it only on the main thread, and destruction clears the bridge. */
static dd2_application *dd2_current_application;
/* Keep a terminal storage receipt after close, so a final event-loop close
 * cannot erase completion before the UI observes it. New applications reset it. */
static dd2_save_store_result dd2_application_last_save_result;

static const dd2_driving *dd2_application_driving(const dd2_application *application) {
    if (application == NULL) {
        return NULL;
    }
    return application->championship == NULL
               ? application->driving
               : dd2_championship_session_driving(application->championship);
}

static const dd2_track *dd2_application_track(const dd2_application *application) {
    if (application == NULL) {
        return NULL;
    }
    return application->championship == NULL
               ? application->track
               : dd2_championship_session_track(application->championship);
}

static void dd2_application_suspend(dd2_application *application) {
    if (application->championship != NULL) {
        dd2_championship_session_suspend(application->championship);
    } else {
        dd2_driving_suspend(application->driving);
    }
}

static bool dd2_application_fit(dd2_camera *camera, const dd2_track *track, bool car) {
    return car ? dd2_camera_fit_mesh(camera, dd2_track_car(track))
               : dd2_camera_fit_scene(camera, dd2_track_scene(track));
}

static dd2_mesh_materials *dd2_application_materials(const dd2_track *track, dd2_camera *camera) {
    dd2_mesh_materials *materials =
        dd2_mesh_materials_create(dd2_track_level(track), dd2_track_textures(track));
    if (materials == NULL || !dd2_application_fit(camera, track, false)) {
        dd2_mesh_materials_destroy(materials);
        return NULL;
    }
    return materials;
}

typedef struct {
    bool drive;
    bool car;
    bool racing;
    bool visible_track;
    dd2_race_mode mode;
} dd2_application_practice;

static int dd2_application_restore_practice(dd2_application *application,
                                            dd2_application_practice choice) {
    dd2_renderer_make_current(application->renderer);
    const unsigned level =
        choice.visible_track ? (unsigned)application->level : (unsigned)application->practice_level;
    dd2_track *track =
        choice.visible_track ? dd2_track_create(application->archive, level) : application->track;
    dd2_driving *driving = dd2_driving_create(dd2_track_road(track), level);
    dd2_camera camera = {0};
    dd2_mesh_materials *materials = dd2_application_materials(track, &camera);
    if (driving == NULL || materials == NULL || !dd2_application_fit(&camera, track, choice.car) ||
        (choice.racing && !dd2_driving_set_race(driving, true, choice.mode))) {
        dd2_mesh_materials_destroy(materials);
        dd2_driving_destroy(driving);
        if (choice.visible_track) {
            dd2_track_destroy(track);
        }
        return 0;
    }
    dd2_mesh_materials_destroy(application->materials);
    dd2_championship_session_destroy(application->championship);
    dd2_driving_destroy(application->driving);
    if (choice.visible_track) {
        dd2_track_destroy(application->track);
        application->track = track;
        application->practice_level = (int)level;
    }
    application->championship = NULL;
    application->driving = driving;
    application->materials = materials;
    application->camera = camera;
    application->level = application->practice_level;
    application->drive = choice.drive;
    application->car = choice.car;
    application->paused = false;
    application->dirty = true;
    dd2_window_release_input(application->window);
    dd2_game_audio_reset_effects(application->audio);
    return 1;
}

int dd2_application_start_championship(int mode) {
    dd2_application *application = dd2_current_application;
    if (application == NULL || (mode != DD2_RACE_WRECKING && mode != DD2_RACE_STOCKCAR)) {
        return 0;
    }
    dd2_renderer_make_current(application->renderer);
    dd2_championship_session *session =
        dd2_championship_session_create(application->archive, (dd2_race_mode)mode);
    if (session == NULL) {
        return 0;
    }
    dd2_camera camera = {0};
    dd2_mesh_materials *materials =
        dd2_application_materials(dd2_championship_session_track(session), &camera);
    if (materials == NULL) {
        dd2_championship_session_destroy(session);
        return 0;
    }
    dd2_mesh_materials_destroy(application->materials);
    dd2_championship_session_destroy(application->championship);
    application->championship = session;
    application->materials = materials;
    application->camera = camera;
    application->level = (int)dd2_championship_session_level(session);
    application->drive = true;
    application->car = false;
    application->paused = false;
    application->dirty = true;
    dd2_application_suspend(application);
    dd2_window_release_input(application->window);
    dd2_game_audio_reset_effects(application->audio);
    return 1;
}

static int dd2_application_championship_transition(dd2_application *application, bool restart) {
    if (application == NULL || application->championship == NULL) {
        return 0;
    }
    dd2_championship_transition *transition =
        restart ? dd2_championship_session_prepare_restart(application->championship)
                : dd2_championship_session_prepare(application->championship);
    if (transition == NULL) {
        return 0;
    }
    dd2_renderer_make_current(application->renderer);
    dd2_camera camera = application->camera;
    const dd2_track *track = dd2_championship_transition_track(transition);
    const bool replaces_track = track != dd2_application_track(application);
    dd2_mesh_materials *materials = application->materials;
    if (replaces_track) {
        materials = dd2_application_materials(track, &camera);
    }
    if (materials == NULL ||
        !dd2_championship_transition_current(application->championship, transition)) {
        if (replaces_track) {
            dd2_mesh_materials_destroy(materials);
        }
        dd2_championship_transition_destroy(transition);
        return 0;
    }
    if (replaces_track) {
        dd2_mesh_materials_destroy(application->materials);
    }
    /* Main-thread synchronous preparation: no owner mutation between the
     * checked transition and commit, which cannot allocate or fail here. */
    if (!dd2_championship_session_commit(application->championship, transition)) {
        if (replaces_track) {
            dd2_mesh_materials_destroy(materials);
            application->materials = NULL;
        }
        dd2_championship_transition_destroy(transition);
        application->failed = true;
        return 0;
    }
    application->materials = materials;
    application->camera = camera;
    application->level = (int)dd2_championship_session_level(application->championship);
    application->paused = false;
    application->dirty = true;
    dd2_window_release_input(application->window);
    dd2_game_audio_reset_effects(application->audio);
    return 1;
}

int dd2_application_continue_championship(void) {
    return dd2_application_championship_transition(dd2_current_application, false);
}
int dd2_application_restart_championship(void) {
    return dd2_application_championship_transition(dd2_current_application, true);
}

int dd2_application_exit_championship(void) {
    dd2_application *application = dd2_current_application;
    if (application == NULL || application->championship == NULL) {
        return 0;
    }
    return dd2_application_restore_practice(application, (dd2_application_practice){0});
}

const dd2_championship *dd2_application_championship_view(void) {
    return dd2_current_application == NULL
               ? NULL
               : dd2_championship_session_state(dd2_current_application->championship);
}
int dd2_application_championship_phase(void) {
    const dd2_championship *state = dd2_application_championship_view();
    return state == NULL ? -1 : (int)state->phase;
}
unsigned dd2_application_championship_points(unsigned driver) {
    const dd2_championship *state = dd2_application_championship_view();
    return state == NULL || driver >= DD2_LEAGUE_DRIVERS ? 0 : state->league.drivers[driver].points;
}
unsigned dd2_application_championship_division(void) {
    const dd2_championship *state = dd2_application_championship_view();
    return state == NULL ? 0 : state->league.drivers[0].division + 1;
}
unsigned dd2_application_championship_round(void) {
    const dd2_championship *state = dd2_application_championship_view();
    const dd2_championship_season *season = dd2_championship_current(state);
    return season == NULL ? 0
                          : season->completed + (unsigned)(state->phase == DD2_CHAMPIONSHIP_RACING);
}
unsigned dd2_application_championship_season(void) {
    const dd2_championship_season *season =
        dd2_championship_current(dd2_application_championship_view());
    if (season == NULL) {
        return 0;
    }
    return (unsigned)(season->number < UINT_MAX ? season->number + 1 : UINT_MAX);
}

static void dd2_application_destroy(dd2_application *application) {
    if (application == NULL) {
        return;
    }
    if (dd2_current_application == application) {
        dd2_current_application = NULL;
    }
    dd2_save_store_destroy(application->saves);
    dd2_mesh_materials_destroy(application->materials);
    dd2_game_audio_destroy(application->audio);
    dd2_renderer_destroy(application->renderer);
    dd2_championship_session_destroy(application->championship);
    dd2_driving_destroy(application->driving);
    dd2_track_destroy(application->track);
    dd2_archive_close(application->archive);
    dd2_file_release(&application->file);
    dd2_window_destroy(application->window);
    free(application);
}

int dd2_application_select_level(int number) {
    dd2_application *application = dd2_current_application;
    if (application == NULL || number < 1 || number > DD2_TRACK_COUNT) {
        return 0;
    }
    dd2_track *track = dd2_track_create(application->archive, (unsigned)number);
    dd2_mesh_materials *materials =
        dd2_mesh_materials_create(dd2_track_level(track), dd2_track_textures(track));
    dd2_camera camera = {0};
    dd2_driving *driving = dd2_driving_create(dd2_track_road(track), (unsigned)number);
    const dd2_race *previous_race = dd2_driving_race(dd2_application_driving(application));
    if (driving != NULL && previous_race != NULL) {
        dd2_race_mode mode = previous_race->rules.mode;
        if ((number <= DD2_APP_RACING_LEVELS && mode == DD2_RACE_TOTAL_DESTRUCTION) ||
            (number > DD2_APP_RACING_LEVELS && mode != DD2_RACE_TOTAL_DESTRUCTION)) {
            mode = DD2_RACE_WRECKING;
        }
        if (!dd2_driving_set_race(driving, true, mode)) {
            dd2_driving_destroy(driving);
            driving = NULL;
        }
    }
    if (driving == NULL || materials == NULL ||
        !dd2_application_fit(&camera, track, application->car)) {
        dd2_mesh_materials_destroy(materials);
        dd2_driving_destroy(driving);
        dd2_track_destroy(track);
        return 0;
    }
    dd2_mesh_materials_destroy(application->materials);
    dd2_championship_session_destroy(application->championship);
    application->championship = NULL;
    dd2_driving_destroy(application->driving);
    dd2_track_destroy(application->track);
    application->track = track;
    application->driving = driving;
    application->materials = materials;
    application->camera = camera;
    application->level = number;
    application->practice_level = number;
    application->dirty = true;
    dd2_game_audio_reset_effects(application->audio);
    return 1;
}

int dd2_application_show_car(int car) {
    dd2_application *application = dd2_current_application;
    if (application == NULL || (car != 0 && car != 1)) {
        return 0;
    }
    if (application->championship != NULL) {
        return dd2_application_restore_practice(
            application, (dd2_application_practice){.car = car != 0, .visible_track = true});
    }
    if (!dd2_application_fit(&application->camera, application->track, car != 0)) {
        return 0;
    }
    application->drive = false;
    dd2_game_audio_reset_effects(application->audio);
    dd2_application_suspend(application);
    application->car = car != 0;
    application->dirty = true;
    return 1;
}

int dd2_application_current_level(void) {
    return dd2_current_application != NULL ? dd2_current_application->level : 0;
}

int dd2_application_current_view(void) {
    const dd2_application *application = dd2_current_application;
    if (application == NULL) {
        return -1;
    }
    const dd2_race *race = dd2_driving_race(dd2_application_driving(application));
    if (application->championship != NULL) {
        return dd2_championship_session_state(application->championship)->mode == DD2_RACE_WRECKING
                   ? DD2_APP_CHAMP_WRECK_VIEW
                   : DD2_APP_CHAMP_STOCK_VIEW;
    }
    if (!application->drive) {
        return (int)application->car;
    }
    if (race == NULL) {
        return 2;
    }
    return (int)race->rules.mode + 3;
}

int dd2_application_load_music(unsigned track) {
    dd2_application *application = dd2_current_application;
    if (application == NULL) {
        return 0;
    }
#ifdef __EMSCRIPTEN__
    return (int)dd2_game_audio_music_load(application->audio, track, "/Music.cdda");
#else
    return (int)dd2_game_audio_music_load(application->audio, track, NULL);
#endif
}

int dd2_application_music_phase(void) {
    dd2_application *application = dd2_current_application;
    return application == NULL || application->audio == NULL
               ? -1
               : (int)dd2_game_audio_music_state(application->audio).phase;
}

unsigned dd2_application_music_track(void) {
    return dd2_game_audio_music_state(
               dd2_current_application == NULL ? NULL : dd2_current_application->audio)
        .track;
}

unsigned dd2_application_music_frame(void) {
    return (unsigned)dd2_game_audio_music_state(
               dd2_current_application == NULL ? NULL : dd2_current_application->audio)
        .frame;
}

unsigned dd2_application_music_fraction(void) {
    return dd2_game_audio_music_state(
               dd2_current_application == NULL ? NULL : dd2_current_application->audio)
        .fraction;
}

unsigned dd2_application_music_rate(void) {
    return dd2_game_audio_rate(dd2_current_application == NULL ? NULL
                                                               : dd2_current_application->audio);
}

int dd2_application_set_music_playing(int playing) {
    return dd2_current_application != NULL && (playing == 0 || playing == 1)
               ? (int)dd2_game_audio_music_play(dd2_current_application->audio, playing != 0)
               : 0;
}

int dd2_application_set_music_gain(unsigned gain) {
    dd2_application *application = dd2_current_application;
    if (application == NULL || gain > DD2_MIXER_GAIN_ONE ||
        (application->audio != NULL && !dd2_game_audio_music_gain(application->audio, gain))) {
        return 0;
    }
    return (int)dd2_configuration_set_music(&application->configuration, gain);
}

int dd2_application_set_effects_gain(unsigned gain) {
    dd2_application *application = dd2_current_application;
    if (application == NULL || gain > DD2_MIXER_GAIN_ONE ||
        (application->audio != NULL && !dd2_game_audio_effects_gain(application->audio, gain))) {
        return 0;
    }
    return (int)dd2_configuration_set_effects(&application->configuration, gain);
}

unsigned dd2_application_music_gain(void) {
    const dd2_application *application = dd2_current_application;
    if (application == NULL) {
        return 0;
    }
    return application->audio == NULL ? application->configuration.music_gain
                                      : dd2_game_audio_music_state(application->audio).gain;
}
unsigned dd2_application_effects_gain(void) {
    return dd2_current_application == NULL
               ? 0
               : dd2_configuration_effects_gain(&dd2_current_application->configuration);
}
int dd2_application_saves_open(const char *location) {
    dd2_application *application = dd2_current_application;
    if (application == NULL) {
        return 0;
    }
    if (application->saves == NULL) {
        application->saves = dd2_save_store_create();
    }
    return (int)dd2_save_store_open(application->saves, location);
}
int dd2_application_saves_poll(void) {
    if (dd2_current_application != NULL && dd2_current_application->saves != NULL) {
        dd2_application_last_save_result = dd2_save_store_poll(dd2_current_application->saves);
    }
    return (int)dd2_application_last_save_result;
}
int dd2_application_saves_phase(void) {
    return (int)dd2_save_store_state(
        dd2_current_application == NULL ? NULL : dd2_current_application->saves);
}
const dd2_save_card *dd2_application_saves_view(void) {
    return dd2_save_store_view(dd2_current_application == NULL ? NULL
                                                               : dd2_current_application->saves);
}
unsigned dd2_application_saves_count(void) {
    return dd2_save_card_count(dd2_application_saves_view());
}
const char *dd2_application_save_name(unsigned logical) {
    dd2_save_card_entry entry = {0};
    if (dd2_current_application == NULL ||
        !dd2_save_card_get(dd2_application_saves_view(), logical, &entry)) {
        return NULL;
    }
    for (unsigned index = 0; index < sizeof(entry.name); ++index) {
        dd2_current_application->save_names[logical][index] = entry.name[index];
    }
    return dd2_current_application->save_names[logical];
}
int dd2_application_save_kind(unsigned logical) {
    dd2_save_card_entry entry = {0};
    return dd2_save_card_get(dd2_application_saves_view(), logical, &entry)
               ? (int)dd2_read_le16(entry.payload.data)
               : 0;
}
int dd2_application_save_preferences(unsigned logical, const char *name) {
    dd2_application *application = dd2_current_application;
    uint8_t block[DD2_SAVE_CARD_BLOCK_BYTES];
    return application != NULL &&
           dd2_configuration_write(&application->configuration,
                                   (dd2_byte_buffer){.data = block, .size = sizeof(block)}) &&
           dd2_save_store_put(application->saves, logical, name,
                              (dd2_byte_view){.data = block, .size = sizeof(block)});
}
int dd2_application_load_preferences(unsigned logical) {
    dd2_application *application = dd2_current_application;
    dd2_configuration next;
    dd2_save_card_entry entry = {0};
    if (application == NULL || dd2_application_saves_phase() != DD2_SAVE_STORE_READY ||
        !dd2_save_card_get(dd2_application_saves_view(), logical, &entry) ||
        !dd2_configuration_read(entry.payload, application->configuration.music_gain, &next)) {
        return 0;
    }
    const dd2_audio_gains gains = {.effects = dd2_configuration_effects_gain(&next),
                                   .music = next.music_gain};
    if (application->audio != NULL && !dd2_game_audio_apply_gains(application->audio, gains)) {
        return 0;
    }
    application->configuration = next;
    return 1;
}
int dd2_application_delete_save(unsigned logical) {
    return dd2_current_application != NULL &&
           dd2_save_store_delete(dd2_current_application->saves, logical);
}
int dd2_application_reload_saves(void) {
    return dd2_current_application != NULL && dd2_save_store_reload(dd2_current_application->saves);
}

const char *dd2_application_player_name(void) {
    return dd2_current_application == NULL ? NULL : dd2_current_application->player_name;
}
const char *dd2_application_driver_name(unsigned driver) {
    if (dd2_current_application == NULL) {
        return NULL;
    }
    return driver == 0 ? dd2_current_application->player_name : dd2_driver_name(driver);
}
static void dd2_application_copy_player(dd2_application *application, const char *name) {
    unsigned index = 0;
    do {
        application->player_name[index] = name[index];
    } while (name[index++] != '\0');
}
int dd2_application_set_player_name(const char *name) {
    dd2_application *application = dd2_current_application;
    if (application == NULL || application->championship != NULL ||
        !dd2_configuration_name_valid(name) ||
        !dd2_configuration_set_player(&application->configuration, name)) {
        return 0;
    }
    dd2_application_copy_player(application, dd2_configuration_player(&application->configuration));
    application->dirty = true;
    return 1;
}
int dd2_application_save_profile(unsigned logical, const char *name) {
    dd2_application *application = dd2_current_application;
    uint8_t block[DD2_SAVE_CARD_BLOCK_BYTES];
    if (application == NULL) {
        return 0;
    }
    dd2_configuration next = application->configuration;
    return dd2_configuration_set_player(&next, application->player_name) &&
           dd2_configuration_write(&next,
                                   (dd2_byte_buffer){.data = block, .size = sizeof(block)}) &&
           dd2_save_store_put(application->saves, logical, name,
                              (dd2_byte_view){.data = block, .size = sizeof(block)});
}
int dd2_application_load_profile(unsigned logical) {
    dd2_application *application = dd2_current_application;
    dd2_configuration next;
    dd2_save_card_entry entry = {0};
    if (application == NULL || application->championship != NULL ||
        dd2_application_saves_phase() != DD2_SAVE_STORE_READY ||
        !dd2_save_card_get(dd2_application_saves_view(), logical, &entry) ||
        !dd2_configuration_read(entry.payload, application->configuration.music_gain, &next)) {
        return 0;
    }
    const char *name = dd2_configuration_player(&next);
    const dd2_audio_gains gains = {.effects = dd2_configuration_effects_gain(&next),
                                   .music = next.music_gain};
    if (name == NULL ||
        (application->audio != NULL && !dd2_game_audio_apply_gains(application->audio, gains))) {
        return 0;
    }
    dd2_application_copy_player(application, name);
    application->configuration = next;
    application->dirty = true;
    return 1;
}
int dd2_application_profile_phase(void) {
    return dd2_current_application == NULL ? DD2_PROFILE_CLOSED
                                           : (int)dd2_current_application->profile_menu.phase;
}
const char *dd2_application_profile_draft(void) {
    return dd2_current_application == NULL ? NULL : dd2_current_application->profile_menu.draft;
}
unsigned dd2_application_profile_slot(void) {
    return dd2_current_application == NULL ? 0 : dd2_current_application->profile_menu.logical;
}
static void dd2_application_profile_message(dd2_application *application, const char *message) {
    application->profile_menu.phase = DD2_PROFILE_MESSAGE;
    application->profile_menu.message = message;
    dd2_window_text_input(application->window, false);
    application->dirty = true;
}
static const char *dd2_application_profile_error(int result) {
    switch (result) {
    case DD2_SAVE_STORE_CONFLICT:
        return "SAVES CHANGED. F4 RELOADS BEFORE RETRY.";
    case DD2_SAVE_STORE_INDETERMINATE:
        return "SAVE UNCONFIRMED. F4 RELOADS SAVES.";
    case DD2_SAVE_STORE_DUPLICATE:
        return "THAT NAME BELONGS TO ANOTHER ENTRY.";
    case DD2_SAVE_STORE_FULL:
        return "ALL FIFTEEN ENTRIES ARE OCCUPIED.";
    case DD2_SAVE_STORE_INVALID:
        return "STORED DATA IS INVALID.";
    default:
        return "SAVE ACTION FAILED. PREVIOUS DATA KEPT.";
    }
}
static void dd2_application_profile_poll(dd2_application *application) {
    dd2_profile_menu *menu = &application->profile_menu;
    if (menu->phase != DD2_PROFILE_OPEN_SAVE && menu->phase != DD2_PROFILE_OPEN_LOAD &&
        menu->phase != DD2_PROFILE_OPEN_DELETE && menu->phase != DD2_PROFILE_WRITING &&
        menu->phase != DD2_PROFILE_DELETING) {
        return;
    }
    const int result = dd2_application_saves_poll();
    if (result == DD2_SAVE_STORE_PENDING) {
        return;
    }
    if (result != DD2_SAVE_STORE_OK) {
        dd2_application_profile_message(application, dd2_application_profile_error(result));
    } else if (menu->phase == DD2_PROFILE_WRITING) {
        dd2_application_profile_message(application, "AUDIO AND PLAYER SAVED.");
    } else if (menu->phase == DD2_PROFILE_DELETING) {
        dd2_application_profile_message(application, "SAVE ENTRY DELETED.");
    } else {
        if (menu->phase == DD2_PROFILE_OPEN_SAVE) {
            menu->phase = DD2_PROFILE_SAVE_SELECT;
        } else if (menu->phase == DD2_PROFILE_OPEN_DELETE) {
            menu->phase = DD2_PROFILE_DELETE_SELECT;
        } else {
            menu->phase = DD2_PROFILE_LOAD_SELECT;
        }
        menu->message = NULL;
        application->dirty = true;
    }
}
static void dd2_application_profile_open(dd2_application *application, dd2_profile_phase opening) {
    dd2_profile_menu *menu = &application->profile_menu;
    menu->logical = 0;
    menu->message = NULL;
    if (application->saves == NULL) {
        application->saves = dd2_save_store_create();
    }
    const int state = dd2_application_saves_phase();
    const bool save = opening == DD2_PROFILE_OPEN_SAVE;
    if (state == DD2_SAVE_STORE_READY && save) {
        menu->phase = DD2_PROFILE_SAVE_SELECT;
    } else if ((state == DD2_SAVE_STORE_CLOSED && dd2_save_store_open_user(application->saves)) ||
               (!save && (state == DD2_SAVE_STORE_NEEDS_RELOAD || state == DD2_SAVE_STORE_READY) &&
                dd2_application_reload_saves())) {
        menu->phase = opening;
    } else {
        dd2_application_profile_message(application, state == DD2_SAVE_STORE_NEEDS_RELOAD
                                                         ? "F4 RELOADS SAVES BEFORE ANOTHER CHANGE."
                                                         : "SAVES COULD NOT BE OPENED.");
    }
}
static void dd2_application_profile_save(dd2_application *application) {
    dd2_profile_menu *menu = &application->profile_menu;
    if (dd2_application_save_profile(menu->logical, menu->draft)) {
        menu->phase = DD2_PROFILE_WRITING;
        menu->message = "SAVING. WAIT FOR CONFIRMATION.";
    } else {
        dd2_application_profile_message(
            application, dd2_application_profile_error(dd2_application_saves_poll()));
    }
}
static void dd2_application_profile_delete(dd2_application *application) {
    dd2_profile_menu *menu = &application->profile_menu;
    if (dd2_application_delete_save(menu->logical)) {
        menu->phase = DD2_PROFILE_DELETING;
        menu->message = "DELETING. WAIT FOR CONFIRMATION.";
    } else {
        dd2_application_profile_message(
            application, dd2_application_profile_error(dd2_application_saves_poll()));
    }
}
static bool dd2_application_profile_begin(dd2_application *application, const dd2_input *input) {
    if (input->pressed[DD2_KEY_PROFILE_NAME]) {
        if (application->championship != NULL) {
            dd2_application_profile_message(application, "LEAVE THE CHAMPIONSHIP TO CHANGE NAME.");
        } else {
            char draft[DD2_CONFIGURATION_NAME_LIMIT + 1] = {0};
            for (unsigned index = 0; index < DD2_CONFIGURATION_NAME_LIMIT; ++index) {
                draft[index] = application->player_name[index];
                if (draft[index] == '\0') {
                    break;
                }
            }
            dd2_profile_menu_edit(&application->profile_menu, DD2_PROFILE_PLAYER_NAME, draft);
        }
        return true;
    }
    if (input->pressed[DD2_KEY_PROFILE_SAVE] || input->pressed[DD2_KEY_PROFILE_LOAD] ||
        input->pressed[DD2_KEY_PROFILE_DELETE]) {
        dd2_profile_phase opening = DD2_PROFILE_OPEN_LOAD;
        if (input->pressed[DD2_KEY_PROFILE_SAVE]) {
            opening = DD2_PROFILE_OPEN_SAVE;
        } else if (input->pressed[DD2_KEY_PROFILE_DELETE]) {
            opening = DD2_PROFILE_OPEN_DELETE;
        }
        dd2_application_profile_open(application, opening);
        return true;
    }
    return false;
}
static void dd2_application_profile_name_input(dd2_application *application,
                                               const dd2_input *input) {
    dd2_profile_menu *menu = &application->profile_menu;
    if (input->pressed[DD2_KEY_BACKSPACE]) {
        dd2_profile_menu_backspace(menu);
    }
    if (input->text[0] != '\0') {
        menu->message = dd2_profile_menu_append(menu, input->text)
                            ? NULL
                            : "USE AT MOST EIGHT ASCII CHARACTERS.";
    }
    if (!input->pressed[DD2_KEY_DRIVE]) {
        return;
    }
    if (menu->phase == DD2_PROFILE_PLAYER_NAME) {
        if (dd2_application_set_player_name(menu->draft)) {
            menu->phase = DD2_PROFILE_CLOSED;
        } else {
            dd2_application_profile_message(application, "PLAYER NAME COULD NOT BE CHANGED.");
        }
    } else if (menu->draft[0] == '\0') {
        menu->message = "ENTER A SAVE NAME.";
    } else if (menu->logical < dd2_application_saves_count()) {
        menu->phase = DD2_PROFILE_CONFIRM;
        menu->message = NULL;
    } else {
        dd2_application_profile_save(application);
    }
}
static void dd2_application_profile_select_input(dd2_application *application,
                                                 const dd2_input *input) {
    dd2_profile_menu *menu = &application->profile_menu;
    int direction = 0;
    if (input->pressed[DD2_KEY_RIGHT]) {
        direction = 1;
    } else if (input->pressed[DD2_KEY_LEFT]) {
        direction = -1;
    }
    dd2_profile_menu_move(menu, direction);
    if (direction != 0) {
        menu->message = NULL;
    }
    if (!input->pressed[DD2_KEY_DRIVE]) {
        return;
    }
    if (menu->phase == DD2_PROFILE_DELETE_SELECT) {
        if (dd2_application_save_name(menu->logical) == NULL) {
            menu->message = "THAT ENTRY IS EMPTY.";
        } else {
            menu->phase = DD2_PROFILE_DELETE_CONFIRM;
            menu->message = NULL;
        }
    } else if (menu->phase == DD2_PROFILE_LOAD_SELECT) {
        dd2_application_profile_message(application,
                                        dd2_application_load_profile(menu->logical)
                                            ? "AUDIO AND PLAYER RESTORED."
                                            : "ENTRY CANNOT RESTORE AUDIO AND PLAYER.");
    } else {
        const char *name = dd2_application_save_name(menu->logical);
        dd2_profile_menu_edit(menu, DD2_PROFILE_SAVE_NAME,
                              dd2_configuration_name_valid(name) ? name : "CONFIG");
    }
}
static bool dd2_application_profile_busy(dd2_profile_phase phase) {
    return phase == DD2_PROFILE_WRITING || phase == DD2_PROFILE_OPEN_SAVE ||
           phase == DD2_PROFILE_OPEN_LOAD || phase == DD2_PROFILE_OPEN_DELETE ||
           phase == DD2_PROFILE_DELETING;
}
static void dd2_application_profile_command(dd2_application *application, const dd2_input *input) {
    dd2_profile_menu *menu = &application->profile_menu;
    if (input->pressed[DD2_KEY_QUIT]) {
        menu->phase = DD2_PROFILE_CLOSED;
        menu->message = NULL;
    } else if (dd2_profile_menu_editing(menu)) {
        dd2_application_profile_name_input(application, input);
    } else if (menu->phase == DD2_PROFILE_SAVE_SELECT || menu->phase == DD2_PROFILE_LOAD_SELECT ||
               menu->phase == DD2_PROFILE_DELETE_SELECT) {
        dd2_application_profile_select_input(application, input);
    } else if (menu->phase == DD2_PROFILE_CONFIRM && input->pressed[DD2_KEY_DRIVE]) {
        dd2_application_profile_save(application);
    } else if (menu->phase == DD2_PROFILE_DELETE_CONFIRM && input->pressed[DD2_KEY_DRIVE]) {
        dd2_application_profile_delete(application);
    } else if (menu->phase == DD2_PROFILE_MESSAGE && input->pressed[DD2_KEY_DRIVE]) {
        menu->phase = DD2_PROFILE_CLOSED;
    }
}
static bool dd2_application_profile_input(dd2_application *application, const dd2_input *input) {
    dd2_profile_menu *menu = &application->profile_menu;
    if (menu->phase == DD2_PROFILE_CLOSED) {
        if (!input->focused || !dd2_application_profile_begin(application, input)) {
            return false;
        }
    } else if (!dd2_application_profile_busy(menu->phase) && input->focused) {
        dd2_application_profile_command(application, input);
    }
    dd2_window_text_input(application->window, dd2_profile_menu_editing(menu));
    dd2_window_menu_input(application->window, menu->phase != DD2_PROFILE_CLOSED);
    dd2_window_release_input(application->window);
    if (input->action_count != 0 || input->text[0] != '\0' || input->pressed[DD2_KEY_LEFT] ||
        input->pressed[DD2_KEY_RIGHT]) {
        application->dirty = true;
    }
    return true;
}

int dd2_application_effect_voice(unsigned query) {
    const unsigned channel = query / DD2_EFFECT_QUERY_COUNT;
    const dd2_effect_query field = (dd2_effect_query)(query % DD2_EFFECT_QUERY_COUNT);
    if (dd2_current_application == NULL || channel >= DD2_MIXER_CHANNELS) {
        return -1;
    }
    const dd2_effect_voice state =
        dd2_game_audio_effect_voice(dd2_current_application->audio, channel);
    switch (field) {
    case DD2_EFFECT_QUERY_SAMPLE:
        return (int)state.sample;
    case DD2_EFFECT_QUERY_PLAYING:
        return (int)state.voice.playing;
    case DD2_EFFECT_QUERY_FRAME:
        return (int)state.voice.frame;
    case DD2_EFFECT_QUERY_FRACTION:
        return (int)state.voice.fraction;
    case DD2_EFFECT_QUERY_FREQUENCY:
        return (int)state.voice.config.frequency;
    case DD2_EFFECT_QUERY_GAIN:
        return (int)state.voice.config.gain;
    case DD2_EFFECT_QUERY_PAN:
        return state.voice.config.pan;
    case DD2_EFFECT_QUERY_LOOP:
        return (int)state.voice.config.loop;
    default:
        return -1;
    }
}

unsigned dd2_application_sound_cues(unsigned cue) {
    const uint64_t count =
        dd2_current_application == NULL
            ? 0
            : dd2_game_audio_cue_count(dd2_current_application->audio, (dd2_sound_cue)cue);
    return count < UINT_MAX ? (unsigned)count : UINT_MAX;
}

unsigned dd2_application_collision_count(void) {
    if (dd2_current_application == NULL) {
        return 0;
    }
    const uint64_t count = dd2_driving_collisions(dd2_application_driving(dd2_current_application));
    return count < UINT_MAX ? (unsigned)count : UINT_MAX;
}

unsigned dd2_application_pair_collision_count(void) {
    if (dd2_current_application == NULL) {
        return 0;
    }
    const uint64_t count =
        dd2_driving_pair_collisions(dd2_application_driving(dd2_current_application));
    return count < UINT_MAX ? (unsigned)count : UINT_MAX;
}
unsigned dd2_application_vehicle_count(void) {
    return dd2_current_application != NULL
               ? dd2_driving_vehicle_count(dd2_application_driving(dd2_current_application))
               : 0;
}
double dd2_application_engine_health(void) {
    return dd2_current_application != NULL
               ? dd2_damage_health(
                     &dd2_driving_damage(dd2_application_driving(dd2_current_application))[0])
               : 1;
}
double dd2_application_region_damage(unsigned region) {
    return dd2_current_application != NULL && region < DD2_DAMAGE_REGIONS
               ? dd2_driving_damage(dd2_application_driving(dd2_current_application))[0]
                     .regions[region]
               : 0;
}
unsigned dd2_application_accident_points(void) {
    return dd2_current_application != NULL
               ? dd2_driving_accidents(dd2_application_driving(dd2_current_application))[0].points
               : 0;
}
unsigned dd2_application_destructions(void) {
    return dd2_current_application != NULL
               ? dd2_driving_accidents(dd2_application_driving(dd2_current_application))[0]
                     .destructions
               : 0;
}
unsigned dd2_application_accident_windows(void) {
    unsigned active = 0;
    if (dd2_current_application != NULL) {
        const dd2_accident_driver *drivers =
            dd2_driving_accidents(dd2_application_driving(dd2_current_application));
        for (unsigned slot = 0;
             slot < dd2_driving_vehicle_count(dd2_application_driving(dd2_current_application));
             ++slot) {
            active += (unsigned)(drivers[slot].remaining != 0);
        }
    }
    return active;
}

static const dd2_lap_driver *dd2_application_lap(void) {
    return dd2_current_application == NULL
               ? NULL
               : dd2_driving_laps(dd2_application_driving(dd2_current_application));
}
unsigned dd2_application_required_laps(void) {
    return dd2_current_application == NULL ? 0
                                           : dd2_course_laps(dd2_driving_course(
                                                 dd2_application_driving(dd2_current_application)));
}
unsigned dd2_application_current_lap(void) {
    const dd2_lap_driver *lap = dd2_application_lap();
    if (lap == NULL) {
        return 0;
    }
    const unsigned required = dd2_application_required_laps();
    const unsigned current = lap->started_laps == 0 ? 1 : lap->started_laps;
    return required == 0 || current < required ? current : required;
}
unsigned dd2_application_completed_laps(void) {
    return dd2_laps_completed(dd2_application_lap());
}
unsigned dd2_application_lap_steps(void) {
    const dd2_lap_driver *lap = dd2_application_lap();
    if (lap == NULL || lap->started_laps == 0) {
        return 0;
    }
    const dd2_race *race = dd2_driving_race(dd2_application_driving(dd2_current_application));
    uint64_t ticks = lap->finished ? lap->last_lap : lap->steps - lap->lap_start;
    if (race != NULL) {
        ticks = race->drivers[0].current_lap_time;
    }
    return ticks < UINT_MAX ? (unsigned)ticks : UINT_MAX;
}
unsigned dd2_application_last_lap_steps(void) {
    const dd2_lap_driver *lap = dd2_application_lap();
    const uint64_t ticks = lap == NULL ? 0 : lap->last_lap;
    return ticks < UINT_MAX ? (unsigned)ticks : UINT_MAX;
}
unsigned dd2_application_best_lap_steps(void) {
    const dd2_lap_driver *lap = dd2_application_lap();
    const uint64_t ticks = lap == NULL ? 0 : lap->best_lap;
    return ticks < UINT_MAX ? (unsigned)ticks : UINT_MAX;
}
int dd2_application_laps_finished(void) {
    const dd2_lap_driver *lap = dd2_application_lap();
    return (int)(lap != NULL && lap->finished);
}

void dd2_application_reset_camera(void) {
    dd2_application *application = dd2_current_application;
    if (application != NULL) {
        if (application->championship != NULL) {
            dd2_application_restart_championship();
            return;
        }
        application->dirty =
            application->drive
                ? dd2_driving_reset(application->driving)
                : dd2_application_fit(&application->camera, application->track, application->car);
        if (application->dirty && application->drive) {
            dd2_game_audio_reset_effects(application->audio);
        }
    }
}

void dd2_application_release_input(void) {
    if (dd2_current_application != NULL) {
        dd2_window_set_focus(dd2_current_application->window, false);
        dd2_application_suspend(dd2_current_application);
        dd2_game_audio_suspend(dd2_current_application->audio, true);
    }
}

void dd2_application_resume_input(void) {
    if (dd2_current_application != NULL) {
        dd2_window_set_focus(dd2_current_application->window, true);
    }
}

int dd2_application_set_driving(int enabled) {
    dd2_application *application = dd2_current_application;
    if (application == NULL || (enabled != 0 && enabled != 1)) {
        return 0;
    }
    if (application->championship != NULL) {
        return dd2_application_restore_practice(
            application, (dd2_application_practice){.drive = enabled != 0, .visible_track = true});
    }
    if (dd2_driving_race(dd2_application_driving(application)) != NULL &&
        !dd2_driving_set_race(application->driving, false, DD2_RACE_WRECKING)) {
        return 0;
    }
    application->drive = enabled != 0;
    dd2_game_audio_reset_effects(application->audio);
    dd2_application_suspend(application);
    dd2_window_release_input(application->window);
    application->dirty = true;
    return 1;
}

int dd2_application_start_race(int mode) {
    dd2_application *application = dd2_current_application;
    if (application == NULL || mode < DD2_RACE_WRECKING || mode > DD2_RACE_TOTAL_DESTRUCTION) {
        return 0;
    }
    if (application->championship != NULL) {
        return dd2_application_restore_practice(
            application,
            (dd2_application_practice){
                .drive = true, .racing = true, .visible_track = true, .mode = (dd2_race_mode)mode});
    }
    if (!dd2_driving_set_race(application->driving, true, (dd2_race_mode)mode)) {
        return 0;
    }
    application->drive = true;
    dd2_game_audio_reset_effects(application->audio);
    application->paused = false;
    application->dirty = true;
    dd2_window_release_input(application->window);
    return 1;
}

int dd2_application_withdraw_race(void) {
    dd2_application *application = dd2_current_application;
    if (application != NULL && application->championship != NULL) {
        return dd2_application_exit_championship();
    }
    if (application == NULL || !application->drive || !dd2_driving_withdraw(application->driving)) {
        return 0;
    }
    application->dirty = true;
    dd2_window_release_input(application->window);
    return 1;
}

int dd2_application_race_phase(void) {
    const dd2_race *race = dd2_current_application == NULL
                               ? NULL
                               : dd2_driving_race(dd2_application_driving(dd2_current_application));
    return race == NULL ? -1 : (int)race->phase;
}

unsigned dd2_application_race_steps(void) {
    const dd2_race *race = dd2_current_application == NULL
                               ? NULL
                               : dd2_driving_race(dd2_application_driving(dd2_current_application));
    if (race == NULL) {
        return 0;
    }
    return race->steps < UINT_MAX ? (unsigned)race->steps : UINT_MAX;
}

unsigned dd2_application_race_place(void) {
    const dd2_race *race = dd2_current_application == NULL
                               ? NULL
                               : dd2_driving_race(dd2_application_driving(dd2_current_application));
    return race == NULL ? 0 : race->drivers[0].place;
}

unsigned dd2_application_race_points(void) {
    const dd2_race *race = dd2_current_application == NULL
                               ? NULL
                               : dd2_driving_race(dd2_application_driving(dd2_current_application));
    return race == NULL || race->phase != DD2_RACE_RESULTS ? 0 : race->drivers[0].total_points;
}

unsigned dd2_application_survival_steps(void) {
    const dd2_race *race = dd2_current_application == NULL
                               ? NULL
                               : dd2_driving_race(dd2_application_driving(dd2_current_application));
    return race == NULL ? 0 : (unsigned)race->survival;
}
unsigned dd2_application_race_alive(void) {
    const dd2_race *race = dd2_current_application == NULL
                               ? NULL
                               : dd2_driving_race(dd2_application_driving(dd2_current_application));
    return race == NULL ? 0 : race->alive;
}

int dd2_application_set_paused(int paused) {
    dd2_application *application = dd2_current_application;
    if (application == NULL || (paused != 0 && paused != 1)) {
        return 0;
    }
    application->paused = paused != 0;
    if (application->paused && application->drive) {
        dd2_game_audio_suspend(application->audio, true);
    }
    dd2_application_suspend(application);
    dd2_window_release_input(application->window);
    return 1;
}

int dd2_application_is_paused(void) {
    return dd2_current_application != NULL ? (int)dd2_current_application->paused : 0;
}

static dd2_camera_motion dd2_application_motion(const dd2_input *input) {
    return (dd2_camera_motion){
        .yaw = (float)((int)input->held[DD2_KEY_RIGHT] - (int)input->held[DD2_KEY_LEFT]),
        .pitch = (float)((int)input->held[DD2_KEY_UP] - (int)input->held[DD2_KEY_DOWN]),
        .zoom = (float)((int)input->held[DD2_KEY_ZOOM_OUT] - (int)input->held[DD2_KEY_ZOOM_IN]),
        .pan_x = (float)((int)input->held[DD2_KEY_PAN_LEFT] - (int)input->held[DD2_KEY_PAN_RIGHT]),
        .pan_y = (float)((int)input->held[DD2_KEY_PAN_DOWN] - (int)input->held[DD2_KEY_PAN_UP])};
}

static void dd2_application_race_input(const dd2_input *input) {
    if (input->pressed[DD2_KEY_WRECKING] || input->pressed[DD2_KEY_STOCKCAR] ||
        input->pressed[DD2_KEY_TIME_TRIAL] || input->pressed[DD2_KEY_TOTAL_DESTRUCTION]) {
        int mode = input->pressed[DD2_KEY_STOCKCAR] ? DD2_RACE_STOCKCAR : DD2_RACE_WRECKING;
        if (input->pressed[DD2_KEY_TIME_TRIAL]) {
            mode = DD2_RACE_TIME_TRIAL;
        } else if (input->pressed[DD2_KEY_TOTAL_DESTRUCTION]) {
            mode = DD2_RACE_TOTAL_DESTRUCTION;
        }
        if (dd2_application_start_race(mode) == 0) {
            puts("This mode is unavailable on this track.");
        }
    }
    if (input->pressed[DD2_KEY_WITHDRAW]) {
        dd2_application_withdraw_race();
    }
}

static void dd2_application_music_input(const dd2_input *input) {
    if (input->pressed[DD2_KEY_MUSIC]) {
        if (dd2_application_music_phase() == DD2_MUSIC_EMPTY) {
            dd2_application_load_music(DD2_MIXER_FIRST_TRACK);
        } else {
            dd2_application_set_music_playing(dd2_application_music_phase() != DD2_MUSIC_PLAYING);
        }
    }
#ifndef __EMSCRIPTEN__
    if (input->pressed[DD2_KEY_MUSIC_NEXT] || input->pressed[DD2_KEY_MUSIC_PREVIOUS]) {
        const unsigned current = dd2_application_music_track();
        unsigned next = DD2_MIXER_FIRST_TRACK;
        if (input->pressed[DD2_KEY_MUSIC_NEXT]) {
            next = current < DD2_MIXER_LAST_TRACK ? current + 1 : DD2_MIXER_FIRST_TRACK;
        } else {
            next = current > DD2_MIXER_FIRST_TRACK ? current - 1 : DD2_MIXER_LAST_TRACK;
        }
        if (dd2_application_load_music(next) == 0) {
            puts("The Redbook track could not be loaded.");
        }
    }
#endif
}

static void dd2_application_championship_input(dd2_application *application,
                                               const dd2_input *input) {
    if (input->pressed[DD2_KEY_CHAMP_WRECKING] || input->pressed[DD2_KEY_CHAMP_STOCKCAR]) {
        const int mode =
            input->pressed[DD2_KEY_CHAMP_STOCKCAR] ? DD2_RACE_STOCKCAR : DD2_RACE_WRECKING;
        if (!dd2_application_start_championship(mode)) {
            puts("The championship could not be started.");
        }
    }
    if (input->pressed[DD2_KEY_DRIVE]) {
        const int phase = dd2_application_championship_phase();
        if (phase == DD2_CHAMPIONSHIP_ROUND_RESULTS || phase == DD2_CHAMPIONSHIP_SEASON_RESULTS) {
            if (!dd2_application_continue_championship()) {
                puts("The next championship race could not be loaded. Results have been retained.");
            }
        } else if (application->championship != NULL) {
            if (!dd2_application_exit_championship()) {
                puts("The track view could not be restored. The championship has been retained.");
            }
        } else {
            dd2_application_set_driving(!application->drive);
        }
    }
}

static void dd2_application_track_input(const dd2_application *application,
                                        const dd2_input *input) {
    if (application->championship == NULL &&
        (input->pressed[DD2_KEY_NEXT] || input->pressed[DD2_KEY_PREVIOUS])) {
        int next = application->level + (input->pressed[DD2_KEY_NEXT] ? 1 : -1);
        if (next < 1) {
            next = DD2_TRACK_COUNT;
        } else if (next > DD2_TRACK_COUNT) {
            next = 1;
        }
        if (dd2_application_select_level(next) == 0) {
            puts("The track could not be loaded.");
        }
    }
}

static void dd2_application_action(dd2_application *application, dd2_key key) {
    dd2_input single = {0};
    single.pressed[key] = true;
    const dd2_input *input = &single;
    if (key == DD2_KEY_QUIT) {
        if (application->championship == NULL) {
            application->running = false;
        } else if (!dd2_application_exit_championship()) {
            puts("The track view could not be restored. The championship has been retained.");
        }
        return;
    }
    dd2_application_music_input(input);
    dd2_application_championship_input(application, input);
    dd2_application_race_input(input);
    if (input->pressed[DD2_KEY_PAUSE] && application->drive) {
        dd2_application_set_paused(!application->paused);
    }
    if (input->pressed[DD2_KEY_VIEW]) {
        dd2_application_show_car(application->drive || application->car ? 0 : 1);
    }
    dd2_application_track_input(application, input);
    if (input->pressed[DD2_KEY_RESET]) {
        dd2_application_reset_camera();
    }
}

static void dd2_application_input(dd2_application *application, const dd2_input *input) {
    for (unsigned index = 0; index < input->action_count && application->running; ++index) {
        const dd2_input_action action = input->actions[index];
        if (action.kind == DD2_INPUT_KEY_ACTION) {
            dd2_application_action(application, action.key);
        } else if (!application->drive) {
            application->dirty =
                dd2_camera_step(&application->camera,
                                (dd2_camera_motion){.zoom = action.wheel > 0 ? -1.0F : 1.0F},
                                dd2_app_wheel_seconds) ||
                application->dirty;
        }
    }
}

static bool dd2_application_draw_scene(dd2_application *application) {
    dd2_renderer_make_current(application->renderer);
    if (application->drive) {
        return dd2_driving_draw(
                   application->materials, dd2_application_track(application),
                   (dd2_driving_view){
                       .vehicle = dd2_driving_vehicle(dd2_application_driving(application)),
                       .damage = dd2_driving_damage(dd2_application_driving(application)),
                       .score = dd2_driving_accidents(dd2_application_driving(application)),
                       .lap = dd2_driving_laps(dd2_application_driving(application)),
                       .race = dd2_driving_race(dd2_application_driving(application)),
                       .required_laps = dd2_course_laps(
                           dd2_driving_course(dd2_application_driving(application))),
                       .wheel_roll = dd2_driving_wheel_roll(dd2_application_driving(application)),
                       .opponents = dd2_driving_vehicles(dd2_application_driving(application)) + 1,
                       .opponent_damage =
                           dd2_driving_damage(dd2_application_driving(application)) + 1,
                       .opponent_rolls =
                           dd2_driving_wheel_rolls(dd2_application_driving(application)) + 1,
                       .opponent_count =
                           dd2_driving_vehicle_count(dd2_application_driving(application)) - 1,
                       .viewport = {.width = DD2_APP_WIDTH, .height = DD2_APP_HEIGHT}}) &&
               (application->championship == NULL ||
                dd2_championship_draw_named(
                    dd2_championship_session_state(application->championship),
                    application->player_name,
                    (dd2_render_options){.width = DD2_APP_WIDTH, .height = DD2_APP_HEIGHT}));
    }
    dd2_camera_apply(&application->camera,
                     (dd2_render_options){.width = DD2_APP_WIDTH, .height = DD2_APP_HEIGHT});
    const bool drawn = application->car
                           ? dd2_mesh_draw(application->materials,
                                           dd2_track_car(dd2_application_track(application)),
                                           (dd2_track_vertex){0})
                           : dd2_scene_draw(application->materials,
                                            dd2_track_scene(dd2_application_track(application)));
    return drawn;
}
static bool dd2_application_draw(dd2_application *application) {
    dd2_renderer_make_current(application->renderer);
    /* The modal pauses simulation/camera motion. Its opaque pane overwrites all
     * previous dialog text in the retained framebuffer; redraw the frozen world
     * only for the first presentation or after returning to normal play. */
    if (!application->world_drawn || application->profile_menu.phase == DD2_PROFILE_CLOSED) {
        if (!dd2_application_draw_scene(application)) {
            return false;
        }
        application->world_drawn = true;
    }
    return dd2_profile_draw(
               &application->profile_menu,
               dd2_application_save_name(application->profile_menu.logical),
               (dd2_render_options){.width = DD2_APP_WIDTH, .height = DD2_APP_HEIGHT}) &&
           dd2_window_present(application->window, dd2_renderer_pixels(application->renderer));
}

static void dd2_application_audio_focus(dd2_application *application, bool focused) {
    dd2_game_audio_suspend(application->audio,
                           (application->drive && application->paused) || !focused);
}

static void dd2_application_audio_update(dd2_application *application,
                                         dd2_vehicle_control control) {
    const dd2_vehicle *vehicle = dd2_driving_vehicle(dd2_application_driving(application));
    const dd2_race *race = dd2_driving_race(dd2_application_driving(application));
    const dd2_vehicle_vector forward =
        dd2_vehicle_rotate(vehicle->rotation, (dd2_vehicle_vector){.z = 1});
    const dd2_engine_sound engine = {
        .running = !dd2_driving_damage(dd2_application_driving(application))[0].retired &&
                   (race == NULL || race->phase != DD2_RACE_RESULTS),
        .speed = (vehicle->velocity.x * forward.x) + (vehicle->velocity.y * forward.y) +
                 (vehicle->velocity.z * forward.z),
        .throttle =
            race != NULL && (race->phase == DD2_RACE_COASTING || race->drivers[0].finish_place != 0)
                ? 0
                : control.throttle};
    dd2_game_audio_update(application->audio, engine,
                          dd2_driving_sound_events(dd2_application_driving(application)));
}

int dd2_application_advance(dd2_driving_frame frame) {
    dd2_application *application = dd2_current_application;
    if (application == NULL || !application->running || !application->drive ||
        !dd2_driving_frame_valid(frame)) {
        return 0;
    }
    if (application->paused || application->profile_menu.phase != DD2_PROFILE_CLOSED) {
        dd2_application_suspend(application);
        return 1;
    }
    dd2_game_audio_suspend(application->audio, false);
    const bool advanced = application->championship == NULL
                              ? dd2_driving_advance(application->driving, frame)
                              : dd2_championship_session_advance(application->championship, frame);
    if (advanced) {
        dd2_application_audio_update(application, frame.control);
        application->dirty = true;
    }
    return advanced;
}

static void dd2_application_drive_frame(const dd2_input *input, float seconds) {
    const dd2_vehicle_control control = {
        .throttle = (double)(input->held[DD2_KEY_UP] || input->held[DD2_KEY_PAN_UP]) -
                    (double)(input->held[DD2_KEY_DOWN] || input->held[DD2_KEY_PAN_DOWN]),
        .brake = (double)input->held[DD2_KEY_BRAKE],
        /* World-up camera faces +Z: screen right is local -X. */
        .steer = (double)(input->held[DD2_KEY_LEFT] || input->held[DD2_KEY_PAN_LEFT]) -
                 (double)(input->held[DD2_KEY_RIGHT] || input->held[DD2_KEY_PAN_RIGHT])};
    if (!dd2_application_advance((dd2_driving_frame){.seconds = seconds, .control = control})) {
        puts("The vehicle could not be updated. Press R to reset.");
        dd2_application_set_paused(1);
    }
}

static void dd2_application_frame(void *context) {
    dd2_application *application = context;
    dd2_application_saves_poll();
    dd2_application_profile_poll(application);
    if (!application->running) {
#ifdef __EMSCRIPTEN__
        dd2_application_close();
#endif
        return;
    }
    dd2_input input = dd2_window_poll(application->window);
    if (input.quit) {
        application->running = false;
    } else {
        const bool profile = dd2_application_profile_input(application, &input);
        if (!profile) {
            dd2_application_input(application, &input);
        }
        dd2_window_refresh_controls(application->window, &input);
        if (!application->running) {
#ifdef __EMSCRIPTEN__
            dd2_application_close();
#endif
            return;
        }
        dd2_application_audio_focus(application, input.focused && !profile);
        const float seconds = dd2_window_elapsed(application->window);
        bool moved = false;
        if (profile) {
            dd2_application_suspend(application);
        } else if (application->drive) {
            if (application->paused || !input.focused) {
                dd2_application_suspend(application);
            } else {
                dd2_application_drive_frame(&input, seconds);
                moved = true;
            }
        } else {
            moved = dd2_camera_step(&application->camera, dd2_application_motion(&input), seconds);
        }
        if (application->dirty || input.redraw || moved) {
            application->dirty = false;
            if (!dd2_application_draw(application)) {
                puts("The display could not be updated.");
                application->failed = true;
                application->running = false;
            }
        }
    }
#ifdef __EMSCRIPTEN__
    if (!application->running) {
        dd2_application_close();
    }
#endif
}

static dd2_application *dd2_application_create(const char *path) {
    dd2_application *application = calloc(1, sizeof(*application));
    if (application == NULL) {
        return NULL;
    }
    if (!dd2_file_read(path, &application->file) ||
        dd2_archive_open(application->file.data, application->file.size, &application->archive) !=
            DD2_ARCHIVE_OK) {
        dd2_application_destroy(application);
        return NULL;
    }
    const dd2_render_options viewport = {.width = DD2_APP_WIDTH, .height = DD2_APP_HEIGHT};
    application->window = dd2_window_create(viewport);
    application->renderer = dd2_renderer_create(&viewport);
    if (application->window == NULL || application->renderer == NULL) {
        dd2_application_destroy(application);
        return NULL;
    }
    application->running = true;
    dd2_configuration_defaults(&application->configuration);
    dd2_application_copy_player(application, dd2_configuration_player(&application->configuration));
    dd2_application_last_save_result = DD2_SAVE_STORE_IDLE;
    application->audio = dd2_game_audio_create(path, application->archive);
    if (application->audio == NULL) {
        puts("Audio output is unavailable.");
    }
    dd2_current_application = application;
#ifndef __EMSCRIPTEN__
    dd2_application_load_music(DD2_MIXER_FIRST_TRACK);
#endif
    return application;
}

int dd2_application_open(const char *path, int level) {
    if (path == NULL || dd2_current_application != NULL || level < 1 || level > DD2_TRACK_COUNT) {
        return 0;
    }
    dd2_application *application = dd2_application_create(path);
    if (application == NULL || !dd2_application_select_level(level)) {
        dd2_application_destroy(application);
        return 0;
    }
    return 1;
}

int dd2_application_close(void) {
    if (dd2_application_saves_poll() == DD2_SAVE_STORE_PENDING) {
        return 0;
    }
#ifdef __EMSCRIPTEN__
    if (dd2_current_application != NULL && dd2_current_application->main_loop) {
        emscripten_cancel_main_loop();
    }
#endif
    dd2_application_destroy(dd2_current_application);
    return 1;
}

const dd2_driving *dd2_application_driving_view(void) {
    return dd2_application_driving(dd2_current_application);
}

const dd2_track *dd2_application_track_view(void) {
    return dd2_application_track(dd2_current_application);
}

dd2_application_image dd2_application_image_view(void) {
    const dd2_application *application = dd2_current_application;
    return application == NULL
               ? (dd2_application_image){0}
               : (dd2_application_image){
                     .pixels = dd2_renderer_pixels(application->renderer),
                     .viewport = {.width = DD2_APP_WIDTH, .height = DD2_APP_HEIGHT}};
}

int dd2_application_present(void) {
    dd2_application *application = dd2_current_application;
    return application != NULL && application->running && dd2_application_draw(application);
}

int dd2_application_poll_frame(void) {
    dd2_application *application = dd2_current_application;
    if (application == NULL || application->main_loop) {
        return -1;
    }
    dd2_application_frame(application);
    application = dd2_current_application;
    if (application == NULL) {
        return 0;
    }
    if (application->failed) {
        return -1;
    }
    return (int)application->running;
}

int dd2_application_run(const char *path, int level) {
    if (!dd2_application_open(path, level)) {
        puts("The original archive or track could not be loaded.");
        return EXIT_FAILURE;
    }
    dd2_application *application = dd2_current_application;
#ifdef __EMSCRIPTEN__
    application->main_loop = true;
    emscripten_set_main_loop_arg(dd2_application_frame, application, 0, 0);
    return EXIT_SUCCESS;
#else
    while (application->running) {
        dd2_application_frame(application);
        dd2_window_wait();
    }
    const bool failed = application->failed;
    dd2_application_close();
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
#endif
}
