#!/usr/bin/env python3
# DD2 pipeline: P-code -> C lifter (prototype).
# Reads the machine-parseable P-code (tools/ghidra/ExportPcode.java output) and emits a C
# function that models x86 registers+flags as a cpu_t struct and memory as a flat byte array.
# This is BIT-FAITHFUL: it emulates the exact x86 semantics, so the `unaff_EBX` register-ABI
# problem (and jump tables) vanish — registers are explicit state set by the caller.
# Usage: pcode_lift.py /tmp/pcode_fn.txt > lifted.c
import sys, re

CT = {1:'uint8_t', 2:'uint16_t', 4:'uint32_t', 8:'uint64_t'}
ST = {1:'int8_t', 2:'int16_t', 4:'int32_t', 8:'int64_t'}

def parse(path):
    insns=[]; fname='lifted'
    for line in open(path):
        line=line.rstrip('\n')
        if line.startswith('# FUNC'):
            m=re.search(r'# FUNC (\S+) @ ([0-9a-f]+)', line)
            if m: fname=m.group(1); entry=m.group(2)
        elif line.startswith('I\t'):
            _,addr,asm=line.split('\t',2); insns.append((addr,asm,[]))
        elif line.startswith('P\t'):
            f=line.split('\t')
            _,addr,seq,out,op,ins = f[0],f[1],f[2],f[3],f[4],(f[5] if len(f)>5 else '')
            insns[-1][2].append((out,op,[x for x in ins.split(',') if x]))
    return fname, entry, insns

GLOBAL = '--global' in sys.argv
if GLOBAL: sys.argv.remove('--global')
REGPFX = 'CPU.' if GLOBAL else 'C->'
uniques={}  # (off,size)->name
regs=set()
def vn_read(v):
    t,val,sz = v.split(':'); sz=int(sz)
    if t=='C': return f'(({CT[sz]})0x{val})'
    if t=='R': regs.add((val,sz)); return f'(*({CT[sz]}*)({REGPFX}r+0x{val}))'
    if t=='U': uniques[(val,sz)]=f'u{val}_{sz}'; return f'u{val}_{sz}'
    if t=='M': return f'(*({CT[sz]}*)(MEM+0x{val}))'
    raise ValueError(v)
def sz_of(v): return int(v.split(':')[2])
def s(v):  # signed read
    sz=sz_of(v); return f'(({ST[sz]}){vn_read(v)})'

def emit_op(out,op,ins,lines):
    def w(expr): lines.append(f'    {assign(out)}{expr};')
    a = ins[0] if ins else None
    b = ins[1] if len(ins)>1 else None
    osz = sz_of(out) if out!='-' else (sz_of(a) if a else 4)
    if op=='COPY': w(vn_read(a))
    elif op=='INT_ADD': w(f'{vn_read(a)} + {vn_read(b)}')
    elif op=='INT_SUB': w(f'{vn_read(a)} - {vn_read(b)}')
    elif op=='INT_MULT': w(f'{vn_read(a)} * {vn_read(b)}')
    elif op=='INT_AND': w(f'{vn_read(a)} & {vn_read(b)}')
    elif op=='INT_OR':  w(f'{vn_read(a)} | {vn_read(b)}')
    elif op=='INT_XOR': w(f'{vn_read(a)} ^ {vn_read(b)}')
    elif op=='INT_LEFT': w(f'{vn_read(a)} << {vn_read(b)}')
    elif op=='INT_RIGHT': w(f'{vn_read(a)} >> {vn_read(b)}')   # logical (unsigned types)
    elif op=='INT_SRIGHT': w(f'{s(a)} >> {vn_read(b)}')
    elif op=='INT_SLESS': w(f'{s(a)} < {s(b)}')
    elif op=='INT_LESS':  w(f'{vn_read(a)} < {vn_read(b)}')
    elif op=='INT_EQUAL': w(f'{vn_read(a)} == {vn_read(b)}')
    elif op=='INT_NOTEQUAL': w(f'{vn_read(a)} != {vn_read(b)}')
    elif op=='INT_SEXT': w(f'({CT[osz]})({ST[osz]}){s(a)}')
    elif op=='INT_ZEXT': w(f'({CT[osz]}){vn_read(a)}')
    elif op=='INT_SDIV': w(f'{s(a)} / {s(b)}')
    elif op=='INT_SREM': w(f'{s(a)} % {s(b)}')
    elif op=='INT_DIV':  w(f'{vn_read(a)} / {vn_read(b)}')
    elif op=='INT_REM':  w(f'{vn_read(a)} % {vn_read(b)}')
    elif op=='POPCOUNT': w(f'__builtin_popcount({vn_read(a)})')
    elif op=='BOOL_AND': w(f'{vn_read(a)} & {vn_read(b)}')
    elif op=='BOOL_OR':  w(f'{vn_read(a)} | {vn_read(b)}')
    elif op=='BOOL_NEGATE': w(f'!{vn_read(a)}')
    elif op=='SUBPIECE':
        shift=int(b.split(':')[1],16)*8
        w(f'({CT[osz]})({vn_read(a)} >> {shift})')
    elif op=='INT_CARRY':   # unsigned overflow (carry) of a+b — clang-compatible builtin
        sz=sz_of(a); w(f'OFL_ADD({CT[sz]}, {vn_read(a)}, {vn_read(b)})')
    elif op=='INT_SCARRY':  # signed overflow of a+b
        w(f'OFL_ADD({ST[sz_of(a)]}, {s(a)}, {s(b)})')
    elif op=='INT_SBORROW': # signed overflow of a-b
        w(f'OFL_SUB({ST[sz_of(a)]}, {s(a)}, {s(b)})')
    elif op=='LOAD':  # out = *(ptr) ; ins=[space, ptr]
        w(f'(*({CT[osz]}*)(MEM + {vn_read(ins[1])}))')
    elif op=='STORE': # *(ptr)=val ; ins=[space, ptr, val]
        lines.append(f'    *({CT[sz_of(ins[2])]}*)(MEM + {vn_read(ins[1])}) = {vn_read(ins[2])};')
    elif op=='CBRANCH':
        tgt=int(ins[0].split(':')[1],16)
        lines.append(f'    if ({vn_read(ins[1])}) goto L_{tgt:x};')
    elif op=='BRANCH':
        lines.append(f'    goto L_{int(ins[0].split(":")[1],16):x};')
    elif op=='CALL' and GLOBAL:
        tgt=int(ins[0].split(':')[1],16)
        lines.append(f'    lifted_{tgt:x}(); /* CALL */')
    elif op=='CALLIND' and GLOBAL:   # indirect call (vtables, fn-ptr tables) -> global addr dispatch
        lines.append(f'    lifted_dispatch({vn_read(ins[0])}); /* CALLIND */')
    elif op=='BRANCHIND' and GLOBAL: # jump table (the 2nd un-decompilable class) -> computed goto over THIS fn's labels
        lines.append(f'    switch ({vn_read(ins[0])}) {{')
        for lab in FUNC_LABELS:
            lines.append(f'      case 0x{lab:x}: goto L_{lab:x};')
        lines.append('      default: return; /* unknown indirect target */')
        lines.append('    }')
    elif op in ('RETURN','CALLIND','CALL','BRANCHIND'):
        lines.append(f'    return; /* {op} (TODO indirect) */')
    else:
        lines.append(f'    /* UNHANDLED {op} {ins} */')

