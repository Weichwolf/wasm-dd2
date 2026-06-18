// Race: N AI cars on a track, deterministic fixed-timestep simulation.
#ifndef DD_RACE_H
#define DD_RACE_H
#include "track.h"
#include "vehicle.h"

#define RACE_MAX_CARS 8

typedef struct {
    Track  track;
    Car    cars[RACE_MAX_CARS];
    int    ncars;
    int    target_laps;
    float  time;
    float  max_time;
    int    done;
    unsigned seed;
} Race;

int  race_init(Race* r, const char* dat, int ncars, int laps, unsigned seed);
void race_step(Race* r, float dt);     // one fixed step (advances time)
int  race_done(const Race* r);
// fill `order` with car indices ranked best-first; returns ncars
int  race_rank(const Race* r, int* order);

#endif
