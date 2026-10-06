#include "game/application.h"

#include "assets/archive.h"
#include "assets/level.h"
#include "assets/track.h"
#include "platform/file.h"
#include "platform/window.h"
#include "render/camera.h"
#include "render/mesh_draw.h"
#include "render/renderer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

enum { DD2_APP_WIDTH = 640, DD2_APP_HEIGHT = 480 };
static const float dd2_app_wheel_seconds = 0.15F;

typedef struct {
    dd2_file file;
    dd2_archive *archive;
    dd2_track *track;
    dd2_renderer *renderer;
    dd2_mesh_materials *materials;
    dd2_window *window;
    dd2_camera camera;
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
    dd2_renderer_destroy(application->renderer);
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
    if (materials == NULL || !dd2_application_fit(&camera, track, application->car)) {
        dd2_mesh_materials_destroy(materials);
        dd2_track_destroy(track);
        return 0;
    }
    dd2_mesh_materials_destroy(application->materials);
    dd2_track_destroy(application->track);
    application->track = track;
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
    application->car = car != 0;
    application->dirty = true;
    return 1;
}

int dd2_application_current_level(void) {
    return dd2_current_application != NULL ? dd2_current_application->level : 0;
}

int dd2_application_current_view(void) {
    return dd2_current_application != NULL ? (int)dd2_current_application->car : -1;
}

void dd2_application_reset_camera(void) {
    dd2_application *application = dd2_current_application;
    if (application != NULL) {
        application->dirty =
            dd2_application_fit(&application->camera, application->track, application->car);
    }
}

void dd2_application_release_input(void) {
    if (dd2_current_application != NULL) {
        dd2_window_release_input(dd2_current_application->window);
    }
}

static dd2_camera_motion dd2_application_motion(const dd2_input *input) {
    return (dd2_camera_motion){
        .yaw = (float)((int)input->held[DD2_KEY_RIGHT] - (int)input->held[DD2_KEY_LEFT]),
        .pitch = (float)((int)input->held[DD2_KEY_UP] - (int)input->held[DD2_KEY_DOWN]),
        .zoom = (float)((int)input->held[DD2_KEY_ZOOM_OUT] - (int)input->held[DD2_KEY_ZOOM_IN]),
        .pan_x = (float)((int)input->held[DD2_KEY_PAN_LEFT] - (int)input->held[DD2_KEY_PAN_RIGHT]),
        .pan_y = (float)((int)input->held[DD2_KEY_PAN_DOWN] - (int)input->held[DD2_KEY_PAN_UP])};
}

static void dd2_application_input(dd2_application *application, const dd2_input *input) {
    if (input->pressed[DD2_KEY_VIEW]) {
        dd2_application_show_car(application->car ? 0 : 1);
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
    if (input->wheel != 0) {
        application->dirty =
            dd2_camera_step(&application->camera,
                            (dd2_camera_motion){.zoom = input->wheel > 0 ? -1.0F : 1.0F},
                            dd2_app_wheel_seconds) ||
            application->dirty;
    }
}

static bool dd2_application_draw(dd2_application *application) {
    dd2_renderer_make_current(application->renderer);
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

static void dd2_application_frame(void *context) {
    dd2_application *application = context;
    const dd2_input input = dd2_window_poll(application->window);
    if (input.quit || input.pressed[DD2_KEY_QUIT]) {
        application->running = false;
    } else {
        dd2_application_input(application, &input);
        const bool moved = dd2_camera_step(&application->camera, dd2_application_motion(&input),
                                           dd2_window_elapsed(application->window));
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
    dd2_current_application = application;
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
