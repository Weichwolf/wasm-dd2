// Track: reconstructs a drivable/renderable road ribbon from LEVEL.DAT section-2
// vertices (ordered as cross-section ribs). Pure core, no GL.
#ifndef DD_TRACK_H
#define DD_TRACK_H
#include "dmath.h"

#define TRACK_K 9            // resampled points per cross-section (across width)
#define TRACK_MAX_RIBS 4096

typedef struct {
    int   nribs;
    // ring of cross-sections, each TRACK_K points (world units, scaled)
    vec3  rib[TRACK_MAX_RIBS][TRACK_K];
    vec3  center[TRACK_MAX_RIBS];   // centerline (mean of each rib)
    float width[TRACK_MAX_RIBS];    // edge-to-edge width
    float seglen[TRACK_MAX_RIBS];   // distance center[i]->center[i+1]
    float s_at[TRACK_MAX_RIBS];     // arc-length at start of each rib
    float total_len;                // centerline loop length
    vec3  bbmin, bbmax;
} Track;

// Load + reconstruct from an extracted LEVEL.DAT file. Returns 1 on success.
int track_load(const char* dat_path, Track* t);

// Sample the centerline at arc-length s (wraps). Fills pos; if tangent!=NULL fills unit forward dir.
void track_sample(const Track* t, float s, vec3* pos, vec3* tangent);

// Nearest centerline arc-length to a world point (coarse). Also returns lateral offset.
float track_project(const Track* t, vec3 p, float* lateral_out);

// Rich localization of a world point relative to the track.
typedef struct {
    float s;          // arc-length on centerline (loop)
    float lateral;    // signed offset from centerline (+right)
    int   rib;        // nearest rib index
    vec3  center;     // centerline point
    vec3  tangent;    // unit forward
    vec3  right;      // unit right (in xz plane)
    float halfwidth;  // half the road width at this rib
} TrackPoint;
TrackPoint track_locate(const Track* t, vec3 p);
// Windowed localization: search only ribs within +/-window of near_rib (wraps).
// Keeps arc-length monotonic so the car doesn't teleport to a parallel track section.
TrackPoint track_locate_local(const Track* t, vec3 p, int near_rib, int window);

#endif
