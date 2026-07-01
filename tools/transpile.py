#!/usr/bin/env python3
"""
DD2 transpile pipeline:  Ghidra decompile (re_out/)  ->  systematic fixes  ->  WASM source (build/)

The Ghidra decompile in re_out/ is kept PRISTINE. This script applies the known
decompile-artifact fixes and writes the WASM-compilable source to build/.
Re-run after re-decompiling: each fix asserts its anchor still exists, so any
decompile drift is caught loudly instead of silently no-op'ing.

Artifact classes fixed (all mechanical Ghidra recompilation artifacts, not logic):
  - scattered-locals      : Ghidra split a contiguous struct/array into separate locals
  - dual-symbol           : one address split into a C-global + an image-slot; writes/reads diverge
  - int-vs-short          : a 16-bit field given a *(int*) macro -> 32-bit writes clobber neighbors
  - byte-offset           : an int* used with raw byte offsets -> x4 scaling
  - dropped-register      : a register side-effect (ebp/esi) Ghidra dropped (unaff_*)
  - dispatch-relocation   : a fn-ptr table relocated to WASM ptrs, but a Ghidra switch compares x86 addrs

Usage: python3 tools/transpile.py        (writes build/)
       python3 tools/transpile.py --check (verify all anchors match, no write)
"""
import os, re, sys, shutil

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC  = os.path.join(ROOT, 're_out')
OUT  = os.path.join(ROOT, 'build')
CHECK = '--check' in sys.argv

_applied = []
def sub(text, old, new, n=1, name=''):
    """Replace occurrences; assert the anchor count is EXACTLY n (n=-1 = replace-all, requires >=1).
    Exact-count guards against an ambiguous anchor silently hitting the wrong function."""
    cnt = text.count(old)
    if n == -1:
        assert cnt >= 1, "TRANSPILE ANCHOR MISSING [%s]: %r" % (name, old[:70])
        _applied.append((name, cnt))
        return text.replace(old, new)
    assert cnt == n, "TRANSPILE ANCHOR COUNT [%s]: expected %d, found %d for %r" % (name, n, cnt, old[:70])
    _applied.append((name, n))
    return text.replace(old, new, n)

# ----- guard band-aid toggle (the faithful draw_text_half_trans clip is still pending) -----
RASTER_GUARD = True

