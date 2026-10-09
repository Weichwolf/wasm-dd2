#include "game/application.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { DD2_MAIN_REFERENCE_ARGUMENTS = 3, DD2_MAIN_PREPARED_ARGUMENTS = 4 };

int main(int argc, char **argv) {
    const bool explicit_prepared = argc >= 2 && strcmp(argv[1], "--assets") == 0;
    const bool prepared = argc == 1 || explicit_prepared;
    const int selection_index = explicit_prepared ? 3 : 2;
    const int limit =
        explicit_prepared ? DD2_MAIN_PREPARED_ARGUMENTS : DD2_MAIN_REFERENCE_ARGUMENTS;
    const char codes[] = "123456789AB";
    const char *selection = argc > selection_index ? argv[selection_index] : "1";
    const char *found = strchr(codes, selection[0]);
    if (argc > limit || (explicit_prepared && argc < 3) || strlen(selection) != 1 ||
        found == NULL) {
        puts("Usage: dd2_app [--assets directory [1..9,A,B]] or [path/Dirinfo [1..9,A,B]]");
        return EXIT_FAILURE;
    }
#ifdef __EMSCRIPTEN__
    const char *root = "/content";
#else
    const char *root = "assets/runtime";
#endif
    const int level = (int)(found - codes) + 1;
    return prepared ? dd2_application_run_prepared(explicit_prepared ? argv[2] : root, level)
                    : dd2_application_run(argv[1], level);
}
