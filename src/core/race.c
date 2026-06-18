#include "race.h"
#include <math.h>
#include <string.h>

#define CAR_R 1.4f   // collision radius

static unsigned rng_next(unsigned* s){ *s = *s*1664525u + 1013904223u; return *s; }
static float rng_f(unsigned* s){ return (float)(rng_next(s) & 0xffffff) / (float)0x1000000; }

int race_init(Race* r, const char* dat, int ncars, int laps, unsigned seed){
    memset(r, 0, sizeof(*r));
    if (!track_load(dat, &r->track)) return 0;
    if (ncars < 1) ncars = 1; if (ncars > RACE_MAX_CARS) ncars = RACE_MAX_CARS;
    r->ncars = ncars; r->target_laps = laps; r->seed = seed; r->time = 0; r->done = 0;
    unsigned s = seed ? seed : 1u;
    for (int i = 0; i < ncars; i++) {
        int row = i / 2, col = i % 2;
        float start_s = 8.0f + row * 7.0f;
        float lateral = (col ? 1.0f : -1.0f) * 3.0f;
        float skill = 0.7f + 0.3f * rng_f(&s);
        vehicle_init(&r->cars[i], &r->track, i, start_s, lateral, skill);
        // spread preferred racing lines across the width so the field fans out
        r->cars[i].pref_lat = (((i % 4) - 1.5f) * 2.0f);
    }
    // generous time budget
    float lap_est = r->track.total_len / 9.0f;     // ~9 m/s pessimistic avg
    r->max_time = lap_est * (laps + 1) + 90.0f;
    return 1;
}

static void resolve_collisions(Race* r){
    for (int i = 0; i < r->ncars; i++)
        for (int j = i+1; j < r->ncars; j++) {
            Car* a = &r->cars[i]; Car* b = &r->cars[j];
            float dx = b->pos.x - a->pos.x, dz = b->pos.z - a->pos.z;
            float d2 = dx*dx + dz*dz;
            float mind = 2*CAR_R;
            if (d2 < mind*mind && d2 > 1e-4f) {
                float d = sqrtf(d2);
                float push = (mind - d) * 0.5f;
                float nx = dx/d, nz = dz/d;
                a->pos.x -= nx*push; a->pos.z -= nz*push;
                b->pos.x += nx*push; b->pos.z += nz*push;
                // gentle bump: only the faster car gives up a little speed; no grinding drain
                if (a->speed > b->speed) a->speed *= 0.94f; else b->speed *= 0.94f;
            }
        }
}

void race_step(Race* r, float dt){
    if (r->done) return;
    for (int i = 0; i < r->ncars; i++) {
        Car* c = &r->cars[i];
        if (c->finished) { c->throttle=0; c->brake=1; vehicle_step(c, &r->track, dt); continue; }
        vehicle_ai(c, &r->track);
        vehicle_step(c, &r->track, dt);
        if (c->lap >= r->target_laps && !c->finished) { c->finished = 1; c->finish_time = r->time; }
    }
    resolve_collisions(r);
    r->time += dt;

    int all = 1;
    for (int i = 0; i < r->ncars; i++) if (!r->cars[i].finished) all = 0;
    if (all || r->time >= r->max_time) r->done = 1;
}

int race_done(const Race* r){ return r->done; }

int race_rank(const Race* r, int* order){
    for (int i = 0; i < r->ncars; i++) order[i] = i;
    // sort: finished-first by finish_time, then by (lap, dist)
    for (int i = 0; i < r->ncars; i++)
        for (int j = i+1; j < r->ncars; j++) {
            const Car* a = &r->cars[order[i]]; const Car* b = &r->cars[order[j]];
            int swap = 0;
            if (a->finished != b->finished) swap = b->finished;
            else if (a->finished) swap = b->finish_time < a->finish_time;
            else { float pa = a->lap*1e7f + a->dist, pb = b->lap*1e7f + b->dist; swap = pb > pa; }
            if (swap) { int tmp = order[i]; order[i] = order[j]; order[j] = tmp; }
        }
    return r->ncars;
}
