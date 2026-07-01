#!/usr/bin/env python3
"""Assemble the dd2h build source (re_out/dd2.c) from the pristine dd2h decompile.
Part of the dd2.exe->dd2h re-base. Steps:
  1. Take the game-code portion of the dd2h decompile (re_out/dd2_decomp.c), excluding statically-linked
     MSVC CRT functions (>=0x45a000, or known libc names, or x87-intrinsic bodies) -- libc/compat provide them.
  2. Prepend: includes + GTE register-file globals (dd2h addrs) + forward declarations (from the decomp's
     function markers, excluding compat/libc names).
Output: re_out/dd2.c  (then transpile applies the fixes as usual).
"""
import re, sys
DECOMP = sys.argv[1] if len(sys.argv) > 1 else "re_out/dd2_decomp.c"
OUT    = sys.argv[2] if len(sys.argv) > 2 else "re_out/dd2.c"
CRT = {'atoi','fopen','fputs','fwrite','printf','remove','strcmp','strdup','strtod','sprintf','vsprintf',
       'fclose','fread','fseek','malloc','free','memcpy','memset','strcpy','strlen','strcat','strncmp',
       'qsort','fprintf','exit','ftell','abort','realloc','calloc','strncpy','atol','rand','srand'}
compat = set(re.findall(r'\b(\w+)\s*\(', open("re_out/ghidra_compat.h").read()))
exclude = compat | CRT
decomp = open(DECOMP, encoding='utf-8', errors='surrogateescape').read()
# 1. keep only game-code functions
parts = re.split(r'(/\* ===== \S+ @ [0-9a-fA-F]+ ===== \*/)', decomp)
kept = [parts[0]]
i = 1
while i < len(parts):
    marker, body = parts[i], parts[i+1] if i+1 < len(parts) else ''
    m = re.match(r'/\* ===== (\S+) @ ([0-9a-fA-F]+) ===== \*/', marker)
    name, addr = m.group(1).strip('"'), int(m.group(2), 16)
    if not (addr >= 0x45a000 or name in CRT or any(x in body for x in ('__fdiv','__fmul','sti_st','__fld','__fst','->_flag','->_ptr','->_cnt','->_base','->_bufsiz','->_file','->_charbuf'))):
        kept += [marker, body]
    i += 2
gamecode = "".join(kept)
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
        if s == '{': break
        if s and not s.startswith(('/*','*')): sig.append(s)
        j += 1
    if not sig or '(' not in ' '.join(sig): continue
    seen.add(name); sigs.append(' '.join(sig) + ';')
prologue = ('#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n#include <stdio.h>\n'
            'int _g_esi=0x74c500,_g_edi=0x4604b6,_g_ebx=0x74c500,_g_ebp=0x74c540;\n'
            '/* forward declarations (decomp markers, excl. compat/libc) */\n' + "\n".join(sigs) + "\n\n")
open(OUT, 'w', encoding='utf-8', errors='surrogateescape').write(prologue + gamecode)
print("assemble_dd2h: %d functions kept, %d forward decls -> %s" % (gamecode.count('/* ===== '), len(sigs), OUT))
