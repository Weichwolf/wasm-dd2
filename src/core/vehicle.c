#include "vehicle.h"
#include <math.h>

#define MAXSPEED   42.0f
#define ACCEL      16.0f
#define BRAKE_DEC  30.0f
#define DRAG       0.45f
#define MAX_YAW    2.4f      // rad/s at full lock
#define CAR_HALF_W 1.1f
#define LOC_WINDOW 20        // rib search window for monotonic localization

static float wrap_angle(float a){ while(a>(float)M_PI)a-=2*(float)M_PI; while(a<-(float)M_PI)a+=2*(float)M_PI; return a; }
static float clampf(float x,float lo,float hi){ return x<lo?lo:(x>hi?hi:x); }

void vehicle_init(Car* c, const Track* t, int id, float start_s, float lateral, float skill){
    vec3 cp, tan; track_sample(t, start_s, &cp, &tan);
    vec3 right = v3(tan.z,0,-tan.x);
    c->id=id;
    c->pos = v3add(cp, v3scale(right, lateral));
    c->pos.y = cp.y + 0.5f;
    c->yaw = atan2f(tan.x, tan.z);
    c->speed=0; c->steer=0; c->throttle=0; c->brake=0;
    c->s=start_s; c->prog=0; c->lap=0; c->dist=0;
    c->finished=0; c->finish_time=0; c->skill=skill; c->stuck_t=0;
    c->pref_lat=lateral; c->prog_mark=0; c->since_prog=0; c->recover_t=0; c->hits=0;
    // initial rib index for start_s
    c->last_rib = 0;
    for (int i = 0; i < t->nribs; i++) if (t->s_at[i] <= start_s) c->last_rib = i; else break;
}

void vehicle_ai(Car* c, const Track* t){
    TrackPoint tp = track_locate_local(t, c->pos, c->last_rib, LOC_WINDOW);

    // Recovery: wedged against a wall (no forward progress). Back up + realign to track.
    if (c->recover_t > 0) {
        c->throttle = -1.0f;   // reverse
        c->brake = 0.0f;
        // steer so that reversing pivots the nose back toward the track centerline
        float toward_center = -clampf(tp.lateral / (tp.halfwidth + 0.01f), -1.0f, 1.0f);
        c->steer = -clampf(toward_center, -1.0f, 1.0f);  // reversed steering while backing up
        return;
    }

    float speed_norm = c->speed / MAXSPEED;
    float look = 7.0f + 26.0f * speed_norm;            // m ahead
    vec3 target, ttan; track_sample(t, tp.s + look, &target, &ttan);

    // desired heading toward look-ahead target, biased back to centerline
    float desired = atan2f(target.x - c->pos.x, target.z - c->pos.z);
    float err = wrap_angle(desired - c->yaw);
    // bias toward this car's preferred racing line (keeps the field spread out)
    float pref = clampf(c->pref_lat, -(tp.halfwidth-1.5f), tp.halfwidth-1.5f);
    float center_bias = -clampf((tp.lateral - pref) / (tp.halfwidth + 0.01f), -1.0f, 1.0f) * 0.5f;
    c->steer = clampf(err * 1.6f + center_bias, -1.0f, 1.0f);

    // upcoming curvature -> target speed
    vec3 t1pos, t1tan, t2pos, t2tan;
    track_sample(t, tp.s + look,        &t1pos, &t1tan);
    track_sample(t, tp.s + look + 18.0f,&t2pos, &t2tan);
    float bend = fabsf(wrap_angle(atan2f(t2tan.x,t2tan.z) - atan2f(t1tan.x,t1tan.z)));
    float corner = clampf(bend * 1.4f, 0.0f, 0.85f);
    float target_speed = MAXSPEED * (1.0f - corner) * (0.86f + 0.14f*c->skill);
    // also slow if pointing far from desired (recovering)
    target_speed *= clampf(1.0f - fabsf(err)*0.5f, 0.35f, 1.0f);

    if (c->speed < target_speed) { c->throttle = 1.0f; c->brake = 0.0f; }
    else if (c->speed > target_speed*1.08f) { c->throttle = 0.0f; c->brake = 0.6f; }
    else { c->throttle = 0.3f; c->brake = 0.0f; }

    // unstick
    if (c->stuck_t > 1.2f) { c->throttle = 1.0f; c->brake = 0.0f;
        c->steer = clampf(center_bias*3.0f + (c->id%2?0.5f:-0.5f), -1.0f, 1.0f); }
}

