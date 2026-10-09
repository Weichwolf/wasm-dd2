#ifndef DD2_ASSETS_CAR_CLASS_H
#define DD2_ASSETS_CAR_CLASS_H

typedef enum { DD2_CAR_ROOKIE, DD2_CAR_AMATEUR, DD2_CAR_PRO, DD2_CAR_CLASSES } dd2_car_class;
enum { DD2_CAR_HANDLING_ONE = 4096 };
typedef struct {
    unsigned front;
    unsigned rear;
} dd2_car_axle_weights;
typedef struct {
    unsigned acceleration;
    unsigned top_speed;
    unsigned grip;
} dd2_car_ratings;
typedef struct {
    const char *name;
    dd2_car_ratings ratings;
    unsigned traction;
    unsigned rear_grip;
    dd2_car_axle_weights coasting;
    dd2_car_axle_weights negative_drive;
} dd2_car_handling;

/* Immutable original class identity, displayed ratings and fixed-point handling
 * coefficients. Positive drive uses equal axle weights. The steered axle is
 * front; original class grip scaling applies to the rear axle only. Ratings are
 * presentation values, not physical multipliers. Invalid identity returns NULL. */
const dd2_car_handling *dd2_car_class_handling(dd2_car_class car_class);

#endif
