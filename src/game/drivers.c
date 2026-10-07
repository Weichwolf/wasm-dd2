#include "game/drivers.h"

#include "game/league.h"

#include <stddef.h>

const char *dd2_driver_name(unsigned driver) {
    static const char *const names[DD2_LEAGUE_DRIVERS] = {
        "PLAYER",        "The Master",     "The Trashman",  "The Skum",    "The Pro",
        "The Goddess",   "Learner Driver", "Psycho",        "The Chief",   "The Optician",
        "The General",   "Heavy Metal H.", "Barmy Army",    "Pyromaniac",  "The Beast",
        "Passion Wagon", "The Undertaker", "Suicide Squad", "The Bouncer", "Rivit"};
    return driver < DD2_LEAGUE_DRIVERS ? names[driver] : NULL;
}
