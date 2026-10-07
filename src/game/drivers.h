#ifndef DD2_GAME_DRIVERS_H
#define DD2_GAME_DRIVERS_H

/* Source NPC roster, indexed by stable driver ID; zero is the default human
 * display name until profile/name entry is connected. Borrowed immutable text.
 * Invalid driver IDs return NULL. */
const char *dd2_driver_name(unsigned driver);

#endif
