#include "track.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORLD_SCALE (1.0f/500.0f)   // raw DAT units -> ~meters (track width ~10-12)

static unsigned char* read_file(const char* path, long* size) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    unsigned char* b = (unsigned char*)malloc(n);
    if (b && fread(b, 1, n, f) != (size_t)n) { free(b); b = NULL; }
    fclose(f);
    if (size) *size = n;
    return b;
}
static unsigned u32(const unsigned char* p) {
    return p[0] | (p[1]<<8) | (p[2]<<16) | ((unsigned)p[3]<<24);
}
static int i32(const unsigned char* p) { return (int)u32(p); }

static int cmpf(const void* a, const void* b) {
    float x = *(const float*)a, y = *(const float*)b;
    return (x > y) - (x < y);
}

// resample polyline pts[0..n) to K points by arc length
static void resample(const vec3* pts, int n, vec3* out, int K) {
    if (n <= 0) { for (int i=0;i<K;i++) out[i] = v3(0,0,0); return; }
    if (n == 1) { for (int i=0;i<K;i++) out[i] = pts[0]; return; }
    float cum[TRACK_K*8]; // n is small (rib width); guard
    if (n > (int)(sizeof(cum)/sizeof(cum[0]))) n = (int)(sizeof(cum)/sizeof(cum[0]));
    cum[0] = 0;
    for (int i = 1; i < n; i++) cum[i] = cum[i-1] + v3len(v3sub(pts[i], pts[i-1]));
    float total = cum[n-1];
    if (total < 1e-6f) { for (int i=0;i<K;i++) out[i] = pts[0]; return; }
    for (int i = 0; i < K; i++) {
        float t = total * (float)i / (float)(K-1);
        int j = 0;
        while (j < n-2 && cum[j+1] < t) j++;
        float seg = cum[j+1] - cum[j];
        float u = seg > 1e-6f ? (t - cum[j]) / seg : 0.0f;
        out[i] = v3lerp(pts[j], pts[j+1], u);
    }
}

