#ifndef DD2_PLATFORM_CONTENT_H
#define DD2_PLATFORM_CONTENT_H

#include "assets/image.h"
#include "assets/track.h"

/* Loads committed DD2ROAD1, DD2SCN1 and DD2MESH2 content beneath a directory.
 * Borrows the immutable root string through every future provider load. File
 * bytes are released before publishing a track; no original-file fallback,
 * conversion or subdivision is performed. Missing assets fail transactionally. */
dd2_track_provider dd2_content_track_provider(const char *root);
/* Model texture callback; user points at the immutable root string. The caller
 * receives an owned PNG image. */
dd2_image *dd2_content_image(void *user, const char *resource);

#endif
