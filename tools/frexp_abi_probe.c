/* Private verification entry linked alongside every production game object.
 * Mode 0 exercises libc formatting; 1 libc frexp; 2 the actual VA dispatch;
 * 3 executes the untouched, position-independent original machine routine.
 * The game main and all other engine functions remain linked, but are not run.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef __EMSCRIPTEN__
#include <sys/mman.h>
#endif

extern struct { unsigned va; void *fn; } dd2_fnmap[];
extern int dd2_fnmap_n;
typedef uint64_t (*watcom_fn)(uint64_t, int *);

int dd2_crt_probe(int mode, const char *input, const char *output, const char *machine)
{
    FILE *in, *out;
    watcom_fn target = NULL;
    double (*volatile host)(double, int *) = frexp;
    uint64_t bits;
    int i;
    if (mode == 0) {
        char actual[128];
        volatile double a = 1.5, b = -0.0, c = 1.0;
        int n = snprintf(actual, sizeof(actual), "%.6f %.6f %.6f", a, b, c);
        if (n != 27 || strcmp(actual, "1.500000 -0.000000 1.000000")) {
            fprintf(stderr, "Unexpected libc formatting: %s (%d)\n", actual, n);
            return 2;
        }
        fputs("CRT_PROBE_FORMAT_OK\n", stdout);
        return 0;
    }
    if (mode == 2) {
        for (i = 0; i < dd2_fnmap_n; ++i)
            if (dd2_fnmap[i].va == 0x45c0f0) target = (watcom_fn)dd2_fnmap[i].fn;
        if (!target) return 3;
    }
    if (mode == 3) {
#ifndef __EMSCRIPTEN__
        unsigned char *code;
        FILE *f = fopen(machine, "rb");
        if (!f) return 4;
        code = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (code == MAP_FAILED) return 5;
        if (fread(code, 1, 123, f) != 123 || fgetc(f) != EOF) return 6;
        fclose(f);
        if (mprotect(code, 4096, PROT_READ | PROT_EXEC)) return 7;
        target = (watcom_fn)code;
#else
        return 8;
#endif
    }
    in = fopen(input, "rb"); out = fopen(output, "wb");
    if (!in || !out || (mode != 1 && !target)) return 9;
    while (fread(&bits, sizeof(bits), 1, in) == 1) {
        struct { int before, exponent, after; } e = {0x12345678, 0x24681357, 0x13572468};
        uint32_t record[4];
        uint64_t result;
        if (mode == 1) {
            double value, fraction;
            memcpy(&value, &bits, sizeof(value));
            fraction = host(value, &e.exponent);
            memcpy(&result, &fraction, sizeof(result));
        } else result = target(bits, &e.exponent);
        record[0] = (uint32_t)result;
        record[1] = (uint32_t)(result >> 32);
        record[2] = (uint32_t)e.exponent;
        record[3] = e.before == 0x12345678 && e.after == 0x13572468;
        if (fwrite(record, sizeof(record), 1, out) != 1) return 10;
    }
    if (ferror(in)) return 11;
    fclose(in);
    return fclose(out) ? 12 : 0;
}

#ifndef __EMSCRIPTEN__
int __wrap_main(int argc, char **argv)
{
    if (argc != 5) return 13;
    return dd2_crt_probe(atoi(argv[1]), argv[2], argv[3], argv[4]);
}
#endif
