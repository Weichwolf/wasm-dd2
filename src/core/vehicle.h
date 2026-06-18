// Vehicle: arcade car physics + AI driver. Pure core (deterministic, no GL).
#ifndef DD_VEHICLE_H
#define DD_VEHICLE_H
#include "track.h"

typedef struct {
    int   id;
    vec3  pos;
    float yaw;          // heading (0 -> +z)
    float speed;        // m/s along heading
    // inputs (set by AI or player)
    float steer;        // [-1,1]
    float throttle;     // [0,1]
    float brake;        // [0,1]
    // race state
    float s;            // centerline arc-length
    float prev_s;
    int   lap;
    float dist;         // cumulative race distance (for ranking)
    int   finished;
    float finish_time;
    // ai
    float skill;        // 0..1
    float stuck_t;      // seconds at near-zero speed
    float pref_lat;     // preferred lateral racing-line offset (m)
} Car;

void vehicle_init(Car* c, const Track* t, int id, float start_s, float lateral, float skill);
void vehicle_ai(Car* c, const Track* t);              // sets steer/throttle/brake
void vehicle_step(Car* c, const Track* t, float dt);  // integrate physics + lap logic

#endif
