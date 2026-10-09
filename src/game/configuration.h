#ifndef DD2_GAME_CONFIGURATION_H
#define DD2_GAME_CONFIGURATION_H

#include "assets/bytes.h"
#include "assets/save_profile.h"

#include <stdbool.h>

enum { DD2_CONFIGURATION_EFFECTS_MAX = 4090, DD2_CONFIGURATION_EXTENSION_BYTES = 16 };
enum { DD2_CONFIGURATION_NAME_LIMIT = 8 };

/* Own all original profile fields. Applying audio preferences does not accept
 * a playable saved game or consume dormant championship/input selections.
 * The music extension occupies only the first sixteen unused suffix bytes;
 * unknown legacy suffixes preserve the current music gain on import. */
typedef struct {
    dd2_save_profile source;
    unsigned music_gain;
} dd2_configuration;

void dd2_configuration_defaults(dd2_configuration *configuration);
/* Exact complete CONFIG/STARTUP payload. Validate consumed effects gain and
 * recognized extension before publishing. Invalid input preserves out; the
 * fallback gain applies only to legacy profiles without a recognized extension. */
bool dd2_configuration_read(dd2_byte_view bytes, unsigned fallback_music_gain,
                            dd2_configuration *out);
/* Explicit save produces CONFIG, retains original fields/reserved suffix and
 * inserts a versioned, checked music record. Supports aliased value/output. */
bool dd2_configuration_write(const dd2_configuration *configuration, dd2_byte_buffer output);
unsigned dd2_configuration_effects_gain(const dd2_configuration *configuration);
bool dd2_configuration_set_effects(dd2_configuration *configuration, unsigned gain);
bool dd2_configuration_set_music(dd2_configuration *configuration, unsigned gain);
/* Single-player identity consumes player zero only. Empty legacy text displays
 * PLAYER; other dormant player/header fields remain untouched. Imported names
 * may use the complete eleven-character source field. New menu entries keep
 * the original eight-character contract; both consume printable ASCII. */
const char *dd2_configuration_player(const dd2_configuration *configuration);
bool dd2_configuration_set_player(dd2_configuration *configuration, const char *name);
bool dd2_configuration_name_valid(const char *name);
bool dd2_configuration_player_valid(const char *name);

#endif
