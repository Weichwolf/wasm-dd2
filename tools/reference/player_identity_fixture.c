/* Immutable original single-player naming builder, including the complete
 * eleven-byte legacy player field. Fixture inputs do not prove name-entry UI. */
#include "pe_fixture.h"

int main(int argc, char **argv) {
    require(argc == 3, "original/output");
    map_original(argv[1]);
    void (*names)(void) = (void (*)(void))(uintptr_t)0x44c550;
    static const char *const values[] = {"PLAYER", "Racer_7!", "LongName_11"};
    FILE *output = fopen(argv[2], "wb");
    require(output != NULL, "open identities");
    for (unsigned scenario = 0; scenario < sizeof(values)/sizeof(values[0]); ++scenario) {
        memset((void *)(uintptr_t)0x93dee0, 0, 20 * 54);
        memset((void *)(uintptr_t)0x93e318, 0, 10 * 12);
        put32(0x467658, 1);
        strcpy((char *)(uintptr_t)0x93e318, values[scenario]);
        names();
        for (unsigned driver = 0; driver < 20; ++driver) {
            require(fwrite((void *)(uintptr_t)(0x93dee0+driver*54), 1, 16, output) == 16,
                    "write driver identity");
        }
    }
    require(fclose(output) == 0, "close identities");
    return 0;
}