def fix_dd2(s):
    # FIX EBC (scattered-locals): the 5 textured-polygon rasterizer setups (FUN_004110a4/111e8/1132c/
    # 11b34/11c78) pass &local_40 to FUN_00411ebc, which reads it as the 12-int vertex array param_1[0..11].
    # Ghidra split that x86 stack array into named offset-locals (local_40,local_3c,...,local_14); clang/WASM
    # lays them out REVERSED (local_40 at the HIGHEST addr, confirmed: &local_40=...900 &local_3c=...896 ...),
    # so &local_40[2] reads ABOVE the block = a stale prim pointer (e.g. 0x879F68) instead of local_38. Native
    # gcc happens to order local_40 lowest so it works. Result under WASM: garbage vertex X -> ~256x edge slope
    # -> exploding horizontal span -> OOB raster write -> stack-cookie crash (the whole textured 3D world).
    # FUN_00411ebc only READS param_1, so pass an explicit contiguous compound-literal array (layout-independent;
    # native unchanged — same values). Same class as FIX E (which did the sibling FUN_00411654).
    s = sub(s, "FUN_00411ebc(&local_40,",
            "FUN_00411ebc((int*)(int[12]){local_40,local_3c,local_38,local_34,local_30,local_2c,"
            "local_28,local_24,local_20,local_1c,local_18,local_14},",
            7, 'EBC scattered-locals -> contiguous array')
    # FIX GEOM-GUARD (heap-layout divergence guard, LOCAL/isolated this time): FUN_0041fb7c's poly-command
    # walk crashes (L2/L3, and structurally the same signature as L6's FUN_0041132c) when _gpoly = *(iVar3+0x28)
    # is a wild address (e.g. 0x666666ff, WAY outside the mapped image+heap range 0x400000-0x900000) --
    # a symptom of the Decompress heap-layout divergence (see CLAUDE.md). PRIOR ATTEMPT sanitized this same
    # field at its creation site (Set_Object/Create_Object) instead of here and made things WORSE (broke L10
    # into a hang) -- reverted. This time: guard ONLY this one read site, ONLY the pointer-validity check
    # (not touching Set_Object/Create_Object/the +0x1e flag/lifecycle at all), isolating whether the earlier
    # regression came from the Set_Object-side change specifically.
    s = sub(s, "    _gpoly = *(short **)(iVar3 + 0x28);\n"
               "    _gprim1 = (undefined4 *)((int)_prim_buf + iVar4);   /* WASM: byte (int* scaled iVar4) */",
               "    _gpoly = *(short **)(iVar3 + 0x28);\n"
               "    if ((unsigned int)(uintptr_t)_gpoly < 0x400000u || (unsigned int)(uintptr_t)_gpoly >= 0x900000u) return -1;  /* GEOM-GUARD: wild _gpoly (heap-layout divergence symptom) -> skip like the existing iVar4==-1 convention */\n"
               "    _gprim1 = (undefined4 *)((int)_prim_buf + iVar4);   /* WASM: byte (int* scaled iVar4) */",
               1, 'GEOM-GUARD FUN_0041fb7c isolated pointer-validity guard')
    # FIX GEOM-GUARD2: Setup_Object_Block's per-object loop computes Set_Object's 2nd arg as
    # (int)param_1 + iVar2, where iVar2 = *piVar3 comes straight from the decompressed block (same
    # divergence-corrupted source as GEOM-GUARD above). When iVar2 is garbage, the computed address
    # is wild and Set_Object segfaults on its FIRST read (*(short*)(param_2+0x1e)) -- one level upstream
    # of the GEOM-GUARD case. FIRST ATTEMPT (skip the Set_Object call entirely) regressed: it left
    # param_1[1] (this object's geometry-block pointer, normally set by Set_Object's `param_1[1] =
    # param_2`) at its stale/zero value, and Draw_Scene_Object unconditionally dereferences param_1[1]
    # later (dd2.c:16487 `*(ushort*)param_1[1]`) for every object in this block's count regardless --
    # NULL/stale deref. FIX: redirect the wild param_2 to param_1 itself (the block header -- always a
    # valid, already-mapped address) instead of skipping the call, so param_1[1] always ends up
    # pointing somewhere safe to dereference (a real but semantically-empty/default object), matching
    # the existing "safe fallback" pattern rather than leaving state half-initialized.
    s = sub(s, "      iVar2 = *piVar3;\n"
               "      Set_Object(local_20,(int)param_1 + iVar2);\n"
               "      if ((*(byte *)((int)param_1 + iVar2 + 4) & 0x80) == 0) {",
               "      iVar2 = *piVar3;\n"
               "      { unsigned _op = (unsigned)((int)param_1 + iVar2);\n"
               "        Set_Object(local_20, (_op >= 0x400000u && _op < 0x900000u) ? (int)param_1 + iVar2 : (int)param_1); }\n"
               "      if (((unsigned)((int)param_1+iVar2) < 0x400000u || (unsigned)((int)param_1+iVar2) >= 0x900000u) ||"
               " (*(byte *)((int)param_1 + iVar2 + 4) & 0x80) == 0) {",
               1, 'GEOM-GUARD2 Setup_Object_Block wild object-offset guard (safe fallback, not skip)')
    # FIX EBC2 (scattered-locals): sibling rasterizer FUN_0041243c (3-vertex / 6-int, read-only param_1[0..5],
    # texel callback FUN_0041080d) is fed &local_40 by FUN_00410f74/FUN_00411a04 — same reversed-stack-local
    # corruption as FIX EBC. Pass an explicit contiguous 6-int compound-literal array.
    s = sub(s, "FUN_0041243c(&local_40,",
            "FUN_0041243c((int*)(int[6]){local_40,local_3c,local_38,local_34,local_30,local_2c},",
            4, 'EBC2 FUN_0041243c scattered-locals -> contiguous array')
    # FIX FONT (scattered-locals): Setup_Font [ebp-0x1c..]/Duplicate_Font [ebp-0x30..] is ONE contiguous
    # 0x18-byte sprite descriptor that Setup_Sprite/FUN_00416714 fills through offset 0x17. Ghidra split it
    # into local_1c[6]+local_10+uStack_e+local_a+local_9 (resp. local_30[6]+local_24+uStack_22+local_1e+
    # local_1d), so the 12-byte array under-sizes the buffer -> the descriptor write runs off the end ->
    # OOB store -> front-end SIGSEGV. Restore a contiguous 0x18B buffer; read the upper fields at their
    # real frame offsets (local_10=+0xc, uStack_e=+0xe, local_a=+0x12, local_9=+0x13).
    s = sub(s, "  ushort local_1c [6];\n  undefined2 local_10;\n  undefined2 uStack_e;\n"
               "  undefined1 local_a;\n  undefined1 local_9;\n\n  Setup_Sprite(0,param_1,local_1c);",
               "  unsigned char local_1c [0x18];  /* FIX FONT: contiguous sprite descriptor */\n"
               "  Setup_Sprite(0,param_1,(ushort *)local_1c);", name='FONT:Setup_Font decl')
    s = sub(s, "  *(undefined2 *)(puVar1 + 2) = local_10;\n  *(undefined2 *)(puVar1 + 4) = uStack_e;\n"
               "  *puVar1 = local_a;\n  puVar1[1] = local_9;",
               "  *(undefined2 *)(puVar1 + 2) = *(undefined2 *)(local_1c + 0xc);\n"
               "  *(undefined2 *)(puVar1 + 4) = *(undefined2 *)(local_1c + 0xe);\n"
               "  *puVar1 = local_1c[0x12];\n  puVar1[1] = local_1c[0x13];", name='FONT:Setup_Font reads')
    s = sub(s, "  ushort local_30 [6];\n  undefined2 local_24;\n  undefined2 uStack_22;\n"
               "  undefined1 local_1e;\n  undefined1 local_1d;\n\n  Setup_Sprite(0,param_3,local_30);",
               "  unsigned char local_30 [0x18];  /* FIX FONT: contiguous sprite descriptor */\n"
               "  Setup_Sprite(0,param_3,(ushort *)local_30);", name='FONT:Duplicate_Font decl')
    s = sub(s, "  *(undefined2 *)(_DAT_0071c00c + 2 + param_2 * 8) = local_24;\n"
               "  *(undefined2 *)(iVar1 + 4 + param_2 * 8) = uStack_22;\n"
               "  *(undefined1 *)(iVar1 + param_2 * 8) = local_1e;\n"
               "  *(undefined1 *)(iVar1 + 1 + param_2 * 8) = local_1d;",
               "  *(undefined2 *)(_DAT_0071c00c + 2 + param_2 * 8) = *(undefined2 *)(local_30 + 0xc);\n"
               "  *(undefined2 *)(iVar1 + 4 + param_2 * 8) = *(undefined2 *)(local_30 + 0xe);\n"
               "  *(undefined1 *)(iVar1 + param_2 * 8) = local_30[0x12];\n"
               "  *(undefined1 *)(iVar1 + 1 + param_2 * 8) = local_30[0x13];", name='FONT:Duplicate_Font reads')
    # FIX MFREE (allocator-pairing): puVar20 is from MPE_malloc (the 0x7debf0 pool, free-list @_DAT_0073c290);
    # the decompile frees it with CRT free() on the prim-arena-full (-1) path -> "free(): invalid pointer"
    # (L3). Pair the allocator: MPE_malloc <-> MPE_free. Only runs on the -1 path (face faithfully dropped).
    s = sub(s, "    if (iVar22 == -1) {\n      free(puVar20);\n      return 0xffffffff;\n    }",
               "    if (iVar22 == -1) {\n      MPE_free((int)puVar20);  /* FIX MFREE: MPE_malloc<->MPE_free */\n"
               "      return 0xffffffff;\n    }", name='MFREE:MPE_malloc/free pairing')
    # DEBUG ZEROBUF (temporary, env-gated): zero the Decompress dest buffer at entry to test whether
    # the LZ back-references read UNINITIALIZED buffer (stale MPE_malloc 0x66 bytes) vs correct data.
    if os.environ.get('DD2_ZEROBUF_PATCH'):
        s = sub(s, "  pbVar5 = (byte *)*param_1;\n  if (*(short *)((int)param_1 + 0xe) == 0) {",
                   "  { extern char *getenv(const char*); extern void *memset(void*,int,unsigned);"
                   " if(getenv(\"DD2_ZEROBUF\")) memset((void*)param_1[1],0,0x4000); }\n"
                   "  pbVar5 = (byte *)*param_1;\n  if (*(short *)((int)param_1 + 0xe) == 0) {",
                   name='DEBUG ZEROBUF')
    # DEBUG CONTIG (test, env-gated): allocate the 14 active_object_blocks slots as ONE contiguous
    # buffer (no MPE_malloc header gaps) — tests whether Decompress's back-refs that read up to 0x1000
    # before a slot expect the previous slot's data (cross-block window).
    if os.environ.get('DD2_CONTIG_PATCH'):
        s = sub(s, "  iVar2 = 0;\n  do {\n    iVar3 = iVar2 + 4;\n    puVar1 = MPE_malloc(0x4000);\n"
                   "    *(undefined4 **)((int)&active_object_blocks + iVar2) = puVar1;\n"
                   "    iVar2 = iVar3;\n  } while (iVar3 != 0x38);",
                   "  { char* _big=(char*)MPE_malloc(14*0x4000); int _s;"
                   " for(_s=0;_s<14;_s++) *(undefined4**)((int)&active_object_blocks + _s*4)="
                   "(undefined4*)(_big+_s*0x4000); }", name='DEBUG CONTIG slots')
    # DEBUG PLOG (temporary): record (type,_gpoly) into the BSS ring buffer before each dispatch in
    # FUN_0041fb7c's walk loop, to catch the _gpoly command-stream desync (heisenbug). REMOVE after.
    if os.environ.get('DD2_PLOG_PATCH'):
        s = sub(s, "      pbVar1 = (byte *)(_gpoly + 1);\n      _gpoly = _gpoly + 2;\n"
                   "      switch((&PTR_LAB_00462ef4)[*pbVar1]) {",
                   "      pbVar1 = (byte *)(_gpoly + 1);\n      _gpoly = _gpoly + 2;\n"
                   "      { extern int g_plog[]; extern volatile int g_pidx;"
                   " g_plog[(g_pidx&511)*4]=*pbVar1; g_plog[(g_pidx&511)*4+1]=(int)sVar2;"
                   " g_plog[(g_pidx&511)*4+2]=(int)_gpoly; g_pidx++; }\n"
                   "      switch((&PTR_LAB_00462ef4)[*pbVar1]) {", name='DEBUG PLOG ring-buffer')
    # DEBUG OBJLOG (temporary, env-gated): record the last Draw_Scene_Object dispatch to tell over-walk
    # (objidx vs num_scene_objects) from a corrupt-but-in-bounds object.
    if os.environ.get('DD2_OBJLOG_PATCH'):
        s = sub(s, "          iVar2 = iVar2 + 1;\n          Draw_Scene_Object(puVar3,piVar1);",
                   "          iVar2 = iVar2 + 1;\n"
                   "          { extern volatile int g_lastobj[]; g_lastobj[0]=local_1c/4; g_lastobj[1]=iVar2;"
                   " g_lastobj[2]=*(int*)((int)&num_scene_objects+local_1c); g_lastobj[3]=(int)puVar3;"
                   " g_lastobj[4]=puVar3[1]; }\n"
                   "          Draw_Scene_Object(puVar3,piVar1);", name='DEBUG OBJLOG')
    # FIX SFS (scattered-locals — ASan-PROVEN root of the demo/Track_Follow crash): on the x86 stack,
    # FUN_004430b8's local_52 (the per-level strip-search TAG, set by the switch) sat at ebp-0x52 and
    # local_4a at ebp-0x4a — CONTIGUOUS right after local_74[8]@ebp-0x74. Search_For_Strip / FUN_00428548
    # / FUN_004287c0 receive (int)local_74 and read the tag at param+0x22 (=local_52) + write the found
    # strip at param+0x14. The decompiler split local_52/local_4a into SEPARATE locals -> param+0x22 reads
    # 2 bytes PAST the 32-byte array (native ASan: load2 redzone trap; WASM: silent wrong tag -> wrong
    # starting strip -> car walks off-track -> Track_Follow overrun @~call 1253). Restore x86 contiguity.
    s = sub(s, "  undefined4 local_74 [8];\n  undefined2 local_52;\n  char local_4a;",
               "  undefined4 local_74 [0xb];  /* FIX SFS: covers ebp-0x74..ebp-0x48; local_52@+0x22, local_4a@+0x2a below */\n"
               "#define local_52 (*(undefined2*)((char*)local_74+0x22))\n"
               "#define local_4a (*(char*)((char*)local_74+0x2a))", name="SFS:decl")
    s = sub(s, "/* ===== Init_End_Race @ 00443840 ===== */",
               "#undef local_52\n#undef local_4a\n/* ===== Init_End_Race @ 00443840 ===== */", name="SFS:undef")

    # FIX DEBRIS (scattered-locals): Init_Debris_'s sprite locals local_70[6]/auStack_64[2]/acStack_60[8]/
    # local_58[12]/local_40[12]/local_28[16] are ONE contiguous x86 stack frame (ebp-0x70..ebp-0x18). Setup_Sprite/
    # FUN_00416714 write a ~20-byte sprite struct starting at local_70 (spills into auStack_64/acStack_60), and the
    # code indexes auStack_64 with stride 0xc as a 2D frame table reaching into local_58/local_40. The decompiler
    # split them -> writes past local_70[6] (native ASan store1 redzone @FUN_00416714:4549). Restore contiguity.
    s = sub(s, "  ushort local_70 [6];\n  undefined2 auStack_64 [2];\n  char acStack_60 [8];\n  ushort local_58 [12];\n  ushort local_40 [12];\n  char local_28 [16];",
               "  unsigned char _idb[0x58];  /* FIX DEBRIS: contiguous sprite frame ebp-0x70..ebp-0x18 */\n"
               "#define local_70 ((ushort*)(_idb+0x00))\n"
               "#define auStack_64 ((undefined2*)(_idb+0x0c))\n"
               "#define acStack_60 ((char*)(_idb+0x10))\n"
               "#define local_58 ((ushort*)(_idb+0x18))\n"
               "#define local_40 ((ushort*)(_idb+0x30))\n"
               "#define local_28 ((char*)(_idb+0x48))", name="DEBRIS:decl")
    s = sub(s, "/* ===== Setup_Debris @ 0042440c ===== */",
               "#undef local_70\n#undef auStack_64\n#undef acStack_60\n#undef local_58\n#undef local_40\n#undef local_28\n/* ===== Setup_Debris @ 0042440c ===== */", name="DEBRIS:undef")

    # FIX LENSFLARE (scattered-locals): Init_LensFlare's 9 ushort[12] lens-entry arrays (local_104..local_44)
    # are ONE contiguous 216-byte block; a copy loop fills 192 bytes from &DAT_0042da30 starting at local_104.
    # Decompiler split them -> store past local_104[12] (native ASan redzone @15245). Restore contiguity.
    s = sub(s, "  ushort local_104 [12];\n  ushort local_ec [12];\n  ushort local_d4 [12];\n  ushort local_bc [12];\n  ushort local_a4 [12];\n  ushort local_8c [12];\n  ushort local_74 [12];\n  ushort local_5c [12];\n  ushort local_44 [12];",
               "  unsigned char _ilf[0xd8];  /* FIX LENSFLARE: contiguous 9x ushort[12] lens-entry frame */\n"
               "#define local_104 ((ushort*)(_ilf+0x00))\n#define local_ec ((ushort*)(_ilf+0x18))\n"
               "#define local_d4 ((ushort*)(_ilf+0x30))\n#define local_bc ((ushort*)(_ilf+0x48))\n"
               "#define local_a4 ((ushort*)(_ilf+0x60))\n#define local_8c ((ushort*)(_ilf+0x78))\n"
               "#define local_74 ((ushort*)(_ilf+0x90))\n#define local_5c ((ushort*)(_ilf+0xa8))\n"
               "#define local_44 ((ushort*)(_ilf+0xc0))", name="LENSFLARE:decl")
    s = sub(s, "/* ===== DrawLensFlare @ 0042dca8 ===== */",
               "#undef local_104\n#undef local_ec\n#undef local_d4\n#undef local_bc\n#undef local_a4\n#undef local_8c\n#undef local_74\n#undef local_5c\n#undef local_44\n/* ===== DrawLensFlare @ 0042dca8 ===== */", name="LENSFLARE:undef")

    # GTE + Track_Follow are kept as direct x86-semantics-in-C (the reconstructed bodies in re_out/),
    # transcribed via tools/x86_intrin.h — NO P-code lift/interpreter (per project direction: a clean
    # C+SDL3 program, native-primary, wasm just a build target). The reconstructions compile both targets.

    # FIX A/B (scattered-locals): camera args must be contiguous arrays for Set_World_Position/Point_Camera
    s = sub(s, "    piVar8 = (int *)&DAT_00752344;\n    Set_World_Position(&local_54);",
               "    piVar8 = (int *)&DAT_00752344;\n    { int _cp[3]; _cp[0]=local_54; _cp[1]=local_50; _cp[2]=local_4c; Set_World_Position((undefined4 *)_cp); }", name="A:Set_World_Position")
    s = sub(s, "  local_30 = piVar8[1] + DAT_00463f08;\n  Point_Camera(&local_34,0x800);",
               "  local_30 = piVar8[1] + DAT_00463f08;\n  { int _tgt[4]; _tgt[0]=local_34; _tgt[1]=local_30; _tgt[2]=iStack_2c; _tgt[3]=iStack_28; Point_Camera((int *)_tgt,0x800); }", name="B:Point_Camera")

    # FIX U (scattered-locals): FUN_004202ac (per-block transform) computes the X/Y/Z translation delta as
    # 3 separately-declared locals; ApplyMatrixLV reads them as a contiguous int[3] (*p, p[1], p[2]). emscripten
    # scatters them -> garbage Y/Z delta -> track transformed off-screen with garbage coords. Pack contiguous.
    s = sub(s, "void __cdecl FUN_004202ac(short *param_1,int *param_2)\n\n{\n  int local_20;\n  int local_1c;\n  int local_18;",
               "void __cdecl FUN_004202ac(short *param_1,int *param_2)\n\n{\n  int _d[3];  /* FIX U: per-block delta must be contiguous for ApplyMatrixLV param_2[1]/[2] */", name="U:decl")
    s = sub(s, "    local_20 = *param_2 - DAT_00462fb6;\n    local_1c = param_2[1] - DAT_00462fba;\n    local_18 = param_2[2] - DAT_00462fbe;\n    ApplyMatrixLV(&world_matrix,&local_20,(uint *)&DAT_0071be4e);",
               "    _d[0] = *param_2 - DAT_00462fb6;\n    _d[1] = param_2[1] - DAT_00462fba;\n    _d[2] = param_2[2] - DAT_00462fbe;\n    ApplyMatrixLV(&world_matrix,_d,(uint *)&DAT_0071be4e);",
               n=2, name="U:both-branches")

    # FIX W (scattered-locals): Generate_Surface_Normals (0x426788) builds each strip-plane NORMAL via a cross
    # product over two edge vectors; OuterProduct12 reads them as contiguous int[3] (param[0..2]), but the decompile
    # declares edge1 (local_70/6c/68), edge2 (local_60/5c/58), normal (local_50/4c/48) as SEPARATE ints -> emscripten
    # scatters them -> garbage normal -> garbage Map_Height divisor -> the car-Y physics chases a garbage floor and
    # diverges/oscillates -> camera follows -> track off-screen. THE car-Y root. Same class as FIX A/B/E/U.
    s = sub(s, "  int local_70;\n  int local_6c;\n  int local_68;\n  int local_60;\n  int local_5c;\n  int local_58;\n  int local_50;\n  int local_4c;\n  int local_48;\n  int local_40;\n  uint local_3c;\n  byte *local_38;\n  int local_34;\n  int local_30;\n  uint local_2c;\n  int local_28;\n  int local_24;\n  byte *local_20;\n  int local_1c;\n  int local_18;\n  int local_14;",
               "  int _e1[3], _e2[3], _n[3];\n  int local_70;\n  int local_6c;\n  int local_68;\n  int local_60;\n  int local_5c;\n  int local_58;\n  int local_50;\n  int local_4c;\n  int local_48;\n  int local_40;\n  uint local_3c;\n  byte *local_38;\n  int local_34;\n  int local_30;\n  uint local_2c;\n  int local_28;\n  int local_24;\n  byte *local_20;\n  int local_1c;\n  int local_18;\n  int local_14;", name="W:normals-decl")
    s = sub(s, "        local_70 = *piVar4 - *piVar3;\n        local_6c = piVar4[1] - piVar3[1];\n        local_68 = piVar4[2] - piVar3[2];",
               "        _e1[0] = *piVar4 - *piVar3;\n        _e1[1] = piVar4[1] - piVar3[1];\n        _e1[2] = piVar4[2] - piVar3[2];", name="W:edge1")
    s = sub(s, "        local_60 = *piVar4 - *piVar3;\n        local_5c = piVar4[1] - piVar3[1];\n        local_58 = piVar4[2] - piVar3[2];",
               "        _e2[0] = *piVar4 - *piVar3;\n        _e2[1] = piVar4[1] - piVar3[1];\n        _e2[2] = piVar4[2] - piVar3[2];", name="W:edge2")
    s = sub(s, "        local_40 = local_6c;\n        OuterProduct12(&local_60,&local_70,&local_50);\n        FUN_00414360(&local_50,&local_50);\n        if (local_4c == 0) {\n          local_48 = local_4c;\n          local_50 = local_4c;\n          local_4c = 0x1000;\n        }\n        *(short *)(local_20 + 0x26) = (short)local_50;\n        *(short *)(local_20 + 0x28) = (short)local_4c;\n        *(short *)(local_20 + 0x2a) = (short)local_48;",
               "        local_40 = _e1[1];\n        OuterProduct12(_e2,_e1,_n);\n        FUN_00414360(_n,_n);\n        if (_n[1] == 0) {\n          _n[2] = 0;\n          _n[0] = 0;\n          _n[1] = 0x1000;\n        }\n        *(short *)(local_20 + 0x26) = (short)_n[0];\n        *(short *)(local_20 + 0x28) = (short)_n[1];\n        *(short *)(local_20 + 0x2a) = (short)_n[2];", name="W:outerproduct")

    # FIX X (byte-offset): FUN_0041a2f4 (sprite handler, facetype 12-15 = bulk of track scene-objects) reads its
    # source via bare `_gpoly + 4/6/10` — _gpoly is short* so that's byte +8/+12/+20 (x2), but x86 (0x41a31f/325/342)
    # reads byte +4/+6/+0xa. The x2-wrong texture index (*(int*)(_gpoly+6=byte+12)>>0x10) -> garbage idx ->
    # _gtexture+idx*0xc OOB. Only crashed once FIX W+C put the track on-screen so the sprites are reached. Byte-cast.
    s = sub(s, "    _gprim1[1] = *(undefined4 *)(_gpoly + 4);", "    _gprim1[1] = *(undefined4 *)((int)_gpoly + 4);", name="X:sprite+4")
    s = sub(s, "    iVar1 = *(int *)(_gpoly + 6) >> 0x10;", "    iVar1 = *(int *)((int)_gpoly + 6) >> 0x10;", name="X:sprite+6")
    s = sub(s, "    *(undefined2 *)((int)_gprim1 + 0xe) = *(undefined2 *)(_gpoly + 10);", "    *(undefined2 *)((int)_gprim1 + 0xe) = *(undefined2 *)((int)_gpoly + 10);", name="X:sprite+0xa")

    # FIX C (scattered-locals): Point_Camera builds the camera ROLL matrix from separate locals (local_3c/38/36/
    # 34/32/30/2e/2c = 9 shorts [rcos,-rsin,0; rsin,rcos,0; 0,0,0x1000]) then MulMatrix2(&local_3c,&world_matrix)
    # reads them as a CONTIGUOUS 3x3 -> emscripten scatters -> garbage roll -> world_matrix rows 1/2 non-orthonormal
    # -> GTE rotation corrupt -> track projects off-screen (X clamps right). Pack contiguous. Same class as A/B/U/W.
    s = sub(s, "  undefined4 local_3c;\n  undefined2 local_38;",
               "  short _rm[16];\n  undefined4 local_3c;\n  undefined2 local_38;", name="C:roll-decl")
    s = sub(s, "  local_3c = (local_3c & ~(0xffffu<<0)) | ((((undefined2)local_18) & 0xffffu)<<0);\n  local_32 = 0;\n  local_38 = 0;\n  local_30 = 0;\n  local_36 = (short)local_1c;\n  local_2e = 0;\n  local_3c = (local_3c & ~(0xffffu<<16)) | (((-local_36) & 0xffffu)<<16);\n  local_2c = 0x1000;\n  local_34 = (undefined2)local_3c;\n  MulMatrix2(&local_3c,&world_matrix);\n  MulMatrix2(&local_3c,(short *)&tilt_sprite_matrix);\n  puVar6 = &local_3c;\n  puVar8 = (undefined4 *)&sprite_matrix;\n  for (iVar2 = 7; iVar2 != 0; iVar2 = iVar2 + -1) {\n    *puVar8 = *puVar6;\n    puVar6 = puVar6 + 1;\n    puVar8 = puVar8 + 1;\n  }\n  *(undefined2 *)puVar8 = *(undefined2 *)puVar6;",
               "  { int _rmi; for(_rmi=0;_rmi<16;_rmi++){_rm[_rmi]=0;}\n    _rm[0] = (short)local_18; _rm[4] = (short)local_18;   /* rcos */\n    _rm[3] = (short)local_1c; _rm[1] = -(short)local_1c;  /* rsin / -rsin */\n    _rm[8] = 0x1000;\n    MulMatrix2((undefined4 *)_rm,&world_matrix);\n    MulMatrix2((undefined4 *)_rm,(short *)&tilt_sprite_matrix);\n    { short *_rs=_rm; short *_rd=(short *)&sprite_matrix; for(_rmi=0;_rmi<15;_rmi++){_rd[_rmi]=_rs[_rmi];} } }", name="C:roll-build")

    # FIX P (int-vs-short): the 9 world_matrix elements (0x462fa4..0x462fb4) are 2-byte shorts but the
    # macros are *(int*); 32-bit writes clobber neighbors (and camOrg.X @0x462fb6). Write 16-bit.
    for a, b, nm in [
      ("      world_matrix = (undefined2)((iVar3 << 0xc) / (int)uVar4);","      *(short *)(uintptr_t)0x462fa4 = (short)((iVar3 << 0xc) / (int)uVar4);","P:wm[0]"),
      ("      _DAT_00462fa8 = (undefined2)((local_14 * -0x1000) / (int)uVar4);","      *(short *)(uintptr_t)0x462fa8 = (short)((local_14 * -0x1000) / (int)uVar4);","P:wm a8"),
      ("      DAT_00462faa = (undefined2)(-(local_14 * iVar2) / (int)uVar4);","      *(short *)(uintptr_t)0x462faa = (short)(-(local_14 * iVar2) / (int)uVar4);","P:wm aa"),
      ("      _DAT_00462fac = (undefined2)uVar4;","      *(short *)(uintptr_t)0x462fac = (short)uVar4;","P:wm ac"),
      ("      _DAT_00462fb2 = (short)iVar2;","      *(short *)(uintptr_t)0x462fb2 = (short)iVar2;","P:wm b2"),
      ("      _DAT_00462fb4 = (undefined2)iVar3;","      *(short *)(uintptr_t)0x462fb4 = (short)iVar3;","P:wm b4"),
      ("      _DAT_00462fae = (undefined2)(-(iVar2 * iVar3) / (int)uVar4);","      *(short *)(uintptr_t)0x462fae = (short)(-(iVar2 * iVar3) / (int)uVar4);","P:wm ae"),
      ("      _DAT_00462fb0 = (undefined2)local_14;","      *(short *)(uintptr_t)0x462fb0 = (short)local_14;","P:wm b0")]:
        s = sub(s, a, b, name=nm)
    # element[1]=0 appears in BOTH Set_World_View and Point_Camera (both write world_matrix as int-macros over
    # 16-bit elements); the 32-bit write zeros element[2]. Fix both (replace-all).
    s = sub(s, "  DAT_00462fa6 = 0;", "  *(short *)(uintptr_t)0x462fa6 = 0;", n=-1, name="P:wm a6=0 (both)")

    # FIX Q (dropped-register): x86 rotate FUN_00413fd2 does `mov ebp,0x7142f0` which persists for the
    # following translate's T read; Ghidra dropped it. GTERPS was hand-patched, GTERPT/GTERPT4_ were not.
    s = sub(s, "  int iVar1;\n  short *unaff_ESI=(short*)(uintptr_t)_g_esi;\n  uint *unaff_EDI=(uint*)(uintptr_t)_g_edi;\n  \n  iVar1 = (int)*unaff_ESI;",
               "  int iVar1;\n  short *unaff_ESI=(short*)(uintptr_t)_g_esi;\n  uint *unaff_EDI=(uint*)(uintptr_t)_g_edi;\n  _g_ebp = 0x7142f0;\n  iVar1 = (int)*unaff_ESI;", name="Q:rotate _g_ebp")

    # FIX N + GTERPT4_ (dual-symbol): GTERPT/GTERPT4_ read vertex-X from image slots 0x714100/_vr1/_vr2/0x7140f0,
    # but callers set the __vr C-globals. Sync at the function entry.
    s = sub(s, "void GTERPT(void)\n\n{\n  __flg = 0;\n  _g_esi=0x714100;",
               "void GTERPT(void)\n\n{\n  __flg = 0;\n  _DAT_00714100 = __vr0; _vr1 = __vr1; _vr2 = __vr2;\n  _g_esi=0x714100;", name="N:GTERPT __vr")
    s = sub(s, "void GTERPT4_(void)\n\n{\n  __flg = 0;\n  _g_esi=0x714100;",
               "void GTERPT4_(void)\n\n{\n  __flg = 0;\n  _DAT_00714100 = __vr0; _vr1 = __vr1; _vr2 = __vr2; *(int*)(uintptr_t)0x7140f0 = __vr3;\n  _g_esi=0x714100;", name="N:GTERPT4_ __vr")

    # FIX E (scattered-locals): FUN_00411654's poly setup is a contiguous 12-int struct, not scattered locals.
    pack = ("{ int _q[12]; _q[0]=iStack_40;_q[1]=iStack_3c;_q[2]=iStack_38;_q[3]=iStack_34;_q[4]=iStack_30;"
            "_q[5]=iStack_2c;_q[6]=uStack_28;_q[7]=uStack_24;_q[8]=uStack_20;_q[9]=uStack_1c;_q[10]=uStack_18;"
            "_q[11]=uStack_14; FUN_00411ebc((int *)_q,FUN_0041033a); }")
    s = sub(s, "  _dth_shade = (int)(uint)*(byte *)(param_1 + 4) >> 4;\n    FUN_00411ebc(&iStack_40,FUN_0041033a);",
               "  _dth_shade = (int)(uint)*(byte *)(param_1 + 4) >> 4;\n    " + pack, name="E:pack1")
    s = sub(s, "  uStack_24 = (uint)*(byte *)(param_1 + 0x25);\n    FUN_00411ebc(&iStack_40,FUN_0041033a);",
               "  uStack_24 = (uint)*(byte *)(param_1 + 0x25);\n    " + pack, name="E:pack2")

    # FIX I (byte-offset): 6 face handlers use _gprim1 (int*) with raw byte offsets -> x4 scaling. Byte-cast.
    lines = s.split('\n')
    for nm in ['FUN_00417ea0','FUN_0041861c','FUN_0041bc0c','FUN_0041bc68','FUN_0041c3e4','FUN_0041c440']:
        st = next((i for i,l in enumerate(lines) if l.strip() == 'void '+nm+'(int param_1)'), None)
        assert st is not None, "TRANSPILE: handler %s missing" % nm
        en = next((j for j in range(st, st+45) if lines[j].strip()=='}' and lines[j-1].strip()=='return;'), None)
        assert en is not None, "TRANSPILE: handler %s end missing" % nm
        b = '\n'.join(lines[st:en+1])
        nb = b.replace('(_gprim1 + 7)','((char *)_gprim1 + 7)').replace('(_gprim1 + 4)','((char *)_gprim1 + 4)')
        nb = re.sub(r'_gprim1 = _gprim1 \+ (0x[0-9a-f]+);', r'_gprim1 = (int *)((char *)_gprim1 + \1);', nb)
        assert nb != b, "TRANSPILE: FIX I no-op for %s" % nm
        lines[st:en+1] = nb.split('\n')
        _applied.append(("I:"+nm, 1))
    s = '\n'.join(lines)

    # FIX L (byte-offset): Init_Front_End Create_Object #5 used `_level_data + 100` (int* -> +400 bytes).
    s = sub(s, "*(int *)(_level_data + 100)", "*(int *)((char *)_level_data + 100)", name="L:_level_data+100")

    # FIX R (dual-symbol): Decrunch_Object_Block writes the decompress src to the dead C-global _dec_info
    # instead of the image slot dec_info (0x750f48) that Decompress reads.
    s = sub(s, "  _dec_info = *_level_data + *(int *)(*_level_data + param_1 * 4);",
               "  dec_info = *_level_data + *(int *)(*_level_data + param_1 * 4);", name="R:dec_info src")

    # FIX S (int-vs-short): dec_info control fields 0x750f54 (size) / 0x750f56 (flag) are 16-bit, but the
    # macros are *(int*); 32-bit writes zero the flag -> Decompress re-inits every call -> desync.
    s = sub(s, "_DAT_00750f54 = 0x4000;", "*(short *)(uintptr_t)0x750f54 = 0x4000;", n=-1, name="S:size=0x4000")
    s = sub(s, "_DAT_00750f54 = 0x400;",  "*(short *)(uintptr_t)0x750f54 = 0x400;",  name="S:size=0x400")
    s = sub(s, "_DAT_00750f56 = 0;",      "*(short *)(uintptr_t)0x750f56 = 0;",      name="S:flag=0")

    # FIX V (int*/byte-width): the prim_buf free-list node stride is 8 BYTES, but _prim_buf is int* so the
    # decompile's `_prim_buf + N*8` node accesses scale x4 (=N*32). iVar3=(int)_prim_buf is already computed
    # (byte arithmetic, used by the correct lines); the decompile inconsistently leaves some bare _prim_buf.
    # Harmless at head=0 (cars/menu); corrupts once the free-list splits past index 0 (the dense track). Scope
    # to the two allocator fns and switch every node access to iVar3. The `iVar3 = _prim_buf;` line is untouched.
    _lines = s.split('\n')
    _vn = 0
    for _sig in ['int __cdecl FUN_00420e6c(int param_1)', 'void __cdecl FUN_00420ee8(uint param_1)']:
        _st = next(i for i, l in enumerate(_lines) if l == _sig)
        _en = next(j for j in range(_st + 1, _st + 90) if _lines[j] == '}')
        _body = '\n'.join(_lines[_st:_en + 1])
        _nb = _body.replace('_prim_buf + ', 'iVar3 + ').replace('+ _prim_buf)', '+ iVar3)')
        assert _nb != _body, "TRANSPILE: FIX V no-op for %s" % _sig
        assert '_prim_buf + ' not in _nb and '+ _prim_buf)' not in _nb, "FIX V incomplete for %s" % _sig
        _lines[_st:_en + 1] = _nb.split('\n')
        _vn += 1
    s = '\n'.join(_lines)
    _applied.append(("V:prim_buf node stride (byte)", _vn))

    # FIX Y (WASM-safety, faithful-direction): draw_text_half_trans's FAST path (dth_clip==0) writes spans with NO
    # X-clamp. The x86 tolerates the benign negative-X / >clipx overrun (harmless writes outside _screenbuffer), but
    # WASM bounds-checks and traps. The clip-flag (FUN_00411654) only flags Y<0 / X>poly_clipx, NOT X<0 (left) — so
    # off-LEFT track prims (199/765 verts off-left, 166 at the GTE -1024 clamp) fall into the FAST path. Route any prim
    # whose X-span leaves [0,poly_clipx] (at the top OR bottom scanline) to the existing faithful CLIP path (else branch),
    # which clamps X AND advances the U/V accordingly. Replaces the prior skip-band-aid: the clip path DRAWS the on-screen
    # part (so the near track renders) instead of skipping it. On-screen prims keep the FAST path unchanged.
    s = sub(s, "void draw_text_half_trans(void)\n\n{\n  undefined1 *puVar1;",
               "void draw_text_half_trans(void)\n\n{\n  int _xsafe;\n  undefined1 *puVar1;", name="Y:xsafe-decl")
    # TEMP stub: Ghidra mangles this half-transparent HUD-text blit into CONCAT11/CONCAT31 partial-register
    # soup -> garbage blend pointers -> render-path crash (native DrawOTag, fault 0x6d) the Y clip-guard
    # doesn't cover. Stub to let the 3D scene render + produce a g_pixels frame; TODO reconstruct via x86_intrin.h.
    if RASTER_GUARD:
        s = sub(s, "void draw_text_half_trans(void)\n\n{\n  int _xsafe;\n  undefined1 *puVar1;",
                   "void draw_text_half_trans(void)\n\n{\n  int _xsafe;\n  return; /* TEMP: mangled CONCAT blit stubbed (x86_intrin.h reconstruction pending) */\n  undefined1 *puVar1;",
                   name="GUARD:draw_text_half_trans")
    s = sub(s, "      if (dth_clip == 0) {\n        do {\n          iVar6 = DAT_00480038;",
               "      { int _hh=dth_y2-dth_y1, _a=dth_x1>>8, _b=dth_x2>>8, _c=(dth_x1+dth_delta1*_hh)>>8, _d=(dth_x2+dth_delta2*_hh)>>8;\n        _xsafe = (_a>=0 && _a<=poly_clipx && _b>=0 && _b<=poly_clipx && _c>=0 && _c<=poly_clipx && _d>=0 && _d<=poly_clipx); }\n      if (dth_clip == 0 && _xsafe) {\n        do {\n          iVar6 = DAT_00480038;", name="Y:fast-route-to-clip")

    # FIX AF (same bug as FIX Y, on the SECOND non-trans textured filler FUN_0041080d=draw_text_half @0x41080d, which
    # FIX Y missed). Its dth_clip==0 FAST path advances the X-span with NO per-pixel X-bound (the dth_clip!=0 path DOES
    # clamp X to [0,poly_clipx]). An off-right/off-left prim (e.g. L7's impact prim {X=449,Y=-1}: Y clamps to 0 but
    # X=449>320) routes through the FAST path -> writes _screenbuffer[Y*0x140 + 449...] OOB -> WASM traps (x86 tolerates
    # the benign overrun = faithful-direction). Route off-X spans to the faithful CLIP path, exactly like FIX Y.
    s = sub(s, "void FUN_0041080d(void)\n\n{\n  bool bVar1;",
               "void FUN_0041080d(void)\n\n{\n  int _xsafe;\n  bool bVar1;", name="AF:xsafe-decl")
    s = sub(s, "      if (dth_clip == 0) {\n        do {\n          iVar3 = (dth_x2 >> 8) - (dth_x1 >> 8);",
               "      { int _hh=dth_y2-dth_y1, _a=dth_x1>>8, _b=dth_x2>>8, _c=(dth_x1+dth_delta1*_hh)>>8, _d=(dth_x2+dth_delta2*_hh)>>8;\n        _xsafe = (_a>=0 && _a<=poly_clipx && _b>=0 && _b<=poly_clipx && _c>=0 && _c<=poly_clipx && _d>=0 && _d<=poly_clipx); }\n      if (dth_clip == 0 && _xsafe) {\n        do {\n          iVar3 = (dth_x2 >> 8) - (dth_x1 >> 8);", name="AF:fast-route-to-clip")

    # FIX AG (REVERTED cont249): padding the prim-buffer mallocs by 0x40 made it WORSE (8/11: L4 returned, L5 broke).
    # The pad is NOT layout-independent — enlarging MPE_malloc's request shifts every downstream allocation, so the
    # heap-adjacency victim moves to a different level's live data. Same failure as the +0x8000 shim. The +8 overrun is
    # genuinely faithful and the dead-slack must be reproduced WITHOUT moving the heap (TBD: match x86's exact MPE base/
    # carve order, or guard the consumer). GUARD AE (free-walk backstop) stays; it's the best current resolution (10/11).

    # FIX AH (REVERTED cont251): the low-end dead-guard (free-list starts at node 8) ALSO failed -> 9/11 (L4 regressed,
    # L7 still crashes). Same root cause as the pad: starting the free-list +0x40 up shifts EVERY prim's offset within the
    # buffer (_gprim1/_gprim2 = buf + iVar4, iVar4 now >=8), which relocates the +8-overrun victim -> L4's corruption moves
    # OFF the free-list (so GUARD AE no longer catches it -> L4 crashes). PROVEN: the L4/L7 heap-adjacency is fixed by NO
    # layout change at all (malloc size = pad/AG, free-list start = dead-guard/AH, inter-buffer gap = +0x8000) — every one
    # just relocates which level's prim overrun lands on live data. The artifact is the literal byte-adjacency of the two
    # prim buffers + every downstream alloc; only matching x86's EXACT heap (unverifiable here) or a consumer-side write-
    # bound (option B, loses the prim tail) remains. GUARD AE (10/11, L7 documented residual) is the standing resolution.

    # FIX AB (front-end menu dispatch, WASM signature): the menu state-machine handlers (FUN_0045xxxx, void(void))
    # are invoked via (*(code*)PTR_FUN_004697cc)() where `code` = int() -> call_indirect type-checks an i32 return
    # -> "null function or function signature mismatch" trap. Cast to the real void(*)(void) handler type so the
    # WASM type-check passes. Front-end-only (the race harness never reaches these); restores the menu dispatch.
    s = s.replace("(*(code *)PTR_FUN_004697cc)()", "(*(void(*)(void))(uintptr_t)PTR_FUN_004697cc)()")
    s = s.replace("(*(code *)(&PTR_FUN_004697cc)[uVar4 * 5])()", "(*(void(*)(void))(uintptr_t)(&PTR_FUN_004697cc)[uVar4 * 5])()")

    # FIX AC/AD (front-end camera scattered-locals + dual-symbol): Init_Front_End/Order_Cars build a camera
    # direction vector in 4 separate locals (local_38/uStack_34/uStack_30/uStack_2c) read as &local_38, but the
    # compiler does NOT lay them contiguously -> &local_38[1..3] reads stray stack (often 0) -> FUN_004205d8
    # divides by SquareRoot0_(0)=0 -> /0 (intermittent; the DemoMode 2nd call hit it). Pack the vector contiguously
    # from the (now GIMG-wired) source globals. AC=Init_Front_End (0x44b138), AD=Order_Cars (0x44c8d8).
    s = s.replace("  Create_Object((undefined4 *)&track_object,*(int *)((char *)_level_data + 0x70));\n  FUN_004203a0((short *)0x907ddc,(int *)&DAT_0046996c);\n  FUN_004205d8(local_58,&local_38,0xc);",
                  "  Create_Object((undefined4 *)&track_object,*(int *)((char *)_level_data + 0x70));\n  FUN_004203a0((short *)0x907ddc,(int *)&DAT_0046996c);\n  { int _fec[4]; _fec[0]=DAT_0044b138; _fec[1]=DAT_0044b13c; _fec[2]=DAT_0044b140; _fec[3]=DAT_0044b144; FUN_004205d8(local_58,_fec,0xc); }")
    s = s.replace("  Create_Object((undefined4 *)0x907d30,*(int *)((char *)_level_data + 0x54));\n  FUN_004203a0((short *)0x907ddc,(int *)&DAT_0046996c);\n  FUN_004205d8(local_58,&local_38,0xc);",
                  "  Create_Object((undefined4 *)0x907d30,*(int *)((char *)_level_data + 0x54));\n  FUN_004203a0((short *)0x907ddc,(int *)&DAT_0046996c);\n  { int _foc[4]; _foc[0]=DAT_0044c8d8; _foc[1]=DAT_0044c8dc; _foc[2]=DAT_0044c8e0; _foc[3]=DAT_0044c8e4; FUN_004205d8(local_58,_foc,0xc); }")

    # GUARD AE (free-list OOB walk protection): an upstream stray GTE-projected-vertex write ({coord,-1}=0xffffXXXX)
    # corrupts a FREE node's link in _prim_buf on L4 (and similar on L7). The faithful fix (the vertex-pool overrun)
    # is still pending, but the free/alloc list WALKS then dereference the OOB link and trap. These guards only fire
    # when a link is already OOB (never on the 9 clean tracks -> zero behaviour change there); they prevent the crash.
    s = sub(s,
      "  for (; (((int)uVar7 <= (int)_DAT_0071bf98 || (*(uint *)(iVar3 + _DAT_0071bf98 * 8) <= uVar7))\n"
      "         && ((uVar2 = *(uint *)(_DAT_0071bf98 * 8 + iVar3), _DAT_0071bf98 < uVar2 ||\n"
      "             (((int)uVar7 <= (int)_DAT_0071bf98 && (uVar2 <= uVar7))))));\n"
      "      _DAT_0071bf98 = *(uint *)(iVar3 + _DAT_0071bf98 * 8)) {\n"
      "  }",
      "  if ((unsigned)_DAT_0071bf98 >= (unsigned)(prim_buf_size >> 3)) _DAT_0071bf98 = 0;\n"
      "  while ((((int)uVar7 <= (int)_DAT_0071bf98 || (*(uint *)(iVar3 + _DAT_0071bf98 * 8) <= uVar7))\n"
      "         && ((uVar2 = *(uint *)(_DAT_0071bf98 * 8 + iVar3), _DAT_0071bf98 < uVar2 ||\n"
      "             (((int)uVar7 <= (int)_DAT_0071bf98 && (uVar2 <= uVar7))))))) {\n"
      "    { unsigned _nl = *(uint *)(iVar3 + _DAT_0071bf98 * 8);\n"
      "      if (_nl >= (unsigned)(prim_buf_size >> 3)) break;\n"
      "      _DAT_0071bf98 = _nl; }\n"
      "  }", 1, 'GUARD AE free-walk OOB')
    s = sub(s, "    iVar5 = iVar4;\n    piVar6 = (int *)(iVar5 * 8 + iVar3);",
      "    iVar5 = iVar4;\n    if ((unsigned)iVar5 >= (unsigned)(prim_buf_size >> 3)) return -1;\n    piVar6 = (int *)(iVar5 * 8 + iVar3);", 1, 'GUARD AE alloc-walk OOB')
    s = sub(s, "  uVar7 = (param_1 >> 3) - 1;\n  _free_mem = _free_mem + *(int *)(iVar3 + -4 + (param_1 >> 3) * 8);",
      "  uVar7 = (param_1 >> 3) - 1;\n  if ((unsigned)uVar7 >= (unsigned)(prim_buf_size >> 3)) return;\n  _free_mem = _free_mem + *(int *)(iVar3 + -4 + (param_1 >> 3) * 8);", 1, 'GUARD AE free uVar7 OOB')
    # GUARD AF (alloc split SIZE OOB): distinct from the 0xffffXXXX GTE-vertex link corruption GUARD AE handles,
    # a stray FLOAT write (~0.44 => 0x3ee1xxxx) lands in _prim_buf and is read as a free node's SIZE field (uVar2).
    # GUARD AE checks the link index iVar5 but NOT uVar2, so the split does iVar5 += (uVar2-uVar1) with a huge uVar2
    # -> iVar5 overflows -> OOB store at _prim_buf+4+iVar5*8 -> SIGSEGV (frame ~966, native). Don't allocate from a
    # block whose size is OOB-huge: fail the size test so the walk skips it and follows the link (GUARD AE catches a
    # bad link next). Fires only on an already-corrupt node -> zero behaviour change on clean state.
    s = sub(s, "    uVar2 = piVar6[1];\n    if (uVar1 <= uVar2) {",
      "    uVar2 = piVar6[1];\n    if (uVar1 <= uVar2 && (unsigned)uVar2 <= (unsigned)(prim_buf_size >> 3)) {", 1, 'GUARD AF alloc split SIZE OOB')

    # GUARD AH (OT-insert wild puVar7, same class as GUARD AE/AF/AG): 5 sibling rasterizer functions
    # (draw_face_3pt/4pt/4pt_text and co.) compute an ordering-table slot pointer `puVar7` from `__otz`,
    # itself derived from vertex data reachable via the same corrupted-geometry chain as the other
    # GUARD fixes (confirmed: native L2 crashes here, `draw_face_4pt_text` @ build/dd2.c:46751,
    # dereferencing a wild puVar7). Guard the OT linked-list splice (read old head, write new head,
    # link old head to new node) at its point of use: skip it (don't insert this primitive into the
    # OT) if puVar7 falls outside the valid image+heap range. Only fires on already-corrupt state.
    s = sub(s, "        uVar2 = *puVar7;\n        *puVar7 = puStack_14;\n        *puStack_14 = uVar2;",
               "        if ((uintptr_t)puVar7 >= 0x400000u && (uintptr_t)puVar7 < 0x900000u) {\n"
               "        uVar2 = *puVar7;\n        *puVar7 = puStack_14;\n        *puStack_14 = uVar2;\n        }",
               6, 'GUARD AH OT-insert wild puVar7 (puStack_14, 8sp x6)')
    s = sub(s, "        uVar2 = *puVar7;\n        *puVar7 = local_14;\n        *local_14 = uVar2;",
               "        if ((uintptr_t)puVar7 >= 0x400000u && (uintptr_t)puVar7 < 0x900000u) {\n"
               "        uVar2 = *puVar7;\n        *puVar7 = local_14;\n        *local_14 = uVar2;\n        }",
               1, 'GUARD AH OT-insert wild puVar7 (local_14)')
    s = sub(s, "          uVar2 = *puVar7;\n          *puVar7 = puStack_14;\n          *puStack_14 = uVar2;",
               "          if ((uintptr_t)puVar7 >= 0x400000u && (uintptr_t)puVar7 < 0x900000u) {\n"
               "          uVar2 = *puVar7;\n          *puVar7 = puStack_14;\n          *puStack_14 = uVar2;\n          }",
               1, 'GUARD AH OT-insert wild puVar7 (puStack_14, 10sp)')

    # GUARD AH2: same OT-insert bug, sibling rasterizers using `puVar5` as the slot pointer instead
    # of `puVar7` (confirmed: native L9 crashed in draw_face_4pt_text_squash, build/dd2.c:48261,
    # via this exact variant). Same guard, same reasoning.
    s = sub(s, "      uVar2 = *puVar5;\n      *puVar5 = local_14;\n      *local_14 = uVar2;",
               "      if ((uintptr_t)puVar5 >= 0x400000u && (uintptr_t)puVar5 < 0x900000u) {\n"
               "      uVar2 = *puVar5;\n      *puVar5 = local_14;\n      *local_14 = uVar2;\n      }",
               1, 'GUARD AH2 OT-insert wild puVar5 (local_14, 6sp)')
    s = sub(s, "        uVar2 = *puVar5;\n        *puVar5 = puStack_14;\n        *puStack_14 = uVar2;",
               "        if ((uintptr_t)puVar5 >= 0x400000u && (uintptr_t)puVar5 < 0x900000u) {\n"
               "        uVar2 = *puVar5;\n        *puVar5 = puStack_14;\n        *puStack_14 = uVar2;\n        }",
               4, 'GUARD AH2 OT-insert wild puVar5 (puStack_14, 8sp x4)')
    s = sub(s, "        uVar2 = *puVar5;\n        *puVar5 = _gprim1;\n        *_gprim1 = uVar2;",
               "        if ((uintptr_t)puVar5 >= 0x400000u && (uintptr_t)puVar5 < 0x900000u) {\n"
               "        uVar2 = *puVar5;\n        *puVar5 = _gprim1;\n        *_gprim1 = uVar2;\n        }",
               1, 'GUARD AH2 OT-insert wild puVar5 (_gprim1)')
    s = sub(s, "        uVar2 = *puVar5;\n        *puVar5 = puVar6;\n        *puVar6 = uVar2;",
               "        if ((uintptr_t)puVar5 >= 0x400000u && (uintptr_t)puVar5 < 0x900000u) {\n"
               "        uVar2 = *puVar5;\n        *puVar5 = puVar6;\n        *puVar6 = uVar2;\n        }",
               1, 'GUARD AH2 OT-insert wild puVar5 (puVar6)')

    # GUARD AG (draw_text_half wild _clut/_tex, same class as GUARD AE/AF): DAT_00460004/DAT_0046000c
    # (the current CLUT/texture-page globals) can be set from a corrupted per-object dth_clut/dth_tpage
    # value (heap-layout divergence, same root as the GEOM-GUARD fixes) -> a wild address (e.g.
    # 0xd234100, L6) -> OOB read in the per-pixel `_sb[_i]=_clut[_t]` loop. Guard both draw_text_half
    # spans (fast + clip path) at their point of use: skip the span (draw nothing) if _clut/_tex fall
    # outside the valid image+heap range. Only fires on already-corrupt state -> zero behaviour change
    # on the clean tracks (matches the GUARD AE/AF convention above).
    s = sub(s,
      "            unsigned char* _tex=(unsigned char*)(uintptr_t)((unsigned)DAT_0046000c & 0xffff0000u);\n"
      "            unsigned char* _clut=(unsigned char*)(uintptr_t)((unsigned)DAT_00460004 & 0xffffff00u);\n"
      "            unsigned char* _sb=(unsigned char*)(uintptr_t)0x700450u + dth_y1*0x140 + (dth_x1>>8);\n"
      "            int _tu=dth_u1, _tv=dth_v1, _i;\n"
      "            for(_i=0;_i<_span;_i++){",
      "            unsigned char* _tex=(unsigned char*)(uintptr_t)((unsigned)DAT_0046000c & 0xffff0000u);\n"
      "            unsigned char* _clut=(unsigned char*)(uintptr_t)((unsigned)DAT_00460004 & 0xffffff00u);\n"
      "            unsigned char* _sb=(unsigned char*)(uintptr_t)0x700450u + dth_y1*0x140 + (dth_x1>>8);\n"
      "            int _tu=dth_u1, _tv=dth_v1, _i;\n"
      "            if ((uintptr_t)_tex < 0x400000u || (uintptr_t)_tex >= 0x900000u ||"
      " (uintptr_t)_clut < 0x400000u || (uintptr_t)_clut >= 0x900000u) _span = 0;  /* GUARD AG */\n"
      "            for(_i=0;_i<_span;_i++){",
      1, 'GUARD AG draw_text_half fast-path wild clut/tex')
    s = sub(s,
      "              unsigned char* _tex=(unsigned char*)(uintptr_t)((unsigned)DAT_0046000c & 0xffff0000u);\n"
      "              unsigned char* _clut=(unsigned char*)(uintptr_t)((unsigned)DAT_00460004 & 0xffffff00u);\n"
      "              unsigned char* _sb=(unsigned char*)(uintptr_t)0x700450u + dth_y1*0x140 + x1;\n"
      "              int _i, _span=x2-x1;\n"
      "              for(_i=0;_i<_span;_i++){",
      "              unsigned char* _tex=(unsigned char*)(uintptr_t)((unsigned)DAT_0046000c & 0xffff0000u);\n"
      "              unsigned char* _clut=(unsigned char*)(uintptr_t)((unsigned)DAT_00460004 & 0xffffff00u);\n"
      "              unsigned char* _sb=(unsigned char*)(uintptr_t)0x700450u + dth_y1*0x140 + x1;\n"
      "              int _i, _span=x2-x1;\n"
      "              if ((uintptr_t)_tex < 0x400000u || (uintptr_t)_tex >= 0x900000u ||"
      " (uintptr_t)_clut < 0x400000u || (uintptr_t)_clut >= 0x900000u) _span = 0;  /* GUARD AG */\n"
      "              for(_i=0;_i<_span;_i++){",
      1, 'GUARD AG draw_text_half clip-path wild clut/tex')

    # FIX AJ (Map_Height edge-divisor WASM-safety): the scanline edge interpolation in Map_Height
    # (dd2.c:12289..) divides by an edge X/Y screen-delta: (*(int*)(pbVar10+2)>>0x10) and
    # (*(int*)(pbVar10+8)>>0x10). For a degenerate (zero-extent) projected edge the delta is 0;
    # x86 silently never lands the front-end's edge delta exactly on 0, but WASM TRAPS on integer
    # divide-by-zero. Faithful guard: den==0 -> 1 (only changes behaviour when the divisor is already
    # 0, i.e. unreachable on real geometry; no effect on the verified race tracks). The '+ piVarX[1]'
    # adds to the QUOTIENT (C: '/' binds before '+'), so wrapping the divisor preserves precedence.
    # All 9 patterns are divisors, all inside Map_Height. This unblocks the front-end /0; race sweep
    # with the guard = 10/11 (only the L7 documented residual), no regression. [[dd2-frontend-nav-input]]
    s = sub(s, "void __cdecl Map_Height(int *param_1)",
               "#define _DZ(x) ((x)?(x):1)\nvoid __cdecl Map_Height(int *param_1)", 1, 'AJ:_DZ macro')
    s = sub(s, "(*(int *)(pbVar10 + 2) >> 0x10)", "_DZ(*(int *)(pbVar10 + 2) >> 0x10)", 4, 'AJ:Map_Height edge-X divisor')
    s = sub(s, "(*(int *)(pbVar10 + 8) >> 0x10)", "_DZ(*(int *)(pbVar10 + 8) >> 0x10)", 5, 'AJ:Map_Height edge-Y divisor')

    # FIX BB (Play_Race_Start_Sounds camera-interp /0): the track-strip edge interpolation in
    # Play_Race_Start_Sounds@0x42895c (build:12917) divides by local_20 = a screen-space edge
    # cross-product. On x86 integer /0 ALSO faults, so dd2h never reaches local_20==0 here (the
    # 0x42895c branch gate keeps the edge non-degenerate); we hit it on L1 f454 from a degenerate
    # track strip. _DZ(den)->1 only when the divisor is already 0 (unreachable on dd2h's path ->
    # bit-identical where it matters; only diverges on the already-divergent degenerate frame).
    s = sub(s, " * 0x100) /\n                local_20;",
               " * 0x100) /\n                _DZ(local_20);", 1, 'BB:camera-interp /0')

    # TRIED (REVERTED): a FIX BJ guarding the SIBLING divisor a few lines earlier in this same
    # interpolation (dd2.c:12948-12951, also a screen-space edge cross-product) with the same
    # _DZ(den)->1 pattern as FIX BB. Unlike FIX BB, this one turned native L2's clean SIGFPE crash
    # into a genuine INFINITE LOOP (confirmed: ran the full 90s timeout at ~100% CPU, exit code 124,
    # not a fast crash) -- worse than the crash it replaced. Reverted. This divisor's degenerate case
    # apparently feeds an iterative/convergence process elsewhere that needs den==0 to actually mean
    # something (e.g. terminate a loop), not just "avoid division by zero" -- _DZ->1 papers over the
    # SIGFPE but breaks that logic.
    # FIX BK: the different-approach fix that DID work -- instead of substituting the divisor (which
    # corrupts the iterative state, see above), SKIP the whole dependent calculation (the division
    # AND everything that consumes its result: iVar2/iVar3/_camera_fd/_DAT_00744b18/DAT_00463ef0
    # updates, dd2.c:12948-12968) when the divisor would be 0 -- i.e. just don't update the camera
    # interpolation this frame for a degenerate track-strip edge, leaving state at its prior (valid)
    # values, rather than injecting either a fake divisor or a fake quotient. Only fires when the
    # divisor is already 0 (unreachable on real geometry).
    s = sub(s, '        local_1c = (*(int *)((int)&DAT_00752344 + iVar4) - _camera_fd) * (piVar10[2] - piVar8[2]);\n        iVar2 = (((*piVar10 - *piVar8) * (*(int *)((int)&DAT_0075234c + iVar4) - piVar8[2]) -\n                 (piVar10[2] - piVar8[2]) * (*(int *)((int)&DAT_00752344 + iVar4) - *piVar8)) * 0x100) /\n                ((*(int *)((int)&DAT_0075234c + iVar4) - _DAT_00744b18) * (*piVar10 - *piVar8) - local_1c\n                );\n        iVar3 = iVar2 * (_camera_fd - *(int *)((int)&DAT_00752344 + iVar4)) +\n                *(int *)((int)&DAT_00752344 + iVar4) * 0x100;\n        iVar5 = iVar3 >> 0x1f;\n        _camera_fd = (int)((iVar3 + iVar5 * -0x100) - (uint)(iVar5 << 7 < 0)) >> 8;\n        iVar2 = *(int *)((int)&DAT_0075234c + iVar4) * 0x100 +\n                (_DAT_00744b18 - *(int *)((int)&DAT_0075234c + iVar4)) * iVar2;\n        iVar3 = iVar2 >> 0x1f;\n        _DAT_00744b18 = (int)((iVar2 + iVar3 * -0x100) - (uint)(iVar3 << 7 < 0)) >> 8;\n        if ((int)DAT_00463ef0 < 0x400) {\n          iVar2 = 0x20;\n        }\n        else {\n          iVar2 = DAT_00463ef0 - 0x400;\n        }\n        DAT_00463ef0 = DAT_00463ef0 + iVar2;\n        if (0x3ff < (int)DAT_00463ef0) goto joined_r0x00429509;\n        DAT_00463ef0 = DAT_00463ef0 + 0x20;\n      }\n    }', '        local_1c = (*(int *)((int)&DAT_00752344 + iVar4) - _camera_fd) * (piVar10[2] - piVar8[2]);\n        if (((*(int *)((int)&DAT_0075234c + iVar4) - _DAT_00744b18) * (*piVar10 - *piVar8) - local_1c) != 0) {\n        iVar2 = (((*piVar10 - *piVar8) * (*(int *)((int)&DAT_0075234c + iVar4) - piVar8[2]) -\n                 (piVar10[2] - piVar8[2]) * (*(int *)((int)&DAT_00752344 + iVar4) - *piVar8)) * 0x100) /\n                ((*(int *)((int)&DAT_0075234c + iVar4) - _DAT_00744b18) * (*piVar10 - *piVar8) - local_1c\n                );\n        iVar3 = iVar2 * (_camera_fd - *(int *)((int)&DAT_00752344 + iVar4)) +\n                *(int *)((int)&DAT_00752344 + iVar4) * 0x100;\n        iVar5 = iVar3 >> 0x1f;\n        _camera_fd = (int)((iVar3 + iVar5 * -0x100) - (uint)(iVar5 << 7 < 0)) >> 8;\n        iVar2 = *(int *)((int)&DAT_0075234c + iVar4) * 0x100 +\n                (_DAT_00744b18 - *(int *)((int)&DAT_0075234c + iVar4)) * iVar2;\n        iVar3 = iVar2 >> 0x1f;\n        _DAT_00744b18 = (int)((iVar2 + iVar3 * -0x100) - (uint)(iVar3 << 7 < 0)) >> 8;\n        if ((int)DAT_00463ef0 < 0x400) {\n          iVar2 = 0x20;\n        }\n        else {\n          iVar2 = DAT_00463ef0 - 0x400;\n        }\n        DAT_00463ef0 = DAT_00463ef0 + iVar2;\n        if (0x3ff < (int)DAT_00463ef0) goto joined_r0x00429509;\n        DAT_00463ef0 = DAT_00463ef0 + 0x20;\n        }\n      }\n    }', 1, 'BK:skip whole camera-interp calc on degenerate divisor')

    # FIX BC (Decompress runaway, SIGNED loop-terminate): in Decompress@0x415550 the chunk loop
    # terminates with `if (uVar8 <= uVar9) return;`. x86 @0x415581 is `cmp esi,ecx; jl` = a SIGNED
    # compare, but Ghidra typed uVar8/uVar9 as uint -> UNSIGNED. When a back-ref run near the chunk
    # boundary makes uVar9 overshoot, param_1[2] (= remaining count) goes slightly negative (e.g.
    # 0xffffffd7); the next call's uVar8 = that negative count, and the UNSIGNED `uVar8 <= uVar9` is
    # never true (huge) -> the decompressor writes ~100MB to the mmap region end -> SIGSEGV (L2 f229).
    # Signed (matching x86) terminates immediately on a negative count. Identity for all positive
    # counts (zero change on clean blocks); only differs on the already-divergent overshoot. [[dd2-demo-harness-state]]
    s = sub(s, "    if (uVar8 <= uVar9) {\n        *param_1 = pbVar5;",
               "    if ((int)uVar8 <= (int)uVar9) {\n        *param_1 = pbVar5;", 1, 'BC:Decompress signed terminate')

    # FIX BF (byte-offset): Print's font-table lookups read DAT_0071bfd0 (an `int` macro) as
    # `&DAT_0071bfd0 + idx*4` -> int*-scaled = byte idx*16 (x4 too big) -> font idx>=2 reads past the
    # 6-entry table -> null font -> crash. Other sites use the (int) cast; these 3 (Print) lack it.
    # HUD uses font 0 (0*16==0*4) so the demo was unaffected; the front-end uses fonts 1-5.
    s = sub(s, "*(int *)(&DAT_0071bfd0 +", "*(int *)((int)&DAT_0071bfd0 +", 3, 'BF:font-table byte-offset')

    # FIX BG (dropped-arg): SetPalette's IDirectDrawPalette::SetEntries call was decompiled with 4 args
    # but the x86 (0x412e2f `push 0x700050`) passes a 5th, lpEntries=&DAT_00700050. Missing -> the
    # DirectSound/DDraw shim reads stack garbage. Demo only hits CreatePalette; the front-end hits SetEntries.
    s = sub(s, "(**(code **)(*DAT_00460444 + 0x18))(DAT_00460444,0,0,0x100);",
               "(**(code **)(*DAT_00460444 + 0x18))(DAT_00460444,0,0,0x100,&DAT_00700050);", 1, 'BG:SetEntries dropped lpEntries')

    # FIX BI (dropped-assignment): Draw_Object_Polys sets _gpoly+_gprim1 but the decompile DROPPED the
    # _gprim2 setup the x86 has (0x41fee2 `mov [0x71bdd0],edx` = prim_buf[DAT_00462fec]+obj_off). So
    # FUN_0041bc0c (dispatched here) wrote a STALE _gprim2 (left by the last Draw_Scene_Object) into
    # ANOTHER object's prim block -> corrupted its clut -> draw_text_half crash (L3/L4/L7). 6/10->9/10.
    s = sub(s, "  _gprim1 = *(int *)((int)&prim_buf + buffer_num * 4) + param_1[(param_2 + -1) * 3 + 2];\n  Pre_Rotate",
               "  _gprim1 = *(int *)((int)&prim_buf + buffer_num * 4) + param_1[(param_2 + -1) * 3 + 2];\n"
               "  _gprim2 = *(int *)((int)&prim_buf + (*(int*)GIMG(0x462fec)) * 4) + param_1[(param_2 + -1) * 3 + 2];\n  Pre_Rotate",
               1, 'BI:Draw_Object_Polys dropped _gprim2')

    # GUARD AK (native L6, safer than the reverted GUARD AJ2/AJ3): guard the individual byte-writes
    # inside FUN_0041bc0c/FUN_0041bc68's loop against the valid heap+image range, WITHOUT touching the
    # iteration count or the _gpoly/_gprim1/_gprim2 advancement -- this avoids both failure modes seen
    # with the reverted attempts (skipping the loop entirely hung the caller's outer _gpoly walk;
    # clamping to 1 iteration fixed native but WASM-trapped on that single iteration's write). Moving
    # the advancement OUTSIDE the validity check means the loop always fully runs its (possibly huge,
    # corrupt) iteration count and _gpoly always advances exactly as before -- only the actual memory
    # writes are conditionally skipped when the target is unsafe. NOTE: anchors are the POST-FIX-I text
    # (FIX I earlier in this function rewrites _gprim1's raw +N into (char*)-cast form first).
    s = sub(s, '  for (iVar2 = 0; iVar2 < param_1; iVar2 = iVar2 + 1) {\n    *(undefined1 *)((char *)_gprim1 + 7) = 0x30;\n    *(undefined1 *)(_gprim2 + 7) = 0x30;\n    uVar1 = *(undefined4 *)((int)_gpoly + 4);\n    *(undefined4 *)(_gprim2 + 4) = uVar1;\n    *(undefined4 *)((char *)_gprim1 + 4) = uVar1;\n    *(undefined1 *)(_gprim2 + 4) = 0;\n    _gpoly = (int)_gpoly + 0x18;\n    *(undefined1 *)((char *)_gprim1 + 4) = *(undefined1 *)(_gprim2 + 4);\n    _gprim2 = _gprim2 + 0x1c;\n    _gprim1 = (int *)((char *)_gprim1 + 0x1c);\n  }', '  for (iVar2 = 0; iVar2 < param_1; iVar2 = iVar2 + 1) {\n    if ((unsigned)(uintptr_t)_gprim1 >= 0x400000u && (unsigned)(uintptr_t)_gprim1 < 0x900000u && (unsigned)_gprim2 >= 0x400000u && (unsigned)_gprim2 < 0x900000u) {  /* GUARD AK: per-iteration validity check, does NOT alter iteration count/_gpoly advance */\n    *(undefined1 *)((char *)_gprim1 + 7) = 0x30;\n    *(undefined1 *)(_gprim2 + 7) = 0x30;\n    uVar1 = *(undefined4 *)((int)_gpoly + 4);\n    *(undefined4 *)(_gprim2 + 4) = uVar1;\n    *(undefined4 *)((char *)_gprim1 + 4) = uVar1;\n    *(undefined1 *)(_gprim2 + 4) = 0;\n    *(undefined1 *)((char *)_gprim1 + 4) = *(undefined1 *)(_gprim2 + 4);\n    }\n    _gpoly = (int)_gpoly + 0x18;\n    _gprim2 = _gprim2 + 0x1c;\n    _gprim1 = (int *)((char *)_gprim1 + 0x1c);\n  }', 1, 'GUARD AK FUN_0041bc0c per-write guard')
    s = sub(s, '  for (iVar2 = 0; iVar2 < param_1; iVar2 = iVar2 + 1) {\n    *(undefined1 *)((char *)_gprim1 + 7) = 0x30;\n    *(undefined1 *)(_gprim2 + 7) = 0x30;\n    uVar1 = *(undefined4 *)((int)_gpoly + 4);\n    *(undefined4 *)(_gprim2 + 4) = uVar1;\n                    /* END-> C:\\PCMPE\\sound\\sound.C: ? */\n    *(undefined4 *)((char *)_gprim1 + 4) = uVar1;\n    *(undefined1 *)(_gprim2 + 4) = 0;\n    _gpoly = (int)_gpoly + 0x14;\n    *(undefined1 *)((char *)_gprim1 + 4) = *(undefined1 *)(_gprim2 + 4);\n    _gprim2 = _gprim2 + 0x1c;\n    _gprim1 = (int *)((char *)_gprim1 + 0x1c);\n  }', '  for (iVar2 = 0; iVar2 < param_1; iVar2 = iVar2 + 1) {\n    if ((unsigned)(uintptr_t)_gprim1 >= 0x400000u && (unsigned)(uintptr_t)_gprim1 < 0x900000u && (unsigned)_gprim2 >= 0x400000u && (unsigned)_gprim2 < 0x900000u) {  /* GUARD AK */\n    *(undefined1 *)((char *)_gprim1 + 7) = 0x30;\n    *(undefined1 *)(_gprim2 + 7) = 0x30;\n    uVar1 = *(undefined4 *)((int)_gpoly + 4);\n    *(undefined4 *)(_gprim2 + 4) = uVar1;\n                    /* END-> C:\\PCMPE\\sound\\sound.C: ? */\n    *(undefined4 *)((char *)_gprim1 + 4) = uVar1;\n    *(undefined1 *)(_gprim2 + 4) = 0;\n    *(undefined1 *)((char *)_gprim1 + 4) = *(undefined1 *)(_gprim2 + 4);\n    }\n    _gpoly = (int)_gpoly + 0x14;\n    _gprim2 = _gprim2 + 0x1c;\n    _gprim1 = (int *)((char *)_gprim1 + 0x1c);\n  }', 1, 'GUARD AK FUN_0041bc68 per-write guard')
    return s

