#include "headless.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../third_party/stb_image_write.h"

static EGLDisplay s_dpy = EGL_NO_DISPLAY;
static EGLContext s_ctx = EGL_NO_CONTEXT;
static GLuint s_fbo, s_color, s_depth;
static int s_w, s_h;

int headless_width(void)  { return s_w; }
int headless_height(void) { return s_h; }

int headless_init(int w, int h) {
    s_w = w; s_h = h;
    PFNEGLGETPLATFORMDISPLAYEXTPROC getPD =
        (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    if (getPD) s_dpy = getPD(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL);
    if (s_dpy == EGL_NO_DISPLAY) s_dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (s_dpy == EGL_NO_DISPLAY) { fprintf(stderr,"egl: no display\n"); return 0; }
    if (!eglInitialize(s_dpy, NULL, NULL)) { fprintf(stderr,"egl: init failed\n"); return 0; }
    eglBindAPI(EGL_OPENGL_ES_API);
    const EGLint ca[] = { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                          EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8, EGL_NONE };
    EGLConfig cfg; EGLint n=0;
    if (!eglChooseConfig(s_dpy, ca, &cfg, 1, &n) || n<1) { fprintf(stderr,"egl: chooseConfig\n"); return 0; }
    const EGLint xa[] = { EGL_CONTEXT_MAJOR_VERSION,3, EGL_CONTEXT_MINOR_VERSION,0, EGL_NONE };
    s_ctx = eglCreateContext(s_dpy, cfg, EGL_NO_CONTEXT, xa);
    if (s_ctx == EGL_NO_CONTEXT) { fprintf(stderr,"egl: createContext\n"); return 0; }
    if (!eglMakeCurrent(s_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, s_ctx)) {
        fprintf(stderr,"egl: makeCurrent 0x%x\n", eglGetError()); return 0;
    }
    glGenRenderbuffers(1, &s_color);
    glBindRenderbuffer(GL_RENDERBUFFER, s_color);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, w, h);
    glGenRenderbuffers(1, &s_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, s_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glGenFramebuffers(1, &s_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, s_color);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s_depth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr,"egl: FBO incomplete\n"); return 0;
    }
    fprintf(stderr,"headless: %s / %s  %dx%d\n", glGetString(GL_VERSION), glGetString(GL_RENDERER), w, h);
    return 1;
}

void headless_begin(void) {
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    glViewport(0, 0, s_w, s_h);
}

int headless_screenshot(const char* path) {
    unsigned char* px = (unsigned char*)malloc((size_t)s_w*s_h*4);
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    glReadPixels(0, 0, s_w, s_h, GL_RGBA, GL_UNSIGNED_BYTE, px);
    // flip vertically
    unsigned char* flip = (unsigned char*)malloc((size_t)s_w*s_h*4);
    for (int y = 0; y < s_h; y++)
        memcpy(flip + (size_t)(s_h-1-y)*s_w*4, px + (size_t)y*s_w*4, (size_t)s_w*4);
    int ok = stbi_write_png(path, s_w, s_h, 4, flip, s_w*4);
    free(px); free(flip);
    return ok;
}

void headless_shutdown(void) {
    if (s_dpy != EGL_NO_DISPLAY) { eglMakeCurrent(s_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (s_ctx != EGL_NO_CONTEXT) eglDestroyContext(s_dpy, s_ctx); eglTerminate(s_dpy); }
}
