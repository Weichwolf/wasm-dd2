import re, struct
data=open('DestructionDerby2/Dirinfo','rb').read(); N=len(data)

# 1) Find BMP anchors: 'BM' + u32 filesize == 78390 (the COPYRIGH/LOADING size)
print("== BMP anchors ('BM' with embedded filesize 78390) ==")
anchors=[]
i=0
while True:
    i=data.find(b'BM', i)
    if i<0 or i+6>N: break
    sz=struct.unpack_from('<I',data,i+2)[0]
    if sz==78390:
        anchors.append(i); print(f"  BM @ 0x{i:08x}  embedded-size={sz}")
    i+=1
print("  (count:", len(anchors), ")")

# 2) Look at the record where contiguity broke (~index 114) and the bytes there
pat=re.compile(rb'[A-Za-z0-9_][A-Za-z0-9_\\./]{4,}\x00')
hits=[(m.start(), m.end()) for m in pat.finditer(data[:3_000_000])]
print("\n== records 108..122 (name, start, field-bytes up to next name) ==")
for k in range(108,123):
    s,e=hits[k]; nxt=hits[k+1][0] if k+1<len(hits) else e
    nm=data[s:e-1].decode('latin1'); fld=data[e:nxt]
    print(f"  [{k:3}] @0x{s:06x} {nm:24} +{len(fld):3}b: {fld[:24].hex(' ')}")

# 3) distinct directory prefixes in the front TOC
prefixes={}
for s,e in hits[:2000]:
    nm=data[s:e-1].decode('latin1')
    p=nm.split('\\')[0] if '\\' in nm else '(root)'
    prefixes[p]=prefixes.get(p,0)+1
print("\n== dir prefixes in first 2000 records ==")
for p,c in sorted(prefixes.items()): print(f"  {p:12} {c}")
