#!/usr/bin/env python3
"""Assemble the dd2h build source (re_out/dd2.c) from the pristine dd2h decompile.
Part of the dd2.exe->dd2h re-base. Steps:
  1. Take the game-code portion of the dd2h decompile (re_out/dd2_decomp.c), excluding statically-linked
     MSVC CRT functions (>=0x45a000, or known libc names, or x87-intrinsic bodies) -- libc/compat provide them.
  2. Prepend: includes + GTE register-file globals (dd2h addrs) + forward declarations (from the decomp's
     function markers, excluding compat/libc names).
Output: re_out/dd2.c  (then `make patch` applies the patches/ series onto it).
"""
import re, sys
DECOMP = sys.argv[1] if len(sys.argv) > 1 else "re_out/dd2_decomp.c"
OUT    = sys.argv[2] if len(sys.argv) > 2 else "re_out/dd2.c"
WIN32 = {'DirectDrawCreate','DirectSoundCreate','timeBeginPeriod','timeEndPeriod','timeSetEvent',
        'timeKillEvent','mciSendCommandA','GetLastError','GetModuleHandleA','GetModuleFileNameA',
        'CreateEventA','CreateMutexA','CreateThread','SetEvent','WaitForSingleObject','ReleaseMutex',
        'ExitProcess','ExitThread','GetVersion','GetCommandLineA','TlsAlloc','TlsFree','TlsGetValue',
        'TlsSetValue','LocalAlloc','LocalFree','DeleteFileA','GetCurrentThreadId','GetCurrentProcessId',
        'GetCurrentThread','GetStdHandle','SetStdHandle','GetConsoleMode','SetConsoleMode','WriteConsoleA',
        'ReadConsoleInputA','GetEnvironmentStrings','joyGetPos','joyGetDevCapsA','_beginthread','_endthread',
        'getch','putch','_control87','Init_DirectSound'}
CRT = {'atoi','fopen','fputs','fwrite','printf','remove','strcmp','strdup','strtod','sprintf','vsprintf',
       'fclose','fread','fseek','malloc','free','memcpy','memset','strcpy','strlen','strcat','strncmp',
       'qsort','fprintf','exit','ftell','abort','realloc','calloc','strncpy','atol','rand','srand','fgetc','fputc','fgets','getc','putc','ungetc','fflush','feof','ferror','clearerr','setvbuf','rewind','fgetpos','fsetpos','tmpfile','_flsbuf','_filbuf','__filbuf','__flsbuf'}
compat = set(re.findall(r'\b(\w+)\s*\(', open("re_out/ghidra_compat.h").read()))
# also every function DEFINED in the hand-written compat/runtime units (they provide the real impls;
# dd2.c must not redefine them -> multiple-definition link errors otherwise).
for _cf in ('dd2_com','dd2_win32','dd2_filio','dd2_stubs','dd2_data','dd2_runtime','dd2_buffers','dd2_input','../tools/native_main'):
    try:
        _ct = open("re_out/%s.c" % _cf, encoding='utf-8', errors='surrogateescape').read()
        for _m in re.finditer(r'\n(?:[A-Za-z_][\w\* ]*?\s)(\w+)\s*\([^;{]*\)\s*\n?\{', _ct):
            compat.add(_m.group(1))

    except FileNotFoundError: pass
exclude = compat | CRT | WIN32
# never exclude functions referenced by the dispatch table (they're real indirect-call targets)
_disp = open("re_out/dd2_dispatch.c", encoding='utf-8', errors='surrogateescape').read()
_keep_names = set(re.findall(r'\(void\*\)&(\w+)', _disp))
decomp = open(DECOMP, encoding='utf-8', errors='surrogateescape').read()
# 1. keep only game-code functions
parts = re.split(r'(/\* ===== \S+ @ [0-9a-fA-F]+ ===== \*/)', decomp)
kept = [parts[0]]
i = 1
while i < len(parts):
    marker, body = parts[i], parts[i+1] if i+1 < len(parts) else ''
    m = re.match(r'/\* ===== (\S+) @ ([0-9a-fA-F]+) ===== \*/', marker)
    name, addr = m.group(1).strip('"'), int(m.group(2), 16)
    # CRT: the statically-linked MSVC runtime (startup/console/math/heap). It lives at >=0x459000 and/or uses
    # x87 intrinsics / MSVC FILE internals. Our native_main.c has its own main + libc, so the CRT is never
    # actually run -- but the game IMAGE contains function pointers into it (dispatch) and a few game functions
    # take its address. So we STUB these (keep the symbol so refs resolve; replace the broken body with a
    # trivial `{ return 0; }`) rather than drop them (which caused undefined refs).
    _is_crt = (addr >= 0x459000) or any(x in body for x in
        ('__fdiv','__fmul','sti_st','__fld','__fst','->_flag','->_ptr','->_cnt','->_base','->_bufsiz',
         '->_file','->_charbuf','_INPUT_RECORD','.Event','log2(','log10('))
    _is_x87 = any(x in body for x in ('__fdiv','__fmul','sti_st','__fld','__fst'))
    if name in {'FUN_004568ee','FUN_004568d7','__prtf','FUN_0045623b'}:  # SPRINTF_EXCL: dd2h sprintf reimpl in dd2h_stubs.c
        i += 2; continue
    if name in WIN32 or name in CRT or name in compat or _is_x87:   # libc/compat/x87 -> drop entirely
        i += 2; continue
    if _is_crt:
        # STUB: int-return, no-arg -> callable in any context (-w allows arg/return mismatch), body removed
        kept += [marker, '\nint ' + name + '(){ return 0; }\n']
    else:
        kept += [marker, body]
    i += 2
