#!/usr/bin/env python3
# Drive dd2h.exe (running under Wine on DISPLAY :99) via X11 XTEST fake key events,
# capturing a frame after each step. No xdotool/sudo needed (python-xlib + XTEST).
import sys, os, time, subprocess
os.environ['DISPLAY'] = ':99'
from Xlib import display, X
from Xlib.ext import xtest
from Xlib import XK

dpy = display.Display()
root = dpy.screen().root
OUT = sys.argv[1] if len(sys.argv) > 1 else '/home/cosmo/Git/wasm-dd2/out/ref_nav'
os.makedirs(OUT, exist_ok=True)

def keycode(name):
    ks = XK.string_to_keysym(name)
    return dpy.keysym_to_keycode(ks)

def tap(name, hold=0.06):
    kc = keycode(name)
    xtest.fake_input(dpy, X.KeyPress, kc); dpy.sync(); time.sleep(hold)
    xtest.fake_input(dpy, X.KeyRelease, kc); dpy.sync(); time.sleep(0.25)

def shot(tag):
    p = os.path.join(OUT, tag + '.png')
    subprocess.run(['import', '-window', 'root', '-display', ':99', p],
                   stderr=subprocess.DEVNULL)
    print('shot', tag, flush=True)

# step list from argv[2]: comma-separated "key" tokens; "shot:NAME" captures.
seq = sys.argv[2].split(',') if len(sys.argv) > 2 else ['shot:cur']
for tok in seq:
    tok = tok.strip()
    if tok.startswith('shot:'):
        shot(tok[5:])
    elif tok.startswith('wait:'):
        time.sleep(float(tok[5:]))
    elif tok:
        tap(tok)
print('done', flush=True)
