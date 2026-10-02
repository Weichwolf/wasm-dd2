#include "dd2_movie_platform.h"
#include "dd2_native.h"
#include <time.h>
#include <stdio.h>
#ifdef DD2_BROWSER
#include <emscripten.h>
EM_JS(int,movie_audio_start,(const int16_t* pcm,int frames,int rate,int channels),{
    /* WaveOut uses the movie's 22050Hz device. A 44100Hz DirectSound context
       would introduce a second source conversion absent in that reference. */
    var ac;
    try { ac=new AudioContext({sampleRate:rate}); } catch(e) { return -1; }
    if(ac.sampleRate!==rate){ ac.close();return -1; }
    Module._dd2movieAc=ac;
    var resume=function(){ if(ac.state==='suspended')ac.resume(); };
    window.addEventListener('click',resume,{once:true});
    window.addEventListener('keydown',resume,{once:true});
    var buffer=ac.createBuffer(channels,frames,rate);
    for(var ch=0;ch<channels;ch++) {
        var samples=buffer.getChannelData(ch);
        for(var i=0;i<frames;i++)samples[i]=HEAP16[(pcm>>1)+i*channels+ch]/32768;
    }
    var source=ac.createBufferSource();source.buffer=buffer;source.connect(ac.destination);
    Module._dd2movieDone=false;Module._dd2movieClock=ac;
    Module._dd2movieSource=source;
    source.onended=function(){Module._dd2movieDone=true;};
    source.start(ac.currentTime);
    return 0;
});
EM_JS(int,movie_audio_done,(),{return Module._dd2movieDone ? 1 : 0;});
EM_JS(void,movie_audio_stop,(),{
    if(Module._dd2movieSource) {
        Module._dd2movieSource.onended=null;
        try { Module._dd2movieSource.stop(); } catch(e) {}
        Module._dd2movieSource.disconnect();Module._dd2movieSource=null;
    }
    Module._dd2movieClock=null;Module._dd2movieDone=true;
    if(Module._dd2movieAc){Module._dd2movieAc.close().catch(function(){});Module._dd2movieAc=null;}
});
EM_JS(unsigned,movie_clock,(),{
    return Math.floor(Module._dd2movieClock ? Module._dd2movieClock.currentTime*1000 : performance.now())>>>0;
});
EM_JS(void,movie_present,(const uint32_t* argb),{
    var canvas=Module.canvas || document.getElementById('canvas');if(!canvas)return;
    if(canvas.width!==640 || canvas.height!==480) { canvas.width=640;canvas.height=480; }
    var ctx=canvas.getContext('2d');
    if(!Module._dd2movieImage)Module._dd2movieImage=ctx.createImageData(640,480);
    var data=Module._dd2movieImage.data;
    for(var i=0;i<640*480;i++) {
        var pixel=HEAPU32[(argb>>2)+i],p=i*4;
        data[p]=(pixel>>>16)&255;data[p+1]=(pixel>>>8)&255;data[p+2]=pixel&255;data[p+3]=255;
    }
    ctx.putImageData(Module._dd2movieImage,0,0);
});
#endif
unsigned dd2_movie_now_ms(void) {
#ifdef DD2_BROWSER
    return movie_clock();
#else
    struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);
    return (unsigned)((uint64_t)ts.tv_sec*1000u+ts.tv_nsec/1000000u);
#endif
}
void dd2_movie_wait(void) {
#ifdef DD2_BROWSER
    emscripten_sleep(1);
#elif defined(DD2_NATIVE_SDL)
    dd2_native_poll();
    { struct timespec delay={0,1000000};nanosleep(&delay,NULL); }
#else
    struct timespec delay={0,1000000};nanosleep(&delay,NULL);
#endif
}
void dd2_movie_present(const uint32_t* argb) {
#ifdef DD2_BROWSER
    movie_present(argb);
#elif defined(DD2_NATIVE_SDL)
    dd2_native_movie_present(argb);
#else
    (void)argb;
#endif
}
int dd2_movie_audio_start(const int16_t* pcm,size_t frames,unsigned rate,unsigned channels) {
#ifdef DD2_BROWSER
    return movie_audio_start(pcm,frames,rate,channels);
#elif defined(DD2_NATIVE_SDL)
    return dd2_native_movie_audio_start(pcm,frames,rate,channels);
#else
    (void)pcm;(void)frames;(void)rate;(void)channels;return 0;
#endif
}
int dd2_movie_audio_done(void) {
#ifdef DD2_BROWSER
    return movie_audio_done();
#elif defined(DD2_NATIVE_SDL)
    return dd2_native_movie_audio_done();
#else
    return 1;
#endif
}
void dd2_movie_audio_stop(void) {
#ifdef DD2_BROWSER
    movie_audio_stop();
#elif defined(DD2_NATIVE_SDL)
    dd2_native_movie_audio_stop();
#endif
}
