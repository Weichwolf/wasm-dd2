#ifndef DD2_GAME_APPLICATION_H
#define DD2_GAME_APPLICATION_H

/* Main-thread UI bridge shared with the browser page. Selection is transactional:
 * failure preserves the previous track/materials/camera. No original addresses
 * or register state are used. A stopped/uninitialized application returns zero. */
int dd2_application_select_level(int number);
int dd2_application_show_car(int car);
int dd2_application_current_level(void);
int dd2_application_current_view(void);
unsigned dd2_application_collision_count(void);
unsigned dd2_application_pair_collision_count(void);
unsigned dd2_application_vehicle_count(void);
void dd2_application_reset_camera(void);
void dd2_application_release_input(void);
void dd2_application_resume_input(void);
int dd2_application_set_driving(int enabled);
int dd2_application_set_paused(int paused);
int dd2_application_is_paused(void);

#endif
