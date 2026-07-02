/* __clutspace -> image slot 0x74c4d4 (dual-symbol fix: draw_text_half's clut base is [0x74c4d4] in asm @0x41002a) */
/* __texturespace -> image slot 0x74c4cc (dual-symbol fix: texture base is [0x74c4cc] in asm @0x41001f) */
/* draw_text_half was NOT unrecovered — it IS FUN_0041080d @0x41080d (the opaque textured-
   span blitter, real 475-byte body in dd2.c). The empty stub here was blanking the entire
   3D scene (all textured-poly rasterizers call &draw_text_half per span). Aliased to the
   real function via `#define draw_text_half FUN_0041080d` in dd2_symbols.h. */
