#include "assets/bytes.h"
#include "assets/save_card.h"
#include "assets/save_profile.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_PROFILE_TEST_MAGIC_BYTE = 0x10,
    DD2_PROFILE_TEST_SENTINEL = 0x5a,
    DD2_PROFILE_TEST_REPLAY = 0x2020,
    DD2_PROFILE_TEST_UNKNOWN = 0xffff,
    DD2_PROFILE_TEST_BAD_CONFIG = 0x1011,
    DD2_PROFILE_TEST_BAD_STARTUP = 0x1021,
    DD2_PROFILE_TEST_BAD_GAME = 0x3031,
    DD2_PROFILE_TEST_STATISTICS_BYTES = 20,
    DD2_PROFILE_TEST_HEADER = 48,
    DD2_PROFILE_TEST_SEASON_BYTES = 940,
    DD2_PROFILE_TEST_DRIVER_OFFSET = 4748,
    DD2_PROFILE_TEST_DRIVER_BYTES = 54,
    DD2_PROFILE_TEST_PLAYER_OFFSET = 5828,
    DD2_PROFILE_TEST_LAP_OFFSET = 5948,
    DD2_PROFILE_TEST_LAP_BYTES = 16,
    DD2_PROFILE_TEST_PATTERN = 251
};
static uint8_t dd2_profile_test_block[DD2_SAVE_CARD_BLOCK_BYTES];
static uint8_t dd2_profile_test_output[DD2_SAVE_CARD_BLOCK_BYTES];
static dd2_save_profile dd2_profile_test_owner;

