#ifndef DD2_GAME_PROFILE_MENU_H
#define DD2_GAME_PROFILE_MENU_H

#include "assets/save_card.h"
#include "game/configuration.h"

#include <stdbool.h>

typedef enum {
    DD2_PROFILE_CLOSED,
    DD2_PROFILE_PLAYER_NAME,
    DD2_PROFILE_OPEN_SAVE,
    DD2_PROFILE_OPEN_LOAD,
    DD2_PROFILE_SAVE_SELECT,
    DD2_PROFILE_LOAD_SELECT,
    DD2_PROFILE_SAVE_NAME,
    DD2_PROFILE_CONFIRM,
    DD2_PROFILE_WRITING,
    DD2_PROFILE_MESSAGE
} dd2_profile_phase;
typedef struct {
    dd2_profile_phase phase;
    unsigned logical;
    char draft[DD2_CONFIGURATION_NAME_LIMIT + 1];
    const char *message; /* Borrowed immutable application status text. */
} dd2_profile_menu;

/* Main-thread frontend value owner. Draft edits stage complete text; invalid
 * bytes/overflow preserve it. An empty edit is valid and resolved on acceptance.
 * Application actions own storage and live-state transitions, not this editor. */
bool dd2_profile_menu_edit(dd2_profile_menu *menu, dd2_profile_phase phase, const char *text);
bool dd2_profile_menu_append(dd2_profile_menu *menu, const char *text);
void dd2_profile_menu_backspace(dd2_profile_menu *menu);
void dd2_profile_menu_move(dd2_profile_menu *menu, int direction);
bool dd2_profile_menu_editing(const dd2_profile_menu *menu);

#endif