def assign(out):
    if out=='-': return ''
    t,val,sz=out.split(':'); sz=int(sz)
    if t=='R': regs.add((val,sz)); return f'*({CT[sz]}*)({REGPFX}r+0x{val}) = '
    if t=='U': uniques[(val,sz)]=f'u{val}_{sz}'; return f'u{val}_{sz} = '
    if t=='M': return f'*({CT[sz]}*)(MEM+0x{val}) = '
    raise ValueError(out)

FUNC_LABELS=[]
def emit_function(path):
    global uniques, FUNC_LABELS
    uniques={}
    fname, entry, insns = parse(path)
    FUNC_LABELS=[int(a,16) for a,_,_ in insns]   # for BRANCHIND computed-goto within this fn
    body=[]
    for addr,asm,ops in insns:
        body.append(f'  L_{int(addr,16):x}:; LIFT_TRACE(0x{int(addr,16):x}); /* {asm} */')
        for out,op,ins in ops:
            emit_op(out,op,ins,body)
    body.append('    return;')
    nm=f'lifted_{int(entry,16):x}'
    sig = f'void {nm}(void)' if GLOBAL else f'void {nm}(cpu_t* C, uint8_t* MEM)'
    out=[f'/* lifted from x86 @ {entry} ({fname}) — bit-faithful P-code emulation */', sig+' {']
    seen={}
    for (off,sz),name in uniques.items(): seen[name]=CT[sz]
    for name in sorted(seen): out.append(f'  {seen[name]} {name} = 0;')
    out += body + ['}']
    return int(entry,16), '\n'.join(out)

# header (once)
print('#include <stdint.h>')
# overflow predicates via clang/gcc-portable statement-expression builtins (emscripten has no _p form)
print('#define OFL_ADD(T,a,b) ({ T _r; __builtin_add_overflow((T)(a),(T)(b),&_r); })')
print('#define OFL_SUB(T,a,b) ({ T _r; __builtin_sub_overflow((T)(a),(T)(b),&_r); })')
print('#ifdef LIFT_TRACE_ON')
print('extern void lift_trace(unsigned);')
print('#define LIFT_TRACE(a) lift_trace(a)')
print('#else')
print('#define LIFT_TRACE(a)')
print('#endif')
print('typedef struct { uint8_t r[4096]; } cpu_t;  /* flat x86 register space (offset-addressed; sub-regs alias). MUST cover high offsets: GP 0x0-0x1c, flags 0x200-0x20b, etc. — 512 was too small and flag writes corrupted adjacent globals. */')
if GLOBAL:
    print('cpu_t CPU; uint8_t* MEM;')
fns=[emit_function(p) for p in sys.argv[1:]]
# forward decls so inter-function CALLs resolve
for e,_ in fns:
    print(f'void lifted_{e:x}(void);' if GLOBAL else f'void lifted_{e:x}(cpu_t*,uint8_t*);')
if GLOBAL:
    # indirect-call dispatch: runtime address -> lifted fn (vtables, OT dispatch, fn-ptr tables)
    print('void lifted_dispatch(uint32_t a){ switch(a){')
    for e,_ in fns:
        print(f'  case 0x{e:x}: lifted_{e:x}(); return;')
    print('  default: return; /* external/compat target — wire to shim */ } }')
for _,code in fns:
    print(code)
