#include "render.h"
#include <GLES3/gl3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---- shaders (GLSL ES 1.00: valid in both GLES3/WebGL2 and GLES2) ----
static const char* VS =
    "attribute vec3 a_pos; attribute vec3 a_nrm; attribute float a_v;\n"
    "uniform mat4 u_mvp; varying vec3 v_nrm; varying float v_v;\n"
    "void main(){ v_nrm=a_nrm; v_v=a_v; gl_Position=u_mvp*vec4(a_pos,1.0); }\n";
static const char* FS =
    "precision mediump float; varying vec3 v_nrm; varying float v_v;\n"
    "uniform vec3 u_col; uniform float u_useramp;\n"
    "void main(){\n"
    "  vec3 N=normalize(v_nrm); vec3 L=normalize(vec3(0.4,0.85,0.3));\n"
    "  float d=0.35+0.65*max(dot(N,L),0.0);\n"
    "  vec3 base = u_col;\n"
    "  if(u_useramp>0.5){ float e=abs(v_v-0.5)*2.0;\n"  // road dark, edges lighter
    "    base = mix(vec3(0.20,0.20,0.22), vec3(0.45,0.42,0.38), e); }\n"
    "  gl_FragColor=vec4(base*d,1.0);\n"
    "}\n";

static GLuint compile(GLenum t, const char* s){
    GLuint sh=glCreateShader(t); glShaderSource(sh,1,&s,NULL); glCompileShader(sh);
    GLint ok=0; glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);
    if(!ok){ char log[512]; glGetShaderInfoLog(sh,512,NULL,log); fprintf(stderr,"shader: %s\n",log);} return sh;
}
static GLuint s_prog, s_track_vbo, s_box_vbo;
static GLint u_mvp, u_col, u_useramp;
static int s_track_verts;

void render_init(void){
    s_prog=glCreateProgram();
    GLuint v=compile(GL_VERTEX_SHADER,VS), f=compile(GL_FRAGMENT_SHADER,FS);
    glAttachShader(s_prog,v); glAttachShader(s_prog,f);
    glBindAttribLocation(s_prog,0,"a_pos"); glBindAttribLocation(s_prog,1,"a_nrm"); glBindAttribLocation(s_prog,2,"a_v");
    glLinkProgram(s_prog);
    GLint ok=0; glGetProgramiv(s_prog,GL_LINK_STATUS,&ok);
    if(!ok){char log[512];glGetProgramInfoLog(s_prog,512,NULL,log);fprintf(stderr,"link: %s\n",log);}
    u_mvp=glGetUniformLocation(s_prog,"u_mvp");
    u_col=glGetUniformLocation(s_prog,"u_col");
    u_useramp=glGetUniformLocation(s_prog,"u_useramp");
    glGenBuffers(1,&s_track_vbo);
    glGenBuffers(1,&s_box_vbo);
    glEnable(GL_DEPTH_TEST);
}

// 7 floats/vertex: pos(3) nrm(3) v(1)
static void push(float* buf, int* n, vec3 p, vec3 nrm, float vc){
    float* o=buf+(*n)*7; o[0]=p.x;o[1]=p.y;o[2]=p.z;o[3]=nrm.x;o[4]=nrm.y;o[5]=nrm.z;o[6]=vc; (*n)++;
}

void render_set_track(const Track* t){
    int maxv = t->nribs*(TRACK_K-1)*6;
    float* buf=(float*)malloc((size_t)maxv*7*sizeof(float));
    int n=0;
    for(int i=0;i<t->nribs;i++){
        int j=(i+1)%t->nribs;
        for(int k=0;k<TRACK_K-1;k++){
            vec3 a=t->rib[i][k], b=t->rib[i][k+1], c=t->rib[j][k+1], d=t->rib[j][k];
            vec3 nrm=v3norm(v3cross(v3sub(b,a),v3sub(d,a)));
            if(nrm.y<0) nrm=v3scale(nrm,-1.0f);
            float v0=(float)k/(TRACK_K-1), v1=(float)(k+1)/(TRACK_K-1);
            push(buf,&n,a,nrm,v0); push(buf,&n,b,nrm,v1); push(buf,&n,c,nrm,v1);
            push(buf,&n,a,nrm,v0); push(buf,&n,c,nrm,v1); push(buf,&n,d,nrm,v0);
        }
    }
    s_track_verts=n;
    glBindBuffer(GL_ARRAY_BUFFER,s_track_vbo);
    glBufferData(GL_ARRAY_BUFFER,(size_t)n*7*sizeof(float),buf,GL_STATIC_DRAW);
    free(buf);
}

void render_begin(float r,float g,float b){
    glEnable(GL_DEPTH_TEST);
    glClearColor(r,g,b,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

static void set_attribs(void){
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,7*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,7*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,7*sizeof(float),(void*)(6*sizeof(float)));
}

void render_track(const float* view,const float* proj){
    float mvp[16]; mat4_mul(mvp,proj,view);
    glUseProgram(s_prog);
    glUniformMatrix4fv(u_mvp,1,GL_FALSE,mvp);
    glUniform1f(u_useramp,1.0f);
    glUniform3f(u_col,0.3f,0.3f,0.3f);
    glBindBuffer(GL_ARRAY_BUFFER,s_track_vbo);
    set_attribs();
    glDrawArrays(GL_TRIANGLES,0,s_track_verts);
}

void render_box(const float* view,const float* proj,vec3 c,vec3 he,float yaw,float r,float g,float b){
    // 12 triangles. Build in local space, transform by yaw+translate via model matrix folded into mvp.
    float cs=cosf(yaw), sn=sinf(yaw);
    float verts[36*7]; int n=0;
    static const int faces[6][4]={{0,1,2,3},{5,4,7,6},{4,0,3,7},{1,5,6,2},{4,5,1,0},{3,2,6,7}};
    static const float corner[8][3]={{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
    vec3 wc[8];
    for(int i=0;i<8;i++){
        float lx=corner[i][0]*he.x, ly=corner[i][1]*he.y, lz=corner[i][2]*he.z;
        wc[i]=v3(c.x+cs*lx - sn*lz, c.y+ly, c.z+sn*lx + cs*lz);
    }
    for(int fi=0;fi<6;fi++){
        const int* q=faces[fi];
        vec3 a=wc[q[0]],bb=wc[q[1]],cc=wc[q[2]],dd=wc[q[3]];
        vec3 nrm=v3norm(v3cross(v3sub(bb,a),v3sub(dd,a)));
        vec3 tri[6]={a,bb,cc,a,cc,dd};
        for(int k=0;k<6;k++){ float* o=verts+n*7; o[0]=tri[k].x;o[1]=tri[k].y;o[2]=tri[k].z;
            o[3]=nrm.x;o[4]=nrm.y;o[5]=nrm.z;o[6]=0.0f; n++; }
    }
    float mvp[16]; mat4_mul(mvp,proj,view);
    glUseProgram(s_prog);
    glUniformMatrix4fv(u_mvp,1,GL_FALSE,mvp);
    glUniform1f(u_useramp,0.0f);
    glUniform3f(u_col,r,g,b);
    glBindBuffer(GL_ARRAY_BUFFER,s_box_vbo);
    glBufferData(GL_ARRAY_BUFFER,(size_t)n*7*sizeof(float),verts,GL_DYNAMIC_DRAW);
    set_attribs();
    glDrawArrays(GL_TRIANGLES,0,n);
}

void render_shutdown(void){}
