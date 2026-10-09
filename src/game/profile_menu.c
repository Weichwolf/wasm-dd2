#include "game/profile_menu.h"

#include "assets/save_card.h"
#include "game/configuration.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

bool dd2_profile_menu_editing(const dd2_profile_menu *menu) {
    return menu != NULL &&
           (menu->phase == DD2_PROFILE_PLAYER_NAME || menu->phase == DD2_PROFILE_SAVE_NAME);
}
bool dd2_profile_menu_edit(dd2_profile_menu *menu, dd2_profile_phase phase, const char *text) {
    if (menu == NULL || (phase != DD2_PROFILE_PLAYER_NAME && phase != DD2_PROFILE_SAVE_NAME) ||
        !dd2_configuration_name_valid(text)) {
        return false;
    }
    char next[DD2_CONFIGURATION_NAME_LIMIT + 1] = {0};
    unsigned index = 0;
    do {
        next[index] = text[index];
    } while (text[index++] != '\0');
    for (index = 0; index < sizeof(next); ++index) {
        menu->draft[index] = next[index];
    }
    menu->phase = phase;
    return true;
}
bool dd2_profile_menu_append(dd2_profile_menu *menu, const char *text) {
    if (!dd2_profile_menu_editing(menu) || !dd2_configuration_name_valid(text) ||
        !dd2_configuration_name_valid(menu->draft)) {
        return false;
    }
    const size_t previous = strlen(menu->draft);
    const size_t added = strlen(text);
    if (previous + added > DD2_CONFIGURATION_NAME_LIMIT) {
        return false;
    }
    char next[DD2_CONFIGURATION_NAME_LIMIT + 1] = {0};
    for (size_t index = 0; index < previous; ++index) {
        next[index] = menu->draft[index];
    }
    for (size_t index = 0; index <= added; ++index) {
        next[previous + index] = text[index];
    }
    for (unsigned index = 0; index < sizeof(next); ++index) {
        menu->draft[index] = next[index];
    }
    return true;
}
void dd2_profile_menu_backspace(dd2_profile_menu *menu) {
    if (dd2_profile_menu_editing(menu) && dd2_configuration_name_valid(menu->draft)) {
        const size_t length = strlen(menu->draft);
        if (length != 0) {
            menu->draft[length - 1] = '\0';
        }
    }
}
void dd2_profile_menu_move(dd2_profile_menu *menu, int direction) {
    if (menu == NULL || menu->logical >= DD2_SAVE_CARD_SLOTS ||
        (menu->phase != DD2_PROFILE_SAVE_SELECT && menu->phase != DD2_PROFILE_LOAD_SELECT)) {
        return;
    }
    if (direction < 0 && menu->logical != 0) {
        --menu->logical;
    } else if (direction > 0 && menu->logical + 1 < DD2_SAVE_CARD_SLOTS) {
        ++menu->logical;
    }
}
