/* Actual original outcome helper and bounded frontend instruction branch.
 * No original machine-code changes; isolated field inputs, not live menus. */
#define _GNU_SOURCE
#include "pe_fixture.h"
#ifdef DD2_ORIGINAL_CHAMP_MENU_FIELDS
#include <setjmp.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>
static sigjmp_buf branch_return;
static volatile unsigned branch_steps;
static void step_handler(int sig,siginfo_t *info,void *context) {
    ucontext_t *state=(ucontext_t *)context;
    unsigned pc=(unsigned)state->uc_mcontext.gregs[REG_EIP];
    (void)sig;(void)info;
    if(pc==0x450630) {
        state->uc_mcontext.gregs[REG_EFL]&=~0x100;
        siglongjmp(branch_return,1);
    }
    if(++branch_steps>32)_exit(2);
}
static void Cross(void) {
    branch_steps=0;
    if(sigsetjmp(branch_return,1)==0) {
        /* TF observes the unchanged x86 branch; stop before the next block.
         * siglongjmp restores the fixture's own register/stack context. */
        __asm__ volatile("pushfl\n\torl $0x100,(%%esp)\n\tpopfl\n\tjmp *%0"
                         : : "r"((uintptr_t)0x4505f3) : "memory");
        __builtin_unreachable();
    }
}
#define Result ((void (*)(void))(uintptr_t)0x4549c4)
#else
void DD2_Cross_Fields(void),FUN_004549c4(void);
#define Cross DD2_Cross_Fields
#define Result FUN_004549c4
#endif

static const unsigned starts[]={0x4673f4,0x93dee0,0x469680,0x46ae18,0x46aef8,0x46a924};
static const unsigned sizes[]={4,32,32,32,48,4};
static void initialize(unsigned type,unsigned league,unsigned rank,unsigned guard) {
    const unsigned char guards[]={0,0x19,0xa5,0xff};
    unsigned i;
    for(i=0;i<6;i++)memset((void *)(uintptr_t)starts[i],guards[guard],sizes[i]);
    put32(0x4673f4,type);put16(0x93def2,league);put16(0x93def4,rank);
}
static void snapshot(FILE *out) {
    unsigned i;
    for(i=0;i<6;i++)require(fwrite((void *)(uintptr_t)starts[i],1,sizes[i],out)==sizes[i],
                           "complete packed menu fields and neighbors");
}
int main(int argc,char **argv) {
    unsigned mode,type,league,rank,guard,description[5],count=0;
    FILE *out;
    require(argc==3,"original executable and output required");map_original(argv[1]);
#ifdef DD2_ORIGINAL_CHAMP_MENU_FIELDS
    {
        struct sigaction action;
        memset(&action,0,sizeof(action));action.sa_sigaction=step_handler;
        action.sa_flags=SA_SIGINFO;sigemptyset(&action.sa_mask);
        require(sigaction(SIGTRAP,&action,NULL)==0,"observe bounded original instructions");
    }
#endif
    out=fopen(argv[2],"wb");require(out!=NULL,"open menu field output");
    for(mode=0;mode<2;mode++)for(type=0;type<6;type++)
    for(league=0;league<(mode?4:1);league++)for(rank=0;rank<(mode?5:1);rank++)
    for(guard=0;guard<4;guard++) {
        initialize(type,league,rank,guard);
        description[0]=mode;description[1]=type;description[2]=league;
        description[3]=rank;description[4]=guard;
        require(fwrite(description,4,5,out)==5,"explicit menu field inputs");
        snapshot(out);if(mode)Result();else Cross();snapshot(out);count++;
    }
    require(count==504,"all packed menu field cases executed");
    require(fclose(out)==0,"close menu field output");return 0;
}
