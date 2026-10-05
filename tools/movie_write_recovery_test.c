/* Nonzero stereo caller PCM through real Wine WaveOut or the production
 * native movie transport. A separate preloader faults the first source write.
 * No sample, playback clock or completion flag is replaced. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
int main(void){
    WAVEFORMATEX format={WAVE_FORMAT_PCM,2,22050,88200,4,16,0};
    HWAVEOUT device;WAVEHDR header={0};MMTIME position={0};
    int16_t source[2205*2];unsigned i;DWORD started;
    for(i=0;i<2205;i++){source[2*i]=12345+i;source[2*i+1]=-23456+i;}
    if(waveOutOpen(&device,WAVE_MAPPER,&format,0,0,CALLBACK_NULL))return 2;
    header.lpData=(char*)source;header.dwBufferLength=sizeof(source);
    if(waveOutPrepareHeader(device,&header,sizeof(header)) ||
       waveOutWrite(device,&header,sizeof(header)))return 3;
    started=GetTickCount();
    while(!(header.dwFlags&WHDR_DONE)){
        if(GetTickCount()-started>2000)return 4;
        Sleep(1);
    }
    position.wType=TIME_SAMPLES;
    if(waveOutGetPosition(device,&position,sizeof(position)))return 5;
    if(waveOutReset(device) || waveOutUnprepareHeader(device,&header,sizeof(header)) ||
       waveOutClose(device))return 6;
    printf("{\"source_frames\":2205,\"done\":1,\"waveout_position\":%lu}\n",position.u.sample);
    return 0;
}
#else
#include <SDL2/SDL.h>
#include "dd2_native_movie_alsa.h"
int main(void){
    int16_t source[2205*2];unsigned i;int started,done=0;Uint32 began;
    if(SDL_Init(SDL_INIT_TIMER))return 2;
    for(i=0;i<2205;i++){source[2*i]=12345+i;source[2*i+1]=-23456+i;}
    started=movie_alsa_start(source,2205,22050);began=SDL_GetTicks();
    if(!started)while(!done){
        done=movie_alsa_done();
        if(SDL_GetTicks()-began>2000)return 3;
        SDL_Delay(1);
    }
    movie_alsa_stop();
    printf("{\"source_frames\":2205,\"start_result\":%d,\"done\":%d,\"closed\":%s}\n",
           started,done,!movie_alsa.device&&!movie_alsa.thread?"true":"false");
    SDL_Quit();return 0;
}
#endif
