#ifndef DD2_ASSETS_TRACK_H
#define DD2_ASSETS_TRACK_H

#include "assets/archive.h"
#include "assets/car.h"
#include "assets/level.h"
#include "assets/mesh.h"
#include "assets/road.h"
#include "assets/scene.h"
#include "assets/textures.h"

typedef struct dd2_track dd2_track;
enum { DD2_TRACK_COUNT = 11 };

/* Owns decoded scene, car, wheels, road and atlas. Level/palette views borrow the archive
 * bytes, which must outlive the track. Playable numbers are 1..11 (LEV1..LEVB).
 * Destruction is safe after partial creation; failed create returns null. */
dd2_track *dd2_track_create(const dd2_archive *archive, unsigned number);
void dd2_track_destroy(dd2_track *track);
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
