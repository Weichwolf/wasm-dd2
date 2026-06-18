// SDL3 + WebGL2 (Emscripten) frontend: self-playing AI races on the real DD2 tracks,
// cycling through the working circuits + arenas. Shares the same core + renderer as the
// headless build. (DVD-look present pass is layered on top in a later step.)
#include "core/race.h"
#include "render/render.h"
#include <SDL3/SDL.h>
#include <GLES3/gl3.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define CANVAS_W 960
#define CANVAS_H 720

static const char* PLAYLIST[] = {
    "assets/raw/LEV5/LEVEL.DAT", "assets/raw/LEV1/LEVEL.DAT", "assets/raw/LEV3/LEVEL.DAT",
    "assets/raw/LEV4/LEVEL.DAT", "assets/raw/LEV6/LEVEL.DAT", "assets/raw/LEV8/LEVEL.DAT",
    "assets/raw/LEV9/LEVEL.DAT", "assets/raw/LEVA/LEVEL.DAT", "assets/raw/LEVB/LEVEL.DAT",
};
static const int PLAYLIST_N = sizeof(PLAYLIST)/sizeof(PLAYLIST[0]);

static const float CAR_COLS[8][3] = {
    {0.90f,0.20f,0.15f},{0.20f,0.45f,0.95f},{0.95f,0.85f,0.15f},{0.20f,0.80f,0.30f},
    {0.95f,0.55f,0.10f},{0.75f,0.25f,0.85f},{0.15f,0.85f,0.85f},{0.85f,0.85f,0.85f},
};

static SDL_Window* g_win;
static Race  g_race;
static int   g_track = 0;
static unsigned g_seed = 1;
static float g_view[16], g_proj[16];

static void setup_cam(const Track* t){
    vec3 ctr = v3scale(v3add(t->bbmin, t->bbmax), 0.5f);
    vec3 size = v3sub(t->bbmax, t->bbmin);
    float radius = v3len(v3(size.x,0,size.z))*0.5f; if (radius<1) radius=1;
    vec3 eye = v3(ctr.x, ctr.y + radius*1.35f, ctr.z - radius*1.0f);
    mat4_lookat(g_view, eye, ctr, v3(0,1,0));
    mat4_perspective(g_proj, 1.0f, (float)CANVAS_W/CANVAS_H, 1.0f, radius*8.0f);
}

static void load_track(int idx){
    g_track = idx % PLAYLIST_N;
    if (!race_init(&g_race, PLAYLIST[g_track], 6, 2, g_seed)) {
        fprintf(stderr, "failed to load %s\n", PLAYLIST[g_track]); return;
    }
    render_set_track(&g_race.track);
    setup_cam(&g_race.track);
    SDL_Log("track %d: %s (%d ribs, %s)", g_track, PLAYLIST[g_track],
            g_race.track.nribs, g_race.arena ? "arena" : "circuit");
}

static void frame(void){
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_N) {
            load_track(g_track + 1);   // skip to next track
        }
    }
    // advance simulation (one fixed step per display frame ~ real time)
    if (!race_done(&g_race)) race_step(&g_race, 1.0f/60.0f);
    else { g_seed++; load_track(g_track + 1); }   // race over -> next track

    render_begin(0.45f, 0.6f, 0.8f);
    render_track(g_view, g_proj);
    for (int i = 0; i < g_race.ncars; i++) {
        Car* c = &g_race.cars[i];
        const float* col = CAR_COLS[i % 8];
        render_box(g_view, g_proj, v3(c->pos.x, c->pos.y+0.6f, c->pos.z),
                   v3(1.0f,0.6f,2.0f), c->yaw, col[0], col[1], col[2]);
    }
    SDL_GL_SwapWindow(g_win);
}

int main(void){
    if (!SDL_Init(SDL_INIT_VIDEO)) { SDL_Log("SDL_Init: %s", SDL_GetError()); return 1; }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    g_win = SDL_CreateWindow("Destruction Derby 2 — WASM", CANVAS_W, CANVAS_H, SDL_WINDOW_OPENGL);
    if (!g_win) { SDL_Log("CreateWindow: %s", SDL_GetError()); return 1; }
    if (!SDL_GL_CreateContext(g_win)) { SDL_Log("GL ctx: %s", SDL_GetError()); return 1; }
    SDL_Log("GL_VERSION: %s", (const char*)glGetString(GL_VERSION));

    render_init();
    load_track(0);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame, 0, 1);
#else
    for (;;) { frame(); SDL_Delay(16); }
#endif
    return 0;
}
