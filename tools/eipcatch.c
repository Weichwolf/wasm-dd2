/* Run a child under ptrace; on SIGSEGV print the faulting EIP (and fault addr). */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <signal.h>
#include <string.h>
int main(int argc,char**argv){
  pid_t c=fork();
  if(c==0){ ptrace(PTRACE_TRACEME,0,0,0);
    execv(argv[1],&argv[1]); _exit(127); }
  int st;
  for(;;){
    waitpid(c,&st,0);
    if(WIFEXITED(st)||WIFSIGNALED(st)){ fprintf(stderr,"child ended\n"); break; }
    if(WIFSTOPPED(st)){
      int sig=WSTOPSIG(st);
      if(sig==SIGSEGV||sig==SIGBUS||sig==SIGFPE){
        struct user_regs_struct r; ptrace(PTRACE_GETREGS,c,0,&r);
        siginfo_t si; ptrace(PTRACE_GETSIGINFO,c,0,&si);
        /* 32-bit child: eip is low 32 of rip */
        unsigned long long rip=r.rip;
        fprintf(stderr,"FAULT sig=%d EIP=0x%llx fault_addr=%p\n",sig,(rip&0xffffffffULL),si.si_addr);
        break;
      }
      ptrace(PTRACE_CONT,c,0,(void*)(long)(sig==SIGTRAP?0:sig));
    }
  }
  ptrace(PTRACE_KILL,c,0,0); kill(c,SIGKILL);
  return 0;
}
