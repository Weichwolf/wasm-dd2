/* Hardware-watchpoint tracer: run /tmp/dd2_probe as a traced child, set DR0 to watch 4-byte
   WRITES at a target address (argv[1], default 0x74d3cc), and log EIP+value of every store.
   ptrace adds no code to the target, so it preserves the layout-sensitive Heisenbug.
   Compile -m32 to match the 32-bit target. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>

int main(int argc, char** argv){
  unsigned long target = argc>1 ? strtoul(argv[1],0,0) : 0x74d3ccUL;
  pid_t child = fork();
  if(child==0){
    ptrace(PTRACE_TRACEME,0,0,0);
    chdir("/home/cosmo/Git/wasm-dd2/DestructionDerby2");
    setenv("DD2_FRAMEDIR","/tmp/fr",1);
    setenv("ASAN_OPTIONS","detect_leaks=0",1);
    execl("/tmp/dd2_probe","dd2_probe",(char*)0);
    _exit(127);
  }
  int st; waitpid(child,&st,0);                 /* initial stop after exec */
  long dr0 = offsetof(struct user, u_debugreg[0]);
  long dr6 = offsetof(struct user, u_debugreg[6]);
  long dr7 = offsetof(struct user, u_debugreg[7]);
  ptrace(PTRACE_POKEUSER, child, (void*)dr0, (void*)target);
  /* DR7: L0=1 (bit0), RW0=01 write (bits16-17), LEN0=11 4-byte (bits18-19) = 0xd0001 */
  ptrace(PTRACE_POKEUSER, child, (void*)dr7, (void*)0xd0001UL);
  ptrace(PTRACE_CONT, child, 0, 0);
  int hits=0;
  for(;;){
    waitpid(child,&st,0);
    if(WIFEXITED(st)){ fprintf(stderr,"child exited %d after %d hits\n",WEXITSTATUS(st),hits); break; }
    if(WIFSIGNALED(st)){ fprintf(stderr,"child KILLED by sig %d after %d hits (CRASH)\n",WTERMSIG(st),hits); break; }
    if(WIFSTOPPED(st)){
      int sig = WSTOPSIG(st);
      if(sig==SIGTRAP){
        long d6 = ptrace(PTRACE_PEEKUSER, child, (void*)dr6, 0);
        if(d6 & 1){
          struct user_regs_struct regs; ptrace(PTRACE_GETREGS, child, 0, &regs);
          long val = ptrace(PTRACE_PEEKDATA, child, (void*)target, 0);
          fprintf(stderr,"HIT#%d eip=0x%08x val=0x%08x\n", ++hits,
                  (unsigned)regs.eip, (unsigned)val);
          ptrace(PTRACE_POKEUSER, child, (void*)dr6, 0);   /* clear status */
        }
        ptrace(PTRACE_CONT, child, 0, 0);
      } else if(sig==SIGSEGV){
        struct user_regs_struct r; ptrace(PTRACE_GETREGS, child, 0, &r);
        fprintf(stderr,"SIGSEGV eip=0x%08x eax=0x%08x ebx=0x%08x ecx=0x%08x edx=0x%08x esi=0x%08x edi=0x%08x ebp=0x%08x esp=0x%08x\n",
          (unsigned)r.eip,(unsigned)r.eax,(unsigned)r.ebx,(unsigned)r.ecx,(unsigned)r.edx,(unsigned)r.esi,(unsigned)r.edi,(unsigned)r.ebp,(unsigned)r.esp);
        long L8 =ptrace(PTRACE_PEEKDATA,child,(void*)(unsigned long)(r.ebp-0x8),0);   /* iVar5(new) */
        long L1c=ptrace(PTRACE_PEEKDATA,child,(void*)(unsigned long)(r.ebp-0x1c),0);  /* uVar2(size) */
        long L14=ptrace(PTRACE_PEEKDATA,child,(void*)(unsigned long)(r.ebp-0x14),0);  /* uVar1 */
        long L10=ptrace(PTRACE_PEEKDATA,child,(void*)(unsigned long)(r.ebp-0x10),0);  /* iVar3=_prim_buf */
        unsigned iv5old=(unsigned)L8 - ((unsigned)L1c-(unsigned)L14);
        unsigned cell=iv5old*8 + (unsigned)L10;
        fprintf(stderr,"ALLOC iVar5new=0x%x uVar2=0x%x uVar1=0x%x _prim_buf=0x%x => iVar5old=0x%x CORRUPT_CELL=0x%x (size field +4 = 0x%x)\n",
          (unsigned)L8,(unsigned)L1c,(unsigned)L14,(unsigned)L10,iv5old,cell,cell+4);
        { long pb=ptrace(PTRACE_PEEKDATA,child,(void*)0x0820cc60UL,0); long ps=ptrace(PTRACE_PEEKDATA,child,(void*)0x463014UL,0); long fm=ptrace(PTRACE_PEEKDATA,child,(void*)0x71bf98UL,0); fprintf(stderr,"PRIMDUMP _prim_buf=0x%08x prim_buf_size=0x%08x _DAT_0071bf98=0x%08x\n",(unsigned)pb,(unsigned)ps,(unsigned)fm); }
        ptrace(PTRACE_KILL, child, 0, 0);   /* do NOT let the child's handler run / re-fault */
        fprintf(stderr,"(killed at clean fault)\n"); break;
      } else {
        ptrace(PTRACE_CONT, child, 0, sig);   /* forward other signals */
      }
    }
  }
  return 0;
}