gamecode = "".join(kept)
# Sanitize Ghidra name artifacts: identifiers like SquareRoot0" / GTERPT4" / IF@DLOG2 / Read_CD_Toc& contain
# ", &, @ which aren't valid C. The original build renamed these (SquareRoot0" -> SquareRoot0_). Apply the
# same globally (defs + calls) so they compile. Only touches identifier-adjacent specials, not string literals
# (a `"` immediately following an alnum with an alnum/paren after is a name artifact, not a string).
gamecode = re.sub(r'([A-Za-z0-9_])["@&`]([A-Za-z0-9_(])', r'\1_\2', gamecode)
gamecode = re.sub(r'([A-Za-z0-9_])["@&`](\s*\()', r'\1_\2', gamecode)   # name"( -> name_(
gamecode = re.sub(r'([A-Za-z0-9_])\*(\s*\()', r'\1_\2', gamecode)   # name*( -> name_(

# Retype void-returning functions that callers use as returning a value (Ghidra mis-detected void return
# for register-return functions). Faithful: they return via register; declaring undefined4 matches callers.
_VOID_RET = ['FirstSavedGame','__set_errno_nt','__ExpandDGROUP','FUN_00456d27','FUN_0045825c',
             'FUN_004580a9','FUN_004580cf']
for _fn in _VOID_RET:
    gamecode = re.sub(r'\bvoid(\s+(?:__cdecl\s+|__fastcall\s+)?' + re.escape(_fn) + r'\s*\()', r'undefined4\1', gamecode)

# 2. forward decls from markers
sigs, seen = [], set()
lines = gamecode.split("\n")
for i, l in enumerate(lines):
    m = re.match(r'/\* ===== (\S+) @ ([0-9a-fA-F]+) ===== \*/', l)
    if not m: continue
    name = m.group(1).strip('"')
    if name in seen or name in exclude: continue
    sig, j = [], i+1
    while j < len(lines) and j < i+12:
        s = lines[j].strip()
        if '{' in s:                       # stop at the body '{' (bare or inline stub '{ return 0; }')
            s = s[:s.find('{')].strip()
            if s and not s.startswith(('/*','*')): sig.append(s)
            break
        if s and not s.startswith(('/*','*')): sig.append(s)
        j += 1
    if not sig or '(' not in ' '.join(sig): continue
    seen.add(name); sigs.append(' '.join(sig) + ';')
prologue = ('#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n#include <stdio.h>\n'
            'int _g_esi=0x74c500,_g_edi=0x4604b6,_g_ebx=0x74c500,_g_ebp=0x74c540,_g_eax=0;\n'
            'int DirectDrawCreate(int,void**,int); int DirectSoundCreate(int,void**,int);\n'
            '/* forward declarations (decomp markers, excl. compat/libc) */\n' + "\n".join(sigs) + "\n\n")

# GTE REGISTER-ARG conversion (= dd2.exe GTE overlay): the GTE matrix/vector fragments (GTERT,
# FUN_00414016/99, RotTrans...) pass their in/out vectors in esi/edi/ebx/ebp. Ghidra models only stack
# args, so it emits bare `short *unaff_ESI;` (uninitialised local -> null deref). Bind each dropped
# register to the emulated-register global (_g_esi/_g_edi/_g_ebx/_g_ebp) the caller-chain sets, exactly
# as the dd2.exe overlay did. The GTE wiring patches (FIX Q/N, GTERPT...) then anchor on this form.
def _conv_unaff(m):
    ind, typ, reg = m.group(1), m.group(2), m.group(3)
    g = {'ESI':'_g_esi','EDI':'_g_edi','EBX':'_g_ebx','EBP':'_g_ebp'}[reg]
    cast = typ.replace(' ', '')            # "short *" -> "short*"
    return "%s%sunaff_%s=(%s)(uintptr_t)%s;" % (ind, typ, reg, cast, g)
gamecode = re.sub(r'(?m)^(\s*)([\w ]+\*? *)unaff_(ESI|EDI|EBX|EBP);$', _conv_unaff, gamecode)
# in_EAX: the dropped EAX input register (e.g. FUN_00456d27 dword-memset takes its dest in EAX). Bind
# to the emulated-register global _g_eax; callers set _g_eax before the call (patches wire them).
def _conv_ineax(m):
    ind, typ = m.group(1), m.group(2)
    if typ.split() and typ.split()[0] in ('return',):   # `return in_EAX;` is a statement, not a decl
        return m.group(0)
    cast = typ.replace(' ', '')
    return "%s%sin_EAX=(%s)(uintptr_t)_g_eax;" % (ind, typ, cast)
