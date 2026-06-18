#include "../src/core/track.h"
#include <stdio.h>
int main(int argc, char** argv) {
    Track t;
    const char* p = argc>1 ? argv[1] : "assets/raw/LEV5/LEVEL.DAT";
    if (!track_load(p, &t)) return 1;
    printf("ribs=%d total_len=%.1f bbox x[%.1f,%.1f] y[%.1f,%.1f] z[%.1f,%.1f]\n",
        t.nribs, t.total_len, t.bbmin.x,t.bbmax.x, t.bbmin.y,t.bbmax.y, t.bbmin.z,t.bbmax.z);
    float wsum=0,wmin=1e30,wmax=0;
    for(int i=0;i<t.nribs;i++){wsum+=t.width[i]; if(t.width[i]<wmin)wmin=t.width[i]; if(t.width[i]>wmax)wmax=t.width[i];}
    printf("width avg=%.1f min=%.1f max=%.1f\n", wsum/t.nribs, wmin, wmax);
    vec3 pos,tan; track_sample(&t, t.total_len*0.25f, &pos,&tan);
    printf("sample@25%%: pos(%.1f,%.1f,%.1f) tan(%.2f,%.2f,%.2f)\n",pos.x,pos.y,pos.z,tan.x,tan.y,tan.z);
    return 0;
}
