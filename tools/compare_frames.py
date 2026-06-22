#!/usr/bin/env python3
# Compare a reference frame (dd2h via Wine) vs a WASM frame. Resizes to 320x240, reports
# mean abs error, % pixels close, and a coarse structural score. node-free, pillow+numpy.
import sys, numpy as np
from PIL import Image
def load(p): return np.asarray(Image.open(p).convert('RGB').resize((320,240))).astype(np.float64)
a=load(sys.argv[1]); b=load(sys.argv[2])
mae=np.mean(np.abs(a-b)); close=np.mean(np.all(np.abs(a-b)<24,axis=2))*100
# coarse structural: correlation of luma
la=a@[.299,.587,.114]; lb=b@[.299,.587,.114]
la-=la.mean(); lb-=lb.mean()
ssim=float((la*lb).sum()/(np.sqrt((la*la).sum()*(lb*lb).sum())+1e-9))
print(f"MAE={mae:.1f}/255  pixels_close={close:.1f}%  luma_corr={ssim:.3f}")
