#include "assets/bytes.h"
#include "assets/image.h"
#include "image_fixture.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool dd2_image_test_fixture(const dd2_png_test_fixture *fixture) {
    uint8_t bytes[DD2_PNG_TEST_ENCODED_MAX] = {0};
    for (size_t index = 0; index < fixture->length; ++index) {
        bytes[index] = fixture->encoded[index];
    }
    dd2_image *image = dd2_image_create_png((dd2_byte_view){bytes, fixture->length});
    if (image == NULL) {
        return false;
    }
    for (size_t index = 0; index < sizeof(bytes); ++index) {
        bytes[index] = 0;
    }
    const bool passed =
        dd2_image_width(image) == 2 && dd2_image_height(image) == 2 &&
        dd2_image_rgba_bytes(image) == DD2_PNG_TEST_PIXEL_BYTES &&
        memcmp(dd2_image_pixels(image), fixture->expected, DD2_PNG_TEST_PIXEL_BYTES) == 0;
    dd2_image_destroy(image);
    if (!passed) {
        return false;
    }
    for (size_t index = 0; index < fixture->length; ++index) {
        bytes[index] = fixture->encoded[index];
    }
    bytes[fixture->length - 1] ^= 1U;
    image = dd2_image_create_png((dd2_byte_view){bytes, fixture->length});
    if (image != NULL) {
        dd2_image_destroy(image);
        return false;
    }
    for (size_t length = 0; length < fixture->length; ++length) {
        image = dd2_image_create_png((dd2_byte_view){fixture->encoded, length});
        if (image != NULL) {
            dd2_image_destroy(image);
            return false;
        }
    }
    return true;
}

int main(void) {
    if (dd2_image_create_png((dd2_byte_view){0}) != NULL || dd2_image_width(NULL) != 0 ||
        dd2_image_height(NULL) != 0 || dd2_image_rgba_bytes(NULL) != 0 ||
        dd2_image_pixels(NULL) != NULL) {
        return EXIT_FAILURE;
    }
    dd2_image_destroy(NULL);
    for (size_t index = 0; index < sizeof(dd2_png_test_fixtures) / sizeof(dd2_png_test_fixtures[0]);
         ++index) {
        if (!dd2_image_test_fixture(&dd2_png_test_fixtures[index])) {
            return EXIT_FAILURE;
        }
    }
    puts("{\"scope\":\"owned PNG colors, row filters, CRC, truncation and "
         "lifetime\",\"pass\":true}");
    return EXIT_SUCCESS;
}
