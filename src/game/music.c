#include "game/music.h"

#include "assets/audio.h"
#include "assets/bytes.h"
#include "audio/mixer.h"
#include "platform/audio_device.h"
#include "platform/file.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { DD2_MUSIC_PATH_BYTES = 4096, DD2_MUSIC_DECIMAL_BASE = 10 };

struct dd2_music_player {
    dd2_audio_device *device;
    dd2_file file;
    char directory[DD2_MUSIC_PATH_BYTES];
};

void dd2_music_player_destroy(dd2_music_player *player) {
    if (player != NULL) {
        dd2_audio_device_destroy(player->device);
        dd2_file_release(&player->file);
        free(player);
    }
}

dd2_music_player *dd2_music_player_create(const char *archive_path) {
    if (archive_path == NULL) {
        return NULL;
    }
    const char *separator = strrchr(archive_path, '/');
    const size_t length = separator == NULL ? 0 : (size_t)(separator - archive_path) + 1;
    dd2_music_player *player = calloc(1, sizeof(*player));
    if (player == NULL) {
        return NULL;
    }
    if (length >= sizeof(player->directory)) {
        dd2_music_player_destroy(player);
        return NULL;
    }
    for (size_t index = 0; index < length; ++index) {
        player->directory[index] = archive_path[index];
    }
    player->device = dd2_audio_device_create();
    if (player->device == NULL) {
        dd2_music_player_destroy(player);
        return NULL;
    }
    return player;
}

static bool dd2_music_player_path(const char *directory, unsigned track,
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

bool dd2_music_player_load(dd2_music_player *player, unsigned track, const char *path) {
    if (player == NULL || track < DD2_MIXER_FIRST_TRACK || track > DD2_MIXER_LAST_TRACK) {
        return false;
    }
    char resolved[DD2_MUSIC_PATH_BYTES];
    if (path == NULL) {
        if (!dd2_music_player_path(player->directory, track, resolved)) {
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
    if (loaded) {
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

dd2_music_state dd2_music_player_state(dd2_music_player *player) {
    dd2_music_state state = {0};
    if (player != NULL) {
        dd2_mixer *mixer = dd2_audio_device_acquire(player->device);
        dd2_mixer_music(mixer, &state);
        dd2_audio_device_release(player->device);
    }
    return state;
}

uint32_t dd2_music_player_rate(const dd2_music_player *player) {
    return player != NULL ? dd2_audio_device_rate(player->device) : 0;
}

bool dd2_music_player_play(dd2_music_player *player, bool playing) {
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

bool dd2_music_player_gain(dd2_music_player *player, unsigned gain) {
    if (player == NULL) {
        return false;
    }
    dd2_mixer *mixer = dd2_audio_device_acquire(player->device);
    const bool changed = dd2_mixer_music_gain(mixer, gain);
    dd2_audio_device_release(player->device);
    return changed;
}

void dd2_music_player_suspend(dd2_music_player *player, bool suspended) {
    if (player != NULL) {
        dd2_audio_device_set_paused(player->device, suspended);
    }
}
