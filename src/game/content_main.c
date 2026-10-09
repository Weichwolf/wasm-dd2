#include "game/content_viewer.h"

#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc > 2) {
        return EXIT_FAILURE;
    }
#ifdef __EMSCRIPTEN__
    const char *root = "/content";
#else
    const char *root = "assets/runtime";
#endif
    return dd2_content_run(argc == 2 ? argv[1] : root);
}
