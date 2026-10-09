#include "game/championship.h"
#include "game/profile_menu.h"
#include "game/race.h"
#include "render/race_draw.h"
#include "render/renderer.h"

#include <GL/softgl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_PROFILE_DRAW_TEST_WIDTH = 640,
    DD2_PROFILE_DRAW_TEST_HEIGHT = 480,
    DD2_PROFILE_DRAW_TEST_X = 48,
    DD2_PROFILE_DRAW_TEST_Y = 270,
    DD2_PROFILE_DRAW_TEST_GLYPH_WIDTH = 15,
    DD2_PROFILE_DRAW_TEST_GLYPH_HEIGHT = 21,
    DD2_PROFILE_DRAW_TEST_CHANNELS = 4,
    DD2_PROFILE_DRAW_TEST_ONE = 255
};
static bool dd2_profile_draw_test_glyph(dd2_renderer *renderer, unsigned character) {
    const dd2_render_options size = {.width = DD2_PROFILE_DRAW_TEST_WIDTH,
                                     .height = DD2_PROFILE_DRAW_TEST_HEIGHT};
    dd2_profile_menu menu = {.phase = DD2_PROFILE_PLAYER_NAME, .draft = {(char)character, '\0'}};
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!dd2_profile_draw(&menu, NULL, size)) {
        return false;
    }
    const uint8_t *pixels = dd2_renderer_pixels(renderer);
    bool lit = false;
    for (unsigned row = DD2_PROFILE_DRAW_TEST_Y;
         row < DD2_PROFILE_DRAW_TEST_Y + DD2_PROFILE_DRAW_TEST_GLYPH_HEIGHT; ++row) {
        for (unsigned column = DD2_PROFILE_DRAW_TEST_X;
             column < DD2_PROFILE_DRAW_TEST_X + DD2_PROFILE_DRAW_TEST_GLYPH_WIDTH; ++column) {
            const size_t offset = ((size_t)row * DD2_PROFILE_DRAW_TEST_WIDTH + column) *
                                  DD2_PROFILE_DRAW_TEST_CHANNELS;
            lit =
                lit || (pixels[offset] == DD2_PROFILE_DRAW_TEST_ONE &&
                        pixels[offset + 1] == DD2_PROFILE_DRAW_TEST_ONE && pixels[offset + 2] == 0);
        }
    }
    return lit == (character != ' ');
}
int main(void) {
    const dd2_render_options size = {.width = DD2_PROFILE_DRAW_TEST_WIDTH,
                                     .height = DD2_PROFILE_DRAW_TEST_HEIGHT};
    dd2_renderer *renderer = dd2_renderer_create(&size);
    if (renderer == NULL) {
        return EXIT_FAILURE;
    }
    bool good = true;
    for (unsigned character = ' '; character <= '~' && good; ++character) {
        good = dd2_profile_draw_test_glyph(renderer, character);
    }
    dd2_championship championship = {0};
    good = good && dd2_championship_reset(&championship, DD2_RACE_STOCKCAR);
    /* Synthetic rule-valid interstitial: verify the named renderer, not a
     * naturally completed physical round. */
    championship.phase = DD2_CHAMPIONSHIP_ROUND_RESULTS;
    championship.history[0].completed = 1;
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    good = good && dd2_championship_draw_named(&championship, "LongName_11", size) &&
           glGetError() == GL_NO_ERROR;
    dd2_renderer_destroy(renderer);
    if (!good && fputs("Profile glyph or named standings rendering failed\n", stderr) == EOF) {
        abort();
    }
    return good ? EXIT_SUCCESS : EXIT_FAILURE;
}
