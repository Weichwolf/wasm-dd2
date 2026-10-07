#include "game/application.h"

#include "assets/archive.h"
#include "assets/level.h"
#include "assets/track.h"
#include "audio/mixer.h"
#include "game/course.h"
#include "game/driving.h"
#include "game/laps.h"
#include "game/music.h"
#include "game/race.h"
#include "physics/damage.h"
#include "physics/vehicle.h"
#include "platform/file.h"
#include "platform/window.h"
#include "render/camera.h"
#include "render/driving_draw.h"
#include "render/mesh_draw.h"
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

enum { DD2_APP_WIDTH = 640, DD2_APP_HEIGHT = 480, DD2_APP_RACING_LEVELS = 7 };
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
    dd2_music_player *music;
    bool drive;
    bool paused;
    int level;
    bool car;
    bool running;
    bool dirty;
    bool failed;
} dd2_application;

/* Sole active application. The callback retains its typed ownership context;
 * exports address it only on the main thread, and destruction clears the bridge. */
static dd2_application *dd2_current_application;

static bool dd2_application_fit(dd2_camera *camera, const dd2_track *track, bool car) {
    return car ? dd2_camera_fit_mesh(camera, dd2_track_car(track))
               : dd2_camera_fit_scene(camera, dd2_track_scene(track));
}

static void dd2_application_destroy(dd2_application *application) {
    if (application == NULL) {
        return;
    }
    if (dd2_current_application == application) {
        dd2_current_application = NULL;
    }
    dd2_mesh_materials_destroy(application->materials);
    dd2_music_player_destroy(application->music);
    dd2_renderer_destroy(application->renderer);
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
    const dd2_race *previous_race = dd2_driving_race(application->driving);
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
    dd2_driving_destroy(application->driving);
    dd2_track_destroy(application->track);
    application->track = track;
    application->driving = driving;
    application->materials = materials;
    application->camera = camera;
    application->level = number;
    application->dirty = true;
    return 1;
}

int dd2_application_show_car(int car) {
    dd2_application *application = dd2_current_application;
    if (application == NULL || (car != 0 && car != 1) ||
        !dd2_application_fit(&application->camera, application->track, car != 0)) {
        return 0;
    }
    application->drive = false;
    dd2_driving_suspend(application->driving);
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
    const dd2_race *race = dd2_driving_race(application->driving);
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
    return (int)dd2_music_player_load(application->music, track, "/Music.cdda");
#else
    return (int)dd2_music_player_load(application->music, track, NULL);
#endif
}

int dd2_application_music_phase(void) {
    dd2_application *application = dd2_current_application;
    return application == NULL || application->music == NULL
               ? -1
               : (int)dd2_music_player_state(application->music).phase;
}

unsigned dd2_application_music_track(void) {
    return dd2_music_player_state(dd2_current_application == NULL ? NULL
                                                                  : dd2_current_application->music)
        .track;
}

unsigned dd2_application_music_frame(void) {
    return (unsigned)dd2_music_player_state(
               dd2_current_application == NULL ? NULL : dd2_current_application->music)
        .frame;
}

unsigned dd2_application_music_fraction(void) {
    return dd2_music_player_state(dd2_current_application == NULL ? NULL
                                                                  : dd2_current_application->music)
        .fraction;
}

unsigned dd2_application_music_rate(void) {
    return dd2_music_player_rate(dd2_current_application == NULL ? NULL
                                                                 : dd2_current_application->music);
}

int dd2_application_set_music_playing(int playing) {
    return dd2_current_application != NULL && (playing == 0 || playing == 1)
               ? (int)dd2_music_player_play(dd2_current_application->music, playing != 0)
               : 0;
}

int dd2_application_set_music_gain(unsigned gain) {
    return dd2_current_application != NULL
               ? (int)dd2_music_player_gain(dd2_current_application->music, gain)
               : 0;
}

unsigned dd2_application_collision_count(void) {
    if (dd2_current_application == NULL) {
        return 0;
    }
    const uint64_t count = dd2_driving_collisions(dd2_current_application->driving);
    return count < UINT_MAX ? (unsigned)count : UINT_MAX;
}

