
/* ---- lifted-code integration glue (compat layer) ----
   MEM is a zero-init global => (MEM+VA) == the literal VA, which is exactly where the C
   build keeps the image (memcpy'd to 0x400000; GLOBAL_BASE 0xA00000 above it). So lifted
   functions share the C address space directly. ESP points at a private scratch stack. */

void GTERT_lifted(void){
    static unsigned char stk[65536];
    *(unsigned int*)(CPU.r+0x10) = (unsigned int)(unsigned long)(stk + 65000); /* ESP @ regspace 0x10 */
    lifted_41397e();   /* GTERT: rotate (M*v) -> translate (+T) -> out @ 0x714100 */
}

/* per-instruction trace stub (no-op unless lifted unit built with -DLIFT_TRACE_ON) */
int g_tf_tracing = 0;
void lift_trace(unsigned a){ (void)a; }

/* Track_Follow(int* param_1): bit-faithful lift of the track-spline walk + Mask_Point_In_Quad.
   The 3 scalar globals it READS (Ghidra split them into C-globals) are synced into their
   original x86 image slots so the lifted reads (MEM=0 -> image VAs) see the loaded values;
   the strip data they point to is shared automatically (same pointer value). cdecl: push
   param_1 then a fake retaddr so the prologue's [EBP+8] resolves to param_1. */
extern int _current_level, _strip_data, _strip_vertex;
void Track_Follow_lifted(int* param_1){
    static unsigned char stk[65536];
    *(int*)(unsigned long)0x8febf4 = _current_level;   /* sync read-only globals into image slots */
    *(int*)(unsigned long)0x744af8 = _strip_data;
    *(int*)(unsigned long)0x744af4 = _strip_vertex;
    unsigned int top = (unsigned int)(unsigned long)(stk + 60000);
    *(unsigned int*)(unsigned long)(top - 4) = (unsigned int)(unsigned long)param_1; /* [ESP+4]=param_1 */
    *(unsigned int*)(unsigned long)(top - 8) = 0;                                    /* [ESP]=retaddr */
    *(unsigned int*)(CPU.r+0x10) = top - 8;                                          /* ESP */
    lifted_426ec4();
}