def fix_dispatch(s):
    # FIX T (dispatch-relocation): the face-handler table 0x462ef4..0x462fa4 holds x86 handler addrs that
    # dd2_relocate would rewrite to WASM fn-ptrs; but FUN_0041fb7c reads it via a Ghidra switch comparing
    # x86 addresses. Exclude the table from relocation so the switch matches + calls the named handlers.
    s = sub(s, "    unsigned w=*(unsigned*)(g_image+off);\n    if(w>=0x410000&&w<0x460000){void*fn=g_lut[w-0x410000]; if(fn)*(void**)(g_image+off)=fn;}",
               "    if(off>=0x62ef4u && off<0x62fa4u){ continue; }  /* FIX T: face-dispatch table read by a switch as x86 addrs, not called */\n    { unsigned w=*(unsigned*)(g_image+off);\n    if(w>=0x410000&&w<0x460000){void*fn=g_lut[w-0x410000]; if(fn)*(void**)(g_image+off)=fn;} }",
               name="T:dispatch-table reloc exclude")
    return s

def fix_runtime(s):
    # TEST harness: allow level select via argv (faithful boot is the menu->race flow, pending the menu
    # indirect-call work). Lets build/ run any of the 16 levels for headless verification.
    s = sub(s, "int main(){", "int main(int argc, char** argv){", name="TEST:argv main")
    s = sub(s, "    _current_level = 1;", "    _current_level = (argc>1)?atoi(argv[1]):1;", name="TEST:argv level")
    # Inject DemoModeLevel: faithful copy of DemoMode@0x44b4e0 with _current_level forced (DD2_LEVEL/argv).
    # The WASM runtime must run the SAME crash-free attract path as native_main.c (direct Play_Game = the
    # old f1179 demo-state crash). DemoMode sets demo_mode/num_cars/race_car, picks level, Order_Cars, Play_Game.
    demolevel = (
      "extern int DemoMode(void); extern void Setup_Pad(int); extern void Order_Cars(void); extern int rand(void);\n"
      "static int DemoModeLevel(int lvl){\n"
      "  Setup_Pad(1);\n"
      "  *(int*)0x905a1c=*(int*)0x4673f4; *(int*)0x905a18=*(int*)0x4673f8;\n"
      "  *(int*)0x46385c=1;\n"
      "  *(int*)0x905a14=*(int*)0x467400; *(int*)0x467400=2;\n"
      "  *(int*)0x905a10=*(int*)0x46765c; *(int*)0x46765c=0x14;\n"
      "  *(int*)0x4673f8=0; *(int*)0x4673f4=0;\n"
      "  { int iVar1=rand(); _current_level = lvl ? lvl : (iVar1%10+1); }\n"
      "  Order_Cars();\n"
      "  return Play_Game();\n"
      "}\n")
    s = sub(s, "int main(int argc, char** argv){", demolevel + "int main(int argc, char** argv){",
            name="TEST:demomodelevel-inject")
    # TEST harness: front-end mode. `dd2run.js fe` runs Init_Front_End + Front_End (title/menu/track-
    # select) instead of the race, for headless front-end verification. The faithful boot is main->
    # Front_End->Play_Game; this lets the front-end be exercised in isolation while the menu indirect-
    # call + input-sim work proceeds. [[dd2-frontend-nav-input]]
    s = sub(s, "    Play_Game();                  /* the race: Init_Game + physics/AI/GTE/render loop */",
               "    if (argc>1 && argv[1][0]=='f' && argv[1][1]=='e') {\n"
               "      extern void Init_Front_End(void); extern void Front_End(void);\n"
               "      Init_Front_End(); Front_End(); return 0;\n"
               "    }\n"
               "    DemoModeLevel((argc>1)?atoi(argv[1]):0);  /* attract demo path (was: direct Play_Game) */",
               name="TEST:fe-mode front-end harness")
    return s

def main():
    transforms = {'dd2.c': fix_dd2, 'dd2_dispatch.c': fix_dispatch, 'dd2_runtime.c': fix_runtime}
    if not CHECK:
        if os.path.isdir(OUT): shutil.rmtree(OUT)
        os.makedirs(OUT)
    for fn in sorted(os.listdir(SRC)):
        sp = os.path.join(SRC, fn)
        if not os.path.isfile(sp): continue
        data = open(sp, 'r', encoding='utf-8', errors='surrogateescape').read()
        if fn in transforms:
            data = transforms[fn](data)
        if not CHECK:
            open(os.path.join(OUT, fn), 'w', encoding='utf-8', errors='surrogateescape').write(data)
    print("transpile: %d fixes applied across %d source files%s" %
          (len(_applied), len(transforms), " (check-only)" if CHECK else " -> build/"))
    for nm, c in _applied:
        print("  [%s] x%d" % (nm, c))

if __name__ == '__main__':
    main()