unsigned dd2_application_pair_collision_count(void) {
    if (dd2_current_application == NULL) {
        return 0;
    }
    const uint64_t count = dd2_driving_pair_collisions(dd2_current_application->driving);
    return count < UINT_MAX ? (unsigned)count : UINT_MAX;
}
unsigned dd2_application_vehicle_count(void) {
    return dd2_current_application != NULL
               ? dd2_driving_vehicle_count(dd2_current_application->driving)
               : 0;
}
double dd2_application_engine_health(void) {
    return dd2_current_application != NULL
               ? dd2_damage_health(&dd2_driving_damage(dd2_current_application->driving)[0])
               : 1;
}
double dd2_application_region_damage(unsigned region) {
    return dd2_current_application != NULL && region < DD2_DAMAGE_REGIONS
               ? dd2_driving_damage(dd2_current_application->driving)[0].regions[region]
               : 0;
}
unsigned dd2_application_accident_points(void) {
    return dd2_current_application != NULL
               ? dd2_driving_accidents(dd2_current_application->driving)[0].points
               : 0;
}
unsigned dd2_application_destructions(void) {
    return dd2_current_application != NULL
               ? dd2_driving_accidents(dd2_current_application->driving)[0].destructions
               : 0;
}
unsigned dd2_application_accident_windows(void) {
    unsigned active = 0;
    if (dd2_current_application != NULL) {
        const dd2_accident_driver *drivers =
            dd2_driving_accidents(dd2_current_application->driving);
        for (unsigned slot = 0; slot < dd2_driving_vehicle_count(dd2_current_application->driving);
             ++slot) {
            active += (unsigned)(drivers[slot].remaining != 0);
        }
    }
    return active;
}

static const dd2_lap_driver *dd2_application_lap(void) {
    return dd2_current_application == NULL ? NULL
                                           : dd2_driving_laps(dd2_current_application->driving);
}
unsigned dd2_application_required_laps(void) {
    return dd2_current_application == NULL
               ? 0
               : dd2_course_laps(dd2_driving_course(dd2_current_application->driving));
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
    const dd2_race *race = dd2_driving_race(dd2_current_application->driving);
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
        application->dirty =
            application->drive
                ? dd2_driving_reset(application->driving)
                : dd2_application_fit(&application->camera, application->track, application->car);
    }
}

void dd2_application_release_input(void) {
    if (dd2_current_application != NULL) {
        dd2_window_set_focus(dd2_current_application->window, false);
        dd2_driving_suspend(dd2_current_application->driving);
        dd2_music_player_suspend(dd2_current_application->music, true);
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
    if (dd2_driving_race(application->driving) != NULL &&
        !dd2_driving_set_race(application->driving, false, DD2_RACE_WRECKING)) {
        return 0;
    }
    application->drive = enabled != 0;
    dd2_driving_suspend(application->driving);
    dd2_window_release_input(application->window);
    application->dirty = true;
    return 1;
}

int dd2_application_start_race(int mode) {
    dd2_application *application = dd2_current_application;
    if (application == NULL || (mode < DD2_RACE_WRECKING || mode > DD2_RACE_TOTAL_DESTRUCTION) ||
        !dd2_driving_set_race(application->driving, true, (dd2_race_mode)mode)) {
        return 0;
    }
    application->drive = true;
    application->paused = false;
    application->dirty = true;
    dd2_window_release_input(application->window);
    return 1;
}

int dd2_application_withdraw_race(void) {
    dd2_application *application = dd2_current_application;
    if (application == NULL || !application->drive || !dd2_driving_withdraw(application->driving)) {
        return 0;
    }
    application->dirty = true;
    dd2_window_release_input(application->window);
    return 1;
}

int dd2_application_race_phase(void) {
    const dd2_race *race =
        dd2_current_application == NULL ? NULL : dd2_driving_race(dd2_current_application->driving);
    return race == NULL ? -1 : (int)race->phase;
}

unsigned dd2_application_race_steps(void) {
    const dd2_race *race =
        dd2_current_application == NULL ? NULL : dd2_driving_race(dd2_current_application->driving);
    if (race == NULL) {
        return 0;
    }
    return race->steps < UINT_MAX ? (unsigned)race->steps : UINT_MAX;
}

unsigned dd2_application_race_place(void) {
    const dd2_race *race =
        dd2_current_application == NULL ? NULL : dd2_driving_race(dd2_current_application->driving);
    return race == NULL ? 0 : race->drivers[0].place;
}

unsigned dd2_application_race_points(void) {
    const dd2_race *race =
        dd2_current_application == NULL ? NULL : dd2_driving_race(dd2_current_application->driving);
    return race == NULL || race->phase != DD2_RACE_RESULTS ? 0 : race->drivers[0].total_points;
}

unsigned dd2_application_survival_steps(void) {
    const dd2_race *race =
        dd2_current_application == NULL ? NULL : dd2_driving_race(dd2_current_application->driving);
    return race == NULL ? 0 : (unsigned)race->survival;
}
unsigned dd2_application_race_alive(void) {
    const dd2_race *race =
        dd2_current_application == NULL ? NULL : dd2_driving_race(dd2_current_application->driving);
    return race == NULL ? 0 : race->alive;
}

int dd2_application_set_paused(int paused) {
    dd2_application *application = dd2_current_application;
    if (application == NULL || (paused != 0 && paused != 1)) {
        return 0;
    }
    application->paused = paused != 0;
    if (application->paused) {
        dd2_music_player_suspend(application->music, true);
    }
    dd2_driving_suspend(application->driving);
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
            puts("Dieser Modus ist auf dieser Strecke nicht verfügbar.");
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
            puts("Redbook-Titel konnte nicht geladen werden.");
        }
    }
