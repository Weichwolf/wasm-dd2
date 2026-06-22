// Authentic level geometry: section-0 chunks -> flat-colored faces + TEXTURED faces (type 14 etc.,
// texture-index @ record+8 -> LEVEL.TDF -> VRAM UV rect + CLUT). See docs/REVERSING.md.
#ifndef DD_GEO_H
#define DD_GEO_H

typedef struct {
    float* v;  int nverts;     // flat tris: x,y,z, r,g,b
    float* tv; int ntverts;    // textured tris: x,y,z, u,v (VRAM px), clutrow
    unsigned char* vram; int vram_w, vram_h;   // 8-bit VRAM indices
    unsigned char* clut; int nclut;             // RGB palettes (nclut x 256 x 3)
} Geo;

int  geo_load(const char* dat_path, Geo* g);   // 1 on success
int  geo_load_car(const char* dat_path, Geo* car);  // real car mesh (LEVEL.DAT obj @+0x40) -> flat tris
void geo_free(Geo* g);

#endif
