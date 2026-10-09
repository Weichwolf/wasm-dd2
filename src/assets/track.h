#ifndef DD2_ASSETS_TRACK_H
#define DD2_ASSETS_TRACK_H

#include "assets/archive.h"
#include "assets/car.h"
#include "assets/car_class.h"
#include "assets/level.h"
#include "assets/mesh.h"
#include "assets/road.h"
#include "assets/scene.h"
#include "assets/textures.h"
#include "assets/world.h"

typedef struct dd2_track dd2_track;
enum { DD2_TRACK_COUNT = 11, DD2_TRACK_BODY_LODS = 3 };
typedef enum {
    DD2_TRACK_MODEL_CLOSE,
    DD2_TRACK_MODEL_MEDIUM,
    DD2_TRACK_MODEL_DISTANT,
    DD2_TRACK_MODEL_WHEEL_PRIMARY,
    DD2_TRACK_MODEL_WHEEL_SECONDARY,
    DD2_TRACK_MODEL_SKY,
    DD2_TRACK_MODEL_COUNT
} dd2_track_model_kind;

/* A loader transfers a complete track or returns NULL. User state is borrowed
 * until the last load, including candidate championship rounds/restarts. */
typedef struct {
    dd2_track *(*load)(const void *user, unsigned number);
    const void *user;
} dd2_track_provider;
typedef struct {
    dd2_byte_view scene;
    dd2_byte_view road;
} dd2_track_prepared_source;
dd2_track_provider dd2_track_reference_provider(const dd2_archive *archive);
/* Reject invalid numbers and wrong-number loader results before publication. */
dd2_track *dd2_track_load(dd2_track_provider provider, unsigned number);

/* Owns decoded scene, car, wheels, road and atlas. Level/palette views borrow the archive
 * bytes, which must outlive the track. Playable numbers are 1..11 (LEV1..LEVB).
 * Destruction is safe after partial creation; failed create returns null. */
dd2_track *dd2_track_create(const dd2_archive *archive, unsigned number);
/* Owns the prepared scene/models and road without any original archive input.
 * Both byte views and model-loader state may be released after creation. All
 * three base body LODs, all 22 livery/LOD variants, two wheels and sky templates
 * must exist. Number/layout must
 * agree. Legacy level/scene/mesh/palette getters return NULL for this owner. */
dd2_track *dd2_track_create_prepared(unsigned number, dd2_track_prepared_source source,
                                     dd2_world_model_loader loader, void *user);
void dd2_track_destroy(dd2_track *track);
unsigned dd2_track_number(const dd2_track *track);
const dd2_world *dd2_track_world(const dd2_track *track);
const dd2_model *dd2_track_prepared_model(const dd2_track *track, dd2_track_model_kind kind);
/* Livery is dd2_car_livery_index(driver, class); detail is close/medium/distant.
 * Immutable models borrow track ownership. Invalid/missing variants return NULL. */
const dd2_model *dd2_track_prepared_car(const dd2_track *track, unsigned livery,
                                        dd2_track_model_kind detail);
const dd2_level_data *dd2_track_level(const dd2_track *track);
const dd2_texture_set *dd2_track_textures(const dd2_track *track);
const dd2_scene *dd2_track_scene(const dd2_track *track);
const dd2_mesh *dd2_track_car(const dd2_track *track);
/* Owned bindings for all twenty stable driver IDs and three human classes. */
const dd2_car_livery *dd2_track_car_livery(const dd2_track *track, unsigned driver,
                                           dd2_car_class car_class);
const dd2_mesh *dd2_track_wheel(const dd2_track *track, unsigned wheel);
const dd2_road *dd2_track_road(const dd2_track *track);

#endif
