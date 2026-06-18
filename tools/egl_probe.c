// De-risk: can we get a headless GLES3 context (no X, no window) and render
// offscreen to an FBO + read pixels? Uses EGL surfaceless (Mesa). If this works,
// the engine's headless screenshot path is viable with zero system installs.
//
//   cc tools/egl_probe.c -o /tmp/egl_probe -lEGL -lGLESv2 && /tmp/egl_probe
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <stdio.h>
#include <stdlib.h>

#define W 128
#define H 128

int main(void) {
    // Prefer the surfaceless platform; fall back to default display.
    EGLDisplay dpy = EGL_NO_DISPLAY;
    PFNEGLGETPLATFORMDISPLAYEXTPROC getPlatformDisplay =
        (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    if (getPlatformDisplay)
        dpy = getPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL);
    if (dpy == EGL_NO_DISPLAY)
        dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (dpy == EGL_NO_DISPLAY) { fprintf(stderr, "no EGL display\n"); return 2; }

    EGLint major, minor;
    if (!eglInitialize(dpy, &major, &minor)) { fprintf(stderr, "eglInitialize failed\n"); return 2; }
    printf("EGL %d.%d  vendor=%s\n", major, minor, eglQueryString(dpy, EGL_VENDOR));

    eglBindAPI(EGL_OPENGL_ES_API);
    const EGLint cfg_attr[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    EGLConfig cfg; EGLint n = 0;
    if (!eglChooseConfig(dpy, cfg_attr, &cfg, 1, &n) || n < 1) {
        fprintf(stderr, "eglChooseConfig failed (n=%d)\n", n); return 2;
    }
    const EGLint ctx_attr[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE };
    EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctx_attr);
    if (ctx == EGL_NO_CONTEXT) { fprintf(stderr, "eglCreateContext failed\n"); return 2; }

    // Surfaceless: make current with no draw/read surface (needs KHR_surfaceless_context).
    if (!eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx)) {
        fprintf(stderr, "surfaceless makeCurrent failed (0x%x)\n", eglGetError()); return 2;
    }
    printf("GL_VERSION  = %s\n", glGetString(GL_VERSION));
    printf("GL_RENDERER = %s\n", glGetString(GL_RENDERER));

    // Offscreen FBO
    GLuint rb, fbo;
    glGenRenderbuffers(1, &rb);
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, W, H);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "FBO incomplete\n"); return 2;
    }
    glViewport(0, 0, W, H);
    glClearColor(0.1f, 0.2f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    unsigned char px[4];
    glReadPixels(W/2, H/2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
    printf("center pixel RGBA = %d %d %d %d (expect ~26 51 204 255)\n", px[0], px[1], px[2], px[3]);

    int ok = (px[2] > 150 && px[0] < 80);
    printf("%s\n", ok ? "HEADLESS GLES3 RENDER: OK" : "HEADLESS GLES3 RENDER: WRONG PIXELS");
    return ok ? 0 : 1;
}
