/* Original DD2 AVI transport: RIFF stream headers and packet extents, Cinepak
 * video and Microsoft ADPCM audio. Preserve source clocks separately: AVI's
 * compressed audio stream rate is not the PCM sample rate. */
#include "dd2_avi.h"
#include "dd2_cinepak.h"
#include "dd2_msadpcm.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
typedef struct { size_t offset,bytes; } Packet;
typedef struct { const uint8_t* id;size_t data,bytes,next; } Chunk;
struct DD2AVI {
    uint8_t* data;
    size_t bytes,audio_bytes,packet_index,pcm_index;
    unsigned video_stream,audio_stream,streams,declared_frames,audio_blocks;
    unsigned decoded,failed;
    DD2AVIInfo info;
    DD2MSADPCM format;
    Packet* packets;
    int16_t* pcm;
    DD2Cinepak* decoder;
};
static unsigned u16(const uint8_t* p) { return p[0]|(unsigned)p[1]<<8; }
static unsigned u32(const uint8_t* p) { return u16(p)|(unsigned)u16(p+2)<<16; }
static int tag(const uint8_t* p,const char* id) { return !memcmp(p,id,4); }
static int chunk(const DD2AVI* m,size_t at,size_t end,Chunk* c) {
    size_t available;
    if(at>end || end>m->bytes || end-at<8)return -1;
    c->id=m->data+at;c->data=at+8;c->bytes=u32(c->id+4);
    available=end-c->data;
    if(c->bytes>available || (c->bytes&1)>available-c->bytes)return -1;
    c->next=c->data+c->bytes+(c->bytes&1);
    if((tag(c->id,"LIST") || tag(c->id,"RIFF")) && c->bytes<4)return -1;
    return 0;
}
static int stream(DD2AVI* m,size_t at,size_t end) {
    const uint8_t *header=NULL,*format=NULL;
    size_t format_bytes=0;
    Chunk c;
    unsigned index=m->streams++;
    while(at<end) {
        if(chunk(m,at,end,&c))return -1;
        if(tag(c.id,"strh")) {
            if(header || c.bytes<56)return -1;
            header=m->data+c.data;
        } else if(tag(c.id,"strf")) {
            if(format)return -1;
            format=m->data+c.data;format_bytes=c.bytes;
        }
        at=c.next;
    }
    if(!header || !format || !u32(header+20) || !u32(header+24))return -1;
    if(tag(header,"vids")) {
        if(m->video_stream!=UINT_MAX || !tag(header+4,"cvid") || format_bytes<40
                || u32(format)<40 || u32(format)>format_bytes || !tag(format+16,"cvid")
                || u16(format+12)!=1 || u16(format+14)!=24)return -1;
        m->video_stream=index;
        m->info.width=u32(format+4);m->info.height=u32(format+8);
        if(!m->info.width || m->info.width>65535 || !m->info.height || m->info.height>65535)return -1;
        m->info.video_scale=u32(header+20);m->info.video_rate=u32(header+24);
        m->info.video_start=u32(header+28);m->info.frames=u32(header+32);
    } else if(tag(header,"auds")) {
        if(m->audio_stream!=UINT_MAX || dd2_msadpcm_format(&m->format,format,format_bytes))return -1;
        m->audio_stream=index;
        m->info.audio_initial_frames=u32(header+16);
        m->info.audio_scale=u32(header+20);m->info.audio_rate=u32(header+24);
        m->info.audio_start=u32(header+28);m->audio_blocks=u32(header+32);
        m->info.pcm_rate=m->format.rate;m->info.pcm_channels=m->format.channels;
    } else return -1;
    return 0;
}
static int headers(DD2AVI* m,size_t at,size_t end) {
    Chunk c;
    unsigned width=0,height=0,streams=0,seen=0;
    while(at<end) {
        if(chunk(m,at,end,&c))return -1;
        if(tag(c.id,"avih")) {
            const uint8_t* p=m->data+c.data;
            if(seen || c.bytes<56)return -1;
            seen=1;m->declared_frames=u32(p+16);streams=u32(p+24);
            width=u32(p+32);height=u32(p+36);
        } else if(tag(c.id,"LIST") && tag(m->data+c.data,"strl")) {
            if(stream(m,c.data+4,c.data+c.bytes))return -1;
        }
        at=c.next;
    }
    return !seen || streams!=m->streams || m->streams!=2
        || m->video_stream==UINT_MAX || m->audio_stream==UINT_MAX
        || !m->info.frames || m->info.frames!=m->declared_frames
        || width!=m->info.width || height!=m->info.height ? -1 : 0;
}
static int packets(DD2AVI* m,size_t at,size_t end,unsigned depth,int decode) {
    Chunk c;
    if(depth>16)return -1;
    while(at<end) {
        if(chunk(m,at,end,&c))return -1;
        if(tag(c.id,"LIST")) {
            if(!tag(m->data+c.data,"rec ") || packets(m,c.data+4,c.data+c.bytes,depth+1,decode))return -1;
        } else if(c.id[0]>='0' && c.id[0]<='9' && c.id[1]>='0' && c.id[1]<='9') {
            unsigned id=(c.id[0]-'0')*10+c.id[1]-'0';
            if(id==m->video_stream && c.id[2]=='d' && c.id[3]=='c') {
                if(m->packet_index>=m->info.frames)return -1;
                if(decode)m->packets[m->packet_index]=(Packet){c.data,c.bytes};
                else if(!c.bytes)m->info.empty_frames++;
                m->packet_index++;
            } else if(id==m->audio_stream && c.id[2]=='w' && c.id[3]=='b') {
                size_t written=0;
                if(c.bytes%m->format.block_bytes)return -1;
                if(decode) {
                    if(dd2_msadpcm_decode(&m->format,m->data+c.data,c.bytes,
                            m->pcm+m->pcm_index*m->format.channels,m->info.pcm_frames-m->pcm_index,&written))return -1;
                    m->pcm_index+=written;
                } else {
                    if(c.bytes>SIZE_MAX-m->audio_bytes)return -1;
                    m->audio_bytes+=c.bytes;
                }
            } else return -1;
        }
        at=c.next;
    }
    return 0;
}
DD2AVI* dd2_avi_open(const uint8_t* data,size_t bytes) {
    DD2AVI* m;
    Chunk c;
    size_t at=12,movi=0,movi_end=0,blocks;
    unsigned seen=0;
    if(!data || bytes<12 || !tag(data,"RIFF") || !tag(data+8,"AVI ")
            || (uint64_t)u32(data+4)+8!=bytes)return NULL;
    m=calloc(1,sizeof(*m));if(!m)return NULL;
    m->video_stream=m->audio_stream=UINT_MAX;
    m->data=malloc(bytes);if(!m->data)goto fail;
    memcpy(m->data,data,bytes);m->bytes=bytes;
    while(at<bytes) {
        if(chunk(m,at,bytes,&c))goto fail;
        if(tag(c.id,"LIST") && tag(m->data+c.data,"hdrl")) {
            if(seen || headers(m,c.data+4,c.data+c.bytes))goto fail;
            seen=1;
        } else if(tag(c.id,"LIST") && tag(m->data+c.data,"movi")) {
            if(movi)goto fail;
            movi=c.data+4;movi_end=c.data+c.bytes;
        }
        at=c.next;
    }
    if(!seen || !movi || packets(m,movi,movi_end,0,0) || m->packet_index!=m->info.frames)goto fail;
    blocks=m->audio_bytes/m->format.block_bytes;
    if(!blocks || blocks!=m->audio_blocks || blocks>SIZE_MAX/m->format.samples_per_block)goto fail;
    m->info.pcm_frames=blocks*m->format.samples_per_block;
    if(m->info.frames>SIZE_MAX/sizeof(Packet)
            || m->info.pcm_frames>SIZE_MAX/(sizeof(int16_t)*m->format.channels))goto fail;
    m->packets=calloc(m->info.frames,sizeof(Packet));
    m->pcm=malloc(m->info.pcm_frames*sizeof(int16_t)*m->format.channels);
    if(!m->packets || !m->pcm)goto fail;
    m->packet_index=0;
    if(packets(m,movi,movi_end,0,1) || m->pcm_index!=m->info.pcm_frames || !m->packets[0].bytes)goto fail;
    m->decoder=dd2_cinepak_create(m->info.width,m->info.height);
    if(!m->decoder)goto fail;
    return m;
fail:
    dd2_avi_close(m);return NULL;
}
void dd2_avi_close(DD2AVI* m) {
    if(!m)return;
    dd2_cinepak_destroy(m->decoder);free(m->data);free(m->packets);free(m->pcm);free(m);
}
const DD2AVIInfo* dd2_avi_info(const DD2AVI* m) { return m ? &m->info : NULL; }
const int16_t* dd2_avi_pcm(const DD2AVI* m) { return m ? m->pcm : NULL; }
const uint8_t* dd2_avi_frame(DD2AVI* m,unsigned frame) {
    if(!m || m->failed || frame>=m->info.frames)return NULL;
    if(m->decoded>frame+1) {
        dd2_cinepak_destroy(m->decoder);
        m->decoder=dd2_cinepak_create(m->info.width,m->info.height);
        if(!m->decoder) { m->failed=1;return NULL; }
        m->decoded=0;
    }
    while(m->decoded<=frame) {
        Packet* packet=m->packets+m->decoded;
        if(dd2_cinepak_decode(m->decoder,m->data+packet->offset,packet->bytes)) {
            m->failed=1;return NULL;
        }
        m->decoded++;
    }
    return dd2_cinepak_pixels(m->decoder);
}
