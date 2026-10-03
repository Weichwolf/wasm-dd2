/* matching-signature stubs for excluded-CRT + a few Win32 fns (replace emcc abort-stubs to run past CRT init) */
#include <time.h>
#include "dd2_native.h"
#include "ghidra_compat.h"
unsigned __doclose(void* p,int q){ return 0; }
char* __cvt(double v,int n,void* d,void* s){ if(d)*(int*)d=0; if(s)*(int*)s=0; return ""; }
void FUN_0042304c(byte* p){ }
void FUN_00456b30(uint a,uint b){ }
uint FUN_0045a174(void* h){ return 0; }
/* GetKeyState: only caller is the keyboard-rebind poller (FUN_0044fe64), which tests bit 0x8000
   for "key down". Back it with the live key-state array dd2_input.c maintains (0 stub = rebind
   never detected a key). Not on any bit-exact path (demo/race never call it). */
extern unsigned char dd2_keystate[256];
short GetKeyState(int k){ dd2_native_poll();return (k>=0 && k<256 && dd2_keystate[k]) ? (short)0x8000 : 0; }
/* GetTickCount: the engine paces itself to 25fps with this (frame limiter in Play_Game).
   Deterministic default: a fake +16ms/call ticker (proven for all bit-exact runs).
   DD2_REALTIME=1 (interactive browser/native play): real milliseconds, so the game runs at
   its faithful realtime speed instead of as-fast-as-possible. */
static unsigned dd2_virtual_ms = 0;
/* Read the platform clock without consuming an engine GetTickCount call. MCI
   and the audio sink share this timeline; deterministic inputs stay unchanged. */
unsigned dd2_platform_ms(void){
    static int mode=-1;
    if(mode<0){ extern char* getenv(const char*); mode = getenv("DD2_REALTIME") ? 1 : 0; }
    if(mode){
#ifdef DD2_BROWSER
        double emscripten_get_now(void);
        return (unsigned)emscripten_get_now();
#else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (unsigned)((uint64_t)ts.tv_sec*1000u + ts.tv_nsec/1000000u);
#endif
    }
    return dd2_virtual_ms;
}
/* Optional read-only reference clock input for headless original comparisons.
 * Each little-endian DWORD is one actual GetTickCount return captured from
 * the original. Replaying does not substitute engine states or frame data.
 * Strict extent checks reject changed control flow, truncation and leftovers.
 * Original hardware breakpoints change elapsed time; that observed timing is
 * an explicit input, not an undebugged/physical-clock acceptance claim. */