void vehicle_ai_arena(Car* c, const Track* t, vec3 target){
    (void)t;
    if (c->recover_t > 0) {   // shared reverse-and-realign
        c->throttle = -1.0f; c->brake = 0.0f; c->steer = (c->id % 2) ? 0.6f : -0.6f; return;
    }
    float desired = atan2f(target.x - c->pos.x, target.z - c->pos.z);
    float err = wrap_angle(desired - c->yaw);
    c->steer = clampf(err * 1.7f, -1.0f, 1.0f);
    float tgt = MAXSPEED * (0.45f + 0.15f*c->skill);
    if (c->speed < tgt) { c->throttle = 1.0f; c->brake = 0.0f; }
    else { c->throttle = 0.25f; c->brake = 0.0f; }
}

void vehicle_step(Car* c, const Track* t, float dt){
    if (c->finished) { c->throttle = 0; c->brake = 1; }
    // longitudinal (throttle may be negative for reverse during recovery)
    c->speed += c->throttle * ACCEL * dt;
    c->speed -= c->brake * BRAKE_DEC * dt;
    c->speed -= DRAG * c->speed * dt;
    if (c->speed < -8.0f) c->speed = -8.0f;
    if (c->speed > MAXSPEED) c->speed = MAXSPEED;

    // steering -> yaw (effective once rolling)
    float eff = c->speed / (c->speed + 5.0f);
    c->yaw += c->steer * MAX_YAW * eff * dt;

    // integrate
    vec3 head = v3(sinf(c->yaw), 0, cosf(c->yaw));
    vec3 oldpos = c->pos;
    vec3 newpos = v3add(c->pos, v3scale(head, c->speed*dt));

    // localize (windowed) + keep on road
    TrackPoint tp = track_locate_local(t, newpos, c->last_rib, LOC_WINDOW);
    c->last_rib = tp.rib;
    float limit = tp.halfwidth - CAR_HALF_W;
    if (limit < 0.5f) limit = 0.5f;
    if (fabsf(tp.lateral) > limit) {
        float sgn = tp.lateral > 0 ? 1.0f : -1.0f;
        newpos = v3add(tp.center, v3scale(tp.right, sgn*limit));
        c->speed *= 0.80f;                  // scrape the wall
        float talign = atan2f(tp.tangent.x, tp.tangent.z);
        c->yaw += wrap_angle(talign - c->yaw) * 0.20f;
    }
    newpos.y = tp.center.y + 0.5f;
    c->pos = newpos;

    // progress / laps: integrate forward motion along the track tangent (seam-free).
    vec3 disp = v3sub(c->pos, oldpos);
    float fwd = disp.x*tp.tangent.x + disp.z*tp.tangent.z;
    c->prog += fwd;
    if (c->prog < 0) c->prog = 0;
    c->dist = c->prog;
    c->lap = (int)(c->prog / t->total_len);
    c->s = tp.s;   // localized arc-length for AI lookahead

    // progress-based stuck detection (catches wall-wedges where speed stays nonzero)
    if (c->prog > c->prog_mark + 4.0f) { c->prog_mark = c->prog; c->since_prog = 0; }
    else c->since_prog += dt;
    if (c->recover_t > 0) {
        c->recover_t -= dt;    // since_prog keeps counting; only real progress resets it
    } else if (c->since_prog > 2.0f) {
        c->recover_t = 0.8f;   // begin reverse-and-realign
    }
    // Escalation: if still no progress after sustained recovery attempts, respawn onto the
    // racing line just ahead (standard stuck-rescue; keeps progress monotonic + deterministic).
    if (c->since_prog > 5.0f && !c->finished) {
        float ts = c->s + 6.0f;
        vec3 p, tan; track_sample(t, ts, &p, &tan);
        c->pos = v3(p.x, p.y + 0.5f, p.z);
        c->yaw = atan2f(tan.x, tan.z);
        c->speed = 3.0f;
        c->prog += 6.0f;
        c->lap = (int)(c->prog / t->total_len);
        c->recover_t = 0; c->since_prog = 0; c->prog_mark = c->prog;
        float tsm = fmodf(ts, t->total_len); if (tsm < 0) tsm += t->total_len;
        c->last_rib = 0;
        for (int i = 0; i < t->nribs; i++) { if (t->s_at[i] <= tsm) c->last_rib = i; else break; }
    }
    if (c->speed < 1.0f) c->stuck_t += dt; else c->stuck_t = 0;
}
