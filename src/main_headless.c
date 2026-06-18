// Headless track viewer / race-runner (native, EGL). M2: render a loaded track to PNG.
#include "core/track.h"
#include "render/render.h"
#include "platform/headless.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 800
#define H 600

int main(int argc, char** argv) {
    const char* dat = argc>1 ? argv[1] : "assets/raw/LEV5/LEVEL.DAT";
    const char* out = argc>2 ? argv[2] : "out/track_view.png";

    Track t;
    if (!track_load(dat, &t)) return 1;
    if (!headless_init(W, H)) return 2;
    render_init();
    render_set_track(&t);

    vec3 ctr = v3scale(v3add(t.bbmin, t.bbmax), 0.5f);
    vec3 size = v3sub(t.bbmax, t.bbmin);
    float radius = v3len(v3(size.x, 0, size.z)) * 0.5f;
    if (radius < 1) radius = 1;

    // bird's-eye 3/4 view
    vec3 eye = v3(ctr.x, ctr.y + radius*1.3f, ctr.z - radius*1.1f);
    float view[16], proj[16];
    mat4_lookat(view, eye, ctr, v3(0,1,0));
    mat4_perspective(proj, 1.0f, (float)W/H, 1.0f, radius*8.0f);

    headless_begin();
    render_begin(0.45f, 0.6f, 0.8f);   // sky
    render_track(view, proj);

    // mark start line: a red box at centerline s=0
    vec3 p, tan; track_sample(&t, 0, &p, &tan);
    render_box(view, proj, v3(p.x, p.y+1.0f, p.z), v3(2.0f,1.0f,2.0f),
               atan2f(tan.x, tan.z), 0.9f, 0.1f, 0.1f);

    if (!headless_screenshot(out)) { fprintf(stderr,"screenshot failed\n"); return 3; }
    printf("rendered %s (%d ribs, len=%.0f) -> %s\n", dat, t.nribs, t.total_len, out);
    headless_shutdown();
    return 0;
}
