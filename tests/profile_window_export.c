/* Read-only observations after the actual application's SDL frames. X11 input
 * changes production state; this helper never edits game/menu/storage owners. */
#include "assets/save_profile.h"
#include "game/application.h"
#include "game/configuration.h"
#include "platform/window.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int phase;
    unsigned slot;
    char name[DD2_SAVE_PROFILE_PLAYER_NAME];
    char draft[DD2_CONFIGURATION_NAME_LIMIT + 1];
} dd2_profile_window_snapshot;
static bool dd2_profile_window_hex(const char *text) {
    for (unsigned index = 0; text[index] != '\0'; ++index) {
        if (printf("%02x", (unsigned)(unsigned char)text[index]) < 0) {
            return false;
        }
    }
    return true;
}
static bool dd2_profile_window_record(dd2_profile_window_snapshot *previous) {
    dd2_profile_window_snapshot next = {.phase = dd2_application_profile_phase(),
                                        .slot = dd2_application_profile_slot()};
    const char *name = dd2_application_player_name();
    const char *draft = dd2_application_profile_draft();
    for (unsigned index = 0; name[index] != '\0'; ++index) {
        next.name[index] = name[index];
    }
    for (unsigned index = 0; draft[index] != '\0'; ++index) {
        next.draft[index] = draft[index];
    }
    if (next.phase == previous->phase && next.slot == previous->slot &&
        strcmp(next.name, previous->name) == 0 && strcmp(next.draft, previous->draft) == 0) {
        return true;
    }
    if (printf("{\"phase\":%d,\"slot\":%u,\"name_hex\":\"", next.phase, next.slot) < 0 ||
        !dd2_profile_window_hex(next.name) || fputs("\",\"draft_hex\":\"", stdout) == EOF ||
        !dd2_profile_window_hex(next.draft) ||
        printf("\",\"race_steps\":%u,\"music_frame\":%u}\n", dd2_application_race_steps(),
               dd2_application_music_frame()) < 0 ||
        fflush(stdout) != 0) {
        return false;
    }
    *previous = next;
    return true;
}
int main(int argc, char **argv) {
    if (argc != 2 || !dd2_application_open(argv[1], 1)) {
        return EXIT_FAILURE;
    }
    dd2_profile_window_snapshot previous = {.phase = -1};
    int running = 1;
    while (running == 1) {
        running = dd2_application_poll_frame();
        if (running == 1 && !dd2_profile_window_record(&previous)) {
            running = -1;
        }
        dd2_window_wait();
    }
    return running == 0 && dd2_application_close() ? EXIT_SUCCESS : EXIT_FAILURE;
}
