/* Extracted engine Play_Movie against an independent WinMM call contract.
 * Exercise success/notify pumping and every early-error cleanup path. This
 * mock does not decode movies or claim an actual Windows/Wine video run. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define __cdecl
typedef uint32_t uint,undefined4,MCIERROR,MCIDEVICEID;
typedef unsigned char undefined1;
typedef uintptr_t DWORD_PTR;
static uint DAT_0046047c=0x3456789a,Movie_Playing;
static int in_640;
static char *s_avivideo_0046c74c="avivideo";
static unsigned failure,calls,pumps,closed,next;
static void require(int ok,const char* reason) {
    if(!ok){fprintf(stderr,"movie params: %s\n",reason);exit(1);}
}
static MCIERROR mciSendCommandA(MCIDEVICEID cd,uint command,uint flags,DWORD_PTR pointer) {
    uint *params=(uint*)pointer;
    unsigned sequence[]={0x803,0x841,0x842,0x806};
    calls++;
    require(params!=NULL,"missing parameter block");
    if(command==0x804) {
        require(next>0 && cd==7 && flags==2 && params[0]==0,"close successful open with WAIT");
        closed++;return failure==command ? 4321 : 0;
    }
    if(!in_640 && next==2)next++;
    require(next<4 && command==sequence[next++],"MCI command order");
    if(command==0x803) {
        require(cd==0 && flags==0x2202,"open type/element with WAIT");
        require(params[2]==(uint)(uintptr_t)s_avivideo_0046c74c,"open contiguous device type");
        require(params[3]==0x12345678,"open contiguous movie filename");
        if(failure!=command)params[1]=7;
    } else {
        require(cd==7,"use MCI_OPEN's returned device ID");
        if(command==0x841) {
            require(flags==0x10002 && params[1]==DAT_0046047c,"set output window");
        } else if(command==0x842) {
            require(flags==0x50002 && params[1]==0 && params[2]==48 && params[3]==640 && params[4]==384,
                    "set contiguous destination rectangle");
        } else {
            require(flags==1 && params[0]==DAT_0046047c && params[1]==0 && params[2]==0 && params[3]==0,
                    "play with original notify window and zero fields");
        }
    }
    return failure==command ? 4321 : 0;
}
static void FUN_00413054(void) {
    require(Movie_Playing==1 && next==4 && closed==0,"pump only after successful play, before close");
    require(++pumps<=3,"completion must exit movie loop");
    if(pumps==3)Movie_Playing=0;
}
#include "movie-function.c"
int main(void) {
    static const struct { const char *label;unsigned wide,error,wanted_calls,wanted_pumps,wanted_close; } cases[]={
        {"wide",1,0,5,3,1},{"narrow",0,0,4,3,1},
        {"open-error",1,0x803,1,0,0},{"window-error",1,0x841,3,0,1},
        {"rectangle-error",1,0x842,4,0,1},{"play-error",1,0x806,5,0,1},
        {"narrow-window-error",0,0x841,3,0,1},{"narrow-play-error",0,0x806,4,0,1},
        {"close-error",1,0x804,5,3,1}
    };
    unsigned i;
    putchar('[');
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);i++) {
        in_640=cases[i].wide;failure=cases[i].error;
        calls=pumps=closed=next=Movie_Playing=0;
        Play_Movie(0x12345678);
        require(calls==cases[i].wanted_calls && pumps==cases[i].wanted_pumps && closed==cases[i].wanted_close,
                "return/cleanup path differs");
        require(Movie_Playing==0,"movie completion state");
        printf("%s{\"label\":\"%s\",\"calls\":%u,\"pumps\":%u,\"closed\":%u}",
               i?",":"",cases[i].label,calls,pumps,closed);
    }
    puts("]");return 0;
}
