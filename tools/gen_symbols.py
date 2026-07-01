#!/usr/bin/env python3
# Generate dd2_symbols.h from a Ghidra data_symbols.txt (name<TAB>addr<TAB>size), preserving the manual
# TYPE refinements + hand-written extern/decl lines from an existing dd2_symbols.h (keyed by symbol NAME).
# Used to re-base the symbol map onto dd2h.exe (dd2h names/addresses) while keeping the accumulated typing.
#   usage: tools/gen_symbols.py <data_symbols.txt> <old dd2_symbols.h> <out dd2_symbols.h>
import sys, re
syms_f, old_f, out_f = sys.argv[1], sys.argv[2], sys.argv[3]
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
    if name in seen or not re.match(r'^[A-Za-z_]\w*$', name): continue
    seen.add(name)
    t = type_by_name.get(name, deftype(size))
    out.append(f"#define {name} (*({t}*)GIMG(0x{addr:08x}))")
    n += 1
open(out_f, 'w', encoding='utf-8', errors='surrogateescape').write('\n'.join(out) + '\n')
print(f"gen_symbols: {n} #defines from {syms_f}, {len(type_by_name)} type refinements preserved -> {out_f}")
