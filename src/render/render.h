// GLES3 / WebGL2 renderer. Context-agnostic (native EGL or emscripten WebGL).
#ifndef DD_RENDER_H
#define DD_RENDER_H
#include "../core/track.h"

void render_init(void);
void render_set_track(const Track* t);
void render_begin(float r, float g, float b);          // clear color+depth
void render_track(const float* view, const float* proj);
// simple oriented box (cars/props): center, half-extents, yaw (rad), rgb
void render_box(const float* view, const float* proj, vec3 c, vec3 he, float yaw, float r, float g, float b);
void render_shutdown(void);

#endif
