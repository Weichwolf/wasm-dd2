/* Differential harness foundation: run reference dd2h.exe under Wine, set a software breakpoint (INT3)
   at a function VA, and on hit dump a memory region to a file. Compare against our build's dump of the
   same VAs (shared memory model: image@0x400000, same globals). Usage: diffmem <bpVA> <dumpVA> <len> <out> */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <signal.h>
#include <string.h>
#include <dirent.h>
static long find_dd2h(void){
  DIR*d=opendir("/proc"); struct dirent*e; long pid=0;
  while((e=readdir(d))){ if(e->d_name[0]<'0'||e->d_name[0]>'9')continue;
    char p[64]; sprintf(p,"/proc/%s/comm",e->d_name); FILE*f=fopen(p,"r"); if(!f)continue;
    char c[64]={0}; fgets(c,64,f); fclose(f); if(!strncmp(c,"dd2h.exe",8)) pid=atol(e->d_name); }
  closedir(d); return pid; }
int main(int argc,char**argv){
  unsigned long bp=strtoul(argv[1],0,0), da=strtoul(argv[2],0,0); long len=atol(argv[3]); const char*out=argv[4];
  pid_t k=fork();
  if(k==0){ setsid(); execlp("sh","sh","-c","DISPLAY=:66 wine dd2h.exe >/tmp/dm.log 2>&1",(char*)0); _exit(127); }
  sleep(6);
  long pid=find_dd2h(); fprintf(stderr,"pid=%ld bp=0x%lx\n",pid,bp);
  if(!pid){ kill(-k,SIGKILL); return 1; }
  if(ptrace(PTRACE_ATTACH,pid,0,0)){ perror("attach"); kill(-k,SIGKILL); return 1; }
  waitpid(pid,0,0);
  char mp[64]; sprintf(mp,"/proc/%ld/mem",pid);
  /* set INT3 at bp via POKETEXT (bypasses read-only code pages) */
  FILE*m=fopen(mp,"rb");
  long word=ptrace(PTRACE_PEEKTEXT,pid,(void*)bp,0);
  long brk=(word & ~0xffL) | 0xCC;
  ptrace(PTRACE_POKETEXT,pid,(void*)bp,(void*)brk);
  ptrace(PTRACE_CONT,pid,0,0);
  int st, hit=0;
  for(;;){ waitpid(pid,&st,0);
    if(WIFEXITED(st)||WIFSIGNALED(st)){ fprintf(stderr,"exited before bp\n"); break; }
    struct user_regs_struct r; ptrace(PTRACE_GETREGS,pid,0,&r);
    if((r.rip&0xffffffffUL)==bp+1){ hit=1;
      char*buf=malloc(len); fseek(m,da,SEEK_SET); long g=fread(buf,1,len,m);
      FILE*o=fopen(out,"wb"); fwrite(buf,1,g,o); fclose(o);
      long nz=0,i; for(i=0;i<g;i++) if(buf[i])nz++;
      fprintf(stderr,"BP HIT, dumped %ld bytes @0x%lx (nz=%ld) -> %s\n",g,da,nz,out); break; }
    ptrace(PTRACE_CONT,pid,0,WIFSTOPPED(st)?WSTOPSIG(st):0); }
  fclose(m); ptrace(PTRACE_KILL,pid,0,0); kill(-k,SIGKILL); kill(k,SIGKILL);
  return hit?0:2; }
