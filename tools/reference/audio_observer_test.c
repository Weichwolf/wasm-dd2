/* Use real ALSA null PCM to verify observer bytes, transport and process scope. */
#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
static void require(int condition,const char *reason){
    if(!condition){fprintf(stderr,"ALSA observer test: %s\n",reason);exit(1);}
}
int main(int argc,char **argv){
    int inside,pass,i;float samples[64];
    require(argc==2,"inside/outside argument required");
    inside=!strcmp(argv[1],"inside");
    require(prctl(PR_SET_NAME,inside?"dd2h.exe":"alsa-unit") == 0,"set test process name");
    for(i=0;i<64;i++)samples[i]=(float)(i-32)/64;
    for(pass=0;pass<2;pass++){
        snd_pcm_t *pcm;snd_pcm_hw_params_t *params;
        unsigned rate=inside?44100:22050,committed;
        require(snd_pcm_open(&pcm,"default",SND_PCM_STREAM_PLAYBACK,0)>=0,"open null PCM");
        require(snd_pcm_hw_params_malloc(&params)>=0,"allocate params");
        require(snd_pcm_hw_params_any(pcm,params)>=0,"initialize params");
        if(inside)require(snd_pcm_hw_params_set_rate(pcm,params,48000,0)<0,"capture rate constraint missing");
        require(snd_pcm_hw_params_set_access(pcm,params,SND_PCM_ACCESS_RW_INTERLEAVED)>=0,"interleaved access");
        require(snd_pcm_hw_params_set_format(pcm,params,SND_PCM_FORMAT_FLOAT_LE)>=0,"float format");
        require(snd_pcm_hw_params_set_channels(pcm,params,2)>=0,"stereo channels");
        require(snd_pcm_hw_params_set_rate(pcm,params,rate,0)>=0,"configure requested rate");
        require(snd_pcm_hw_params(pcm,params)>=0,"commit hardware params");
        require(snd_pcm_hw_params_get_rate(params,&committed,NULL)>=0 && committed==rate,"outside rate changed");
        snd_pcm_hw_params_free(params);
        require(snd_pcm_prepare(pcm)>=0,"prepare");
        require(snd_pcm_writei(pcm,samples,16)==16,"first accepted PCM");
        require(snd_pcm_drop(pcm)>=0,"drop");
        require(snd_pcm_writei(pcm,samples,16)<0,"write while stopped must fail");
        require(snd_pcm_prepare(pcm)>=0,"restart");
        require(snd_pcm_writei(pcm,samples+32,16)==16,"second accepted PCM");
        require(snd_pcm_rewind(pcm,4)>=0,"rewind");
        require(snd_pcm_close(pcm)>=0,"close");
    }
    puts("PASS real ALSA writes/failed write/drop/prepare/rewind/close and rate scope");
    return 0;
}
