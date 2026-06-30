#!/usr/bin/env python3
"""recover_regargs.py — programmatically recover the register-passed arguments that Ghidra
DROPS for DD2's non-standard register-convention functions (the GTE wrappers GTERPT/GTERPS/...
and the matrix fragments), by reading dd2h.exe's real disassembly instead of hand-guessing.

WHY: Ghidra decompiles ~90% of dd2h.exe faithfully (decompile->compile->identical). The residue
is functions that pass arguments in registers (esi/edi/ebx/ebp -> fixed globals) and call helper
fragments. Ghidra's decompiler models only stack args, so it emits bare `GTERPT();` / `unaff_ESI`
placeholders, losing the dataflow. This tool reconstructs that dataflow directly from the binary:
for each function it tracks the esi/edi/ebx/ebp register file across `mov reg,imm32` / `lea` and
emits, for every `call`, the recovered argument tuple. Output is a faithful spec the overlay (or a
generator) can use, with NO hand-guessing.

Usage:
  tools/recover_regargs.py <func_va> [<func_va> ...]      # e.g. 0x4139e9 0x413a29
  tools/recover_regargs.py --gte                          # the known GTE entry points
"""
import re, sys, subprocess

BIN = "DestructionDerby2/dd2h.exe"
REGS = ("eax", "ebx", "ecx", "edx", "esi", "edi", "ebp")

# Named globals we know, so output is readable (extend freely; unknowns print as hex).
KNOWN = {
    0x74c6f0: "ROTMATRIX", 0x74c702: "TRANSVEC",
    0x74c500: "IN_V0", 0x74c510: "IN_V1", 0x74c4e0: "IN_V2",
    0x74c540: "OUT_V0", 0x74c550: "OUT_V1", 0x74c520: "OUT_V2",
    0x4604b6: "SCRATCH_ROT",
}

def disasm(va_start, va_stop):
    out = subprocess.run(
        ["objdump", "-d", "-M", "intel",
         f"--start-address={va_start:#x}", f"--stop-address={va_stop:#x}", BIN],
        capture_output=True, text=True).stdout
    rows = []
    for ln in out.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\t[0-9a-f ]+\t(\S+)\s*(.*)", ln)
        if m:
            rows.append((int(m.group(1), 16), m.group(2), m.group(3).strip()))
    return rows

def name(v):
    return KNOWN.get(v, f"{v:#x}")

def recover(va):
    rows = disasm(va, va + 0x400)
    regfile = {}            # reg -> ("imm",val) | ("lea",expr)
    calls = []
    for addr, op, args in rows:
        if op == "mov":
            m = re.match(r"(e[a-z][a-z]),0x([0-9a-f]+)$", args)
            if m and m.group(1) in REGS:
                regfile[m.group(1)] = ("imm", int(m.group(2), 16))
                continue
            m = re.match(r"(e[a-z][a-z]),(e[a-z][a-z])$", args)
            if m and m.group(1) in REGS and m.group(2) in regfile:
                regfile[m.group(1)] = regfile[m.group(2)]
                continue
        elif op == "lea":
            m = re.match(r"(e[a-z][a-z]),(.*)$", args)
            if m and m.group(1) in REGS:
                regfile[m.group(1)] = ("lea", m.group(2))
                continue
        elif op == "call":
            m = re.match(r"0x([0-9a-f]+)", args)
            tgt = int(m.group(1), 16) if m else args
            snap = {}
            for r in ("esi", "edi", "ebx", "ebp"):
                if r in regfile:
                    k, v = regfile[r]
                    snap[r] = name(v) if k == "imm" else f"lea({v})"
            calls.append((addr, tgt, snap))
        elif op in ("ret", "retn"):
            break
    return calls

def main():
    av = sys.argv[1:]
    if not av or av[0] == "--gte":
        av = ["0x4139e9", "0x413a29", "0x4139a7"]   # GTERPT(1v), GTERPT(3v), GTERPS
    for a in av:
        va = int(a, 16)
        print(f"\n### function {va:#x} — recovered register-arg call sequence ###")
        for addr, tgt, snap in recover(va):
            sub = name(tgt) if isinstance(tgt, int) else tgt
            argstr = ", ".join(f"{r}={snap[r]}" for r in ("esi","edi","ebx","ebp") if r in snap)
            tgtname = sub if isinstance(tgt, str) else f"{tgt:#x}"
            print(f"  {addr:#x}: call {tgtname}({argstr})")

if __name__ == "__main__":
    main()