#endif
}

static void dd2_application_input(dd2_application *application, const dd2_input *input) {
    dd2_application_music_input(input);
    if (input->pressed[DD2_KEY_DRIVE]) {
        dd2_application_set_driving(!application->drive);
    }
    dd2_application_race_input(input);
    if (input->pressed[DD2_KEY_PAUSE] && application->drive) {
        dd2_application_set_paused(!application->paused);
    }
    if (input->pressed[DD2_KEY_VIEW]) {
        dd2_application_show_car(application->drive || application->car ? 0 : 1);
    }
    if (input->pressed[DD2_KEY_NEXT] || input->pressed[DD2_KEY_PREVIOUS]) {
        int next = application->level + (input->pressed[DD2_KEY_NEXT] ? 1 : -1);
        if (next < 1) {
            next = DD2_TRACK_COUNT;
        } else if (next > DD2_TRACK_COUNT) {
            next = 1;
        }
        if (dd2_application_select_level(next) == 0) {
            puts("Strecke konnte nicht geladen werden.");
        }
    }
    if (input->pressed[DD2_KEY_RESET]) {
        dd2_application_reset_camera();
    }
    if (input->wheel != 0 && !application->drive) {
        application->dirty =
            dd2_camera_step(&application->camera,
                            (dd2_camera_motion){.zoom = input->wheel > 0 ? -1.0F : 1.0F},
                            dd2_app_wheel_seconds) ||
            application->dirty;
    }
}

static bool dd2_application_draw(dd2_application *application) {
    dd2_renderer_make_current(application->renderer);
    if (application->drive) {
        return dd2_driving_draw(
                   application->materials, application->track,
                   (dd2_driving_view){
                       .vehicle = dd2_driving_vehicle(application->driving),
                       .damage = dd2_driving_damage(application->driving),
                       .score = dd2_driving_accidents(application->driving),
                       .lap = dd2_driving_laps(application->driving),
                       .race = dd2_driving_race(application->driving),
                       .required_laps = dd2_course_laps(dd2_driving_course(application->driving)),
                       .wheel_roll = dd2_driving_wheel_roll(application->driving),
                       .opponents = dd2_driving_vehicles(application->driving) + 1,
                       .opponent_damage = dd2_driving_damage(application->driving) + 1,
                       .opponent_rolls = dd2_driving_wheel_rolls(application->driving) + 1,
                       .opponent_count = dd2_driving_vehicle_count(application->driving) - 1,
                       .viewport = {.width = DD2_APP_WIDTH, .height = DD2_APP_HEIGHT}}) &&
               dd2_window_present(application->window, dd2_renderer_pixels(application->renderer));
    }
    dd2_camera_apply(&application->camera,
                     (dd2_render_options){.width = DD2_APP_WIDTH, .height = DD2_APP_HEIGHT});
    const bool drawn =
        application->car
            ? dd2_mesh_draw(application->materials, dd2_track_car(application->track),
                            (dd2_track_vertex){0})
            : dd2_scene_draw(application->materials, dd2_track_scene(application->track));
    return drawn &&
           dd2_window_present(application->window, dd2_renderer_pixels(application->renderer));
}