int track_load(const char* path, Track* t) {
    long n; unsigned char* d = read_file(path, &n);
    if (!d) { fprintf(stderr, "track_load: cannot open %s\n", path); return 0; }

    // section offset table (u32, relative to start; ends at first pointer value)
    unsigned ptr[64]; int nptr = 0; unsigned first = 0xffffffff;
    for (long i = 0; i + 4 <= n && nptr < 64; i += 4) {
        unsigned v = u32(d + i);
        if (nptr == 0) { if (v == 0 || v > (unsigned)n) break; first = v; ptr[nptr++] = v; }
        else { if ((unsigned)i >= first) break; ptr[nptr++] = v; }
    }
    if (nptr < 3) { free(d); fprintf(stderr,"track_load: bad header\n"); return 0; }

    // section 2 = vertices; size = next distinct offset - off2
    unsigned off2 = ptr[2];
    unsigned end2 = (unsigned)n;
    for (int i = 0; i < nptr; i++) if (ptr[i] > off2 && ptr[i] < end2) end2 = ptr[i];
    int nv = (int)((end2 - off2) / 12);
    if (nv < 8) { free(d); fprintf(stderr,"track_load: too few verts (%d)\n", nv); return 0; }

    vec3* V = (vec3*)malloc(sizeof(vec3) * nv);
    for (int i = 0; i < nv; i++) {
        const unsigned char* p = d + off2 + i*12;
        V[i] = v3(i32(p)*WORLD_SCALE, i32(p+4)*WORLD_SCALE, i32(p+8)*WORLD_SCALE);
    }
    free(d);

    // consecutive xz distances -> median -> split ribs at >4*median jumps
    float* dist = (float*)malloc(sizeof(float) * (nv > 1 ? nv-1 : 1));
    for (int i = 0; i < nv-1; i++) {
        float dx = V[i+1].x - V[i].x, dz = V[i+1].z - V[i].z;
        dist[i] = sqrtf(dx*dx + dz*dz);
    }
    float* tmp = (float*)malloc(sizeof(float) * (nv > 1 ? nv-1 : 1));
    memcpy(tmp, dist, sizeof(float)*(nv-1));
    qsort(tmp, nv-1, sizeof(float), cmpf);
    float med = tmp[(nv-1)/2];
    float thr = med * 4.0f;
    free(tmp);

    t->nribs = 0;
    t->bbmin = v3(1e30f,1e30f,1e30f); t->bbmax = v3(-1e30f,-1e30f,-1e30f);
    int start = 0;
    for (int i = 0; i <= nv-1; i++) {
        int boundary = (i == nv-1) || (dist[i] > thr);
        if (boundary) {
            int len = i - start + 1;
            if (len >= 2 && t->nribs < TRACK_MAX_RIBS) {
                resample(V + start, len, t->rib[t->nribs], TRACK_K);
                t->nribs++;
            }
            start = i + 1;
        }
    }
    free(dist); free(V);
    if (t->nribs < 8) { fprintf(stderr,"track_load: too few ribs (%d)\n", t->nribs); return 0; }

    // Some cross-sections include a far scenery vertex (a "spur"), which would
    // pull the centerline off-road. Detect spur ribs (abnormally wide) and rebuild
    // their points by interpolating the nearest normal neighbours, so the road stays smooth.
    {
        float* w = (float*)malloc(sizeof(float)*t->nribs);
        for (int i = 0; i < t->nribs; i++) w[i] = v3len(v3sub(t->rib[i][0], t->rib[i][TRACK_K-1]));
        float* ws = (float*)malloc(sizeof(float)*t->nribs);
        memcpy(ws, w, sizeof(float)*t->nribs);
        qsort(ws, t->nribs, sizeof(float), cmpf);
        float medw = ws[t->nribs/2];
        free(ws);
        for (int pass = 0; pass < 4; pass++) {
            int changed = 0;
            for (int i = 0; i < t->nribs; i++) {
                if (w[i] <= medw * 1.7f) continue;
                int a = (i - 1 + t->nribs) % t->nribs, b = (i + 1) % t->nribs;
                if (w[a] > medw*1.7f && w[b] > medw*1.7f) continue;  // wait for a good neighbour
                for (int k = 0; k < TRACK_K; k++)
                    t->rib[i][k] = v3lerp(t->rib[a][k], t->rib[b][k], 0.5f);
                w[i] = v3len(v3sub(t->rib[i][0], t->rib[i][TRACK_K-1]));
                changed = 1;
            }
            if (!changed) break;
        }
        free(w);
    }

    // Light Laplacian smoothing along the track (removes reconstruction kinks that
    // would otherwise make sharp spots un-navigable). Preserves overall shape.
    {
        vec3 (*tmp)[TRACK_K] = malloc(sizeof(*tmp) * t->nribs);
        for (int pass = 0; pass < 2; pass++) {
            for (int i = 0; i < t->nribs; i++) {
                int a = (i-1+t->nribs)%t->nribs, b = (i+1)%t->nribs;
                for (int k = 0; k < TRACK_K; k++)
                    tmp[i][k] = v3add(v3scale(t->rib[i][k],0.5f),
                                      v3add(v3scale(t->rib[a][k],0.25f), v3scale(t->rib[b][k],0.25f)));
            }
            memcpy(t->rib, tmp, sizeof(*tmp) * t->nribs);
        }
        free(tmp);
    }

    // centerline, width, bbox
    for (int i = 0; i < t->nribs; i++) {
        vec3 c = v3(0,0,0);
        for (int k = 0; k < TRACK_K; k++) {
            c = v3add(c, t->rib[i][k]);
            vec3 p = t->rib[i][k];
            if (p.x<t->bbmin.x)t->bbmin.x=p.x; if (p.y<t->bbmin.y)t->bbmin.y=p.y; if (p.z<t->bbmin.z)t->bbmin.z=p.z;
            if (p.x>t->bbmax.x)t->bbmax.x=p.x; if (p.y>t->bbmax.y)t->bbmax.y=p.y; if (p.z>t->bbmax.z)t->bbmax.z=p.z;
        }
        t->center[i] = v3scale(c, 1.0f/TRACK_K);
        t->width[i] = v3len(v3sub(t->rib[i][0], t->rib[i][TRACK_K-1]));
    }
    t->total_len = 0;
    for (int i = 0; i < t->nribs; i++) {
        int j = (i+1) % t->nribs;
        t->seglen[i] = v3len(v3sub(t->center[j], t->center[i]));
        t->s_at[i] = t->total_len;
        t->total_len += t->seglen[i];
    }
    return 1;
}