static FILE* dd2_tick_file;
static unsigned dd2_tick_calls;
static int dd2_tick_init,dd2_tick_failed;
static const char* dd2_tick_path;
static unsigned dd2_tick_level;
unsigned dd2_tick_replay_calls(void){return dd2_tick_calls;}
static void dd2_tick_close(void){
    if(!dd2_tick_file)return;
    if(!dd2_tick_failed && fgetc(dd2_tick_file)!=EOF){
        fprintf(stderr,"[clock-replay] unconsumed tick records\n");fclose(dd2_tick_file);dd2_tick_file=NULL;exit(1);
    }
    fclose(dd2_tick_file);dd2_tick_file=NULL;
    if(!dd2_tick_failed)fprintf(stderr,"[clock-replay] consumed=%u complete\n",dd2_tick_calls);
}
unsigned GetTickCount(void){
    if(!dd2_tick_init){
        dd2_tick_path=getenv("DD2_TICK_REPLAY");dd2_tick_init=1;
        if(dd2_tick_path){
            if(getenv("DD2_REALTIME")){fprintf(stderr,"[clock-replay] requires headless clock mode\n");exit(1);}
            {
                const char* level=getenv("DD2_TICK_LEVEL");
                if(level){char* end;unsigned long value=strtoul(level,&end,10);
                    if(!*level || *end || value<1 || value>10){fprintf(stderr,"[clock-replay] invalid target level\n");exit(1);}
                    dd2_tick_level=(unsigned)value;
                }
            }
        }
    }
    if(dd2_tick_path && !dd2_tick_file){
        if(dd2_tick_level && *(unsigned*)(uintptr_t)0x936ff4!=dd2_tick_level){
            if(*(unsigned*)(uintptr_t)0x7746c0==0 && *(unsigned*)(uintptr_t)0x46385c==1)
                fprintf(stderr,"[clock-replay] warmup level=%u\n",*(unsigned*)(uintptr_t)0x936ff4);
        }else{
            dd2_tick_file=fopen(dd2_tick_path,"rb");
            if(!dd2_tick_file){fprintf(stderr,"[clock-replay] cannot open clock input\n");exit(1);}
            /* Later original attract races inherit the DEMO MODE blink counter.
             * A fixed-level headless run starts from the boot image instead.
             * Match this explicitly observed initial state ONCE, then let the
             * original engine update it normally; no subsequent state replay. */
            {
                const char* flash=getenv("DD2_RACE_FLASH_INITIAL");
                if(flash){
                    char* end;unsigned long value=strtoul(flash,&end,10);
                    if(!*flash || *end || value>80){fprintf(stderr,"[clock-replay] invalid initial demo flash state\n");exit(1);}
                    if(getenv("DD2_RACE_FLASH_REQUIRE")){
                        if(*(unsigned*)(uintptr_t)0x4652a0!=(unsigned)value){fprintf(stderr,"[clock-replay] calculated initial demo flash differs\n");exit(1);}
                    }else *(unsigned*)(uintptr_t)0x4652a0=(unsigned)value;
                }
            }
            atexit(dd2_tick_close);
        }
    }
    if(dd2_tick_file){
        unsigned char bytes[4];size_t n=fread(bytes,1,4,dd2_tick_file);
        if(n!=4){
            dd2_tick_failed=1;fprintf(stderr,"[clock-replay] %s\n",n?"partial tick record":"clock input exhausted");exit(1);
        }
        dd2_virtual_ms=(unsigned)bytes[0]|((unsigned)bytes[1]<<8)|((unsigned)bytes[2]<<16)|((unsigned)bytes[3]<<24);
        /* Full-history entries are calculated from all preceding API inputs;
         * logging them never substitutes engine state from a reference. */
        if(getenv("DD2_RACE_FULL_HISTORY") &&
                *(unsigned*)(uintptr_t)0x7746c0==0 && *(unsigned*)(uintptr_t)0x46385c==1){
            extern unsigned dd2_random_replay_calls(void);
            fprintf(stderr,"[clock-replay] history-entry {\"level\":%u,\"cf\":%u,\"ticks\":0,\"countdown\":%d,\"frame_skip\":%d,\"quit\":%d,\"demo_flash\":%u,\"initial_clock\":%u,\"clock_offset\":%u,\"rng_calls\":%u}\n",
                *(unsigned*)(uintptr_t)0x936ff4,*(unsigned*)(uintptr_t)0x462ff0,
                *(int*)(uintptr_t)0x784298,*(int*)(uintptr_t)0x7746b8,*(int*)(uintptr_t)0x7746ac,
                *(unsigned*)(uintptr_t)0x4652a0,dd2_virtual_ms,dd2_tick_calls,dd2_random_replay_calls());
        }
        dd2_tick_calls++;return dd2_virtual_ms;
    }
    if(!getenv("DD2_REALTIME")) dd2_virtual_ms += 16;
    return dd2_platform_ms();
}

void* LockResource(void* h){ return h; /* dd2h passes raw in-memory WAV pointers (sound-bank blob
    + offset, FUN_00416688 -> DSLoadSoundBuffer), never real HRSRC handles: identity is the
    faithful Windows behavior for already-mapped memory */ }

