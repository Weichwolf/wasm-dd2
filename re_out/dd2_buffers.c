unsigned char* __clutspace;     /* assigned raw VA (0x6c0100) by the code; valid since image is at 0x400000 */
unsigned char* __texturespace;  /* assigned raw VA (0x490000) by the code */
/* draw_text_half was NOT unrecovered — it IS FUN_0041080d @0x41080d (the opaque textured-
   span blitter, real 475-byte body in dd2.c). The empty stub here was blanking the entire
   3D scene (all textured-poly rasterizers call &draw_text_half per span). Aliased to the
   real function via `#define draw_text_half FUN_0041080d` in dd2_symbols.h. */
