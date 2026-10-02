/* WinMM CD-Audio backend for the port. The engine still selects tracks, stops,
 * resumes and repeats them through its original MCI calls. This device reads
 * exact s16le/44100Hz/stereo CDDA bytes into the same ordered DirectSound device
 * as effects. DD2_CDPCM captures consumed source frames; DD2_MIXPCM captures
 * the shared device's final Float32 PCM. There is one clock and browser sink.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef DD2_BROWSER
#include <emscripten.h>
#endif
#include "dd2_cd.h"
#include "dd2_sound.h"
#include "dd2_disc.h"

#define CD_DEVICE 1
#define MODE_STOP 525
#define MODE_PLAY 526
#define MODE_PAUSE 529
#define INVALID_DEVICE 257
#define UNRECOGNIZED 261
#define HARDWARE_ERROR 262
#define MISSING_PARAMETER 273
#define UNSUPPORTED 274
#define NOT_READY 276
#define OUT_OF_RANGE 282
#define MUST_SHARE 291
#define BAD_TIME_FORMAT 293
#define NULL_PARAMETER 297

static int cd_open, cd_mode = MODE_STOP, cd_track = -1;
static unsigned cd_format = 2;
static uint64_t cd_position, cd_end, cd_origin;
static void* cd_sound;
static FILE *cd_file, *cd_capture;
static int cd_capture_init;
#ifdef DD2_BROWSER
static unsigned char *cd_data;
static int cd_data_size;
#endif

typedef struct { uint64_t start; unsigned count; int valid; short pcm[4096*2]; } CDPage;
static CDPage cd_pages[2];
static unsigned cd_next_page;

static uint64_t track_start(int track) { return (uint64_t)dd2_cd_sectors[track-1] * 588; }
static int position_track(uint64_t position) {
    int track = 1;
    while (track < DD2_CD_TRACKS && position >= track_start(track+1)) track++;
    return track;
}
static void release_track(void) {
    if (cd_file) fclose(cd_file);
    cd_file = NULL;
#ifdef DD2_BROWSER
    free(cd_data); cd_data = NULL; cd_data_size = 0;
#endif
    cd_track = -1;
}
static int load_track(int track) {
    char path[1024];
    const char *root = getenv("DD2_CD_ROOT");
    uint64_t expected;
    if (track < 2 || track > DD2_CD_TRACKS) return OUT_OF_RANGE;
    if (track == cd_track) return 0;
    release_track();
    if (!root) root = "Redbook";
    if (snprintf(path,sizeof(path),"%s/track%02d.cdda",root,track) >= (int)sizeof(path)) return NOT_READY;
    expected = (track_start(track+1) - track_start(track))*4;
#ifdef DD2_BROWSER
    {
        int error = 0;
        emscripten_wget_data(path, (void**)&cd_data, &cd_data_size, &error);
        if (error || !cd_data || (uint64_t)cd_data_size != expected) { release_track(); return NOT_READY; }
    }
#else
    cd_file = fopen(path,"rb");
    if (!cd_file) return NOT_READY;
    if (fseek(cd_file,0,SEEK_END) || (uint64_t)ftell(cd_file) != expected) { release_track(); return NOT_READY; }
#endif
    cd_track = track;
    return 0;
}
static int decode_time(unsigned value, uint64_t *result) {
    uint64_t sector;
    if (cd_format == 10) {
        unsigned track = value & 255, minute = value>>8 & 255, second = value>>16 & 255, frame = value>>24;
        if (!track || track > DD2_CD_TRACKS || second >= 60 || frame >= 75) return OUT_OF_RANGE;
        sector = dd2_cd_sectors[track-1] + (minute*60u+second)*75u + frame;
    } else if (cd_format == 2) {
        unsigned minute = value & 255, second = value>>8 & 255, frame = value>>16 & 255;
        if (second >= 60 || frame >= 75) return OUT_OF_RANGE;
        sector = (minute*60u+second)*75u+frame;
        if (sector < 150) return OUT_OF_RANGE;
        sector -= 150;
    } else {
        /* CD MCI milliseconds are absolute MSF time (including the 150-sector
           lead-in), rounded to a CD sector. Wine mcicda CalcFrame/CalcTime
           use the same 1ms origin, not a raw sample offset. */
        if (!value) return OUT_OF_RANGE;
        sector = ((uint64_t)(value-1)*75+500)/1000;
        if (sector < 150) return OUT_OF_RANGE;
        sector -= 150;
    }
    *result = sector * 588;
    return sector <= dd2_cd_sectors[DD2_CD_TRACKS] ? 0 : OUT_OF_RANGE;
}
static unsigned encode_position(uint64_t position) {
    unsigned sector = (unsigned)(position/588), track, minute, second, frame;
    if (cd_format == 0) return (sector+150)*1000/75+1;
    if (cd_format == 10) {
        track = position_track(position);
        sector -= dd2_cd_sectors[track-1];
        minute = sector/4500; second = sector/75%60; frame = sector%75;
        return track | minute<<8 | second<<16 | frame<<24;
    }
    sector += 150;
    minute = sector/4500; second = sector/75%60; frame = sector%75;
    return minute | second<<8 | frame<<16;
}

