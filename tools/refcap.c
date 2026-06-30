/* Launch wine dd2h.exe; poll current_frame@0x462ff0 via ptrace; at target frame dump
   _screenbuffer@0x700450 + report current_level@0x8febf4. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>
static long find_dd2h(void){
  DIR*d=opendir("/proc"); struct dirent*e; long pid=0;
  while((e=readdir(d))){ if(e->d_name[0]<'0'||e->d_name[0]>'9')continue;
    char p[64]; sprintf(p,"/proc/%s/comm",e->d_name); FILE*f=fopen(p,"r"); if(!f)continue;
    char c[64]={0}; fgets(c,64,f); fclose(f);
    if(strncmp(c,"dd2h.exe",8)==0) pid=atol(e->d_name); }
  closedir(d); return pid;
}
static int rd(long pid,unsigned long a,void*buf,long n){
  char path[64]; sprintf(path,"/proc/%ld/mem",pid); FILE*m=fopen(path,"rb"); if(!m)return -1;
  fseek(m,a,SEEK_SET); long g=fread(buf,1,n,m); fclose(m); return g;
}
int main(int argc,char**argv){
  long target=atol(argv[1]); const char*out=argv[2];
  pid_t k=fork();
  if(k==0){ setsid(); execlp("sh","sh","-c","DISPLAY=:96 wine dd2h.exe >/tmp/wr4.log 2>&1",(char*)0); _exit(127); }
  sleep(8);
  long pid=find_dd2h(); fprintf(stderr,"pid=%ld\n",pid);
  if(!pid){ kill(-k,SIGKILL); return 1; }
  int tries=0;
  for(;;){
    if(ptrace(PTRACE_ATTACH,pid,0,0)){ perror("attach"); break; }
    waitpid(pid,0,0);
    int frame=0,lvl=-1; rd(pid,0x462ff0,&frame,4); rd(pid,0x8febf4,&lvl,4);
    if(frame>=target){
      char*buf=malloc(320*240); rd(pid,0x700450,buf,320*240);
      FILE*o=fopen(out,"wb"); fwrite(buf,1,320*240,o); fclose(o);
      long nz=0,i; for(i=0;i<320*240;i++) if(buf[i])nz++;
      fprintf(stderr,"CAPTURED frame=%d level=%d nonzero=%ld -> %s\n",frame,lvl,nz,out);
      ptrace(PTRACE_DETACH,pid,0,0); break;
    }
    ptrace(PTRACE_DETACH,pid,0,0);
    usleep(20000);
    if(++tries>2000){ fprintf(stderr,"timeout frame=%d\n",frame); break; }
  }
  kill(-k,SIGKILL); kill(k,SIGKILL);
  return 0;
}
