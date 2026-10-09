#ifndef DD2_RENDER_TRACK_DRAW_H
#define DD2_RENDER_TRACK_DRAW_H

#include "assets/track.h"
#include "render/camera.h"
#include "render/driving_draw.h"
#include "render/model_draw.h"
#include "render/sky_draw.h"
#include "render/world_draw.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct dd2_track_draw dd2_track_draw;
typedef struct {
    dd2_world_draw_stats world;
    dd2_sky_draw_stats sky;
    size_t vehicle_models;
    size_t culled_models;
    size_t vehicle_triangles;
    size_t vehicle_batches;
    size_t body_lods[3];
    size_t uploaded_textures;
} dd2_track_draw_stats;

/* Borrows the immutable track and image-loader state until destruction. Owns
 * reference materials or prepared world/vehicle batches with one shared texture
 * cache. Destroy before the track and its current GL context. Prepared drawing
 * uses meters, three body LODs and explicit suspended/steered/rolling wheels.
 * Prepared driving adds camera-centered sky patches; deformation/cockpits remain
 * separate work. */
dd2_track_draw *dd2_track_draw_create(const dd2_track *track, dd2_model_image_loader loader,
                                      void *user);
void dd2_track_draw_destroy(dd2_track_draw *draw);
bool dd2_track_draw_fit(const dd2_track *track, dd2_camera *camera, bool car);
bool dd2_track_draw_inspect(dd2_track_draw *draw, const dd2_camera *camera, bool car,
                            dd2_render_options viewport, dd2_car_class car_class);
bool dd2_track_draw_driving(dd2_track_draw *draw, dd2_driving_view view);
dd2_track_draw_stats dd2_track_draw_statistics(const dd2_track_draw *draw);

#endif
