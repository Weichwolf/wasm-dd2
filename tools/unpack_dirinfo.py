#!/usr/bin/env python3
"""Unpack the Destruction Derby 2 `Dirinfo` archive.

Format (reverse-engineered):
  TOC at file start: a run of records, each = NAME '\0' + binary field.
  The common record has a 9-byte field:
      [flag u8][reserved u16=0][sector u16 LE][size u32 LE]
      data offset = sector * 2048   (CD sector units)
  A few header entries (the two BMPs and FONT.BNK) use a shorter variant;
  we resolve those from the unallocated gaps after placing all 9-byte files.

Proof of the format: consecutive LEV0 files tile end-to-end under
offset=sector*2048 (CLT_end == DAT_start == ...), and the last entry ends
within a sector of EOF.
"""
import os, re, sys, struct, json

SECTOR = 2048
HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DIRINFO = os.path.join(HERE, 'DestructionDerby2', 'Dirinfo')
OUT = os.path.join(HERE, 'assets', 'raw')

PATH_RE = re.compile(rb'[A-Za-z0-9_][A-Za-z0-9_\\./]{3,}\x00')

def parse_toc(data):
    """Walk records from offset 0 until the contiguous TOC ends."""
    recs = []
    pos = 0
    N = len(data)
    while pos < N:
        end = data.find(b'\x00', pos)
        if end < 0:
            break
        name = data[pos:end]
        if not (name and 32 <= name[0] < 127 and all(32 <= c < 127 for c in name)
                and (b'\\' in name or b'.' in name)):
            break
        field_start = end + 1
        # Next record name begins at the next printable-path run.
        m = PATH_RE.search(data, field_start)
        if not m:
            break
        nxt = m.start()
        field = data[field_start:nxt]
        if len(field) > 64:   # big gap => TOC ended (data region follows)
            recs.append((name.decode('latin1'), field_start, field[:9], True))
            break
        recs.append((name.decode('latin1'), field_start, field, False))
        pos = nxt
    return recs

def decode9(field):
    flag = field[0]
    sector = struct.unpack_from('<H', field, 3)[0]
    size = struct.unpack_from('<I', field, 5)[0]
    return flag, sector * SECTOR, size

def main():
    data = open(DIRINFO, 'rb').read()
    N = len(data)
    recs = parse_toc(data)
    print(f"Dirinfo: {N} bytes, parsed {len(recs)} TOC records\n")

    placed = []   # (offset, size, name, flag)
    special = []  # (name, field)
    for name, foff, field, last in recs:
        if len(field) == 9:
            flag, off, size = decode9(field)
            placed.append([off, size, name, flag])
        elif len(field) == 6:
            # COPYRIGH.BMP: [u16 sector][u32 size]
            sec = struct.unpack_from('<H', field, 0)[0]
            sz = struct.unpack_from('<I', field, 2)[0]
            placed.append([sec * SECTOR, sz, name, -1]); special.append((name, field))
        elif len(field) == 7:
            # LOADING.BMP: [u8 flag][u16 sector][u32 size]
            sec = struct.unpack_from('<H', field, 1)[0]
            sz = struct.unpack_from('<I', field, 3)[0]
            placed.append([sec * SECTOR, sz, name, field[0]]); special.append((name, field))
        elif len(field) == 10 and field[:4] == b'RAW\x00':
            # FONT.BNK: "RAW\0" + [u16 sector][u32 size]
            sec = struct.unpack_from('<H', field, 4)[0]
            sz = struct.unpack_from('<I', field, 6)[0]
            placed.append([sec * SECTOR, sz, name, -2]); special.append((name, field))
        else:
            special.append((name, field))

    placed.sort()
    # validate: within file, no overlap (beyond sector alignment)
    problems = 0
    covered = 0
    prev_end = 0
    for i, (off, size, name, flag) in enumerate(placed):
        end = off + size
        ok = (off >= 0 and end <= N)
        covered += size
        tag = "" if ok else "  <-- OUT OF RANGE"
        if not ok:
            problems += 1
        if i < len(placed) - 1:
            nxt = placed[i+1][0]
            gap = nxt - end
            if gap < -SECTOR:   # real overlap (more than alignment slack)
                tag += f"  <-- OVERLAP {gap}"
                problems += 1
        print(f"  {name:22} off=0x{off:08x} size={size:8d} end=0x{end:08x} flag=0x{flag:02x}{tag}")

    print(f"\n9-byte files: {len(placed)}, problems: {problems}")
    print(f"data covered by 9-byte files: {covered} bytes ({100*covered/N:.1f}% of file)")

    # gaps between placed files (candidate homes for special records)
    print("\nUnallocated gaps (>= 4 KB):")
    spans = sorted([(o, o+s) for o, s, _, _ in placed])
    cursor = 0
    gaps = []
    for o, e in spans:
        if o - cursor >= 4096:
            gaps.append((cursor, o))
            print(f"  gap 0x{cursor:08x} .. 0x{o:08x}  ({o-cursor} bytes)")
        cursor = max(cursor, e)
    if N - cursor >= 4096:
        gaps.append((cursor, N))
        print(f"  gap 0x{cursor:08x} .. 0x{N:08x}  ({N-cursor} bytes)  [tail]")

    print("\nSpecial (short-field) records:")
    for name, field in special:
        print(f"  {name:22} field({len(field)})={field.hex(' ')}")

    # extract 9-byte files
    for off, size, name, flag in placed:
        if off + size > N:
            continue
        p = os.path.join(OUT, name.replace('\\', '/'))
        os.makedirs(os.path.dirname(p), exist_ok=True)
        with open(p, 'wb') as f:
            f.write(data[off:off+size])
    print(f"\nExtracted {len(placed)} files -> {OUT}")

    # write manifest
    manifest = [{"name": n, "offset": o, "size": s, "flag": fl} for o, s, n, fl in placed]
    with open(os.path.join(HERE, 'assets', 'dirinfo_manifest.json'), 'w') as f:
        json.dump({"file_size": N, "sector": SECTOR, "files": manifest,
                   "special": [{"name": n, "field": fld.hex()} for n, fld in special]}, f, indent=2)

if __name__ == '__main__':
    main()
