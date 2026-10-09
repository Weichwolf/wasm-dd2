#include "platform/content.h"

#include "assets/bytes.h"
#include "assets/model.h"
#include "assets/track.h"
#include "platform/file.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

enum { DD2_CONTENT_PATH_BYTES = 4096 };

typedef struct {
    const char *root;
} dd2_content_directory;

static bool dd2_content_read(const char *root, const char *resource, dd2_file *file) {
    const size_t prefix = strlen(root);
    const size_t suffix = strlen(resource);
    if (prefix >= DD2_CONTENT_PATH_BYTES || suffix >= DD2_CONTENT_PATH_BYTES - prefix - 1) {
        return false;
    }
    char path[DD2_CONTENT_PATH_BYTES] = {0};
    for (size_t index = 0; index < prefix; ++index) {
        path[index] = root[index];
    }
    path[prefix] = '/';
    for (size_t index = 0; index <= suffix; ++index) {
        path[prefix + 1 + index] = resource[index];
    }
    return dd2_file_read(path, file);
}

static dd2_model *dd2_content_model(void *user, const char *resource) {
    const dd2_content_directory *directory = user;
    dd2_file file = {0};
    if (!dd2_content_read(directory->root, resource, &file)) {
        return NULL;
    }
    dd2_model *model = dd2_model_create((dd2_byte_view){file.data, file.size});
    dd2_file_release(&file);
    return model;
}

static dd2_track *dd2_content_track(const void *user, unsigned number) {
    const char *root = user;
    const char codes[] = "0123456789ab";
    if (root == NULL || root[0] == '\0' || number == 0 || number > DD2_TRACK_COUNT) {
        return NULL;
    }
    char scene_path[] = "reference/scenes/level-0.dd2scene";
    char road_path[] = "roads/level-0.dd2road";
    scene_path[sizeof("reference/scenes/level-") - 1] = codes[number];
    road_path[sizeof("roads/level-") - 1] = codes[number];
    dd2_file scene = {0};
    dd2_file road = {0};
    dd2_track *track = NULL;
    dd2_content_directory directory = {.root = root};
    if (dd2_content_read(root, road_path, &road) && dd2_content_read(root, scene_path, &scene)) {
        track =
            dd2_track_create_prepared(number,
                                      (dd2_track_prepared_source){.scene = {scene.data, scene.size},
                                                                  .road = {road.data, road.size}},
                                      dd2_content_model, &directory);
    }
    dd2_file_release(&road);
    dd2_file_release(&scene);
    return track;
}

dd2_track_provider dd2_content_track_provider(const char *root) {
    return (dd2_track_provider){.load = root == NULL || root[0] == '\0' ? NULL : dd2_content_track,
                                .user = root};
}
