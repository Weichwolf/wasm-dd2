#ifndef DD2_GAME_MUSIC_H
#define DD2_GAME_MUSIC_H

#include "audio/mixer.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct dd2_music_player dd2_music_player;

/* Owns the audio device and one bounded complete CDDA file. Native paths are
 * resolved beside Dirinfo in Redbook/; a supplied path selects a browser-local
 * file instead. Loading is transactional and starts a repeating track. Failed
 * loading preserves the old PCM/transport. No proprietary data is bundled. */
dd2_music_player *dd2_music_player_create(const char *archive_path);
void dd2_music_player_destroy(dd2_music_player *player);
bool dd2_music_player_load(dd2_music_player *player, unsigned track, const char *path);
bool dd2_music_player_play(dd2_music_player *player, bool playing);
bool dd2_music_player_gain(dd2_music_player *player, unsigned gain);
void dd2_music_player_suspend(dd2_music_player *player, bool suspended);
dd2_music_state dd2_music_player_state(dd2_music_player *player);
uint32_t dd2_music_player_rate(const dd2_music_player *player);

#endif
