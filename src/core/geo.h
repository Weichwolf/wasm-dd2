// Authentic level geometry: decode section-0 LZSS object chunks -> type-12 flat-colored quads
// (≈90% of faces) into a colored triangle buffer for the renderer. See docs/REVERSING.md.
#ifndef DD_GEO_H
#define DD_GEO_H

// Interleaved vertex: x,y,z, r,g,b (floats). 3 verts/tri.
typedef struct { float* v; int nverts; } Geo;

int  geo_load(const char* dat_path, Geo* g);   // 1 on success; fills g->v (malloc'd) + nverts
void geo_free(Geo* g);

#endif
