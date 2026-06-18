// Headless GL platform: EGL surfaceless GLES3 context + offscreen FBO + PNG screenshot.
// Native only (Mesa llvmpipe). The WASM build uses SDL3/WebGL instead.
#ifndef DD_HEADLESS_H
#define DD_HEADLESS_H

int  headless_init(int w, int h);      // create context + FBO; 1 on success
void headless_begin(void);             // bind the offscreen FBO + set viewport
int  headless_screenshot(const char* png_path);  // read FBO -> PNG (y-flipped)
void headless_shutdown(void);
int  headless_width(void);
int  headless_height(void);

#endif
