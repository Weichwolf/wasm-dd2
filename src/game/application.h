#ifndef DD2_GAME_APPLICATION_H
#define DD2_GAME_APPLICATION_H

/* Main-thread UI bridge shared with the browser page. Selection is transactional:
 * failure preserves the previous track/materials/camera. No original addresses
 * or register state are used. A stopped/uninitialized application returns zero. */
int dd2_application_select_level(int number);
int dd2_application_show_car(int car);
int dd2_application_current_level(void);
int dd2_application_current_view(void);
/* Native: load beside Dirinfo from Redbook/trackNN.cdda. Browser: load the
 * staged /Music.cdda local file. Failure preserves the current playing track.
 * Phase -1 means no device/application; otherwise dd2_music_phase. */
int dd2_application_load_music(unsigned track);
int dd2_application_music_phase(void);
unsigned dd2_application_music_track(void);
unsigned dd2_application_music_frame(void);
unsigned dd2_application_music_fraction(void);
unsigned dd2_application_music_rate(void);
int dd2_application_set_music_playing(int playing);
int dd2_application_set_music_gain(unsigned gain);
unsigned dd2_application_collision_count(void);
unsigned dd2_application_pair_collision_count(void);
unsigned dd2_application_vehicle_count(void);
double dd2_application_engine_health(void);
double dd2_application_region_damage(unsigned region);
unsigned dd2_application_accident_points(void);
unsigned dd2_application_destructions(void);
unsigned dd2_application_accident_windows(void);
unsigned dd2_application_current_lap(void);
unsigned dd2_application_required_laps(void);
unsigned dd2_application_completed_laps(void);
unsigned dd2_application_lap_steps(void);
unsigned dd2_application_last_lap_steps(void);
unsigned dd2_application_best_lap_steps(void);
int dd2_application_laps_finished(void);
void dd2_application_reset_camera(void);
void dd2_application_release_input(void);
void dd2_application_resume_input(void);
int dd2_application_set_driving(int enabled);
int dd2_application_set_paused(int paused);
int dd2_application_is_paused(void);

/* Start a fresh race: 0 Wrecking (including arenas), 1 Stockcar, 2 Time Trial (circuits),
 * 3 Total Destruction (arenas).
 * Phase -1 means no race; otherwise dd2_race_phase. Results remain visible
 * until reset, view change or another race/track selection. */
int dd2_application_start_race(int mode);
int dd2_application_withdraw_race(void);
int dd2_application_race_phase(void);
unsigned dd2_application_race_steps(void);
unsigned dd2_application_race_place(void);
unsigned dd2_application_race_points(void);
unsigned dd2_application_survival_steps(void);
unsigned dd2_application_race_alive(void);

#endif
