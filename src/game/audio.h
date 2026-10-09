#ifndef DD2_GAME_AUDIO_H
#define DD2_GAME_AUDIO_H

#include "assets/archive.h"
#include "audio/effects.h"
#include "audio/mixer.h"
#include "game/sound_events.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct dd2_game_audio dd2_game_audio;

/* Owns the device, sound-bank metadata/effects and one complete CDDA file.
 * Bank PCM borrows the archive, which must outlive this owner. Native paths are
 * resolved beside Dirinfo in Redbook/; a supplied path selects a browser-local
 * file instead. Loading is transactional and starts a repeating track. Failed
 * loading preserves the old PCM/transport. No proprietary data is bundled. */
dd2_game_audio *dd2_game_audio_create(const char *archive_path, const dd2_archive *archive);
void dd2_game_audio_destroy(dd2_game_audio *player);
bool dd2_game_audio_music_load(dd2_game_audio *player, unsigned track, const char *path);
/* Transactionally select a repeating track in READY at cursor zero, without
 * starting playback. Explicit play starts it later; failed preparation retains
 * the entire previous source/transport/gain. Device suspension is independent. */
bool dd2_game_audio_music_prepare(dd2_game_audio *player, unsigned track, const char *path);
bool dd2_game_audio_music_play(dd2_game_audio *player, bool playing);
bool dd2_game_audio_music_gain(dd2_game_audio *player, unsigned gain);
void dd2_game_audio_suspend(dd2_game_audio *player, bool suspended);
dd2_music_state dd2_game_audio_music_state(dd2_game_audio *player);
uint32_t dd2_game_audio_rate(const dd2_game_audio *player);

void dd2_game_audio_reset_effects(dd2_game_audio *audio);
bool dd2_game_audio_update(dd2_game_audio *audio, dd2_engine_sound engine,
                           const dd2_sound_batch *events);
bool dd2_game_audio_effects_gain(dd2_game_audio *audio, unsigned gain);
typedef struct {
    unsigned effects;
    unsigned music;
} dd2_audio_gains;
/* Validate both values before changing either; one device lock excludes the
 * callback from observing a partially applied preference load. */
bool dd2_game_audio_apply_gains(dd2_game_audio *audio, dd2_audio_gains gains);
dd2_effect_voice dd2_game_audio_effect_voice(dd2_game_audio *audio, unsigned channel);
uint64_t dd2_game_audio_cue_count(dd2_game_audio *audio, dd2_sound_cue cue);

#endif
