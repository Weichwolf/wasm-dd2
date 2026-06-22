// PSX-style texture VRAM (faithful): assemble the 256-wide 8-bit texture space from a level's
// TX page directory + palette, expose named sprites (LEVEL.SPR) as on-demand GL textures.
// (Buffer is ~256x6592, taller than WebGL max texture size, so sprites upload as sub-rect textures.)
#ifndef DD_VRAM_H
#define DD_VRAM_H
#include <GLES3/gl3.h>

int   vram_init(const char* level);                 // assemble full VRAM (all TX pages) + PAL/SPR/FONT
GLuint vram_sprite_tex(const char* name, int* w, int* h);  // upload a named sprite -> RGBA texture (0 if absent)
int   vram_text_measure(const char* s);             // pixel width of a string in the bitmap font
GLuint vram_text_tex(const char* s, unsigned char r,unsigned char g,unsigned char b, int* w,int* h); // render text (ink rgb) -> RGBA tex

#endif
