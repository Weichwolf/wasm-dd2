/* matching-signature stubs for excluded-CRT + a few Win32 fns (replace emcc abort-stubs to run past CRT init) */
#include "ghidra_compat.h"
unsigned __doclose(void* p,int q){ return 0; }
char* __cvt(double v,int n,void* d,void* s){ if(d)*(int*)d=0; if(s)*(int*)s=0; return ""; }
void FUN_0042304c(byte* p){ }
void FUN_00456b30(uint a,uint b){ }
uint FUN_0045a174(void* h){ return 0; }
short GetKeyState(int k){ return 0; }
unsigned GetTickCount(void){ static unsigned t=0; t+=16; return t; }
void* LockResource(void* h){ return (void*)1; }
