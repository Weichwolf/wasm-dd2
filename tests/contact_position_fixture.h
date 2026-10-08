#ifndef DD2_TEST_CONTACT_POSITION_FIXTURE_H
#define DD2_TEST_CONTACT_POSITION_FIXTURE_H

#include "physics/contact_group.h"

/* Arena-8 owner step 10574: only normal translation geometry is frozen here.
 * An independent enumeration of all 4096 active sets finds one admissible
 * least-norm solution; rows 3, 5 and 11 are released. Exact source query hash:
 * 6d2ebf8978e04f1c617f8e9e7713ad8b716162a7dd4d72030b5b1517ba1d6778.
 * No motion/friction is assigned: this isolates position branch selection. */
enum { DD2_POSITION_FIXTURE_CONTACTS = 12 };
static const dd2_group_contact dd2_position_fixture_contacts[] = {
    {.first = 4,
     .second = 5,
     .normal = {.x = -0.89601664477932341, .y = -0.24586117807762381, .z = 0.36973835802184052},
     .penetration = 0},
    {.first = 17,
     .second = DD2_VEHICLE_NO_PARTNER,
     .normal = {.x = -0.99794272839347342, .y = 0, .z = 0.064111706002804886},
     .penetration = -8.7119958657422113e-05},
    {.first = 19,
     .second = DD2_VEHICLE_NO_PARTNER,
     .normal = {.x = -0.99984994819060502, .y = 0, .z = -0.017322849166482471},
     .penetration = -8.6064910242566848e-05},
    {.first = 2,
     .second = 15,
     .normal = {.x = 0.88604738601495447, .y = 0.28054601673180668, .z = -0.36907175756481747},
     .penetration = -0.00019983922397592835},
    {.first = 2,
     .second = 17,
     .normal = {.x = -0.88594225651985725, .y = -0.28051938022302153, .z = 0.36934427764863992},
     .penetration = -6.217592026035168e-05},
    {.first = 5,
     .second = 13,
     .normal = {.x = 0.89611892329603271, .y = 0.24588947650667209, .z = -0.36947156948002591},
     .penetration = -0.00018221156524100834},
    {.first = 5,
     .second = 19,
     .normal = {.x = -0.86032024172733701, .y = -0.23667805086246119, .z = -0.45147821864865495},
     .penetration = -2.181705235670961e-05},
    {.first = 13,
     .second = 15,
     .normal = {.x = -0.90162871292027402, .y = -0.22418535493314751, .z = 0.36987374963793901},
     .penetration = -0.00010622679788907519},
    {.first = 14,
     .second = 15,
     .normal = {.x = -0.90464434696641705, .y = -0.21272710042221141, .z = 0.36927738388325815},
     .penetration = -9.2931255727179973e-05},
    {.first = 15,
     .second = 16,
     .normal = {.x = -0.88604651195285189, .y = -0.28053658201845971, .z = 0.36908102742566579},
     .penetration = -0.00019973219129454378},
    {.first = 16,
     .second = 17,
     .normal = {.x = -0.88604430709788473, .y = -0.28054374372930008, .z = 0.36908087692777886},
     .penetration = -6.2309282564239343e-05},
    {.first = 16,
     .second = 18,
     .normal = {.x = -0.88069393178281064, .y = -0.29742675269760649, .z = 0.368667228406175},
     .penetration = -0.00017273724545319169},
};
static const dd2_vehicle_vector dd2_position_fixture_offsets[] = {
    {.x = 0, .y = 0, .z = 0},
    {.x = 0, .y = 0, .z = 0},
    {.x = -5.4103440869765391e-05, .y = -1.7130985218311803e-05, .z = 2.2555416156404445e-05},
    {.x = 0, .y = 0, .z = 0},
    {.x = -0.00016366055496082942, .y = -4.4907398854646538e-05, .z = 6.7533996401443442e-05},
    {.x = -5.1128916676875332e-05, .y = -1.418217154872122e-05, .z = -0.00018025105609165414},
    {.x = 0, .y = 0, .z = 0},
    {.x = 0, .y = 0, .z = 0},
    {.x = 0, .y = 0, .z = 0},
    {.x = 0, .y = 0, .z = 0},
    {.x = 0, .y = 0, .z = 0},
    {.x = 0, .y = 0, .z = 0},
    {.x = 0, .y = 0, .z = 0},
    {.x = -0.00013858210178012694, .y = -3.4457728807607863e-05, .z = 5.6850320851144514e-05},
    {.x = -0.00015080969992123763, .y = -3.5462897974622097e-05, .z = 6.1560779811296632e-05},
    {.x = -4.8170932755916608e-05, .y = -3.6957166272599725e-05, .z = 2.2200028858872348e-05},
    {.x = -5.4025999432834586e-05, .y = -1.7108974342360184e-05, .z = 2.2504769019453247e-05},
    {.x = -2.2900825806320205e-05, .y = 0.00014111775261550167, .z = -0.0001555670854337988},
    {.x = 0, .y = 0, .z = 0},
    {.x = -1.5820832145633943e-05, .y = 5.9089570403367757e-05, .z = 0.00010872163266048011}};
#endif