/* The supported Watcom CRT's per-thread LCG, verified in dd2h.exe:
 * rand 0x456cbc, pre-update seed pointer EAX at 0x456cc6, returned EAX
 * at 0x456cde; srand 0x456cdf. The old 0x456afc/0x456b1f/MSVC comment
 * identified unrelated CRT code in this supported binary.
 * Every normal call retains the existing seed*0x41c64e6d+0x3039 algorithm.
 * Optional level-scoped verification initializes the private RNG from the
 * first original pre-seed unless REQUIRE_INITIAL is set. Full-history "all"
 * mode always requires the naturally calculated seed. Both modes COMPUTE and
 * check every post-seed/return, never substituting later recorded values. */
static unsigned _dd2_rand_seed = 1;
unsigned g_rand_calls = 0;
static FILE* dd2_random_file;
static unsigned dd2_random_calls,dd2_random_level;
static int dd2_random_init,dd2_random_failed,dd2_random_all;
static const char* dd2_random_path;
unsigned dd2_random_replay_calls(void){return dd2_random_calls;}
static void dd2_random_close(void){
    if(!dd2_random_file)return;
    if(!dd2_random_failed && fgetc(dd2_random_file)!=EOF){
        fprintf(stderr,"[random-reference] unconsumed random records\n");fclose(dd2_random_file);dd2_random_file=NULL;exit(1);
    }
    fclose(dd2_random_file);dd2_random_file=NULL;
    if(!dd2_random_failed)fprintf(stderr,"[random-reference] consumed=%u complete\n",dd2_random_calls);
}
static unsigned dd2_le_word(const unsigned char* bytes){
    return (unsigned)bytes[0]|((unsigned)bytes[1]<<8)|((unsigned)bytes[2]<<16)|((unsigned)bytes[3]<<24);
}
int rand(void){
    unsigned before,after,value;
    unsigned char record[12];int verify=0;
    if(!dd2_random_init){
        dd2_random_init=1;dd2_random_path=getenv("DD2_RANDOM_REFERENCE");
        if(dd2_random_path){
            const char* level=getenv("DD2_RANDOM_LEVEL");char* end=NULL;
            dd2_random_all=level && !strcmp(level,"all");
            if(!dd2_random_all){
                dd2_random_level=level?(unsigned)strtoul(level,&end,10):0;
                if(!level || !*level || *end || dd2_random_level<1 || dd2_random_level>10){
                    fprintf(stderr,"[random-reference] requires an exact level in 1..10 or all\n");exit(1);
                }
            }
        }
    }
    if(dd2_random_path && (dd2_random_all || *(unsigned*)(uintptr_t)0x936ff4==dd2_random_level)){
        size_t count;
        if(!dd2_random_file){
            dd2_random_file=fopen(dd2_random_path,"rb");
            if(!dd2_random_file){fprintf(stderr,"[random-reference] cannot open random reference\n");exit(1);}
            atexit(dd2_random_close);
        }
        count=fread(record,1,12,dd2_random_file);
        if(count!=12){
            dd2_random_failed=1;fprintf(stderr,"[random-reference] %s\n",count?"partial random record":"random reference exhausted");exit(1);
        }
        if(!dd2_random_calls){
            if(dd2_random_all || getenv("DD2_RANDOM_REQUIRE_INITIAL")){
                if(_dd2_rand_seed!=dd2_le_word(record)){
                    dd2_random_failed=1;fprintf(stderr,"[random-reference] calculated initial seed differs\n");exit(1);
                }
            }else _dd2_rand_seed=dd2_le_word(record);
        }
        verify=1;
    }
    before=_dd2_rand_seed;after=before*0x41c64e6dU+0x3039U;value=(after>>16)&0x7fffu;
    _dd2_rand_seed=after;g_rand_calls++;
    if(verify){
        if(before!=dd2_le_word(record) || after!=dd2_le_word(record+4) || value!=dd2_le_word(record+8)){
            dd2_random_failed=1;fprintf(stderr,"[random-reference] calculated random state/result differs at call %u\n",dd2_random_calls);exit(1);
        }
        dd2_random_calls++;
    }
    return (int)value;
}
void srand(unsigned seed){_dd2_rand_seed=seed;}