/* Two read-only pages retain both sides of a FIR lookahead/page/track boundary.
 * The reader never advances the MCI cursor; only consumed device source frames
 * do. A track fetch therefore cannot independently move the CD clock. */
static CDPage* source_page(uint64_t position) {
    CDPage* page;
    int i,track;
    uint64_t offset,remaining;
    for(i=0;i<2;i++)if(cd_pages[i].valid && position>=cd_pages[i].start &&
            position<cd_pages[i].start+cd_pages[i].count)return &cd_pages[i];
    if(position>=cd_end)return NULL;
    track=position_track(position);
    if(load_track(track))return NULL;
    page=&cd_pages[cd_next_page++%2];page->valid=0;
    page->start=track_start(track)+(position-track_start(track))/4096*4096;
    remaining=track_start(track+1)-page->start;
    if(remaining>cd_end-page->start)remaining=cd_end-page->start;
    page->count=remaining>4096?4096:(unsigned)remaining;
    offset=(page->start-track_start(track))*4;
#ifdef DD2_BROWSER
    memcpy(page->pcm,cd_data+offset,page->count*4);
#else
    if(fseek(cd_file,(long)offset,SEEK_SET) || fread(page->pcm,4,page->count,cd_file)!=page->count)return NULL;
#endif
    page->valid=1;
    return page;
}
static float read_source(void* context,uint64_t frame,int channel) {
    uint64_t position=cd_origin+frame;
    CDPage* page;
    (void)context;
    page=source_page(position);
    if(!page){cd_mode=MODE_STOP;dd2_snd_music_stop(cd_sound);return 0.0f;}
    return page->pcm[(position-page->start)*2+channel]/32768.0f;
}
static void consume_source(void* context,unsigned first,unsigned frames) {
    uint64_t position=cd_origin+first;
    (void)context;
    if (!cd_capture_init) {
        const char *capture = getenv("DD2_CDPCM");
        cd_capture_init = 1;
        if (capture) cd_capture = fopen(capture,"wb");
    }
    if(cd_capture)while(frames){
        CDPage* page=source_page(position);
        unsigned count;
        if(!page){cd_mode=MODE_STOP;dd2_snd_music_stop(cd_sound);return;}
        count=page->count-(unsigned)(position-page->start);
        if(count>frames)count=frames;
        fwrite(page->pcm+(position-page->start)*2,4,count,cd_capture);
        position+=count;frames-=count;
    }
    else position+=frames;
    cd_position=position;
    if (cd_capture) fflush(cd_capture);
    if (cd_position >= cd_end) cd_mode = MODE_STOP;
}
void dd2_cd_pump(void) { dd2_snd_mix_flip(); }
static void destroy_sound(void) {
    dd2_snd_music_destroy(cd_sound);cd_sound=NULL;
}

