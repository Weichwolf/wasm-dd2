// Vehicle: car physics + AI driver. Pure core (deterministic, no GL).
#ifndef DD_VEHICLE_H
#define DD_VEHICLE_H
#include "track.h"
#include "fixed.h"

typedef struct {
    int   id;
    vec3  pos;
    float yaw;          // heading (0 -> +z)
    float speed;        // m/s along heading
    // Q12 fixed-point sim state (DD2_FIXED): integer-deterministic dynamics per docs/spec/04.
    fx    qx, qz;       // Q12 world position (metres)
    fx    qspd;         // Q12 speed along heading (m/s)
    int   qyaw;         // heading as 0..0xFFF (0x1000 = full circle)
    // inputs (set by AI or player)
    float steer;        // [-1,1]
    float throttle;     // [0,1]
    float brake;        // [0,1]
    // race state
    float s;            // localized centerline arc-length (for AI lookahead)
    float prog;         // integrated forward progress (seam-free; drives laps)
    int   lap;
    float dist;         // cumulative race distance (for ranking) == prog
    int   finished;
    float finish_time;
    // ai
    float skill;        // 0..1
    float stuck_t;      // seconds at near-zero speed
    float pref_lat;     // preferred lateral racing-line offset (m)
    int   last_rib;     // last localized rib (for windowed/monotonic tracking)
    float prog_mark;    // progress watermark for stuck detection
    float since_prog;   // seconds since meaningful forward progress
    float recover_t;    // >0 => in reverse-and-realign recovery
    int   hits;         // collision count (demolition ranking)
    float vx, vz;       // world velocity vector (DD2 tire model only)
} Car;

void vehicle_init(Car* c, const Track* t, int id, float start_s, float lateral, float skill);
void vehicle_ai(Car* c, const Track* t);              // circuit AI: follow racing line
void vehicle_ai_arena(Car* c, const Track* t, vec3 target);  // demolition AI: head to target
void vehicle_step(Car* c, const Track* t, float dt);  // integrate physics + lap logic

#endif
