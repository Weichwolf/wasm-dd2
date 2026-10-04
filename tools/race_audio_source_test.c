/* Chronological mixer component replay, using the production source mixer.
 * Observed cursors/gains are assertions. No cursor or phase is fitted/reset. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef __EMSCRIPTEN__
#include <sys/mman.h>
#endif
#include "dd2h_stubs.c"
unsigned dd2_platform_ms(void){return 0;}
void FUN_0041345c(void){abort();}
static void require(int ok,const char *message){
    if(!ok){fprintf(stderr,"Race audio replay: %s\n",message);exit(1);}
}
static unsigned word(FILE *input){
    unsigned char bytes[4];
    require(fread(bytes,1,4,input)==4,"complete operation word");
    return (unsigned)bytes[0]|(unsigned)bytes[1]<<8|(unsigned)bytes[2]<<16|(unsigned)bytes[3]<<24;
}
int main(int argc,char **argv){
    FILE *input,*output;void *device;DSBuf *sources[256]={0};
    unsigned commands,i,mixes=0,blocks=0,frames=0;uint64_t rendered=0;
    float *mixed=NULL;
    require(argc==3,"trace input and output required");
#ifndef __EMSCRIPTEN__
    require(mmap((void*)0x400000,0x590000,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map game counter region");
#endif
    setenv("DD2_SOUND","1",1);setenv("DD2_SND_RATE","44100",1);
    unsetenv("DD2_REALTIME");unsetenv("DD2_AUDIO_FRAME_CLOCK");
    input=fopen(argv[1],"rb");output=fopen(argv[2],"wb");require(input&&output,"open replay files");
    {char magic[8];require(fread(magic,1,8,input)==8&&!memcmp(magic,"DD2MX01",8),"replay header");}
    commands=word(input);require(commands>0&&commands<100000,"bounded command count");
    require(DirectSoundCreate(0,&device,0)==0,"create production device");
    for(i=0;i<commands;i++){
        unsigned op=word(input),id;
        if(op==8){
            require(!mixed,"new primary before old block ended");
            frames=word(input);require(frames>0&&frames<=65536,"primary extent");
            mixed=calloc((size_t)frames*2,sizeof(float));require(mixed!=NULL,"allocate primary");blocks++;continue;
        }
        if(op==10){
            require(mixed!=NULL,"end without primary block");
            require(fwrite(mixed,8,frames,output)==frames,"complete primary output");
            rendered+=frames;free(mixed);mixed=NULL;continue;
        }
        id=word(input);require(id<256,"bounded source identity");
        if(op==1){
            unsigned bytes=word(input),flags=word(input),rate=word(input),channels=word(input),bits=word(input),align=word(input),length=word(input);
            unsigned char wave[18]={1,0};uint32_t descriptor[5]={20,0,0,0,0};
            require(!sources[id]&&bytes>0&&bytes<16000000&&length<=bytes,"source creation extent");
            memcpy(wave+2,&channels,2);memcpy(wave+4,&rate,4);
            {unsigned average=rate*align;memcpy(wave+8,&average,4);}
            memcpy(wave+12,&align,2);memcpy(wave+14,&bits,2);
            descriptor[1]=flags;descriptor[2]=bytes;descriptor[4]=(uint32_t)(uintptr_t)wave;
            require(ds_createbuffer(device,(int*)descriptor,&sources[id],0)==0,"create original-format source");
            require(fread(sources[id]->pcm,1,length,input)==length,"explicit original bank samples");
        }else if(op==2){
            unsigned parent=word(input);require(parent<256&&sources[parent]&&!sources[id],"duplicate lifetime");
            require(ds_dupbuffer(device,sources[parent],&sources[id])==0,"production duplicate");
        }else{
            DSBuf *source=sources[id];require(source!=NULL,"live source required");
            if(op==3){dsb_release(source);sources[id]=NULL;}
            else if(op==4){
                unsigned kind=word(input);int control_value=(int)word(input),result;
                if(kind==0)result=dsb_setpos(source,(unsigned)control_value);
                else if(kind==1)result=dsb_setpan(source,control_value);
                else if(kind==2)result=dsb_setvolume(source,control_value);
                else if(kind==3)result=dsb_setfreq(source,control_value);
                else{require(0,"unknown source control");result=-1;}
                require(result==0,"actual source control failed");
            }else if(op==5){require(dsb_play(source,0,0,(int)word(input))==0,"actual play flags");}
            else if(op==6){require(dsb_stop(source)==0,"actual stop");}
            else if(op==7){
                unsigned offset=word(input),length=word(input);
                require(offset<=source->size&&length<=source->size-offset,"CD ring write extent");
                require(fread(source->pcm+offset,1,length,input)==length,"actual CD sectors");
            }else if(op==9){
                unsigned cursor=word(input),looping=word(input),left=word(input),right=word(input);
                unsigned actual;require(mixed!=NULL&&source->playing,"actual active primary/source");
                dsb_getpos(source,&actual,NULL);
                if(actual!=cursor){fprintf(stderr,"command=%u source=%u cursor=%u expected=%u phase=%u frequency=%d\n",i,id,actual,cursor,source->phase,source->freq);require(0,"independently advanced source cursor differs");}
                require((unsigned)source->looping==looping,"actual loop flags differ");
                if(left!=0xffffffffu){
                    float gl=dd2_amp_float(source->vol-(source->pan>0?source->pan:0));
                    float gr=dd2_amp_float(source->vol+(source->pan<0?source->pan:0));
                    volatile float expected_left=(float)left/65535.0f,expected_right=(float)right/65535.0f;
                    if(gl!=expected_left||gr!=expected_right){fprintf(stderr,"command=%u source=%u volume=%d pan=%d gain=%g/%g expected=%g/%g\n",i,id,source->vol,source->pan,(double)gl,(double)gr,(double)expected_left,(double)expected_right);require(0,"actual source gain differs");}
                }else require(dd2_amp_float(source->vol-(source->pan>0?source->pan:0))==0.0f&&
                              dd2_amp_float(source->vol+(source->pan<0?source->pan:0))==0.0f,"unobserved audible source gain");
                ds_mix_buffer(source,mixed,NULL,NULL,(int)frames);mixes++;
            }else require(0,"unknown source operation");
        }
    }
    require(!mixed&&fgetc(input)==EOF,"complete replay command extent");
    for(i=0;i<256;i++)if(sources[i])dsb_release(sources[i]);
    ds_release(device);require(fclose(input)==0&&fclose(output)==0,"close replay files");
    printf("{\"blocks\":%u,\"source_mix_calls\":%u,\"frames\":%llu}\n",blocks,mixes,(unsigned long long)rendered);
    return 0;
}
