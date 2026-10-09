/* Immutable original packer with factory scalar values observed in actual
 * configuration UI run 0056. Mapped PE lap tables/bindings remain untouched;
 * source season/driver/player regions are initially zero. Not a live save test. */
#include "pe_fixture.h"

int main(int argc, char **argv) {
    static const unsigned addresses[] = {
        0x4673f8, 0x4673f4, 0x467400, 0x4673fc, 0x467404, 0x467408,
        0x4604c2, 0x467410, 0x754451, 0x467414, 0x467418, 0x46741c,
        0x93ded0, 0x93decc, 0x93dec4, 0x93dec8, 0x93dec0, 0x467654,
        0x467658, 0x46765c, 0x467660, 0x4682f0, 0x4682f4};
    static const unsigned values[] = {
        0, 0, 0, 0, 4, 1, 26000, 4090, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 20, 1, 0, 0};
    void (*pack)(uint16_t *, uint16_t) = (void (*)(uint16_t *, uint16_t))(uintptr_t)0x44ae0c;
    uint16_t block[4096] = {0};
    unsigned index;
    FILE *out;
    require(argc == 3, "original executable and output required");
    map_original(argv[1]);
    for (index = 0; index < sizeof(addresses) / sizeof(addresses[0]); ++index) {
        put32(addresses[index], values[index]);
    }
    pack(block, 0x1010);
    out = fopen(argv[2], "wb");
    require(out != NULL, "open factory output");
    require(fwrite(block, 1, sizeof(block), out) == sizeof(block), "write factory profile");
    require(fclose(out) == 0, "close factory output");
    return 0;
}
