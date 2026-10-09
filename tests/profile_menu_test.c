#include "assets/save_card.h"
#include "assets/save_profile.h"
#include "game/configuration.h"
#include "game/profile_menu.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void dd2_profile_test_require(bool good, const char *message) {
    if (!good) {
        if (fputs(message, stderr) == EOF) {
            abort();
        }
        exit(EXIT_FAILURE);
    }
}
static void dd2_profile_test_editor(void) {
    dd2_profile_menu menu = {0};
    dd2_profile_test_require(dd2_profile_menu_edit(&menu, DD2_PROFILE_PLAYER_NAME, ""),
                             "Cannot start empty name\n");
    for (unsigned character = ' '; character <= '~'; ++character) {
        const char text[] = {(char)character, '\0'};
        dd2_profile_test_require(dd2_profile_menu_append(&menu, text) &&
                                     menu.draft[0] == (char)character,
                                 "Printable ASCII name rejected\n");
        dd2_profile_menu_backspace(&menu);
    }
    dd2_profile_test_require(dd2_profile_menu_append(&menu, "Abcd") &&
                                 dd2_profile_menu_append(&menu, menu.draft) &&
                                 strcmp(menu.draft, "AbcdAbcd") == 0,
                             "Aliased editor append changed text\n");
    dd2_profile_test_require(
        !dd2_profile_menu_append(&menu, "X") && !dd2_profile_menu_append(&menu, "\n") &&
            !dd2_profile_menu_append(&menu, "\xc3\xa4") && strcmp(menu.draft, "AbcdAbcd") == 0,
        "Invalid editor input changed draft\n");
    dd2_profile_test_require(!dd2_profile_menu_edit(&menu, DD2_PROFILE_PLAYER_NAME, "123456789") &&
                                 menu.phase == DD2_PROFILE_PLAYER_NAME,
                             "Overlong edit changed phase\n");
    menu.phase = DD2_PROFILE_SAVE_SELECT;
    dd2_profile_menu_move(&menu, -1);
    dd2_profile_test_require(menu.logical == 0, "Selection underflow\n");
    for (unsigned index = 0; index <= DD2_SAVE_CARD_SLOTS; ++index) {
        dd2_profile_menu_move(&menu, 1);
    }
    dd2_profile_test_require(menu.logical == DD2_SAVE_CARD_SLOTS - 1, "Selection exceeds card\n");
    menu.phase = DD2_PROFILE_WRITING;
    dd2_profile_menu_move(&menu, -1);
    dd2_profile_test_require(menu.logical == DD2_SAVE_CARD_SLOTS - 1 &&
                                 !dd2_profile_menu_append(&menu, "X"),
                             "Pending menu accepts edit\n");
}
static void dd2_profile_test_identity(void) {
    dd2_configuration owner;
    dd2_configuration_defaults(&owner);
    dd2_profile_test_require(strcmp(dd2_configuration_player(&owner), "PLAYER") == 0,
                             "Empty source identity missing default\n");
    dd2_profile_test_require(dd2_configuration_set_player(&owner, "LongName_11") &&
                                 strcmp(dd2_configuration_player(&owner), "LongName_11") == 0 &&
                                 !dd2_configuration_name_valid(dd2_configuration_player(&owner)),
                             "Complete legacy name field rejected\n");
    char tail[DD2_SAVE_PROFILE_PLAYER_NAME];
    for (unsigned index = 0; index < sizeof(tail); ++index) {
        tail[index] = owner.source.players[0][index];
    }
    dd2_profile_test_require(dd2_configuration_set_player(&owner, "Bob") &&
                                 strcmp(dd2_configuration_player(&owner), "Bob") == 0,
                             "Name change failed\n");
    enum { DD2_PROFILE_TEST_SHORT_NAME_BYTES = 4 };
    for (unsigned index = DD2_PROFILE_TEST_SHORT_NAME_BYTES; index < sizeof(tail); ++index) {
        dd2_profile_test_require(owner.source.players[0][index] == tail[index],
                                 "Name change cleared retained suffix\n");
    }
    dd2_profile_test_require(dd2_configuration_set_player(&owner, owner.source.players[0]) &&
                                 !dd2_configuration_set_player(&owner, "123456789012") &&
                                 !dd2_configuration_set_player(&owner, "bad\n") &&
                                 strcmp(dd2_configuration_player(&owner), "Bob") == 0,
                             "Aliased/invalid identity changed source\n");
    dd2_profile_test_require(dd2_configuration_set_player(&owner, "") &&
                                 strcmp(dd2_configuration_player(&owner), "PLAYER") == 0,
                             "Empty edit did not select default\n");
}
int main(void) {
    dd2_profile_test_editor();
    dd2_profile_test_identity();
    return EXIT_SUCCESS;
}
