/* Isolated immutable original-x86 packer, not a live-game save/restart test.
 * Pattern all copied regions, retain terminated names with nonzero tails and
 * write 48 complete physical blocks: three supported kinds, sixteen seeds. */
#include "pe_fixture.h"

static void pattern(unsigned address, unsigned size, unsigned seed) {
    unsigned index;
    unsigned char *bytes = (unsigned char *)(uintptr_t)address;
    for (index = 0; index < size; ++index) {
        bytes[index] = (unsigned char)(1 + (index * 13 + seed * 37) % 251);
    }
}
static void name(unsigned address) {
    unsigned char *bytes = (unsigned char *)(uintptr_t)address;
    bytes[0] = 'N';
    bytes[1] = 'M';
    bytes[2] = 0;
}
static void initialize(unsigned seed) {
    static const unsigned words[] = {
        0x4673f8, 0x4673f4, 0x467400, 0x4673fc, 0x467404, 0x467408,
        0x4604c2, 0x467410, 0x754451, 0x467414, 0x467418, 0x46741c,
        0x93ded0, 0x93decc, 0x93dec4, 0x93dec8, 0x93dec0, 0x467654,
        0x467658, 0x46765c, 0x467660, 0x4682f0, 0x4682f4
    };
    unsigned index, season, offset;
    for (index = 0; index < sizeof(words) / sizeof(words[0]); ++index) {
        put32(words[index], 0xa5a50000u | ((seed * 1237 + index * 3087) & 0xffffu));
    }
    pattern(0x93e7c0, 4700, seed);
    pattern(0x93dee0, 1080, seed + 1);
    pattern(0x93e318, 120, seed + 2);
    pattern(0x4680c0, 560, seed + 3);
    pattern(0x46757a, 18, seed + 4);
    for (season = 0; season < 5; ++season) {
        offset = 0x93e7c0 + season * 940;
        for (index = 0; index < 31; ++index) {
            name(offset + index * 20);
        }
        for (index = 0; index < 20; ++index) {
            name(offset + 620 + index * 16);
        }
    }
    for (index = 0; index < 20; ++index) {
        name(0x93dee0 + index * 54);
    }
    for (index = 0; index < 10; ++index) {
        name(0x93e318 + index * 12);
    }
    for (index = 0; index < 35; ++index) {
        name(0x4680c0 + index * 16);
    }
}
int main(int argc, char **argv) {
    static const uint16_t kinds[] = {0x1010, 0x1020, 0x3030};
    void (*pack)(uint16_t *, uint16_t) =
        (void (*)(uint16_t *, uint16_t))(uintptr_t)0x44ae0c;
    unsigned kind, seed, index;
    uint16_t block[4096];
    FILE *out;
    require(argc == 3, "original executable and output required");
    map_original(argv[1]);
    out = fopen(argv[2], "wb");
    require(out != NULL, "open original profile output");
    for (kind = 0; kind < sizeof(kinds) / sizeof(kinds[0]); ++kind) {
        for (seed = 0; seed < 16; ++seed) {
            initialize(seed);
            for (index = 0; index < sizeof(block); ++index) {
                ((unsigned char *)block)[index] =
                    (unsigned char)(1 + (index * 19 + seed * 41 + kind * 7) % 251);
            }
            pack(block, kinds[kind]);
            require(fwrite(block, 1, sizeof(block), out) == sizeof(block),
                    "write complete original block");
        }
    }
    require(fclose(out) == 0, "close original profile output");
    return 0;
}
