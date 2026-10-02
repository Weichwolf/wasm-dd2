#ifndef DD2_AVI_H
#define DD2_AVI_H
#include <stddef.h>
#include <stdint.h>
typedef struct DD2AVI DD2AVI;
typedef struct {
    unsigned width,height,frames,empty_frames;
    unsigned video_scale,video_rate,video_start;
    unsigned audio_scale,audio_rate,audio_start,audio_initial_frames;
    unsigned pcm_rate,pcm_channels;
    size_t pcm_frames;
} DD2AVIInfo;
/* Owns a copy of the complete RIFF file. No platform I/O, playback clock or
 * presentation is implied by this source container/decoder interface. */
DD2AVI* dd2_avi_open(const uint8_t* data,size_t bytes);
void dd2_avi_close(DD2AVI* movie);
const DD2AVIInfo* dd2_avi_info(const DD2AVI* movie);
const int16_t* dd2_avi_pcm(const DD2AVI* movie);
/* Zero-based source frame. Forward requests decode every intervening packet;
 * backward requests rebuild codebooks from frame zero. Empty packets hold the
 * previous image. The returned RGB24 storage is valid until the next request
 * or close. Invalid frame numbers leave decoder state unchanged. */
const uint8_t* dd2_avi_frame(DD2AVI* movie,unsigned frame);
#endif
