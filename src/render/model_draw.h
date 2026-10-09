#ifndef DD2_RENDER_MODEL_DRAW_H
#define DD2_RENDER_MODEL_DRAW_H

#include "assets/image.h"
#include "assets/model.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct dd2_model_draw dd2_model_draw;
typedef dd2_image *(*dd2_model_image_loader)(void *user, const char *resource);

typedef struct {
    float front_steer;
    float wheel_roll;
    float steering;
    float eye[3];
    bool lighting;
    bool clockwise_front;
} dd2_model_draw_options;

/* Borrows the immutable model; owns batched indices and context-local albedo
 * textures with complete mip chains. Loader results transfer ownership and are
 * destroyed after upload. The same current GL context must outlive this cache.
 * The caller owns matrices/viewport and clear. Drawing owns lighting, texture,
 * blend, cull and client-array state and leaves arrays/lighting/textures/blending
 * disabled, depth writes enabled and texture unit zero active. Eye is model-local;
 * animation angles are degrees. Normal/roughness maps are not evaluated yet. */
dd2_model_draw *dd2_model_draw_create(const dd2_model *model, dd2_model_image_loader loader,
                                      void *user);
void dd2_model_draw_destroy(dd2_model_draw *draw);
bool dd2_model_draw_frame(dd2_model_draw *draw, dd2_model_draw_options options);
size_t dd2_model_draw_batch_count(const dd2_model_draw *draw);

#endif
