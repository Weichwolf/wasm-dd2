// Minimal deterministic math (float, column-major mat4). Shared core <-> render.
#ifndef DD_DMATH_H
#define DD_DMATH_H
#include <math.h>

typedef struct { float x, y, z; } vec3;

static inline vec3 v3(float x, float y, float z) { return (vec3){x, y, z}; }
static inline vec3 v3add(vec3 a, vec3 b) { return v3(a.x+b.x, a.y+b.y, a.z+b.z); }
static inline vec3 v3sub(vec3 a, vec3 b) { return v3(a.x-b.x, a.y-b.y, a.z-b.z); }
static inline vec3 v3scale(vec3 a, float s) { return v3(a.x*s, a.y*s, a.z*s); }
static inline float v3dot(vec3 a, vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline vec3 v3cross(vec3 a, vec3 b) {
    return v3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}
static inline float v3len(vec3 a) { return sqrtf(v3dot(a, a)); }
static inline vec3 v3norm(vec3 a) { float l = v3len(a); return l > 1e-6f ? v3scale(a, 1.0f/l) : a; }
static inline vec3 v3lerp(vec3 a, vec3 b, float t) { return v3add(a, v3scale(v3sub(b, a), t)); }

// column-major 4x4
static inline void mat4_identity(float* m) {
    for (int i = 0; i < 16; i++) m[i] = 0;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}
static inline void mat4_mul(float* o, const float* a, const float* b) {
    float t[16];
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) {
            float s = 0;
            for (int k = 0; k < 4; k++) s += a[k*4+r] * b[c*4+k];
            t[c*4+r] = s;
        }
    for (int i = 0; i < 16; i++) o[i] = t[i];
}
static inline void mat4_perspective(float* m, float fovy, float asp, float zn, float zf) {
    float f = 1.0f / tanf(fovy * 0.5f);
    for (int i = 0; i < 16; i++) m[i] = 0;
    m[0] = f/asp; m[5] = f;
    m[10] = (zf+zn)/(zn-zf); m[11] = -1.0f;
    m[14] = (2.0f*zf*zn)/(zn-zf);
}
// look-at view matrix
static inline void mat4_lookat(float* m, vec3 eye, vec3 at, vec3 up) {
    vec3 f = v3norm(v3sub(at, eye));
    vec3 s = v3norm(v3cross(f, up));
    vec3 u = v3cross(s, f);
    m[0]=s.x; m[4]=s.y; m[8]=s.z;  m[12]=-v3dot(s,eye);
    m[1]=u.x; m[5]=u.y; m[9]=u.z;  m[13]=-v3dot(u,eye);
    m[2]=-f.x;m[6]=-f.y;m[10]=-f.z;m[14]= v3dot(f,eye);
    m[3]=0;   m[7]=0;   m[11]=0;   m[15]=1;
}
#endif
