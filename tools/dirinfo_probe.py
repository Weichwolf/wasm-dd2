import re, sys
from collections import Counter
data=open('DestructionDerby2/Dirinfo','rb').read(); N=len(data)
pat=re.compile(rb'[A-Za-z0-9_][A-Za-z0-9_\\./]{4,}\x00')
hits=[(m.start(), m.end()) for m in pat.finditer(data)]
gaps=Counter(); detail=[]; contig=len(hits)
for k in range(len(hits)-1):
    ne=hits[k][1]; ns=hits[k+1][0]; g=ns-ne
    if g<0 or g>64:
        contig=k+1; break
    gaps[g]+=1
    if k<24:
        detail.append((data[hits[k][0]:hits[k][1]-1].decode('latin1'), g, data[ne:ns].hex(' ')))
print("file size:", N)
print("contiguous TOC records before first big gap:", contig)
print("gap histogram:", dict(sorted(gaps.items())))
print("\nfirst 24 records (name [gaplen] field-bytes):")
for nm,g,hx in detail: print(f"  {nm:26}[{g:2}] {hx}")
