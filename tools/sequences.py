#!/usr/bin/env python3
"""Per-track race screenshot SEQUENCES: capture frames across a full race and assemble a
horizontal strip per track, demonstrating plausible racing over time (cars progressing)."""
import os, subprocess, glob
from PIL import Image, ImageDraw
HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RACE = os.path.join(HERE, 'build-native', 'dd2_race')
PLAYABLE = ['1','2','3','4','5','6','7','8','9','A','B']
N = 5                      # frames per strip
CELL = (300, 225)

def strip(lev):
    dat = os.path.join(HERE, 'assets', 'raw', f'LEV{lev}', 'LEVEL.DAT')
    outdir = os.path.join(HERE, 'out', f'seq_LEV{lev}')
    os.makedirs(outdir, exist_ok=True)
    for f in glob.glob(outdir+'/*.png'): os.remove(f)
    subprocess.run([RACE, dat, outdir, '2', '6', '1', '4.0'], capture_output=True, text=True)
    frames = sorted(glob.glob(outdir+'/frame*.png'))
    if not frames: return None
    pick = [frames[int(i*(len(frames)-1)/(N-1))] for i in range(N)]   # evenly spaced
    sheet = Image.new('RGB', (CELL[0]*N, CELL[1]), (12,13,16))
    dr = ImageDraw.Draw(sheet)
    for i, fp in enumerate(pick):
        im = Image.open(fp).convert('RGB').resize(CELL)
        sheet.paste(im, (i*CELL[0], 0))
        dr.rectangle([i*CELL[0], 0, i*CELL[0]+CELL[0]-1, 12], fill=(0,0,0))
        dr.text((i*CELL[0]+4, 1), f'LEV{lev} t{i+1}/{N}', fill=(230,200,120))
    out = os.path.join(HERE, 'out', f'seq_LEV{lev}.png')
    sheet.save(out)
    return out, len(frames)

if __name__ == '__main__':
    # combined sheet: one strip row per track
    rows = []
    for lev in PLAYABLE:
        r = strip(lev)
        if r: rows.append(r[0]); print(f'LEV{lev}: {r[1]} frames -> {r[0]}')
    imgs = [Image.open(p) for p in rows]
    W = max(im.width for im in imgs); H = sum(im.height for im in imgs)
    sheet = Image.new('RGB', (W, H), (12,13,16)); y = 0
    for im in imgs: sheet.paste(im, (0, y)); y += im.height
    sheet.save(os.path.join(HERE, 'out', 'all_sequences.png'))
    print('combined -> out/all_sequences.png')
