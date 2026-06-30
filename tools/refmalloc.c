/* Minimal: attach to dd2h.exe, INT3 at MPE_malloc@0x4235e4 via POKETEXT, log every size arg. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <string.h>
#include <dirent.h>
static long find(void){ DIR*d=opendir("/proc"); struct dirent*e; long pid=0;
  while((e=readdir(d))){ if(e->d_name[0]<'0'||e->d_name[0]>'9')continue;
    char p[64]; sprintf(p,"/proc/%s/comm",e->d_name); FILE*f=fopen(p,"r"); if(!f)continue;
    char c[64]={0}; fgets(c,64,f); fclose(f); if(!strncmp(c,"dd2h.exe",8)) pid=atol(e->d_name); }
  closedir(d); return pid; }
static long g_orig; static unsigned long BP=0x4235e4;
int main(int argc,char**argv){ int N=argc>1?atoi(argv[1]):20; long pid=find();
  if(!pid){ fprintf(stderr,"no pid\n"); return 1; }
  if(ptrace(PTRACE_ATTACH,pid,0,0)){ perror("attach"); return 1; } waitpid(pid,0,0);
  ptrace(PTRACE_SETOPTIONS,pid,0,(void*)0x00100000 /*EXITKILL*/);
  g_orig=ptrace(PTRACE_PEEKTEXT,pid,(void*)BP,0);
  fprintf(stderr,"orig byte@0x%lx = 0x%02lx\n", BP, g_orig&0xff);
  ptrace(PTRACE_POKETEXT,pid,(void*)BP,(void*)((g_orig&~0xffL)|0xCC));
  ptrace(PTRACE_CONT,pid,0,0);
  int n=0;
  for(;;){ int st; if(waitpid(pid,&st,0)<0)break;
    if(WIFEXITED(st)||WIFSIGNALED(st)){ fprintf(stderr,"child gone\n"); break; }
    if(!WIFSTOPPED(st)) continue;
    if(WSTOPSIG(st)==SIGTRAP){ struct user_regs_struct r; ptrace(PTRACE_GETREGS,pid,0,&r);
      if((r.rip&0xffffffffUL)==BP+1){
        unsigned long esp=r.rsp&0xffffffffUL, sz=ptrace(PTRACE_PEEKDATA,pid,(void*)(esp+4),0)&0xffffffffUL;
        printf("0x%lx ",sz); fflush(stdout);
        r.rip=BP; ptrace(PTRACE_SETREGS,pid,0,&r);
        ptrace(PTRACE_POKETEXT,pid,(void*)BP,(void*)g_orig);
        ptrace(PTRACE_SINGLESTEP,pid,0,0); waitpid(pid,0,0);
        ptrace(PTRACE_POKETEXT,pid,(void*)BP,(void*)((g_orig&~0xffL)|0xCC));
        if(++n>=N) break;
        ptrace(PTRACE_CONT,pid,0,0);
      } else ptrace(PTRACE_CONT,pid,0,0);
    } else ptrace(PTRACE_CONT,pid,0,WSTOPSIG(st)); }
  printf("\n"); ptrace(PTRACE_POKETEXT,pid,(void*)BP,(void*)g_orig);
  ptrace(PTRACE_DETACH,pid,0,0); return 0; }
