/* A private virtual PCM device with a monotonic real-time sample clock.
 * Unlike ALSA null, its playback buffer drains only at the negotiated rate.
 * It discards device samples; wine_audio.c observes the real mixer's writes.
 * ALSA's ioplug handles buffer bounds/rewinds; pointer reports clock underruns.
 */
#define _GNU_SOURCE
#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
typedef struct {
    snd_pcm_ioplug_t io;
    uint64_t epoch,position,boundary;
    int running;
} Device;
static uint64_t now_ns(void){
    struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);
    return (uint64_t)ts.tv_sec*1000000000+(uint64_t)ts.tv_nsec;
}
static uint64_t position(Device *device){
    uint64_t elapsed=device->running?now_ns()-device->epoch:0;
    return device->position+elapsed/1000000000*device->io.rate+
           elapsed%1000000000*device->io.rate/1000000000;
}
static int start(snd_pcm_ioplug_t *io){
    Device *device=io->private_data;struct itimerspec timer={0};
    uint64_t ns=(uint64_t)io->period_size*1000000000/io->rate;
    if(!ns)ns=1;
    timer.it_value.tv_sec=timer.it_interval.tv_sec=(time_t)(ns/1000000000);
    timer.it_value.tv_nsec=timer.it_interval.tv_nsec=(long)(ns%1000000000);
    device->epoch=now_ns();device->running=1;
    return timerfd_settime(io->poll_fd,0,&timer,NULL)<0?-errno:0;
}
static int stop(snd_pcm_ioplug_t *io){
    Device *device=io->private_data;struct itimerspec timer={0};
    device->position=position(device);device->running=0;
    return timerfd_settime(io->poll_fd,0,&timer,NULL)<0?-errno:0;
}
static snd_pcm_sframes_t pointer(snd_pcm_ioplug_t *io){
    Device *device=io->private_data;
    uint64_t current=position(device)%device->boundary;
    snd_pcm_uframes_t available=snd_pcm_ioplug_avail(io,io->hw_ptr,io->appl_ptr);
    uint64_t advance=(current+device->boundary-io->hw_ptr)%device->boundary;
    uint64_t queued=available<=io->buffer_size?io->buffer_size-available:0;
    if(io->state==SND_PCM_STATE_RUNNING && advance>queued){
        struct itimerspec timer={0};
        device->position=io->appl_ptr;device->running=0;
        timerfd_settime(io->poll_fd,0,&timer,NULL);
        return -EPIPE;
    }
    if(io->state==SND_PCM_STATE_DRAINING && advance>queued)
        current=io->appl_ptr;
    return (snd_pcm_sframes_t)current;
}
static int prepare(snd_pcm_ioplug_t *io){
    Device *device=io->private_data;
    stop(io);device->position=0;device->epoch=0;
    return 0;
}
static int sw_params(snd_pcm_ioplug_t *io,snd_pcm_sw_params_t *params){
    Device *device=io->private_data;snd_pcm_uframes_t boundary;
    int result=snd_pcm_sw_params_get_boundary(params,&boundary);
    if(result>=0)device->boundary=boundary;
    return result;
}
static snd_pcm_sframes_t transfer(snd_pcm_ioplug_t *io,const snd_pcm_channel_area_t *areas,
                                  snd_pcm_uframes_t offset,snd_pcm_uframes_t size){
    (void)io;(void)areas;(void)offset;return (snd_pcm_sframes_t)size;
}
static int pause_device(snd_pcm_ioplug_t *io,int paused){return paused?stop(io):start(io);}
static int close_device(snd_pcm_ioplug_t *io){
    close(io->poll_fd);free(io->private_data);return 0;
}
static int poll_revents(snd_pcm_ioplug_t *io,struct pollfd *fds,unsigned count,unsigned short *revents){
    uint64_t expirations;
    if(count!=1)return -EINVAL;
    *revents=fds[0].revents&(POLLERR|POLLHUP);
    if(fds[0].revents&POLLIN){
        while(read(io->poll_fd,&expirations,sizeof(expirations))==(ssize_t)sizeof(expirations)){}
        *revents|=POLLOUT;
    }
    return 0;
}
static const snd_pcm_ioplug_callback_t callbacks={
    .start=start,.stop=stop,.pointer=pointer,.transfer=transfer,.close=close_device,
    .prepare=prepare,.sw_params=sw_params,.pause=pause_device,.poll_revents=poll_revents
};
SND_PCM_PLUGIN_DEFINE_FUNC(dd2clock){
    Device *device;int result;
    const unsigned access[]={SND_PCM_ACCESS_RW_INTERLEAVED};
    const unsigned formats[]={SND_PCM_FORMAT_FLOAT_LE,SND_PCM_FORMAT_S16_LE};
    unsigned rate=44100;const char *setting=getenv("DD2_AUDIO_RATE");
    (void)root;(void)conf;
    if(stream!=SND_PCM_STREAM_PLAYBACK)return -ENODEV;
    if(setting)rate=(unsigned)strtoul(setting,NULL,10);
    if(rate!=22050 && rate!=44100 && rate!=48000)return -EINVAL;
    device=calloc(1,sizeof(*device));if(!device)return -ENOMEM;
    device->boundary=0x40000000;
    device->io.version=SND_PCM_IOPLUG_VERSION;device->io.name="DD2 clocked reference PCM";
    device->io.flags=SND_PCM_IOPLUG_FLAG_MONOTONIC|SND_PCM_IOPLUG_FLAG_BOUNDARY_WA;
    device->io.callback=&callbacks;device->io.private_data=device;
    device->io.poll_fd=timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK|TFD_CLOEXEC);
    device->io.poll_events=POLLIN;
    if(device->io.poll_fd<0){result=-errno;free(device);return result;}
    result=snd_pcm_ioplug_create(&device->io,name,stream,mode);
    if(result<0){close(device->io.poll_fd);free(device);return result;}
#define CONSTRAIN(expression) do{result=(expression);if(result<0){snd_pcm_ioplug_delete(&device->io);return result;}}while(0)
    CONSTRAIN(snd_pcm_ioplug_set_param_list(&device->io,SND_PCM_IOPLUG_HW_ACCESS,1,access));
    CONSTRAIN(snd_pcm_ioplug_set_param_list(&device->io,SND_PCM_IOPLUG_HW_FORMAT,2,formats));
    CONSTRAIN(snd_pcm_ioplug_set_param_minmax(&device->io,SND_PCM_IOPLUG_HW_CHANNELS,2,2));
    CONSTRAIN(snd_pcm_ioplug_set_param_minmax(&device->io,SND_PCM_IOPLUG_HW_RATE,rate,rate));
    CONSTRAIN(snd_pcm_ioplug_set_param_minmax(&device->io,SND_PCM_IOPLUG_HW_PERIOD_BYTES,128,65536));
    CONSTRAIN(snd_pcm_ioplug_set_param_minmax(&device->io,SND_PCM_IOPLUG_HW_BUFFER_BYTES,256,262144));
    CONSTRAIN(snd_pcm_ioplug_set_param_minmax(&device->io,SND_PCM_IOPLUG_HW_PERIODS,2,64));
    *pcmp=device->io.pcm;return 0;
}
SND_PCM_PLUGIN_SYMBOL(dd2clock);
