#include "assets/lz.h"

#include "assets/bytes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    DD2_LZ_HEADER_BYTES = 4,
    DD2_LZ_WINDOW_BYTES = 4096,
    DD2_LZ_LENGTH_MASK = 15,
    DD2_LZ_HIGH_MASK = 240
};

typedef struct {
    dd2_byte_view source;
    dd2_byte_buffer output;
    size_t read;
    size_t write;
    size_t length;
} dd2_lz_state;

static bool dd2_lz_token(dd2_lz_state *state, bool literal) {
    if (literal) {
        if (state->read == state->source.size) {
            return false;
        }
        state->output.data[state->write++] = state->source.data[state->read++];
        return true;
    }
    if (state->source.size - state->read < 2) {
        return false;
    }
    const unsigned low = state->source.data[state->read++];
    const unsigned high = state->source.data[state->read++];
    const size_t distance = DD2_LZ_WINDOW_BYTES - (low | ((high & DD2_LZ_HIGH_MASK) << 4));
    const size_t count = (high & DD2_LZ_LENGTH_MASK) + 3;
    if (distance > state->write || count > state->length - state->write) {
        return false;
    }
    for (size_t index = 0; index < count; ++index) {
        state->output.data[state->write] = state->output.data[state->write - distance];
        ++state->write;
    }
    return true;
}

bool dd2_lz_decode(dd2_byte_view source, dd2_byte_buffer output, size_t *written) {
    if (written == NULL) {
        return false;
    }
    *written = 0;
    if (source.data == NULL || source.size < DD2_LZ_HEADER_BYTES || output.data == NULL) {
        return false;
    }
    dd2_lz_state state = {.source = source,
                          .output = output,
                          .read = DD2_LZ_HEADER_BYTES,
                          .length = dd2_read_le32(source.data)};
    if (state.length > output.size) {
        return false;
    }
    while (state.write < state.length) {
        if (state.read == source.size) {
            return false;
        }
        const uint8_t control = source.data[state.read++];
        for (unsigned bit = 0; bit < DD2_BYTE_BITS && state.write < state.length; ++bit) {
            if (!dd2_lz_token(&state, (control & (1U << bit)) != 0)) {
                return false;
            }
        }
    }
    *written = state.write;
    return true;
}
