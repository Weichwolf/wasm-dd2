// 2D UI layer: BMP loading + fullscreen/atlas blits for the faithful front-end.
// Renders into whatever framebuffer is bound (the low-res FBO), GLES3/WebGL2.
#ifndef DD_UI_H
#define DD_UI_H
#include <GLES3/gl3.h>

void ui_init(void);
// Load an 8-bit (or 24/32-bit) Windows BMP file into an RGBA texture (oriented upright).
// Returns texture id (0 on failure); *w/*h get pixel size if non-NULL.
GLuint ui_load_bmp(const char* path, int* w, int* h);
// Draw a texture filling the current viewport (front-end backgrounds/screens).
void ui_blit_fullscreen(GLuint tex);

#endif
