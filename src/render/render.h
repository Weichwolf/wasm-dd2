// GLES3 / WebGL2 renderer. Context-agnostic (native EGL or emscripten WebGL).
#ifndef DD_RENDER_H
#define DD_RENDER_H
#include "../core/track.h"

void render_init(void);
void render_set_track(const Track* t);
void render_begin(float r, float g, float b);          // clear color+depth
void render_sky(void);                                  // vertical gradient sky + depth clear
void render_track(const float* view, const float* proj);
// simple oriented box (cars/props): center, half-extents, yaw (rad), rgb
void render_box(const float* view, const float* proj, vec3 c, vec3 he, float yaw, float r, float g, float b);
// authentic level geometry: interleaved pos3+rgb3 vertices (triangles)
void render_geo_set(const float* verts, int nverts);
void render_geo(const float* view, const float* proj);
// textured geometry: tv = pos3+uv2(VRAM px)+clutrow; vram = 8-bit indices; clut = nclut x 256 x RGB
void render_geo_set_tex(const float* tv, int ntv, const unsigned char* vram, int vw, int vh,
                        const unsigned char* clut, int nclut);
void render_geo_tex(const float* view, const float* proj);
void render_shutdown(void);

#endif
