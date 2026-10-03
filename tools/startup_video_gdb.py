"""GDB-only first-presentation observer for the unmodified original application.

One hardware breakpoint at the return of the actual primary Flip call. No
inferior memory/register writes; every recorded presentation must return DD_OK.
Debugger timing excludes live/audio/display-hardware equivalence from scope.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import gdb
from artifacts import check_space


def record_startup(output, frames, skip_movie=True):
    directory=Path(output)/'startup'
    directory.mkdir()
    inferior=gdb.selected_inferior()
    def read(address,size):
        return bytes(inferior.read_memory(address,size))
    def integer(address):
        return int.from_bytes(read(address,4),'little',signed=True)
    if integer(0x462cd4)!=1:
        raise RuntimeError('Observer was not attached during the actual initial movie')
    records=[]
    # Unmodified PE 412cbe: call [ebx+0x2c], 412cc1: mov ebx,eax.
    breakpoint=gdb.Breakpoint('*0x412cc1',type=gdb.BP_HARDWARE_BREAKPOINT)
    breakpoint.silent=True
    if skip_movie:
        subprocess.run(['xdotool','search','--name','PC-DD2','windowfocus','key','Escape'],
                       env=os.environ,check=True,timeout=5)
    try:
        for index in range(frames):
            gdb.execute('continue')
            if int(gdb.parse_and_eval('$pc'))!=0x412cc1 or int(gdb.parse_and_eval('$eax'))!=0:
                raise RuntimeError('Original primary Flip did not return DD_OK at its actual call site')
            if integer(0x936ff4)!=0 or integer(0x462cd4)!=0 or integer(0x462ff0)!=0:
                raise RuntimeError('Startup capture left the frontend scope')
            prefix=f'frame{index:05d}'
            framebuffer=read(0x700450,307200)
            palette=read(0x700050,1024)
            (directory/(prefix+'.bin')).write_bytes(framebuffer)
            (directory/(prefix+'.pal')).write_bytes(palette)
            records.append(dict(index=index,prefix=prefix,cf=integer(0x462ff0),level=integer(0x936ff4),
                poly_list=integer(0x940010),restart_cd_audio=integer(0x467420),
                cd_playing=integer(0x462d70),highlight_phase=integer(0x4699cc),
                framebuffer_sha256=hashlib.sha256(framebuffer).hexdigest(),
                palette_sha256=hashlib.sha256(palette).hexdigest()))
            check_space(output)
    finally:
        breakpoint.delete()
    result=dict(stage='Successful original primary Flip return @0x412cc1',frames=records,
        initial_movie_observed=True,intro_skip=skip_movie,input='real X11 Escape' if skip_movie else 'complete intro',
        breakpoints='one hardware breakpoint; no inferior memory/register writes',
        scope='First frontend presentations from actual application initialization; '
              'intro output, live timing, audio and physical display not compared')
    (directory/'startup.json').write_text(json.dumps(result,indent=2)+'\n')
    print(f'Original startup video: all {len(records)} first presentations -> {directory}',flush=True)
