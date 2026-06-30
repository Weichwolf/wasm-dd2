/* ptrace hardware watchpoint: catch writes to a fixed address (DR0), print EIP of each writer. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <signal.h>
#include <stddef.h>
static long DBG(int n){ return offsetof(struct user, u_debugreg) + n*sizeof(long); }
int main(int argc,char**argv){
  unsigned long wa=strtoul(argv[1],0,0);
  pid_t c=fork();
  if(c==0){ ptrace(PTRACE_TRACEME,0,0,0); execv(argv[2],&argv[2]); _exit(127); }
  int st; waitpid(c,&st,0);                 /* initial exec stop */
  ptrace(PTRACE_POKEUSER,c,(void*)DBG(0),(void*)wa);       /* DR0 = addr */
  ptrace(PTRACE_POKEUSER,c,(void*)DBG(7),(void*)0xd0001UL); /* DR7: L0 + write + len4 */
  ptrace(PTRACE_CONT,c,0,0);
  int hits=0;
  for(;;){
    waitpid(c,&st,0);
    if(WIFEXITED(st)||WIFSIGNALED(st)){ fprintf(stderr,"child ended\n"); break; }
    int sig=WSTOPSIG(st);
    struct user_regs_struct r; ptrace(PTRACE_GETREGS,c,0,&r);
    unsigned long long rip=r.rip&0xffffffffULL;
    if(sig==SIGTRAP){
      unsigned long dr6=ptrace(PTRACE_PEEKUSER,c,(void*)DBG(6),0);
      if(dr6&0x1){ /* watchpoint 0 hit */
        unsigned long val; FILE*m; char p[64]; sprintf(p,"/proc/%d/mem",c);
        m=fopen(p,"rb"); fseek(m,wa,SEEK_SET); fread(&val,1,4,m); fclose(m);
        fprintf(stderr,"WRITE #%d to 0x%lx by EIP=0x%llx newval=0x%lx\n",++hits,wa,rip,val&0xffffffff);
        ptrace(PTRACE_POKEUSER,c,(void*)DBG(6),0); /* clear DR6 */
      }
      ptrace(PTRACE_CONT,c,0,0);
    } else if(sig==SIGSEGV){
      fprintf(stderr,"SEGV at EIP=0x%llx (watchpoint writes seen: %d)\n",rip,hits); break;
    } else ptrace(PTRACE_CONT,c,0,(void*)(long)sig);
  }
  ptrace(PTRACE_KILL,c,0,0); kill(c,SIGKILL); return 0;
}
