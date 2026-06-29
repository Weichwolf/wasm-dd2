
/* ---- lifted-code integration glue (compat layer) ----
   MEM is a zero-init global => (MEM+VA) == the literal VA, which is exactly where the C
   build keeps the image (memcpy'd to 0x400000; GLOBAL_BASE=0xA00000 above it). So lifted
   functions share the C address space directly. ESP points at a private scratch stack. */
void GTERT_lifted(void){
    static unsigned char stk[65536];
    *(unsigned int*)(CPU.r+0x10) = (unsigned int)(unsigned long)(stk + 65000); /* ESP @ regspace 0x10 */
    lifted_41397e();   /* GTERT: rotate (M*v) -> translate (+T) -> out @ 0x714100 */
}
