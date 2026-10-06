/* Read-only accepted PulseAudio client PCM observer. The daemon's consumption
 * and DAC output are deliberately outside this observation boundary. */
#define _GNU_SOURCE
#include <pulse/pulseaudio.h>
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct capture {
    pa_stream *stream;
    FILE *pcm, *journal;
    uint64_t frames;
    size_t frame_bytes;
    struct capture *next;
} Capture;
static Capture *captures;
static unsigned serial;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static void *pulse;
static void load_pulse(void) { pulse = dlopen("libpulse.so.0", RTLD_LAZY | RTLD_LOCAL); }
static void *symbol(const char *name) {
    pthread_once(&once, load_pulse);
    void *result = pulse ? dlsym(pulse, name) : NULL;
    if (!result) { fprintf(stderr, "[pulse-capture] Missing symbol %s\n", name); abort(); }
    return result;
}
#define REAL(name) ((__typeof__(&name))symbol(#name))
static uint64_t now_ns(void) {
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t)) abort();
    return (uint64_t)t.tv_sec * 1000000000 + (uint64_t)t.tv_nsec;
}
static const char *active_root(void) {
    const char *root = getenv("DD2_PULSE_CAPTURE"), *wanted = getenv("DD2_AUDIO_PROCESS");
    if (!root || !wanted) return NULL;
    FILE *f = fopen("/proc/self/comm", "r");
    if (!f) return NULL;
    char name[32]; int valid = fgets(name, sizeof(name), f) != NULL;
    fclose(f);
    if (!valid) return NULL;
    name[strcspn(name, "\n")] = 0;
    return !strcmp(name, wanted) ? root : NULL;
}
static void error(const char *root, const char *message) {
    char path[4096];
    if (snprintf(path, sizeof(path), "%s/error.txt", root) >= (int)sizeof(path)) abort();
    FILE *f = fopen(path, "a");
    if (!f) abort();
    fprintf(f, "%s\n", message); fclose(f);
}
static Capture *find(pa_stream *stream) {
    for (Capture *c = captures; c; c = c->next) if (c->stream == stream) return c;
    return NULL;
}
static void flush(Capture *c, const char *root) {
    if (fflush(c->pcm) || fflush(c->journal) || ferror(c->pcm) || ferror(c->journal))
        error(root, "Cannot flush accepted PCM/journal");
}
static Capture *create(pa_stream *stream, const char *root) {
    const pa_sample_spec *spec = REAL(pa_stream_get_sample_spec)(stream);
    if (!spec || !REAL(pa_sample_spec_valid)(spec)) {
        error(root, "Invalid stream sample specification"); return NULL;
    }
    Capture *c = calloc(1, sizeof(*c));
    if (!c) { error(root, "Cannot allocate capture"); return NULL; }
    c->stream = stream; c->frame_bytes = REAL(pa_frame_size)(spec);
    char path[4096]; const unsigned id = serial++;
    if (snprintf(path, sizeof(path), "%s/pulse-%d-%u.pcm", root, (int)getpid(), id) >= (int)sizeof(path)) abort();
    c->pcm = fopen(path, "wbx");
    if (snprintf(path, sizeof(path), "%s/pulse-%d-%u.jsonl", root, (int)getpid(), id) >= (int)sizeof(path)) abort();
    c->journal = fopen(path, "wx");
    if (!c->pcm || !c->journal || !c->frame_bytes) {
        if (c->pcm) fclose(c->pcm);
        if (c->journal) fclose(c->journal);
        free(c); error(root, "Cannot create fresh capture files"); return NULL;
    }
    Dl_info library;
    if (!dladdr((void *)REAL(pa_stream_write), &library) || !library.dli_fname ||
        strpbrk(library.dli_fname, "\"\\\n\r")) abort();
    fprintf(c->journal, "{\"event\":\"format\",\"time_ns\":%"PRIu64",\"pid\":%d,"
            "\"rate\":%u,\"channels\":%u,\"frame_bytes\":%zu,\"format\":\"%s\","
            "\"client_library\":\"%s\"}\n",
            now_ns(), (int)getpid(), spec->rate, spec->channels, c->frame_bytes,
            REAL(pa_sample_format_to_string)(spec->format), library.dli_fname);
    c->next = captures; captures = c;
    flush(c, root); return c;
}
int pa_stream_write(pa_stream *stream, const void *data, size_t bytes,
                    pa_free_cb_t free_cb, int64_t offset, pa_seek_mode_t seek) {
    const int before_errno = errno;
    const char *root = active_root();
    if (!root) {
        errno = before_errno;
        return REAL(pa_stream_write)(stream, data, bytes, free_cb, offset, seek);
    }
    pthread_mutex_lock(&mutex);
    Capture *c = find(stream);
    if (!c) c = create(stream, root);
    /* Pulse owns begin_write buffers after submission. Save their original
     * bytes before forwarding the real call, rather than reading freed data. */
    void *copy = NULL;
    if (c && bytes) {
        if (bytes % c->frame_bytes || !data || offset || seek != PA_SEEK_RELATIVE || free_cb) {
            error(root, "Unsupported partial frame or nonsequential Pulse write");
        } else {
            copy = malloc(bytes);
            if (copy) memcpy(copy, data, bytes);
            else error(root, "Cannot copy original submitted PCM");
        }
    }
    errno = before_errno;
    const uint64_t begin = now_ns();
    const int result = REAL(pa_stream_write)(stream, data, bytes, free_cb, offset, seek), saved = errno;
    const uint64_t end = now_ns();
    if (c) {
        if (result >= 0 && bytes && copy && fwrite(copy, 1, bytes, c->pcm) != bytes)
            error(root, "Incomplete accepted PCM capture");
        fprintf(c->journal, "{\"event\":\"write\",\"time_ns\":%"PRIu64","
                "\"call_begin_ns\":%"PRIu64",\"call_end_ns\":%"PRIu64","
                "\"offset_frames\":%"PRIu64",\"requested_bytes\":%zu,\"result\":%d}\n",
                now_ns(), begin, end, c->frames, bytes, result);
        if (result >= 0) c->frames += bytes / c->frame_bytes;
        flush(c, root);
    }
    free(copy); pthread_mutex_unlock(&mutex); errno = saved; return result;
}
int pa_stream_disconnect(pa_stream *stream) {
    const int result = REAL(pa_stream_disconnect)(stream), saved = errno;
    pthread_mutex_lock(&mutex);
    Capture *c = find(stream);
    if (c && result >= 0) {
        fprintf(c->journal, "{\"event\":\"close\",\"time_ns\":%"PRIu64",\"frames\":%"PRIu64","
                "\"result\":%d}\n", now_ns(), c->frames, result);
        flush(c, getenv("DD2_PULSE_CAPTURE"));
        int failed = fclose(c->pcm);
        failed |= fclose(c->journal);
        if (failed) error(getenv("DD2_PULSE_CAPTURE"), "Cannot close capture files");
        Capture **link = &captures;
        while (*link != c) link = &(*link)->next;
        *link = c->next; free(c);
    }
    pthread_mutex_unlock(&mutex); errno = saved; return result;
}
