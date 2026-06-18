#!/usr/bin/env python3
"""Build a labeled 4x4 montage of all 16 level addresses LEV0..LEVF: a mid-race frame for
each playable track, ABSENT/NON-TRACK markers otherwise. One artifact proving track state."""
import os, subprocess, glob
from PIL import Image, ImageDraw, ImageFont
HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RACE = os.path.join(HERE, 'build-native', 'dd2_race')
CELL = (360, 270)
LEVELS = ['0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F']

def midframe(lev):
    dat = os.path.join(HERE, 'assets', 'raw', f'LEV{lev}', 'LEVEL.DAT')
    if not os.path.exists(dat):
        return None, 'ABSENT'
    outdir = os.path.join(HERE, 'out', f'mont_LEV{lev}')
    os.makedirs(outdir, exist_ok=True)
    for f in glob.glob(outdir+'/*.png'): os.remove(f)
    r = subprocess.run([RACE, dat, outdir, '2', '6', '1', '6.0'],
                       capture_output=True, text=True)
    frames = sorted(glob.glob(outdir+'/frame*.png'))
    if not frames:
        return None, 'NON-TRACK'
    mid = frames[len(frames)//2]
    kind = 'arena' if 'arena' in r.stdout else 'circuit'
    return mid, kind

def main():
    cols, rows = 4, 4
    W, H = cols*CELL[0], rows*CELL[1]
    sheet = Image.new('RGB', (W, H), (12,13,16))
    dr = ImageDraw.Draw(sheet)
    for i, lev in enumerate(LEVELS):
        cx, cy = (i % cols)*CELL[0], (i//cols)*CELL[1]
        mid, kind = midframe(lev)
        if mid:
            im = Image.open(mid).convert('RGB').resize(CELL)
            sheet.paste(im, (cx, cy))
            label = f'LEV{lev}  {kind}'
        else:
            dr.rectangle([cx+2,cy+2,cx+CELL[0]-2,cy+CELL[1]-2], outline=(60,60,70))
            dr.text((cx+CELL[0]//2-30, cy+CELL[1]//2), kind, fill=(150,150,160))
            label = f'LEV{lev}'
        dr.rectangle([cx, cy, cx+CELL[0]-1, cy+14], fill=(0,0,0))
        dr.text((cx+4, cy+2), label, fill=(230,200,120))
    out = os.path.join(HERE, 'out', 'all_tracks_montage.png')
    sheet.save(out)
    print('saved', out)

if __name__ == '__main__':
    main()
