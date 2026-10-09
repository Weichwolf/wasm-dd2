#include "render/color.h"
#include "render/renderer.h"

#include <GL/softgl.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_COLOR_TEST_CHANNELS = 4,
    DD2_COLOR_TEST_SAMPLES = UINT8_MAX + 1,
    DD2_COLOR_TEST_LINEAR_HALF = 128,
    DD2_COLOR_TEST_LINEAR_QUARTER = 64,
    DD2_COLOR_TEST_ENCODED_HALF = 188,
    DD2_COLOR_TEST_ENCODED_QUARTER = 137,
    DD2_COLOR_TEST_ENCODED_LOW = 13
};
static const double dd2_color_test_scale = UINT8_MAX;
static const double dd2_color_test_exponent = 2.4;
static const double dd2_color_test_offset = 0.055;
static const double dd2_color_test_gain = 1.055;
static const double dd2_color_test_slope = 12.92;
static const double dd2_color_test_decode_break = 0.04045;
static const double dd2_color_test_encode_break = 0.0031308;

/* Independent double-precision transfer oracle checks every byte, including
 * both sides of the standard breakpoints and all alpha values. */
static uint8_t dd2_color_test_decode(unsigned input) {
    const double value = (double)input / dd2_color_test_scale;
    const double linear =
        value <= dd2_color_test_decode_break
            ? value / dd2_color_test_slope
            : pow((value + dd2_color_test_offset) / dd2_color_test_gain, dd2_color_test_exponent);
    return (uint8_t)lround(linear * dd2_color_test_scale);
}

static uint8_t dd2_color_test_encode(unsigned input) {
    const double value = (double)input / dd2_color_test_scale;
    const double encoded = value <= dd2_color_test_encode_break
                               ? value * dd2_color_test_slope
                               : (dd2_color_test_gain * pow(value, 1 / dd2_color_test_exponent)) -
                                     dd2_color_test_offset;
    return (uint8_t)lround(encoded * dd2_color_test_scale);
}

static bool dd2_color_test_tables(void) {
    uint8_t linear[DD2_COLOR_TEST_SAMPLES * DD2_COLOR_TEST_CHANNELS] = {0};
    uint8_t expected[sizeof(linear)] = {0};
    uint8_t actual[sizeof(linear)] = {0};
    for (unsigned input = 0; input < DD2_COLOR_TEST_SAMPLES; ++input) {
        if (dd2_color_srgb8_to_linear8((uint8_t)input) != dd2_color_test_decode(input) ||
            dd2_color_linear8_to_srgb8((uint8_t)input) != dd2_color_test_encode(input)) {
            return false;
        }
        for (size_t channel = 0; channel < DD2_COLOR_TEST_CHANNELS; ++channel) {
            const size_t index = ((size_t)input * DD2_COLOR_TEST_CHANNELS) + channel;
            const unsigned value = channel % 2 == 0 ? input : UINT8_MAX - input;
            linear[index] = (uint8_t)value;
            expected[index] = channel == DD2_COLOR_TEST_CHANNELS - 1 ? (uint8_t)value
                                                                     : dd2_color_test_encode(value);
        }
    }
    uint8_t source_copy[sizeof(linear)] = {0};
    for (size_t index = 0; index < sizeof(linear); ++index) {
        source_copy[index] = linear[index];
    }
    if (!dd2_color_present_rgba8(actual, linear, sizeof(linear)) ||
        memcmp(actual, expected, sizeof(actual)) != 0 ||
        memcmp(linear, source_copy, sizeof(linear)) != 0 ||
        !dd2_color_present_rgba8(linear, linear, sizeof(linear)) ||
        memcmp(linear, expected, sizeof(linear)) != 0) {
        return false;
    }
    return !dd2_color_present_rgba8(NULL, actual, sizeof(actual)) &&
           !dd2_color_present_rgba8(actual, NULL, sizeof(actual)) &&
           !dd2_color_present_rgba8(actual, actual, sizeof(actual) - 1) &&
           memcmp(actual, expected, sizeof(actual)) == 0;
}

static bool dd2_color_test_framebuffer(int samples) {
    dd2_renderer *renderer = dd2_renderer_create(&(dd2_render_options){
        .width = 1, .height = 1, .samples = samples, .output = DD2_RENDER_LINEAR_TO_SRGB});
    if (renderer == NULL) {
        return false;
    }
    const uint8_t linear[] = {DD2_COLOR_TEST_LINEAR_HALF, DD2_COLOR_TEST_LINEAR_QUARTER, 1,
                              DD2_COLOR_TEST_LINEAR_QUARTER};
    const uint8_t expected[] = {DD2_COLOR_TEST_ENCODED_HALF, DD2_COLOR_TEST_ENCODED_QUARTER,
                                DD2_COLOR_TEST_ENCODED_LOW, DD2_COLOR_TEST_LINEAR_QUARTER};
    glClearColor((float)linear[0] / UINT8_MAX, (float)linear[1] / UINT8_MAX,
                 (float)linear[2] / UINT8_MAX, (float)linear[3] / UINT8_MAX);
    glClear(GL_COLOR_BUFFER_BIT);
    const uint8_t *pixels = dd2_renderer_pixels(renderer);
    bool passed = pixels != NULL && memcmp(pixels, expected, sizeof(expected)) == 0;
    pixels = dd2_renderer_pixels(renderer);
    passed = passed && pixels != NULL && memcmp(pixels, expected, sizeof(expected)) == 0;
    uint8_t raw[sizeof(linear)] = {0};
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, raw);
    passed = passed && memcmp(raw, linear, sizeof(raw)) == 0 && glGetError() == GL_NO_ERROR;
    dd2_renderer_destroy(renderer);
    return passed;
}

int main(void) {
    const dd2_render_options invalid = {.width = 1, .height = 1, .output = DD2_RENDER_OUTPUT_COUNT};
    if (dd2_renderer_create(&invalid) != NULL || !dd2_color_test_tables() ||
        !dd2_color_test_framebuffer(0) || !dd2_color_test_framebuffer(2) ||
        !dd2_color_test_framebuffer(4)) {
        return EXIT_FAILURE;
    }
    puts("{\"scope\":\"sRGB transfer, alpha, independent display copy and resolved "
         "readback\",\"pass\":true}");
    return EXIT_SUCCESS;
}
