#include "assets/car_class.h"

#include <stddef.h>

static const dd2_car_handling dd2_car_handling_table[DD2_CAR_CLASSES] = {
    {.name = "Rookie",
     .ratings = {1, 2, 5},
     .traction = 3900,
     .rear_grip = 5000,
     .coasting = {.front = 1950, .rear = 2146},
     .negative_drive = {.front = 2048, .rear = 2048}},
    {.name = "Amateur",
     .ratings = {3, 4, 2},
     .traction = 4000,
     .rear_grip = 4500,
     .coasting = {.front = 1900, .rear = 2196},
     .negative_drive = {.front = 2370, .rear = 1726}},
    {.name = "Pro",
     .ratings = {5, 5, 1},
     .traction = DD2_CAR_HANDLING_ONE,
     .rear_grip = DD2_CAR_HANDLING_ONE,
     .coasting = {.front = 1820, .rear = 2276},
     .negative_drive = {.front = 2670, .rear = 1426}}};

const dd2_car_handling *dd2_car_class_handling(dd2_car_class car_class) {
    return (unsigned)car_class < DD2_CAR_CLASSES ? &dd2_car_handling_table[car_class] : NULL;
}
