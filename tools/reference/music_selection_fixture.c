/* Execute only unmodified original CD transport routines. The imported MCI
 * device is an isolated observer; no machine code or game files are changed.
 * Caller-context values come separately from the frozen frontend/game source. */
#include "pe_fixture.h"

enum { MCI_PLAY = 0x806, MCI_STOP = 0x808, MCI_SEEK = 0x830, MCI_STATUS = 0x814 };
static unsigned calls;
static unsigned device_mode;
static unsigned device_error;
static unsigned commands[4];
static uint32_t __attribute__((stdcall)) observe(unsigned device, unsigned command, unsigned flags,
                                                 uintptr_t parameters) {
    require(device == 7, "isolated MCI device identity");
    require(calls < 4, "bounded MCI observer");
    commands[calls++] = command;
    if (command == MCI_STATUS) {
        require(flags == 0x100 && read32((void *)(parameters + 8)) == 4,
                "original mode status request");
        put32((unsigned)parameters + 4, device_mode);
    } else if (command == MCI_SEEK || command == MCI_PLAY) {
        require((flags == 12 || flags == 8) &&
                    read32((void *)(parameters + 8)) == read32((void *)0x462d80),
                "original selected track end");
    } else {
        require(command == MCI_STOP && flags == 0, "original stop request");
    }
    return device_error;
}
static void output(const char *label, unsigned source_track, unsigned repeat) {
    unsigned index;
    printf("{\"case\":\"%s\",\"source_track\":%u,\"repeat_input\":%u,"
           "\"from\":%u,\"to\":%u,\"repeat\":%u,\"playing\":%u,\"commands\":[",
           label, source_track, repeat, read32((void *)0x462d7c), read32((void *)0x462d80),
           read32((void *)0x462d6c), read32((void *)0x462d70));
    for (index = 0; index < calls; ++index) {
        printf("%s%u", index ? "," : "", commands[index]);
    }
    puts("]}");
    calls = 0;
}
int main(int argc, char **argv) {
    void (*select_track)(char, unsigned) = (void (*)(char, unsigned))(uintptr_t)0x41617c;
    void (*start)(void) = (void (*)(void))(uintptr_t)0x4161ec;
    void (*loop)(void) = (void (*)(void))(uintptr_t)0x416228;
    void (*pause_track)(void) = (void (*)(void))(uintptr_t)0x4162e4;
    void (*resume)(void) = (void (*)(void))(uintptr_t)0x416314;
    void (*countdown)(void) = (void (*)(void))(uintptr_t)0x42fda0;
    unsigned track, repeat;
    require(argc == 2, "supported original executable required");
    map_original(argv[1]);
    require(mprotect((void *)0x410000, 0x50000, PROT_READ | PROT_EXEC) == 0,
            "protect original machine code from writes");
    put32(0x95030c, (uint32_t)(uintptr_t)observe);
    put32(0x462d74, 1);
    put32(0x462d78, 7);
    for (track = 1; track <= 18; ++track) {
        for (repeat = 0; repeat <= 1; ++repeat) {
            select_track((char)track, repeat);
            output("select", track, repeat);
        }
    }
    select_track(12, 1);
    calls = 0;
    start();
    output("start", 12, 1);
    pause_track();
    output("pause", 12, 1);
    resume();
    output("resume", 12, 1);
    device_mode = 526;
    loop();
    output("loop-playing", 12, 1);
    device_mode = 525;
    loop();
    output("loop-ended", 12, 1);
    pause_track();
    calls = 0;
    loop();
    output("loop-paused", 12, 1);
    select_track(1, 0);
    calls = 0;
    start();
    calls = 0;
    loop();
    output("loop-no-repeat", 1, 0);
    pause_track();
    calls = 0;
    device_error = 1;
    start();
    output("failed-start", 1, 0);
    device_error = 0;
    put32(0x462d74, 0);
    select_track(12, 1);
    start();
    loop();
    pause_track();
    resume();
    output("disabled", 12, 1);
    put32(0x462d74, 1);
    put32(0x462d68, 0);
    select_track(1, 1);
    calls = 0;
    put32(0x77cf74, 0);
    put32(0x784298, 3);
    countdown();
    require(read32((void *)0x784298) == 2, "countdown before green");
    output("countdown-wait", 1, 1);
    put32(0x77cf74, 1);
    countdown();
    require(read32((void *)0x784298) == 2, "held countdown");
    output("countdown-held", 1, 1);
    put32(0x77cf74, 0);
    countdown();
    require(read32((void *)0x784298) == 1, "green countdown boundary");
    output("countdown-go", 1, 1);
    printf("{\"case\":\"menu-level-map\",\"levels\":[");
    for (track = 0; track < 11; ++track) {
        printf("%s%u", track ? "," : "", read32((void *)(uintptr_t)(0x467424 + track * 4)));
    }
    puts("]}");
    return 0;
}