static TrackPoint locate_at_rib(const Track* t, vec3 p, int best) {
    int j = (best+1) % t->nribs;
    TrackPoint tp;
    tp.rib = best;
    tp.center = t->center[best];
    tp.tangent = v3norm(v3sub(t->center[j], t->center[best]));
    tp.right = v3(tp.tangent.z, 0, -tp.tangent.x);
    vec3 rel = v3sub(p, tp.center);
    tp.lateral = v3dot(rel, tp.right);
    float along = v3dot(rel, tp.tangent);
    if (along < 0) along = 0;
    if (along > t->seglen[best]) along = t->seglen[best];
    tp.s = t->s_at[best] + along;
    tp.halfwidth = t->width[best] * 0.5f;
    return tp;
}

TrackPoint track_locate_local(const Track* t, vec3 p, int near_rib, int window) {
    int best = near_rib; float bd = 1e30f;
    for (int d = -window; d <= window; d++) {
        int i = ((near_rib + d) % t->nribs + t->nribs) % t->nribs;
        float dx = p.x - t->center[i].x, dz = p.z - t->center[i].z;
        float dd = dx*dx + dz*dz;
        if (dd < bd) { bd = dd; best = i; }
    }
    return locate_at_rib(t, p, best);
}

void track_sample(const Track* t, float s, vec3* pos, vec3* tangent) {
    s = fmodf(s, t->total_len); if (s < 0) s += t->total_len;
    float acc = 0; int i = 0;
    for (i = 0; i < t->nribs; i++) {
        if (acc + t->seglen[i] >= s) break;
        acc += t->seglen[i];
    }
    if (i >= t->nribs) i = t->nribs-1;
    int j = (i+1) % t->nribs;
    float u = t->seglen[i] > 1e-6f ? (s - acc)/t->seglen[i] : 0;
    if (pos) *pos = v3lerp(t->center[i], t->center[j], u);
    if (tangent) *tangent = v3norm(v3sub(t->center[j], t->center[i]));
}

float track_project(const Track* t, vec3 p, float* lateral_out) {
    int best = 0; float bd = 1e30f; float acc = 0, bestacc = 0;
    for (int i = 0; i < t->nribs; i++) {
        float dx = p.x - t->center[i].x, dz = p.z - t->center[i].z;
        float d = dx*dx + dz*dz;
        if (d < bd) { bd = d; best = i; bestacc = acc; }
        acc += t->seglen[i];
    }
    if (lateral_out) {
        int j = (best+1) % t->nribs;
        vec3 fwd = v3norm(v3sub(t->center[j], t->center[best]));
        vec3 rel = v3sub(p, t->center[best]);
        // signed lateral = cross(fwd, rel).y
        *lateral_out = fwd.z*rel.x - fwd.x*rel.z;
    }
    return bestacc;
}

TrackPoint track_locate(const Track* t, vec3 p) {
    int best = 0; float bd = 1e30f, acc = 0, bestacc = 0;
    for (int i = 0; i < t->nribs; i++) {
        float dx = p.x - t->center[i].x, dz = p.z - t->center[i].z;
        float d = dx*dx + dz*dz;
        if (d < bd) { bd = d; best = i; bestacc = acc; }
        acc += t->seglen[i];
    }
    int j = (best+1) % t->nribs;
    TrackPoint tp;
    tp.rib = best;
    tp.center = t->center[best];
    tp.tangent = v3norm(v3sub(t->center[j], t->center[best]));
    tp.right = v3(tp.tangent.z, 0, -tp.tangent.x);   // rotate tangent -90 about +y
    vec3 rel = v3sub(p, tp.center);
    tp.lateral = v3dot(rel, tp.right);
    // refine s by projecting rel onto tangent within this segment
    float along = v3dot(rel, tp.tangent);
    if (along < 0) along = 0;
    if (along > t->seglen[best]) along = t->seglen[best];
    tp.s = bestacc + along;
    tp.halfwidth = t->width[best] * 0.5f;
    return tp;
}