int dd2_mci_send(unsigned device, unsigned command, unsigned flags, uint32_t *params) {
    uint64_t from, to;
    int error;
    dd2_cd_pump(); /* Flush the shared device before changing either source. */
    if (command == 0x803) { /* MCI_OPEN */
        const char *type;
        if (!params) return NULL_PARAMETER;
        if (!(flags & 0x2000) || (flags & 0x200)) return UNSUPPORTED; /* AVI remains a separate device. */
        type = (const char*)(uintptr_t)params[2];
        if (!type || strcmp(type,"cdaudio")) return UNSUPPORTED;
        if (cd_open) return MUST_SHARE;
#ifndef DD2_BROWSER
        {
            int track;
            for (track=2;track<=DD2_CD_TRACKS;track++) {
                error = load_track(track);
                if (error) return error;
            }
            release_track();
        }
#endif
        cd_open = 1; cd_mode = MODE_STOP; cd_format = 2;
        cd_position = track_start(2); cd_end = track_start(DD2_CD_TRACKS+1);
        cd_pages[0].valid=cd_pages[1].valid=0;
        params[1] = CD_DEVICE;
        return 0;
    }
    if (!cd_open || device != CD_DEVICE) return INVALID_DEVICE;
    switch (command) {
    case 0x804: /* MCI_CLOSE */
        destroy_sound();release_track();cd_pages[0].valid=cd_pages[1].valid=0;
        cd_open = 0; cd_mode = MODE_STOP; return 0;
    case 0x80d: /* MCI_SET */
        if (!params) return NULL_PARAMETER;
        if (flags & 0x400) {
            if (params[1] != 0 && params[1] != 2 && params[1] != 10) return BAD_TIME_FORMAT;
            cd_format = params[1];
        }
        return 0;
    case 0x814: /* MCI_STATUS */
        if (!params) return NULL_PARAMETER;
        if (!(flags & 0x100)) return MISSING_PARAMETER;
        switch (params[2]) {
        case 2: params[1] = encode_position(cd_position); return 0;
        case 3: params[1] = DD2_CD_TRACKS; return 0;
        case 4: params[1] = cd_mode; return 0;
        case 5: case 7: params[1] = 1; return 0;
        case 6: params[1] = cd_format; return 0;
        case 8: params[1] = position_track(cd_position); return 0;
        default: return UNSUPPORTED;
        }
    case 0x806: /* MCI_PLAY: FROM/TO, or TO only after the original's MCI_STOP pause. */
        if (!params) return NULL_PARAMETER;
        from = cd_position; to = track_start(DD2_CD_TRACKS+1);
        if (flags & 4) { error = decode_time(params[1],&from); if (error) return error; }
        if (flags & 8) { error = decode_time(params[2],&to); if (error) return error; }
        if (from < track_start(2)) from = track_start(2); /* skip the data track */
        if (to < from) return OUT_OF_RANGE;
        destroy_sound();cd_mode = MODE_STOP;
        if (from < to) { error = load_track(position_track(from)); if (error) return error; }
        cd_position = from; cd_end = to;
        cd_origin=from;
        cd_pages[0].valid=cd_pages[1].valid=0;
        if(from<to){cd_sound=dd2_snd_music_create((unsigned)(to-from),read_source,consume_source,NULL);
            dd2_snd_music_play(cd_sound);}
        cd_mode = from < to ? MODE_PLAY : MODE_STOP;
        if (getenv("DD2_CDLOG")) printf("[CD] play track=%d frame=%u mode=%d\n",position_track(from),
            (unsigned)(from-track_start(position_track(from))),cd_mode);
        return 0;
    case 0x808:
        destroy_sound();cd_mode = MODE_STOP;
        if (getenv("DD2_CDLOG")) printf("[CD] stop track=%d frame=%u\n",position_track(cd_position),
            (unsigned)(cd_position-track_start(position_track(cd_position))));
        return 0; /* MCI_STOP keeps the cursor */
    /* Wine mcicda Pause/Resume leave a drained worker stopped when stopEvent
     * is signalled. They also retain the existing state on repeated controls.
     * A retained completed source must not be restarted through its reset
     * DirectSound cursor; only a new MCI_PLAY creates a new transport. */
    case 0x809:
        if(cd_mode==MODE_PLAY){dd2_snd_music_stop(cd_sound);cd_mode=MODE_PAUSE;}
        return 0;
    case 0x855:
        if(cd_mode==MODE_PAUSE){dd2_snd_music_play(cd_sound);cd_mode=MODE_PLAY;}
        return 0;
    case 0x830: return UNRECOGNIZED; /* Wine's CD driver also rejects MCI_CUE; game ignores it. */
    default: return UNRECOGNIZED;
    }
}
