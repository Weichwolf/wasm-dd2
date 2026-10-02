/* Verify the virtual device's clock with exact timestamp bounds, not a guessed
 * timing tolerance. Filled buffers drain at rate, pause freezes, rewind frees
 * precisely its returned frames. Uses the real ALSA ioplug API. */
#include <alsa/asoundlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
static uint64_t now_ns(void){
    struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);
    return (uint64_t)ts.tv_sec*1000000000+(uint64_t)ts.tv_nsec;
}
static void require(int condition,const char *reason){
    if(!condition){fprintf(stderr,"Clocked PCM test: %s\n",reason);exit(1);}
}
int main(void){
    snd_pcm_t *pcm;snd_pcm_hw_params_t *hw;snd_pcm_sw_params_t *sw;
    snd_pcm_uframes_t buffer=22050,period=441,boundary;
    snd_pcm_sframes_t available,frozen,rewound;
    uint64_t start_before,start_after,read_before,read_after;
    float *zeros;
    require(snd_pcm_open(&pcm,"default",SND_PCM_STREAM_PLAYBACK,SND_PCM_NONBLOCK)>=0,"open clock device");
    require(snd_pcm_hw_params_malloc(&hw)>=0,"allocate hardware params");
    require(snd_pcm_hw_params_any(pcm,hw)>=0,"initialize hardware params");
    require(snd_pcm_hw_params_set_access(pcm,hw,SND_PCM_ACCESS_RW_INTERLEAVED)>=0,"interleaved access");
    require(snd_pcm_hw_params_set_format(pcm,hw,SND_PCM_FORMAT_FLOAT_LE)>=0,"float32 format");
    require(snd_pcm_hw_params_set_channels(pcm,hw,2)>=0,"stereo channels");
    require(snd_pcm_hw_params_set_rate(pcm,hw,44100,0)>=0,"44100 Hz rate");
    require(snd_pcm_hw_params_set_period_size_near(pcm,hw,&period,NULL)>=0,"period size");
    require(snd_pcm_hw_params_set_buffer_size_near(pcm,hw,&buffer)>=0,"buffer size");
    require(snd_pcm_hw_params(pcm,hw)>=0,"commit hardware params");
    snd_pcm_hw_params_free(hw);
    require(snd_pcm_sw_params_malloc(&sw)>=0,"allocate software params");
    require(snd_pcm_sw_params_current(pcm,sw)>=0,"current software params");
    require(snd_pcm_sw_params_get_boundary(sw,&boundary)>=0,"buffer boundary");
    require(snd_pcm_sw_params_set_start_threshold(pcm,sw,boundary)>=0,"manual start");
    require(snd_pcm_sw_params(pcm,sw)>=0,"commit software params");
    snd_pcm_sw_params_free(sw);
    zeros=calloc(buffer*2,sizeof(*zeros));require(zeros!=NULL,"allocate sample buffer");
    require(snd_pcm_prepare(pcm)>=0,"prepare stream");
    require(snd_pcm_writei(pcm,zeros,buffer)==(snd_pcm_sframes_t)buffer,"fill complete buffer");
    require(snd_pcm_avail_update(pcm)==0,"prepared buffer consumed samples before start");
    start_before=now_ns();require(snd_pcm_start(pcm)>=0,"start stream");start_after=now_ns();
    require(snd_pcm_wait(pcm,100)>0,"period timer did not wake the ALSA poll waiter");
    usleep(50000);
    read_before=now_ns();available=snd_pcm_avail_update(pcm);read_after=now_ns();
    require(available>=0,"unexpected underrun");
    require((uint64_t)available>=(read_before-start_after)*44100/1000000000 &&
            (uint64_t)available<=(read_after-start_before)*44100/1000000000,"sample clock outside exact timestamp bounds");
    require(snd_pcm_pause(pcm,1)>=0,"pause stream");
    frozen=snd_pcm_avail_update(pcm);usleep(50000);
    require(snd_pcm_avail_update(pcm)==frozen,"clock advanced while paused");
    rewound=snd_pcm_rewind(pcm,100);require(rewound==100,"rewind count");
    require(snd_pcm_avail_update(pcm)==frozen+rewound,"rewind did not free the exact sample count");
    start_before=now_ns();require(snd_pcm_pause(pcm,0)>=0,"resume stream");start_after=now_ns();
    usleep(50000);
    read_before=now_ns();available=snd_pcm_avail_update(pcm)-frozen-rewound;read_after=now_ns();
    require(available>=0 && (uint64_t)available>=(read_before-start_after)*44100/1000000000 &&
            (uint64_t)available<=(read_after-start_before)*44100/1000000000,"resumed clock outside timestamp bounds");
    require(snd_pcm_drop(pcm)>=0,"drop stream");
    require(snd_pcm_prepare(pcm)>=0,"prepare after drop");
    require(snd_pcm_avail_update(pcm)==(snd_pcm_sframes_t)buffer,"prepare did not reset pointers");
    require(snd_pcm_writei(pcm,zeros,64)==64,"queue underrun fixture");
    require(snd_pcm_start(pcm)>=0,"start short buffer");
    usleep(10000);
    snd_pcm_avail_update(pcm);
    require(snd_pcm_state(pcm)==SND_PCM_STATE_XRUN,"missing hardware underrun state");
    require(snd_pcm_close(pcm)>=0,"close device");free(zeros);
    puts("PASS sample-clock timestamp bounds, poll, manual start, pause/resume, rewind, prepare reset and underrun");
    return 0;
}
