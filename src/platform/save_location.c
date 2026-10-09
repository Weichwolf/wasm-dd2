#include "platform/save_location.h"

#include "platform/save_store.h"

#include <stdbool.h>
#include <stddef.h>
#ifndef __EMSCRIPTEN__
#include <SDL_filesystem.h>
#include <SDL_stdinc.h>
#endif

bool dd2_save_store_open_user(dd2_save_store *store) {
#ifdef __EMSCRIPTEN__
    return dd2_save_store_open(store, "wasm-dd2-saves-v1");
#else
    char *directory = SDL_GetPrefPath("Weichwolf", "wasm-dd2");
    const bool accepted = directory != NULL && dd2_save_store_open(store, directory);
    SDL_free(directory);
    return accepted;
#endif
}
