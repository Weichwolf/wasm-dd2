#include "game/application.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    const char codes[] = "123456789AB";
    const char *selection = argc == 3 ? argv[2] : "1";
    const char *found = strchr(codes, selection[0]);
    if (argc > 3 || strlen(selection) != 1 || found == NULL) {
        puts("Usage: dd2_app [path/Dirinfo] [1..9,A,B]");
        return EXIT_FAILURE;
    }
    return dd2_application_run(argc >= 2 ? argv[1] : "DestructionDerby2/Dirinfo",
                               (int)(found - codes) + 1);
}
