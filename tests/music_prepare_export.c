#include "assets/archive.h"
#include "audio/mixer.h"
#include "game/audio.h"
#include "platform/file.h"

#include <SDL_timer.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_PREPARE_GAIN = 128,
    DD2_PREPARE_WAIT_MS = 160,
    DD2_PREPARE_ARGUMENTS = 4,
    DD2_PREPARE_INITIAL_TRACK = 13,
    DD2_PREPARE_NEXT_TRACK = 14
};

static unsigned dd2_prepare_checks;

static bool dd2_prepare_expect(bool valid, const char *label) {
    ++dd2_prepare_checks;
    if (!valid) {
        if (fputs("Music preparation failed: ", stderr) == EOF || fputs(label, stderr) == EOF ||
            fputc('\n', stderr) == EOF) {
            return false;
        }
    }
    return valid;
}

static bool dd2_prepare_same(dd2_music_state before, dd2_music_state after) {
    return before.phase == after.phase && before.track == after.track &&
           before.frame == after.frame && before.fraction == after.fraction &&
           before.gain == after.gain && before.repeat == after.repeat;
}

static bool dd2_prepare_ready(dd2_game_audio *audio, unsigned track) {
    const dd2_music_state state = dd2_game_audio_music_state(audio);
    return state.phase == DD2_MUSIC_READY && state.track == track && state.frame == 0 &&
           state.fraction == 0 && state.repeat && state.gain == DD2_PREPARE_GAIN;
}

static bool dd2_prepare_silent(dd2_game_audio *audio, unsigned track) {
    dd2_game_audio_suspend(audio, false);
    SDL_Delay(DD2_PREPARE_WAIT_MS);
    dd2_game_audio_suspend(audio, true);
    return dd2_prepare_expect(dd2_prepare_ready(audio, track), "READY callback cursor");
}

static bool dd2_prepare_failure(dd2_game_audio *audio, const char *malformed) {
    const dd2_music_state before = dd2_game_audio_music_state(audio);
    return dd2_prepare_expect(!dd2_game_audio_music_prepare(audio, 1, malformed), "low track") &&
           dd2_prepare_expect(
               !dd2_game_audio_music_prepare(audio, DD2_MIXER_LAST_TRACK + 1, malformed),
               "high track") &&
           dd2_prepare_expect(
               !dd2_game_audio_music_prepare(audio, DD2_MIXER_FIRST_TRACK, malformed),
               "malformed PCM") &&
           dd2_prepare_expect(!dd2_game_audio_music_prepare(audio, DD2_MIXER_FIRST_TRACK, ""),
                              "missing file") &&
           dd2_prepare_expect(dd2_prepare_same(before, dd2_game_audio_music_state(audio)),
                              "failed preparation preserves complete state");
}

static bool dd2_prepare_play(dd2_game_audio *audio) {
    const dd2_music_state before = dd2_game_audio_music_state(audio);
    if (!dd2_prepare_expect(dd2_game_audio_music_play(audio, true), "explicit start/resume")) {
        return false;
    }
    dd2_game_audio_suspend(audio, false);
    SDL_Delay(DD2_PREPARE_WAIT_MS);
    dd2_game_audio_suspend(audio, true);
    const dd2_music_state after = dd2_game_audio_music_state(audio);
    return dd2_prepare_expect(
        after.phase == DD2_MUSIC_PLAYING && after.track == before.track &&
            after.gain == before.gain && after.repeat &&
            (before.frame != after.frame || before.fraction != after.fraction),
        "actual callback advances selected source");
}

static bool dd2_prepare_transport(dd2_game_audio *audio, const char *pattern,
                                  const char *malformed) {
    if (!dd2_prepare_expect(dd2_game_audio_music_gain(audio, DD2_PREPARE_GAIN), "gain") ||
        !dd2_prepare_expect(dd2_game_audio_music_prepare(audio, DD2_PREPARE_INITIAL_TRACK, pattern),
                            "initial prepare") ||
        !dd2_prepare_expect(dd2_prepare_ready(audio, DD2_PREPARE_INITIAL_TRACK), "initial READY") ||
        !dd2_prepare_silent(audio, DD2_PREPARE_INITIAL_TRACK) ||
        !dd2_prepare_failure(audio, malformed) || !dd2_prepare_play(audio) ||
        !dd2_prepare_failure(audio, malformed)) {
        return false;
    }
    if (!dd2_prepare_expect(dd2_game_audio_music_prepare(audio, DD2_PREPARE_NEXT_TRACK, pattern),
                            "playing replacement") ||
        !dd2_prepare_expect(dd2_prepare_ready(audio, DD2_PREPARE_NEXT_TRACK),
                            "replacement READY") ||
        !dd2_prepare_silent(audio, DD2_PREPARE_NEXT_TRACK) || !dd2_prepare_play(audio) ||
        !dd2_prepare_expect(dd2_game_audio_music_play(audio, false), "pause") ||
        !dd2_prepare_failure(audio, malformed)) {
        return false;
    }
    const dd2_music_state paused = dd2_game_audio_music_state(audio);
    dd2_game_audio_suspend(audio, false);
    SDL_Delay(DD2_PREPARE_WAIT_MS);
    dd2_game_audio_suspend(audio, true);
    return dd2_prepare_expect(paused.phase == DD2_MUSIC_PAUSED &&
                                  dd2_prepare_same(paused, dd2_game_audio_music_state(audio)),
                              "paused callback") &&
           dd2_prepare_play(audio) &&
           dd2_prepare_expect(dd2_game_audio_music_load(audio, DD2_MIXER_LAST_TRACK, pattern),
                              "immediate load") &&
           dd2_prepare_expect(dd2_game_audio_music_state(audio).phase == DD2_MUSIC_PLAYING,
                              "existing load starts immediately") &&
           dd2_prepare_play(audio);
}

int main(int argc, char **argv) {
    if (argc != DD2_PREPARE_ARGUMENTS) {
        if (fputs("Usage: music_prepare_export Dirinfo pattern.cdda malformed.cdda\n", stderr) ==
            EOF) {
            return EXIT_FAILURE;
        }
        return EXIT_FAILURE;
    }
    dd2_file file = {0};
    dd2_archive *archive = NULL;
    if (!dd2_file_read(argv[1], &file) ||
        dd2_archive_open(file.data, file.size, &archive) != DD2_ARCHIVE_OK) {
        dd2_file_release(&file);
        return EXIT_FAILURE;
    }
    dd2_game_audio *audio = dd2_game_audio_create(argv[1], archive);
    const bool valid =
        dd2_prepare_expect(audio != NULL, "device creation") &&
        dd2_prepare_transport(audio, argv[2], argv[3]) &&
        dd2_prepare_expect(!dd2_game_audio_music_prepare(NULL, DD2_PREPARE_INITIAL_TRACK, argv[2]),
                           "null owner");
    /* Closing joins the callback before either music or bank PCM is released. */
    dd2_game_audio_suspend(audio, false);
    dd2_game_audio_destroy(audio);
    dd2_archive_close(archive);
    dd2_file_release(&file);
    printf("{\"pass_\":%s,\"checks\":%u}\n", valid ? "true" : "false", dd2_prepare_checks);
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
