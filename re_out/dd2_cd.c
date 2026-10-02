/* WinMM CD-Audio backend for the port. The engine still selects tracks, stops,
 * resumes and repeats them through its original MCI calls. This device reads
 * exact s16le/44100Hz/stereo CDDA bytes; it does not mix or downsample them into
 * the game's separate 22050Hz effects stream. DD2_CDPCM captures its source PCM.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef DD2_BROWSER
#include <emscripten.h>
#endif
#include "dd2_cd.h"
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
static unsigned cd_format = 2, cd_last_ms;
static uint64_t cd_position, cd_end;
static unsigned cd_remainder;
static FILE *cd_file, *cd_capture;
static int cd_capture_init;
#ifdef DD2_BROWSER
static unsigned char *cd_data;
static int cd_data_size;
EM_JS(void, cd_sink_stop, (), {
    if (Module._dd2cdsources) {
        Module._dd2cdsources.forEach(function(source) { try { source.stop(); } catch(e){} });
        Module._dd2cdsources = [];
    }
    Module._dd2cdt = 0;
});
EM_JS(void, cd_sink_push, (const short *pcm, int frames), {
    if (!Module._dd2ac) {
        Module._dd2ac = new AudioContext({sampleRate:44100});
        Module._dd2t = 0;
        var resume = function() { if (Module._dd2ac.state === 'suspended') Module._dd2ac.resume(); };
        window.addEventListener('keydown', resume);
        window.addEventListener('click', resume);
    }
    var ac = Module._dd2ac;
    if (ac.state !== 'running') return;
    var buffer = ac.createBuffer(2, frames, 44100);
    var left = buffer.getChannelData(0), right = buffer.getChannelData(1);
    for (var i=0; i<frames; i++) {
        left[i] = HEAP16[(pcm>>1)+i*2] / 32768;
        right[i] = HEAP16[(pcm>>1)+i*2+1] / 32768;
    }
    var source = ac.createBufferSource();
    source.buffer = buffer; source.connect(ac.destination);
    var sources = Module._dd2cdsources || (Module._dd2cdsources=[]);
    sources.push(source);
    source.onended = function() { var i=sources.indexOf(source); if (i>=0) sources.splice(i,1); };
    if (!Module._dd2cdt || Module._dd2cdt < ac.currentTime) Module._dd2cdt = ac.currentTime + 0.04;
    source.start(Module._dd2cdt);
    Module._dd2cdt += frames/44100;
});
#else
static void cd_sink_stop(void) {}
#endif

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

void dd2_cd_pump(void) {
    unsigned now = dd2_audio_ms(), elapsed = now-cd_last_ms;
    uint64_t frames;
    cd_last_ms = now;
    if (!cd_open || cd_mode != MODE_PLAY) return;
    frames = (uint64_t)elapsed*44100+cd_remainder;
    cd_remainder = (unsigned)(frames%1000);
    frames /= 1000;
    if (frames > cd_end-cd_position) frames = cd_end-cd_position;
    if (!cd_capture_init) {
        const char *capture = getenv("DD2_CDPCM");
        cd_capture_init = 1;
        if (capture) cd_capture = fopen(capture,"wb");
    }
    while (frames) {
        static short pcm[4096*2];
        unsigned count = frames > 4096 ? 4096 : (unsigned)frames;
        int track = position_track(cd_position);
        uint64_t remaining = track_start(track+1)-cd_position, offset;
        if (count > remaining) count = (unsigned)remaining;
        if (load_track(track)) { cd_mode = MODE_STOP; cd_sink_stop(); return; }
        offset = (cd_position-track_start(track))*4;
#ifdef DD2_BROWSER
        memcpy(pcm,cd_data+offset,count*4);
#else
        if (fseek(cd_file,(long)offset,SEEK_SET) || fread(pcm,4,count,cd_file) != count) {
            cd_mode = MODE_STOP; cd_sink_stop(); return;
        }
#endif
        if (cd_capture) fwrite(pcm,4,count,cd_capture);
#ifdef DD2_BROWSER
        cd_sink_push(pcm,count);
#endif
        cd_position += count;
        frames -= count;
    }
    if (cd_capture) fflush(cd_capture);
    if (cd_position >= cd_end) cd_mode = MODE_STOP;
}

int dd2_mci_send(unsigned device, unsigned command, unsigned flags, uint32_t *params) {
    uint64_t from, to;
    int error;
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
        cd_last_ms = dd2_audio_ms(); cd_remainder = 0;
        params[1] = CD_DEVICE;
        return 0;
    }
    if (!cd_open || device != CD_DEVICE) return INVALID_DEVICE;
    dd2_cd_pump();
    switch (command) {
    case 0x804: /* MCI_CLOSE */
        cd_sink_stop(); release_track(); cd_open = 0; cd_mode = MODE_STOP; return 0;
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
        cd_sink_stop(); cd_mode = MODE_STOP;
        if (from < to) { error = load_track(position_track(from)); if (error) return error; }
        cd_position = from; cd_end = to;
        cd_last_ms = dd2_audio_ms();
        if (flags & 4) cd_remainder = 0;
        cd_mode = from < to ? MODE_PLAY : MODE_STOP;
        if (getenv("DD2_CDLOG")) printf("[CD] play track=%d frame=%u mode=%d\n",position_track(from),
            (unsigned)(from-track_start(position_track(from))),cd_mode);
        return 0;
    case 0x808:
        cd_sink_stop(); cd_mode = MODE_STOP;
        if (getenv("DD2_CDLOG")) printf("[CD] stop track=%d frame=%u\n",position_track(cd_position),
            (unsigned)(cd_position-track_start(position_track(cd_position))));
        return 0; /* MCI_STOP keeps the cursor */
    case 0x809: cd_sink_stop(); cd_mode = MODE_PAUSE; return 0;
    case 0x855: cd_last_ms = dd2_audio_ms(); cd_mode = MODE_PLAY; return 0;
    case 0x830: return UNRECOGNIZED; /* Wine's CD driver also rejects MCI_CUE; game ignores it. */
    default: return UNRECOGNIZED;
    }
}
