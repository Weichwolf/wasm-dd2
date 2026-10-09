#ifndef DD2_ASSETS_WORLD_H
#define DD2_ASSETS_WORLD_H

#include "assets/bounds.h"
#include "assets/bytes.h"
#include "assets/model.h"

#include <stddef.h>
#include <stdint.h>

enum { DD2_WORLD_NAME_BYTES = 32, DD2_WORLD_PATH_BYTES = 64 };

typedef struct dd2_world dd2_world;
typedef dd2_model *(*dd2_world_model_loader)(void *user, const char *resource);

typedef struct {
    char path[DD2_WORLD_PATH_BYTES];
    dd2_bounds bounds;
    dd2_model *model;
} dd2_world_resource;

typedef struct {
    char name[DD2_WORLD_NAME_BYTES];
    uint32_t resource;
    float position[3];
} dd2_world_instance;

typedef struct {
    char name[DD2_WORLD_NAME_BYTES];
    uint32_t resource;
} dd2_world_template;

/* Owns decoded DD2SCN1 placements, templates and all transferred loader models.
 * Each unique resource loads once. Source bytes/user state can be released after
 * creation. Bounds must exactly enclose the actual loaded model; malformed
 * metadata/missing models discard the complete partial owner. Getters borrow
 * immutable arrays/models until destruction. No original-game decoder is used. */
dd2_world *dd2_world_create(dd2_byte_view bytes, dd2_world_model_loader loader, void *user);
void dd2_world_destroy(dd2_world *world);
size_t dd2_world_resource_count(const dd2_world *world);
size_t dd2_world_instance_count(const dd2_world *world);
size_t dd2_world_template_count(const dd2_world *world);
const dd2_world_resource *dd2_world_resources(const dd2_world *world);
const dd2_world_instance *dd2_world_instances(const dd2_world *world);
const dd2_world_template *dd2_world_templates(const dd2_world *world);

#endif
