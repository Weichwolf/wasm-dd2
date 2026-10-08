#ifndef DD2_REWRITE_LINEAR_PRESSURE_FIXTURE_H
#define DD2_REWRITE_LINEAR_PRESSURE_FIXTURE_H

#include "physics/contact_group.h"

/* Actual WASM round-2 tick 137455 at cae5001; field isolation also fails
 * natively. Slot 13 is remapped to zero; other bodies are unconnected. These
 * are rewrite states on original level 2, not original executable output. */

/* Captured query SHA256: d56b87d85cf361f572e2738e670faca03003d0258c70a8aac7d5a4facdf0d031 */
static const dd2_vehicle dd2_friction_linear_pressure_native_bodies[] = {
    {.position = {.x = 27113.667784965084, .y = 8586.691316088918, .z = 67254.185065598736},
     .rotation = {.x = -0.86419561378510357,
                  .y = 0.21357876918606664,
                  .z = 0.42558588681254633,
                  .w = 0.16256292139824274},
     .velocity = {.x = -0.25189813452362786, .y = -10.229195475021111, .z = 0.50775783813677444},
     .angular_velocity = {.x = 0.018827248161562279,
                          .y = -0.00026907361605598042,
                          .z = -0.003526669826630492},
     .steps = 137256}};
static const dd2_group_contact dd2_friction_linear_pressure_native_contacts[] = {
    {.first = 0,
     .second = DD2_VEHICLE_NO_PARTNER,
     .point = {.x = 26849.58110398152, .y = 8640.7415983451156, .z = 66828.35318337864},
     .normal = {.x = 0.22940131075702708, .y = 0.97325399246863586, .z = -0.012316848899706152},
     .penetration = 4.5032337234185427e-15,
     .friction = 0.80000000000000004},
    {.first = 0,
     .second = DD2_VEHICLE_NO_PARTNER,
     .point = {.x = 27412.251575141847, .y = 8513.6914653390777, .z = 67268.830651344062},
     .normal = {.x = -0.65980117321510712, .y = 0, .z = 0.75144022505051478},
     .penetration = -0.00014072763792019033,
     .friction = 0.25},
    {.first = 0,
     .second = DD2_VEHICLE_NO_PARTNER,
     .point = {.x = 27377.754465948648, .y = 8532.6409763686715, .z = 67680.016947818833},
     .normal = {.x = 0.23136005789828434, .y = 0.97242396191320646, .z = -0.029396630866905662},
     .penetration = -5.5879418775648759e-05,
     .friction = 0.80000000000000004}};

/* Captured query SHA256: b14b63efa907c4a01eee41f8055f80d24099c62fc957b51054381f3cc17f06db */
static const dd2_vehicle dd2_friction_linear_pressure_wasm_bodies[] = {
    {.position = {.x = 27113.667784965084, .y = 8586.691316088918, .z = 67254.185065598736},
     .rotation = {.x = -0.86419561378510379,
                  .y = 0.21357876918606669,
                  .z = 0.42558588681254644,
                  .w = 0.16256292139824277},
     .velocity = {.x = -0.25189813452362608, .y = -10.229195475021111, .z = 0.507757838136774},
     .angular_velocity = {.x = 0.018827248161562286,
                          .y = -0.00026907361605598215,
                          .z = -0.0035266698266304903},
     .steps = 137256}};
static const dd2_group_contact dd2_friction_linear_pressure_wasm_contacts[] = {
    {.first = 0,
     .second = DD2_VEHICLE_NO_PARTNER,
     .point = {.x = 26849.58110398152, .y = 8640.7415983451156, .z = 66828.35318337864},
     .normal = {.x = 0.22940131075702708, .y = 0.97325399246863586, .z = -0.012316848899706152},
     .penetration = 7.1677689825189184e-15,
     .friction = 0.80000000000000004},
    {.first = 0,
     .second = DD2_VEHICLE_NO_PARTNER,
     .point = {.x = 27412.251575141847, .y = 8513.6914653390777, .z = 67268.830651344062},
     .normal = {.x = -0.65980117321510712, .y = 0, .z = 0.75144022505051478},
     .penetration = -0.00014072763792019033,
     .friction = 0.25},
    {.first = 0,
     .second = DD2_VEHICLE_NO_PARTNER,
     .point = {.x = 27377.754465948648, .y = 8532.6409763686715, .z = 67680.016947818833},
     .normal = {.x = 0.23136005789828434, .y = 0.97242396191320646, .z = -0.029396630866905662},
     .penetration = -5.5879418773872403e-05,
     .friction = 0.80000000000000004}};

#endif
