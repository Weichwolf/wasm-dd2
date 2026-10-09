#ifndef DD2_GAME_DRIVERS_H
#define DD2_GAME_DRIVERS_H

/* Source NPC roster, indexed by stable driver ID; zero is the default human
 * fallback. The application owns its active human name separately.
 * Borrowed immutable text.
 * Invalid driver IDs return NULL. */
const char *dd2_driver_name(unsigned driver);

#endif