static void dd2_profile_test_require(bool valid, const char *message) {
    if (!valid) {
        if (fputs(message, stderr) == EOF) {
            abort();
        }
        exit(EXIT_FAILURE);
    }
}
static void dd2_profile_test_copy(void *out, size_t size, const void *input) {
    unsigned char *destination = out;
    const unsigned char *source = input;
    for (size_t index = 0; index < size; ++index) {
        destination[index] = source[index];
    }
}
static void dd2_profile_test_fill(dd2_byte_buffer buffer, unsigned char value) {
    unsigned char *destination = buffer.data;
    for (size_t index = 0; index < buffer.size; ++index) {
        destination[index] = value;
    }
}
static dd2_byte_view dd2_profile_test_view(void) {
    return (dd2_byte_view){.data = dd2_profile_test_block, .size = sizeof(dd2_profile_test_block)};
}
static dd2_byte_buffer dd2_profile_test_buffer(void) {
    return (dd2_byte_buffer){.data = dd2_profile_test_output,
                             .size = sizeof(dd2_profile_test_output)};
}
static void dd2_profile_test_name(size_t offset, size_t size) {
    dd2_profile_test_block[offset] = 'N';
    dd2_profile_test_block[offset + 1] = 0;
    /* Keep nonzero bytes beyond the terminator, including the last byte. */
    dd2_profile_test_block[offset + size - 1] = 'Z';
}
static void dd2_profile_test_fixture(void) {
    for (size_t index = 0; index < sizeof(dd2_profile_test_block); ++index) {
        dd2_profile_test_block[index] = (uint8_t)(1 + (index % DD2_PROFILE_TEST_PATTERN));
    }
    dd2_profile_test_block[0] = DD2_PROFILE_TEST_MAGIC_BYTE;
    dd2_profile_test_block[1] = DD2_PROFILE_TEST_MAGIC_BYTE;
    for (unsigned season = 0; season < DD2_SAVE_PROFILE_SEASONS; ++season) {
        size_t offset = DD2_PROFILE_TEST_HEADER + ((size_t)season * DD2_PROFILE_TEST_SEASON_BYTES);
        for (unsigned track = 0; track < DD2_SAVE_PROFILE_TRACKS + DD2_SAVE_PROFILE_DRIVERS;
             ++track) {
            dd2_profile_test_name(offset, DD2_SAVE_PROFILE_NAME);
            offset += DD2_PROFILE_TEST_STATISTICS_BYTES;
        }
        for (unsigned driver = 0; driver < DD2_SAVE_PROFILE_DRIVERS; ++driver) {
            dd2_profile_test_name(offset, DD2_SAVE_PROFILE_NAME);
            offset += DD2_SAVE_PROFILE_NAME;
        }
    }
    for (unsigned driver = 0; driver < DD2_SAVE_PROFILE_DRIVERS; ++driver) {
        dd2_profile_test_name(DD2_PROFILE_TEST_DRIVER_OFFSET +
                                  ((size_t)driver * DD2_PROFILE_TEST_DRIVER_BYTES),
                              DD2_SAVE_PROFILE_NAME);
    }
    for (unsigned player = 0; player < DD2_SAVE_PROFILE_PLAYERS; ++player) {
        dd2_profile_test_name(DD2_PROFILE_TEST_PLAYER_OFFSET +
                                  ((size_t)player * DD2_SAVE_PROFILE_PLAYER_NAME),
                              DD2_SAVE_PROFILE_PLAYER_NAME);
    }
    for (unsigned lap = 0; lap < DD2_SAVE_PROFILE_CIRCUITS * DD2_SAVE_PROFILE_LAPS; ++lap) {
        dd2_profile_test_name(DD2_PROFILE_TEST_LAP_OFFSET +
                                  ((size_t)lap * DD2_PROFILE_TEST_LAP_BYTES),
                              DD2_SAVE_PROFILE_LAP_NAME);
    }
}
static void dd2_profile_test_rejected_read(dd2_byte_view bytes) {
    dd2_save_profile before;
    dd2_profile_test_copy(&before, sizeof(before), &dd2_profile_test_owner);
    dd2_profile_test_require(!dd2_save_profile_read(bytes, &dd2_profile_test_owner) &&
                                 memcmp(&before, &dd2_profile_test_owner, sizeof(before)) == 0,
                             "invalid read changed the value owner\n");
}
static void dd2_profile_test_invalid_name(size_t offset, size_t size) {
    uint8_t saved[DD2_SAVE_PROFILE_NAME];
    dd2_profile_test_copy(saved, size, &dd2_profile_test_block[offset]);
    dd2_profile_test_fill((dd2_byte_buffer){.data = &dd2_profile_test_block[offset], .size = size},
                          'X');
    dd2_profile_test_rejected_read(dd2_profile_test_view());
    dd2_profile_test_copy(&dd2_profile_test_block[offset], size, saved);
}
static void dd2_profile_test_invalid_text(void) {
    for (unsigned season = 0; season < DD2_SAVE_PROFILE_SEASONS; ++season) {
        size_t offset = DD2_PROFILE_TEST_HEADER + ((size_t)season * DD2_PROFILE_TEST_SEASON_BYTES);
        for (unsigned track = 0; track < DD2_SAVE_PROFILE_TRACKS + DD2_SAVE_PROFILE_DRIVERS;
             ++track) {
            dd2_profile_test_invalid_name(offset, DD2_SAVE_PROFILE_NAME);
            offset += DD2_PROFILE_TEST_STATISTICS_BYTES;
        }
        for (unsigned driver = 0; driver < DD2_SAVE_PROFILE_DRIVERS; ++driver) {
            dd2_profile_test_invalid_name(offset, DD2_SAVE_PROFILE_NAME);
            offset += DD2_SAVE_PROFILE_NAME;
        }
    }
    for (unsigned driver = 0; driver < DD2_SAVE_PROFILE_DRIVERS; ++driver) {
        dd2_profile_test_invalid_name(DD2_PROFILE_TEST_DRIVER_OFFSET +
                                          ((size_t)driver * DD2_PROFILE_TEST_DRIVER_BYTES),
                                      DD2_SAVE_PROFILE_NAME);
    }
    for (unsigned player = 0; player < DD2_SAVE_PROFILE_PLAYERS; ++player) {
        dd2_profile_test_invalid_name(DD2_PROFILE_TEST_PLAYER_OFFSET +
                                          ((size_t)player * DD2_SAVE_PROFILE_PLAYER_NAME),
                                      DD2_SAVE_PROFILE_PLAYER_NAME);
    }
    for (unsigned lap = 0; lap < DD2_SAVE_PROFILE_CIRCUITS * DD2_SAVE_PROFILE_LAPS; ++lap) {
        dd2_profile_test_invalid_name(DD2_PROFILE_TEST_LAP_OFFSET +
                                          ((size_t)lap * DD2_PROFILE_TEST_LAP_BYTES),
                                      DD2_SAVE_PROFILE_LAP_NAME);
    }
}
static void dd2_profile_test_failure(void) {
    for (size_t size = 0; size < DD2_SAVE_CARD_BLOCK_BYTES; ++size) {
        dd2_byte_view view = dd2_profile_test_view();
        view.size = size;
        dd2_profile_test_rejected_read(view);
        dd2_profile_test_require(
            !dd2_save_profile_write(
                &dd2_profile_test_owner,
                (dd2_byte_buffer){.data = dd2_profile_test_output, .size = size}),
            "truncated output accepted\n");
    }
    dd2_profile_test_rejected_read(
        (dd2_byte_view){.data = dd2_profile_test_block, .size = DD2_SAVE_CARD_BLOCK_BYTES + 1});
    dd2_profile_test_rejected_read((dd2_byte_view){.size = DD2_SAVE_CARD_BLOCK_BYTES});
    dd2_profile_test_require(
        !dd2_save_profile_read(dd2_profile_test_view(), NULL) &&
            !dd2_save_profile_write(NULL, dd2_profile_test_buffer()) &&
            !dd2_save_profile_write(&dd2_profile_test_owner,
                                    (dd2_byte_buffer){.size = DD2_SAVE_CARD_BLOCK_BYTES}) &&
            !dd2_save_profile_write(&dd2_profile_test_owner,
                                    (dd2_byte_buffer){.data = dd2_profile_test_output,
                                                      .size = DD2_SAVE_CARD_BLOCK_BYTES + 1}),
        "invalid pointers/extent accepted\n");
    const uint16_t unsupported[] = {0,
                                    DD2_PROFILE_TEST_REPLAY,
                                    DD2_PROFILE_TEST_UNKNOWN,
                                    DD2_PROFILE_TEST_BAD_CONFIG,
                                    DD2_PROFILE_TEST_BAD_STARTUP,
                                    DD2_PROFILE_TEST_BAD_GAME};
    for (size_t index = 0; index < sizeof(unsupported) / sizeof(unsupported[0]); ++index) {
        dd2_profile_test_block[0] = (uint8_t)unsupported[index];
        dd2_profile_test_block[1] = (uint8_t)(unsupported[index] >> DD2_BYTE_BITS);
        dd2_profile_test_rejected_read(dd2_profile_test_view());
    }
    dd2_profile_test_block[0] = DD2_PROFILE_TEST_MAGIC_BYTE;
    dd2_profile_test_block[1] = DD2_PROFILE_TEST_MAGIC_BYTE;
    dd2_profile_test_invalid_text();
    dd2_profile_test_fill(dd2_profile_test_buffer(), DD2_PROFILE_TEST_SENTINEL);
    const int16_t saved_kind = dd2_profile_test_owner.header.kind;
    dd2_profile_test_owner.header.kind = DD2_PROFILE_TEST_REPLAY;
    dd2_profile_test_require(
        !dd2_save_profile_write(&dd2_profile_test_owner, dd2_profile_test_buffer()),
        "replay written with profile schema\n");
    dd2_profile_test_owner.header.kind = saved_kind;
    const char saved_name =
        dd2_profile_test_owner.laps[DD2_SAVE_PROFILE_CIRCUITS - 1][DD2_SAVE_PROFILE_LAPS - 1]
            .name[1];
    dd2_profile_test_owner.laps[DD2_SAVE_PROFILE_CIRCUITS - 1][DD2_SAVE_PROFILE_LAPS - 1].name[1] =
        'X';
    dd2_profile_test_require(
        !dd2_save_profile_write(&dd2_profile_test_owner, dd2_profile_test_buffer()),
        "unterminated typed name accepted\n");
    dd2_profile_test_owner.laps[DD2_SAVE_PROFILE_CIRCUITS - 1][DD2_SAVE_PROFILE_LAPS - 1].name[1] =
        saved_name;
    for (size_t index = 0; index < sizeof(dd2_profile_test_output); ++index) {
        dd2_profile_test_require(dd2_profile_test_output[index] == DD2_PROFILE_TEST_SENTINEL,
                                 "failed encoding partially changed output\n");
    }
}
static void dd2_profile_test_alias(void) {
    /* Overlap uses the struct's byte representation only as caller storage;
     * it is never the decoding schema. No padding/layout equivalence assumed. */
    union {
        dd2_save_profile profile;
        uint8_t bytes[sizeof(dd2_save_profile) + DD2_SAVE_CARD_BLOCK_BYTES];
    } overlap;
    overlap.profile = dd2_profile_test_owner;
    uint8_t *aliased = &overlap.bytes[sizeof(dd2_save_profile) / 2];
    dd2_profile_test_require(
        dd2_save_profile_write(
            &overlap.profile,
            (dd2_byte_buffer){.data = aliased, .size = DD2_SAVE_CARD_BLOCK_BYTES}) &&
            memcmp(aliased, dd2_profile_test_block, DD2_SAVE_CARD_BLOCK_BYTES) == 0,
        "overlapping encoding lost source bytes\n");
    dd2_profile_test_require(
        dd2_save_profile_read((dd2_byte_view){.data = aliased, .size = DD2_SAVE_CARD_BLOCK_BYTES},
                              &overlap.profile) &&
            dd2_save_profile_write(&overlap.profile, dd2_profile_test_buffer()) &&
            memcmp(dd2_profile_test_output, dd2_profile_test_block, DD2_SAVE_CARD_BLOCK_BYTES) == 0,
        "overlapping decoding lost input bytes\n");
}
int main(void) {
    dd2_profile_test_fixture();
    dd2_profile_test_require(
        dd2_save_profile_read(dd2_profile_test_view(), &dd2_profile_test_owner) &&
            dd2_save_profile_write(&dd2_profile_test_owner, dd2_profile_test_buffer()) &&
            memcmp(dd2_profile_test_block, dd2_profile_test_output, DD2_SAVE_CARD_BLOCK_BYTES) == 0,
        "owned nonzero fixture round trip differs\n");
    dd2_profile_test_failure();
    dd2_profile_test_alias();
    return EXIT_SUCCESS;
}
