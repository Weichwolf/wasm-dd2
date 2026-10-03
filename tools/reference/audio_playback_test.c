/* Exercise the real ALSA device with independent known byte patterns.
 * The expected output uses frozen ALSA delay counters for partial playback,
 * never the capture journal or another mixer implementation.
 */
#include <alsa/asoundlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>
static void require(int ok,const char *reason){
    if(!ok){fprintf(stderr,"Played PCM test: %s\n",reason);exit(1);}
}
static unsigned frame_bytes;
static void pattern(unsigned char *data,unsigned frames,unsigned tag,int floating){
    unsigned i;
    for(i=0;i<frames*2;i++){
        int value=(int)((i+tag*991)%30000)-15000;
        if(floating)((float*)data)[i]=value/32768.0f;
        else ((int16_t*)data)[i]=(int16_t)value;
    }
}
static unsigned frozen(snd_pcm_t *pcm,unsigned frames){
    snd_pcm_sframes_t delay,again;
    require(snd_pcm_pause(pcm,1)>=0,"pause");
    require(snd_pcm_delay(pcm,&delay)>=0 && delay>0 && delay<(snd_pcm_sframes_t)frames,"partial frozen delay");
    usleep(5000);
    require(snd_pcm_delay(pcm,&again)>=0 && again==delay,"pause advanced sample clock");
    return frames-(unsigned)delay;
}
int main(int argc,char **argv){
    snd_pcm_t *pcm;snd_pcm_hw_params_t *hw;snd_pcm_sw_params_t *sw;
    snd_pcm_uframes_t buffer=8192,period=256,boundary;
    unsigned rate,spent_drop,spent_rewind;int floating;
    unsigned char *a,*b;FILE *expected;
    require(argc==3 || (argc==4 && (!strcmp(argv[3],"outside") || !strcmp(argv[3],"active-prepare"))),"format, expected PCM path and optional scope/active-prepare test required");
    floating=!strcmp(argv[1],"float");require(floating || !strcmp(argv[1],"s16"),"format");
    require(prctl(PR_SET_NAME,argc==4 && !strcmp(argv[3],"outside")?"alsa-unit":"dd2h.exe")==0,"set scoped process name");
    rate=getenv("DD2_AUDIO_RATE")?(unsigned)atoi(getenv("DD2_AUDIO_RATE")):44100;
    frame_bytes=floating?8:4;
    require(snd_pcm_open(&pcm,"default",SND_PCM_STREAM_PLAYBACK,0)>=0,"open");
    require(snd_pcm_hw_params_malloc(&hw)>=0 && snd_pcm_hw_params_any(pcm,hw)>=0,"hardware params");
    require(snd_pcm_hw_params_set_access(pcm,hw,SND_PCM_ACCESS_RW_INTERLEAVED)>=0,"access");
    require(snd_pcm_hw_params_set_format(pcm,hw,floating?SND_PCM_FORMAT_FLOAT_LE:SND_PCM_FORMAT_S16_LE)>=0,"sample format");
    require(snd_pcm_hw_params_set_channels(pcm,hw,2)>=0 && snd_pcm_hw_params_set_rate(pcm,hw,rate,0)>=0,"rate/channels");
    require(snd_pcm_hw_params_set_period_size_near(pcm,hw,&period,NULL)>=0 &&
            snd_pcm_hw_params_set_buffer_size_near(pcm,hw,&buffer)>=0 && snd_pcm_hw_params(pcm,hw)>=0,"buffer");
    snd_pcm_hw_params_free(hw);
    require(snd_pcm_sw_params_malloc(&sw)>=0 && snd_pcm_sw_params_current(pcm,sw)>=0 &&
            snd_pcm_sw_params_get_boundary(sw,&boundary)>=0 &&
            snd_pcm_sw_params_set_start_threshold(pcm,sw,boundary)>=0 && snd_pcm_sw_params(pcm,sw)>=0,"manual start");
    snd_pcm_sw_params_free(sw);
    a=malloc(buffer*frame_bytes);b=malloc(buffer*frame_bytes);expected=fopen(argv[2],"wb");
    require(a && b && expected,"allocate expected samples");
    /* Rewind replaces the queued tail; pause/resume must preserve the prefix. */
    pattern(a,buffer,1,floating);pattern(b,64,2,floating);
    require(snd_pcm_prepare(pcm)>=0 && snd_pcm_writei(pcm,a,buffer)==(snd_pcm_sframes_t)buffer,"first queue");
    require(snd_pcm_rewind(pcm,64)==64 && snd_pcm_writei(pcm,b,64)==64,"replace queued tail");
    require(snd_pcm_start(pcm)>=0,"first start");usleep(2000);frozen(pcm,buffer);
    require(snd_pcm_pause(pcm,0)>=0 && snd_pcm_drain(pcm)>=0,"first resume/drain");
    require(fwrite(a,frame_bytes,buffer-64,expected)==buffer-64 && fwrite(b,frame_bytes,64,expected)==64,"first expected output");
    /* Dropping prepared data produces no samples; dropping after a pause
     * retains exactly the independently measured consumed prefix. */
    pattern(a,256,3,floating);
    require(snd_pcm_prepare(pcm)>=0 && snd_pcm_writei(pcm,a,256)==256 && snd_pcm_drop(pcm)>=0,"discard before start");
    pattern(a,buffer,4,floating);
    require(snd_pcm_prepare(pcm)>=0 && snd_pcm_writei(pcm,a,buffer)==(snd_pcm_sframes_t)buffer && snd_pcm_start(pcm)>=0,"partial drop start");
    usleep(2000);spent_drop=frozen(pcm,buffer);
    require(snd_pcm_drop(pcm)>=0 && fwrite(a,frame_bytes,spent_drop,expected)==spent_drop,"partial drop output");
    /* Rewind every remaining paused sample, then replace it with a new tail. */
    pattern(a,buffer,5,floating);pattern(b,128,6,floating);
    require(snd_pcm_prepare(pcm)>=0 && snd_pcm_writei(pcm,a,buffer)==(snd_pcm_sframes_t)buffer && snd_pcm_start(pcm)>=0,"partial rewind start");
    usleep(2000);spent_rewind=frozen(pcm,buffer);
    require(snd_pcm_rewind(pcm,buffer-spent_rewind)==(snd_pcm_sframes_t)(buffer-spent_rewind),"rewind all pending samples");
    require(snd_pcm_writei(pcm,b,128)==128 && snd_pcm_pause(pcm,0)>=0 && snd_pcm_drain(pcm)>=0,"replace/resume/drain tail");
    require(fwrite(a,frame_bytes,spent_rewind,expected)==spent_rewind && fwrite(b,frame_bytes,128,expected)==128,"partial rewind expected");
    /* Multiple full buffers exercise both source offsets and physical wrap. */
    pattern(a,buffer,7,floating);
    require(snd_pcm_prepare(pcm)>=0 && snd_pcm_writei(pcm,a,buffer)==(snd_pcm_sframes_t)buffer && snd_pcm_start(pcm)>=0,"wrap start");
    require(fwrite(a,frame_bytes,buffer,expected)==buffer,"wrap first expected");
    pattern(a,buffer,8,floating);
    require(snd_pcm_writei(pcm,a,buffer)==(snd_pcm_sframes_t)buffer && fwrite(a,frame_bytes,buffer,expected)==buffer,"wrap second buffer");
    pattern(a,buffer,9,floating);
    require(snd_pcm_writei(pcm,a,buffer)==(snd_pcm_sframes_t)buffer && snd_pcm_drain(pcm)>=0 && fwrite(a,frame_bytes,buffer,expected)==buffer,"wrap third buffer");
    /* XRUN consumes the complete queued prefix, never invented extra samples. */
    pattern(a,512,10,floating);
    require(snd_pcm_prepare(pcm)>=0 && snd_pcm_writei(pcm,a,512)==512 && snd_pcm_start(pcm)>=0,"underrun start");
    usleep(512000000/rate+5000);
    snd_pcm_avail_update(pcm);
    require(snd_pcm_state(pcm)==SND_PCM_STATE_XRUN && fwrite(a,frame_bytes,512,expected)==512,"underrun prefix");
    if(argc==4 && !strcmp(argv[3],"active-prepare")){
        require(snd_pcm_prepare(pcm)>=0 && snd_pcm_writei(pcm,a,512)==512 && snd_pcm_start(pcm)>=0,"active prepare setup");
        usleep(1000);
        require(snd_pcm_prepare(pcm)>=0,"direct prepare while running");
    }
    require(snd_pcm_close(pcm)>=0 && fclose(expected)==0,"close output");free(a);free(b);
    printf("{\"rate\":%u,\"buffer_frames\":%lu,\"partial_drop_frames\":%u,\"partial_rewind_frames\":%u,\"expected_frames\":%lu}\n",
           rate,(unsigned long)buffer,spent_drop,spent_rewind,(unsigned long)buffer*4+spent_drop+spent_rewind+128+512);
    return 0;
}
