#include "game/audio.h"

#include "assets/archive.h"
#include "assets/audio.h"
#include "assets/bytes.h"
#include "audio/effects.h"
#include "audio/mixer.h"
#include "game/sound_events.h"
#include "platform/audio_device.h"
#include "platform/file.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { DD2_MUSIC_PATH_BYTES = 4096, DD2_MUSIC_DECIMAL_BASE = 10 };

struct dd2_game_audio {
    dd2_audio_device *device;
    dd2_file file;
    dd2_sound_bank *bank;
    dd2_effects *effects;
    char directory[DD2_MUSIC_PATH_BYTES];
};

void dd2_game_audio_destroy(dd2_game_audio *player) {
    if (player != NULL) {
        dd2_audio_device_destroy(player->device);
        dd2_file_release(&player->file);
        dd2_effects_destroy(player->effects);
        dd2_sound_bank_destroy(player->bank);
        free(player);
    }
}

dd2_game_audio *dd2_game_audio_create(const char *archive_path, const dd2_archive *archive) {
    if (archive_path == NULL) {
        return NULL;
    }
    const char *separator = strrchr(archive_path, '/');
    const size_t length = separator == NULL ? 0 : (size_t)(separator - archive_path) + 1;
    dd2_game_audio *player = calloc(1, sizeof(*player));
    if (player == NULL) {
        return NULL;
    }
    if (length >= sizeof(player->directory)) {
        dd2_game_audio_destroy(player);
        return NULL;
    }
    for (size_t index = 0; index < length; ++index) {
        player->directory[index] = archive_path[index];
    }
    dd2_asset asset = {0};
    if (dd2_archive_find(archive, "VAGS\\BANK1.SBK", &asset)) {
        player->bank = dd2_sound_bank_create((dd2_byte_view){asset.bytes, asset.size});
        player->effects = dd2_effects_create(player->bank);
    }
    if (player->effects == NULL) {
        dd2_game_audio_destroy(player);
        return NULL;
    }
    player->device = dd2_audio_device_create();
    if (player->device == NULL) {
        dd2_game_audio_destroy(player);
        return NULL;
    }
    dd2_game_audio_reset_effects(player);
    return player;
}

static bool dd2_game_audio_path(const char *directory, unsigned track,
                                char path[DD2_MUSIC_PATH_BYTES]) {
    const char suffix[] = "Redbook/track00.cdda";
    const size_t length = strlen(directory);
    if (length > DD2_MUSIC_PATH_BYTES - sizeof(suffix)) {
        return false;
    }
    for (size_t index = 0; index < length; ++index) {
        path[index] = directory[index];
    }
    for (size_t index = 0; index < sizeof(suffix); ++index) {
        path[length + index] = suffix[index];
    }
    const size_t digits = length + sizeof("Redbook/track") - 1;
    path[digits] = (char)('0' + (track / DD2_MUSIC_DECIMAL_BASE));
    path[digits + 1] = (char)('0' + (track % DD2_MUSIC_DECIMAL_BASE));
    return true;
}

static bool dd2_game_audio_music_replace(dd2_game_audio *player, unsigned track, const char *path,
                                         bool start) {
    if (player == NULL || track < DD2_MIXER_FIRST_TRACK || track > DD2_MIXER_LAST_TRACK) {
        return false;
    }
    char resolved[DD2_MUSIC_PATH_BYTES];
    if (path == NULL) {
        if (!dd2_game_audio_path(player->directory, track, resolved)) {
            return false;
        }
        path = resolved;
    }
    dd2_file next = {0};
    dd2_pcm_view pcm = {0};
    if (!dd2_file_read(path, &next) ||
        !dd2_cdda_decode((dd2_byte_view){next.data, next.size}, &pcm)) {
        dd2_file_release(&next);
        return false;
    }
    dd2_mixer *mixer = dd2_audio_device_acquire(player->device);
    const bool loaded = dd2_mixer_music_select(mixer, track, &pcm, true);
    if (loaded && start) {
        dd2_mixer_music_start(mixer);
    }
    dd2_audio_device_release(player->device);
    if (!loaded) {
        dd2_file_release(&next);
        return false;
    }
    dd2_file_release(&player->file);
    player->file = next;
    return true;
}

bool dd2_game_audio_music_load(dd2_game_audio *player, unsigned track, const char *path) {
    return dd2_game_audio_music_replace(player, track, path, true);
}

bool dd2_game_audio_music_prepare(dd2_game_audio *player, unsigned track, const char *path) {
    return dd2_game_audio_music_replace(player, track, path, false);
}