static void dd2_application_audio_focus(dd2_application *application, bool focused) {
    dd2_music_player_suspend(application->music, application->paused || !focused);
}

static void dd2_application_frame(void *context) {
    dd2_application *application = context;
    const dd2_input input = dd2_window_poll(application->window);
    if (input.quit || input.pressed[DD2_KEY_QUIT]) {
        application->running = false;
    } else {
        dd2_application_input(application, &input);
        dd2_application_audio_focus(application, input.focused);
        const float seconds = dd2_window_elapsed(application->window);
        bool moved = false;
        if (application->drive) {
            if (application->paused || !input.focused) {
                dd2_driving_suspend(application->driving);
            } else {
                const dd2_vehicle_control control = {
                    .throttle = (double)(input.held[DD2_KEY_UP] || input.held[DD2_KEY_PAN_UP]) -
                                (double)(input.held[DD2_KEY_DOWN] || input.held[DD2_KEY_PAN_DOWN]),
                    .brake = (double)input.held[DD2_KEY_BRAKE],
                    /* World-up camera faces +Z: screen right is local -X. */
                    .steer = (double)(input.held[DD2_KEY_LEFT] || input.held[DD2_KEY_PAN_LEFT]) -
                             (double)(input.held[DD2_KEY_RIGHT] || input.held[DD2_KEY_PAN_RIGHT])};
                if (!dd2_driving_advance(
                        application->driving,
                        (dd2_driving_frame){.seconds = seconds, .control = control})) {
                    puts("Fahrzeug konnte nicht aktualisiert werden. Mit R zurücksetzen.");
                    dd2_application_set_paused(1);
                }
                moved = true;
            }
        } else {
            moved = dd2_camera_step(&application->camera, dd2_application_motion(&input), seconds);
        }
        if (application->dirty || input.redraw || moved) {
            application->dirty = false;
            if (!dd2_application_draw(application)) {
                puts("Darstellung konnte nicht aktualisiert werden.");
                application->failed = true;
                application->running = false;
            }
        }
    }
#ifdef __EMSCRIPTEN__
    if (!application->running) {
        emscripten_cancel_main_loop();
        dd2_application_destroy(application);
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
    application->music = dd2_music_player_create(path);
    if (application->music == NULL) {
        puts("Audioausgabe ist nicht verfügbar.");
    }
    dd2_current_application = application;
#ifndef __EMSCRIPTEN__
    dd2_application_load_music(DD2_MIXER_FIRST_TRACK);
#endif
    return application;
}

int main(int argc, char **argv) {
    const char codes[] = "123456789AB";
    const char *selection = argc == 3 ? argv[2] : "1";
    const char *found = strchr(codes, selection[0]);
    if (argc > 3 || strlen(selection) != 1 || found == NULL) {
        puts("Aufruf: dd2_app [Pfad/Dirinfo] [1..9,A,B]");
        return EXIT_FAILURE;
    }
    dd2_application *application =
        dd2_application_create(argc >= 2 ? argv[1] : "DestructionDerby2/Dirinfo");
    if (application == NULL || dd2_application_select_level((int)(found - codes) + 1) == 0) {
        puts("Originaldatei oder Strecke konnte nicht geladen werden.");
        dd2_application_destroy(application);
        return EXIT_FAILURE;
    }
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(dd2_application_frame, application, 0, 0);
    return EXIT_SUCCESS;
#else
    while (application->running) {
        dd2_application_frame(application);
        dd2_window_wait();
    }
    const bool failed = application->failed;
    dd2_application_destroy(application);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
#endif
}
