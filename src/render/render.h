// GLES3 / WebGL2 renderer. Context-agnostic (native EGL or emscripten WebGL).
#ifndef DD_RENDER_H
#define DD_RENDER_H
#include "../core/track.h"

void render_init(void);
void render_set_track(const Track* t);
void render_begin(float r, float g, float b);          // clear color+depth
void render_sky(void);                                  // vertical gradient sky + depth clear
void render_set_sky_bright(float k);                    // per-level: 1=clear daytime, 0=overcast/stormy
void render_set_ground_color(float r, float g, float b);// per-track drivable-surface tint
void render_set_track_bright(float k);                  // 1=tarmac/grass ramp; low=dark stormy ground
void render_track(const float* view, const float* proj);
// simple oriented box (cars/props): center, half-extents, yaw (rad), rgb
void render_box(const float* view, const float* proj, vec3 c, vec3 he, float yaw, float r, float g, float b);
// real car mesh (pos3+rgb3 tris in car-local space): upload once, then draw per car (pos/yaw/tint).
void render_car_set(const float* tv, int nverts);
void render_car(const float* view, const float* proj, vec3 pos, float yaw, float tr, float tg, float tb, float dmg);
// authentic level geometry: interleaved pos3+rgb3 vertices (triangles)
void render_geo_set(const float* verts, int nverts);
void render_geo(const float* view, const float* proj);
// textured geometry: tv = pos3+uv2(VRAM px)+clutrow; vram = 8-bit indices; clut = nclut x 256 x RGB
void render_geo_set_tex(const float* tv, int ntv, const unsigned char* vram, int vw, int vh,
                        const unsigned char* clut, int nclut);
void render_geo_tex(const float* view, const float* proj);
// software-GTE-style affine renderer for textured geo (CPU transform + near-plane clip + affine UV).
void gte_render_set(const float* tv, int ntv, const unsigned char* vram, int vw, int vh,
                    const unsigned char* clut, int nclut);
void gte_render(const float* view, const float* proj);
void gte_set_fog(float r, float g, float b);   // per-level fog/depth-cue colour
void gte_set_levbright(float k);                // per-level in-race brightness (stormy tracks < 1)
void render_shutdown(void);

#endif
