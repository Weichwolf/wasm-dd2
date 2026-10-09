#include "game/content_viewer.h"

#include <stdlib.h>
#include <string.h>

enum { DD2_CONTENT_PREVIEW_ARGUMENTS = 6, DD2_CONTENT_PREVIEW_POSE = 5 };

int main(int argc, char **argv) {
    if (argc != DD2_CONTENT_PREVIEW_ARGUMENTS || strlen(argv[3]) != 1 || argv[3][0] < '0' ||
        argv[3][0] > '2' || (strcmp(argv[4], "exterior") != 0 && strcmp(argv[4], "cockpit") != 0) ||
        (strcmp(argv[DD2_CONTENT_PREVIEW_POSE], "rest") != 0 &&
         strcmp(argv[DD2_CONTENT_PREVIEW_POSE], "steer") != 0)) {
        return EXIT_FAILURE;
    }
    const dd2_content_selection selection = {
        .detail = argv[3][0] - '0',
        .cockpit = strcmp(argv[4], "cockpit") == 0,
        .pose = strcmp(argv[DD2_CONTENT_PREVIEW_POSE], "steer") == 0};
    return dd2_content_capture((dd2_content_capture_options){
               .root = argv[1], .path = argv[2], .selection = selection})
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
