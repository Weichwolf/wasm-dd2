#ifndef DD2_ASSETS_BOUNDS_H
#define DD2_ASSETS_BOUNDS_H

/* Axis-aligned bounds in the same meter-space coordinates as owned models. */
typedef struct {
    float minimum[3];
    float maximum[3];
} dd2_bounds;

#endif