dd2_music_state dd2_game_audio_music_state(dd2_game_audio *player) {
    dd2_music_state state = {0};
    if (player != NULL) {
        dd2_mixer *mixer = dd2_audio_device_acquire(player->device);
        dd2_mixer_music(mixer, &state);
        dd2_audio_device_release(player->device);
    }
    return state;
}

uint32_t dd2_game_audio_rate(const dd2_game_audio *player) {
    return player != NULL ? dd2_audio_device_rate(player->device) : 0;
}

bool dd2_game_audio_music_play(dd2_game_audio *player, bool playing) {
    if (player == NULL) {
        return false;
    }
    dd2_mixer *mixer = dd2_audio_device_acquire(player->device);
    dd2_music_state state = {0};
    dd2_mixer_music(mixer, &state);
    bool changed = state.phase != DD2_MUSIC_EMPTY;
    if (playing) {
        changed = state.phase == DD2_MUSIC_PLAYING ||
                  (state.phase == DD2_MUSIC_PAUSED ? dd2_mixer_music_resume(mixer)
                                                   : dd2_mixer_music_start(mixer));
    } else if (state.phase == DD2_MUSIC_PLAYING) {
        changed = dd2_mixer_music_pause(mixer);
    }
    dd2_audio_device_release(player->device);
    return changed;
}

bool dd2_game_audio_music_gain(dd2_game_audio *player, unsigned gain) {
    if (player == NULL) {
        return false;
    }
    dd2_mixer *mixer = dd2_audio_device_acquire(player->device);
    const bool changed = dd2_mixer_music_gain(mixer, gain);
    dd2_audio_device_release(player->device);
    return changed;
}

void dd2_game_audio_suspend(dd2_game_audio *player, bool suspended) {
    if (player != NULL) {
        dd2_audio_device_set_paused(player->device, suspended);
    }
}

void dd2_game_audio_reset_effects(dd2_game_audio *audio) {
    if (audio != NULL) {
        dd2_mixer *mixer = dd2_audio_device_acquire(audio->device);
        dd2_effects_reset(audio->effects, mixer);
        dd2_audio_device_release(audio->device);
    }
}

bool dd2_game_audio_update(dd2_game_audio *audio, dd2_engine_sound engine,
                           const dd2_sound_batch *events) {
    if (audio == NULL) {
        return false;
    }
    dd2_mixer *mixer = dd2_audio_device_acquire(audio->device);
    const bool updated = dd2_effects_update(audio->effects, mixer, engine, events);
    dd2_audio_device_release(audio->device);
    return updated;
}

bool dd2_game_audio_effects_gain(dd2_game_audio *audio, unsigned gain) {
    if (audio == NULL) {
        return false;
    }
    dd2_mixer *mixer = dd2_audio_device_acquire(audio->device);
    const bool changed = dd2_effects_gain(audio->effects, mixer, gain);
    dd2_audio_device_release(audio->device);
    return changed;
}

bool dd2_game_audio_apply_gains(dd2_game_audio *audio, dd2_audio_gains gains) {
    if (audio == NULL || gains.effects > DD2_MIXER_GAIN_ONE || gains.music > DD2_MIXER_GAIN_ONE) {
        return false;
    }
    dd2_mixer *mixer = dd2_audio_device_acquire(audio->device);
    const bool valid = mixer != NULL && audio->effects != NULL;
    if (valid) {
        /* Both setters are allocation-free and accept these validated owners
         * and ranges. Keep the lock until both values have been applied. */
        dd2_effects_gain(audio->effects, mixer, gains.effects);
        dd2_mixer_music_gain(mixer, gains.music);
    }
    dd2_audio_device_release(audio->device);
    return valid;
}

dd2_effect_voice dd2_game_audio_effect_voice(dd2_game_audio *audio, unsigned channel) {
    dd2_effect_voice voice = {.sample = DD2_EFFECT_NO_SAMPLE};
    if (audio != NULL) {
        dd2_mixer *mixer = dd2_audio_device_acquire(audio->device);
        voice = dd2_effects_voice(audio->effects, mixer, channel);
        dd2_audio_device_release(audio->device);
    }
    return voice;
}

uint64_t dd2_game_audio_cue_count(dd2_game_audio *audio, dd2_sound_cue cue) {
    uint64_t count = 0;
    if (audio != NULL) {
        dd2_audio_device_acquire(audio->device);
        count = dd2_effects_played(audio->effects, cue);
        dd2_audio_device_release(audio->device);
    }
    return count;
}
