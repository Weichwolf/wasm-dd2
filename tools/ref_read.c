/* Launch wine dd2h.exe as a descendant, PTRACE_SEIZE the dd2h.exe process, and read its
   8-bit _screenbuffer @0x700450 (320x240) at intervals. ptrace_scope=1 permits tracing a
   descendant, which also grants /proc/pid/mem read access. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

static pid_t find_dd2h(void){
  DIR* d = opendir("/proc"); struct dirent* e; pid_t found=0;
  while((e=readdir(d))){
    if(e->d_name[0]<'0'||e->d_name[0]>'9') continue;
    char pth[64]; snprintf(pth,sizeof pth,"/proc/%s/comm",e->d_name);
    FILE* f=fopen(pth,"r"); if(!f) continue;
    char nm[64]={0}; if(fgets(nm,sizeof nm,f)){ if(strncmp(nm,"dd2h.exe",8)==0) found=atoi(e->d_name); }
    fclose(f); if(found) break;
  }
  closedir(d); return found;
}

int main(void){
  pid_t child = fork();
  if(child==0){
    setsid();
    chdir("/home/cosmo/Git/wasm-dd2/DestructionDerby2");
    setenv("WINEPREFIX","/home/cosmo/.wine-dd2",1);
    setenv("WINEDEBUG","-all",1);
    setenv("DISPLAY",":99",1);
    int dn=open("/dev/null",O_WRONLY); dup2(dn,1); dup2(dn,2);
    execlp("wine","wine","dd2h.exe",(char*)0);
    _exit(127);
  }
  /* parent: wait for dd2h.exe to appear */
  pid_t pid=0; for(int i=0;i<60;i++){ pid=find_dd2h(); if(pid) break; sleep(1); }
  if(!pid){ fprintf(stderr,"dd2h.exe not found\n"); return 1; }
  fprintf(stderr,"dd2h pid=%d, seizing...\n",pid);
  if(ptrace(PTRACE_SEIZE,pid,0,0)<0){ perror("PTRACE_SEIZE"); return 2; }
  sleep(8); /* let it boot into the demo */
  char mempath[64]; snprintf(mempath,sizeof mempath,"/proc/%d/mem",pid);
  for(int s=0;s<6;s++){
    if(ptrace(PTRACE_INTERRUPT,pid,0,0)<0) perror("INTERRUPT");
    int st; waitpid(pid,&st,0);
    int fd=open(mempath,O_RDONLY);
    if(fd<0){ perror("open mem"); }
    else {
      unsigned char buf[320*240];
      ssize_t n=pread(fd,buf,sizeof buf,0x700450);
      close(fd);
      if(n>0){
        int nz=0; for(size_t i=0;i<(size_t)n;i++) if(buf[i]) nz++;
        char out[128]; snprintf(out,sizeof out,"/tmp/refdd2_%d.bin",s);
        int o=open(out,O_WRONLY|O_CREAT|O_TRUNC,0644); write(o,buf,n); close(o);
        fprintf(stderr,"snap %d: read %zd bytes, %d nonzero (%d%%) -> %s\n",s,n,nz,(int)(100*nz/n),out);
      } else perror("pread");
    }
    ptrace(PTRACE_CONT,pid,0,0);
    sleep(3);
  }
  ptrace(PTRACE_DETACH,pid,0,0);
  kill(child,SIGTERM);
  return 0;
}
