#!/usr/bin/env python3
# Generate dd2_symbols.h from a Ghidra data_symbols.txt (name<TAB>addr<TAB>size), preserving the manual
# TYPE refinements + hand-written extern/decl lines from an existing dd2_symbols.h (keyed by symbol NAME).
# Used to re-base the symbol map onto dd2h.exe (dd2h names/addresses) while keeping the accumulated typing.
#   usage: tools/gen_symbols.py <data_symbols.txt> <old dd2_symbols.h> <out dd2_symbols.h>
import sys, re
syms_f, old_f, out_f = sys.argv[1], sys.argv[2], sys.argv[3]
# exclude function names (they are real C functions in the decompile, not GIMG data)
func_names = set()
_dc = sys.argv[4] if len(sys.argv) > 4 else None
if _dc:
    import re as _re
    _t = open(_dc, encoding='utf-8', errors='surrogateescape').read()
    func_names = set(_re.findall(r'/\* ===== (\S+) @ ', _t))
    for _m in _re.finditer(r'\n[A-Za-z_][\w \*]*?\b(\w+)\s*\(', _t):
        func_names.add(_m.group(1))
# 1) manual type refinements + passthrough lines from the old header
type_by_name = {}
passthrough = []   # non-#define lines (externs, function decls, the header guard, includes)
defre = re.compile(r'^#define (\S+) \(\*\((\w[\w ]*?)\s*\*\)GIMG\(0x[0-9a-fA-F]+\)\)\s*$')
for line in open(old_f, encoding='utf-8', errors='surrogateescape'):
    m = defre.match(line)
    if m:
        type_by_name[m.group(1)] = m.group(2).strip()
    elif line.startswith('#define ') or 'GIMG(' in line:
        pass  # a #define we will regenerate; skip
    else:
        passthrough.append(line.rstrip('\n'))
# 2) default C type from symbol byte-size
def deftype(sz):
    return {1: 'undefined1', 2: 'undefined2', 4: 'undefined4'}.get(sz, 'undefined1')
# 3) emit
out = []
seen = set()
for name, txt in [(None, l) for l in passthrough]:  # header/externs first (already in order)
    out.append(txt)
n = 0
for line in open(syms_f, encoding='utf-8', errors='surrogateescape'):
    p = line.rstrip('\n').split('\t')
    if len(p) < 2: continue
    name, addr = p[0], int(p[1], 16)
    size = int(p[2]) if len(p) > 2 and p[2] else 0
    # Ghidra's LABEL export includes switch-case CODE labels (default/caseD_N/switchD*/joined_*)
    # and C keywords -- they are jump targets, not data; #define'ing them breaks the C parse.
    if name in ('default','case','switch','break','continue','else','goto','return') \
       or re.match(r'^(caseD_|switchD|joined_|LAB_)', name): continue
    if name in seen or name in func_names or not re.match(r'^[A-Za-z_]\w*$', name): continue
    seen.add(name)
    t = type_by_name.get(name, deftype(size))
    out.append(f"#define {name} (*({t}*)GIMG(0x{addr:08x}))")
    n += 1
# Emit address-encoded names (DAT_XXXXXXXX/PTR_DAT_.../LAB_.../UNK_...) referenced in the code but not
# in the Ghidra symbol export (dd2.exe auto-created these; the fresh dd2h analysis named differently). The
# name literally encodes the VA, so generate the GIMG define mechanically. Scan the decomp source for them.
import re as _re2
_src = open(_dc, encoding='utf-8', errors='surrogateescape').read() if _dc else ''
_encoded = _re2.compile(r'\b((?:DAT|PTR_DAT|PTR_FUN|PTR_LAB|LAB|UNK|PTR)_([0-9a-fA-F]{8}))\b')
for _m in _encoded.finditer(_src):
    _nm, _hx = _m.group(1), _m.group(2)
    if _nm in seen: continue
    seen.add(_nm)
    _t = type_by_name.get(_nm, 'undefined1' if _nm.startswith('DAT') or _nm.startswith('UNK') else 'undefined4')
    out.append(f"#define {_nm} (*({_t}*)GIMG(0x{int(_hx,16):08x}))")
    n += 1
open(out_f, 'w', encoding='utf-8', errors='surrogateescape').write('\n'.join(out) + '\n')
print(f"gen_symbols: {n} #defines from {syms_f}, {len(type_by_name)} type refinements preserved -> {out_f}")
