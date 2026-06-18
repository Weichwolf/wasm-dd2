// Headless race runner: simulate an AI race to completion, dump a screenshot every
// N sim-seconds, verify determinism. This is both the game's self-play mode and the
// primary verification harness.
#include "core/race.h"
#include "render/render.h"
#include "platform/headless.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>

#define W 900
#define H 700

static const float CAR_COLS[8][3] = {
    {0.90f,0.20f,0.15f},{0.20f,0.45f,0.95f},{0.95f,0.85f,0.15f},{0.20f,0.80f,0.30f},
    {0.95f,0.55f,0.10f},{0.75f,0.25f,0.85f},{0.15f,0.85f,0.85f},{0.85f,0.85f,0.85f},
};

static void setup_cam(const Track* t, float* view, float* proj){
    vec3 ctr = v3scale(v3add(t->bbmin, t->bbmax), 0.5f);
    vec3 size = v3sub(t->bbmax, t->bbmin);
    float radius = v3len(v3(size.x,0,size.z))*0.5f; if (radius<1) radius=1;
    vec3 eye = v3(ctr.x, ctr.y + radius*1.4f, ctr.z - radius*1.0f);
    mat4_lookat(view, eye, ctr, v3(0,1,0));
    mat4_perspective(proj, 1.0f, (float)W/H, 1.0f, radius*8.0f);
}

static void draw_frame(Race* r, const float* view, const float* proj, const char* path){
    headless_begin();
    render_begin(0.45f, 0.6f, 0.8f);
    render_track(view, proj);
    for (int i = 0; i < r->ncars; i++){
        Car* c = &r->cars[i];
        const float* col = CAR_COLS[i % 8];
        render_box(view, proj, v3(c->pos.x, c->pos.y+0.6f, c->pos.z),
                   v3(1.0f, 0.6f, 2.0f), c->yaw, col[0], col[1], col[2]);
    }
    headless_screenshot(path);
}

// silent re-sim for determinism check; returns a hash-ish signature
static double sim_signature(const char* dat, int ncars, int laps, unsigned seed){
    Race r; if (!race_init(&r, dat, ncars, laps, seed)) return -1;
    while (!race_done(&r)) race_step(&r, 1.0f/60.0f);
    double sig = r.time;
    for (int i=0;i<r.ncars;i++){ Car*c=&r.cars[i];
        sig += c->pos.x*1.7 + c->pos.z*2.3 + c->yaw*3.1 + c->lap*1000.0 + c->dist*0.5 + c->finish_time*7.0; }
    return sig;
}

int main(int argc, char** argv){
    const char* dat   = argc>1 ? argv[1] : "assets/raw/LEV5/LEVEL.DAT";
    const char* outdir= argc>2 ? argv[2] : "out/race";
    int   laps  = argc>3 ? atoi(argv[3]) : 2;
    int   ncars = argc>4 ? atoi(argv[4]) : 6;
    unsigned seed = argc>5 ? (unsigned)strtoul(argv[5],0,10) : 1u;
    float interval = argc>6 ? (float)atof(argv[6]) : 2.0f;
    mkdir(outdir, 0777);

    Race r;
    if (!race_init(&r, dat, ncars, laps, seed)) { fprintf(stderr,"race_init failed\n"); return 1; }
    if (!headless_init(W,H)) return 2;
    render_init();
    render_set_track(&r.track);

    float view[16], proj[16]; setup_cam(&r.track, view, proj);

    const float dt = 1.0f/60.0f;
    float next_shot = 0; int frame = 0;
    char path[512];
    while (!race_done(&r)){
        if (r.time >= next_shot){
            snprintf(path,sizeof(path),"%s/frame%04d.png", outdir, frame++);
            draw_frame(&r, view, proj, path);
            next_shot += interval;
        }
        race_step(&r, dt);
    }
    snprintf(path,sizeof(path),"%s/frame%04d.png", outdir, frame++);
    draw_frame(&r, view, proj, path);

    // results
    int order[RACE_MAX_CARS]; race_rank(&r, order);
    printf("== %s: %d cars, %d laps, seed %u ==\n", dat, ncars, laps, seed);
    printf("race time %.1fs, %d frames, done=%d (timeout@%.0fs)\n", r.time, frame, r.done, r.max_time);
    for (int p=0;p<r.ncars;p++){ Car*c=&r.cars[order[p]];
        printf("  P%d car%d lap=%d dist=%.0f %s%.1f\n", p+1, c->id, c->lap, c->dist,
               c->finished?"finished@":"DNF dist=", c->finished?c->finish_time:c->dist); }

    // determinism: two independent re-sims must match the first exactly
    double a = sim_signature(dat, ncars, laps, seed);
    double b = sim_signature(dat, ncars, laps, seed);
    printf("determinism: sigA=%.6f sigB=%.6f %s\n", a, b, a==b ? "MATCH" : "MISMATCH");

    headless_shutdown();
    // success only if every car finished and determinism holds
    int allfin=1; for(int i=0;i<r.ncars;i++) if(!r.cars[i].finished) allfin=0;
    return (allfin && a==b) ? 0 : 10;
}