gamecode = re.sub(r'(?m)^(\s*)([A-Za-z_][\w ]*\*? *)in_EAX;$', _conv_ineax, gamecode)

# OT-buffer fix (= dd2.exe Allocate_OT_ FIX): pad the OT malloc with guard slots + zero it, so overruns
# don't corrupt adjacent heap (ClearOTagR/DrawOTag walk it). dd2h addrs: _DAT_007542ee/_DAT_0075437c.
gamecode = gamecode.replace("  _DAT_007542ee = MPE_malloc(param_1 << 2);",
  "  { char* _a=(char*)MPE_malloc((param_1+64)<<2); int _i; if(_a){for(_i=0;_i<((param_1+64)<<2);_i++)_a[_i]=0;} *(int*)GIMG(0x7542ee)=(int)(_a+(32<<2)); }")
gamecode = gamecode.replace("  _DAT_0075437c = MPE_malloc(param_1 << 2);",
  "  { char* _a=(char*)MPE_malloc((param_1+64)<<2); int _i; if(_a){for(_i=0;_i<((param_1+64)<<2);_i++)_a[_i]=0;} *(int*)GIMG(0x75437c)=(int)(_a+(32<<2)); }")


# OT byte-stride fix: the two OT buffers are 0x8e BYTES apart (_DAT_007542ee/_DAT_0075437c); after the
# pointer-type transfer, &DAT_007542ee is int* so `+ buffer_num*0x8e` scaled by 4 -> wrong. Byte-cast it.
gamecode = gamecode.replace("(&DAT_007542ee + buffer_num * 0x8e)", "((char*)&DAT_007542ee + buffer_num * 0x8e)")
gamecode = gamecode.replace("&DAT_007542ee + buffer_num * 0x8e", "(int*)((char*)&DAT_007542ee + buffer_num * 0x8e)")
gamecode = gamecode.replace("&db + buffer_num * 0x8e", "(int)((char*)&db + buffer_num * 0x8e)")
gamecode = gamecode.replace("&DAT_007542ee + buffer_num * 0x8e", "(char*)&DAT_007542ee + buffer_num * 0x8e")


# Load_Textures rect fix (= dd2.exe scattered-locals FIX): LoadImage reads its rect as a contiguous
# 8-byte struct, but Ghidra split it into local_18/16/14/12 (may not be laid out contiguously) -> garbage.
# Replace with a contiguous undefined2[4] array.
gamecode = gamecode.replace(
  "  undefined2 local_18;\n  undefined2 local_16;\n  undefined2 local_14;\n  undefined2 local_12;",
  "  undefined2 dd2_rect[4];  /* contiguous rect (was scattered local_18/16/14/12) */")
gamecode = gamecode.replace("    local_18 = *(undefined2 *)((int)param_2 + 6);","    dd2_rect[0] = *(undefined2 *)((int)param_2 + 6);")
gamecode = gamecode.replace("    local_16 = (undefined2)param_2[2];","    dd2_rect[1] = (undefined2)param_2[2];")
gamecode = gamecode.replace("    local_14 = (undefined2)((*param_2 >> 0x10) / 2);","    dd2_rect[2] = (undefined2)((*param_2 >> 0x10) / 2);")
gamecode = gamecode.replace("    local_12 = (undefined2)param_2[1];","    dd2_rect[3] = (undefined2)param_2[1];")
gamecode = gamecode.replace("    LoadImage((int *)&local_18,puVar3 + 1);","    LoadImage((int *)dd2_rect,puVar3 + 1);")


# asset-loader gap fix (= dd2.exe FUN_00415160): capture the real asset offset(entry[0])+size(entry[1])
# before MPE_free; use dd2_asset_off for the sector seek (not &DAT_0074ef18=the path buffer) and pass the
# EXACT dd2_asset_size to the handler (not puVar2[1]>>8 which is overwritten garbage).
gamecode = gamecode.replace(
  "    puVar2 = *(undefined4 **)(iVar5 + 0xc);\n    MPE_free(iVar5);",
  "    puVar2 = *(undefined4 **)(iVar5 + 0xc);\n    dd2_asset_off = *(int *)iVar5; dd2_asset_size = *(int *)(iVar5 + 4);\n    MPE_free(iVar5);")
gamecode = gamecode.replace("    puVar7 = &DAT_0074ef18;", "    puVar7 = (undefined1 *)dd2_asset_off;")
gamecode = gamecode.replace(
  "    iVar5 = (*(code *)(&File_Func_List)[(puVar2[1] & 0xff) >> 3])(*puVar2,(uint)puVar2[1] >> 8);",
  "    iVar5 = (*(code *)(&File_Func_List)[(puVar2[1] & 0xff) >> 3])(*puVar2,dd2_asset_size);")

open(OUT, 'w', encoding='utf-8', errors='surrogateescape').write(prologue + gamecode)
print("assemble_dd2h: %d functions kept, %d forward decls -> %s" % (gamecode.count('/* ===== '), len(sigs), OUT))
