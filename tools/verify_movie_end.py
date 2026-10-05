#!/usr/bin/env python3
"""Observe Wine's actual exclusive default AVI play end near each film's end.

A real MCI seek bounds this API fixture. It does not prove chronological
original/port output, playback clocks, PCM, or full-movie pixels.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

from artifacts import WORK, check_space, prepare_output
from verify_movie_codec import packets
from verify_sound_cursor import wine_probe

ROOT=Path(__file__).resolve().parent.parent
SOURCE=ROOT/'tools/movie_end_test.c'


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mingw',default='i686-w64-mingw32-gcc')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();output=prepare_output(args.output)
    if WORK not in output.parents:raise ValueError('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=False)
    exe=output/'movie-end.exe'
    subprocess.run([args.mingw,'-O2','-Wall','-Wextra','-Werror',str(SOURCE),
                    '-lwinmm','-o',str(exe)],check=True)
    report=dict(scope=__doc__,pass_=False,source_sha256=sha(SOURCE),
                fixture_sha256=sha(exe),wine_version=subprocess.check_output(['wine','--version'],text=True).strip(),films=[])
    env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    for filename in ('Intro.avi','Outro.avi'):
        film=output/Path(filename).stem.lower();film.mkdir()
        path=ROOT/'DestructionDerby2'/filename
        dimensions,compressed=packets(path);count=len(compressed);start=count-2
        result=wine_probe(exe,film,env,arguments=('Z:'+str(path).replace('/','\\'),str(start)),wine_debug='-all,+mciavi')
        shutil.rmtree(film/'wine-prefix');check_space(output)
        trace=(film/'wine.log').read_text(errors='replace')
        plays=list(re.finditer(r'MCIAVI_mciPlay Playing from frame=(\d+) to frame=(\d+)',trace))
        if len(plays)!=1:raise RuntimeError('Missing/ambiguous actual MCI default play endpoint')
        painted=[int(n) for n in re.findall(r'MCIAVI_PaintFrame Painting frame (\d+)',trace[plays[0].end():])]
        observed=list(map(int,plays[0].groups()))
        if result!=dict(start=start,default_play_completed=True) or observed!=[start,count-1] or painted!=[start]:
            raise RuntimeError(f'Actual Wine default play end differs: {observed}, {painted}')
        report['films'].append(dict(file=filename,avi_sha256=sha(path),dimensions=dimensions,
            source_frames=count,observed_play_from=observed[0],observed_exclusive_play_to=observed[1],
            actual_painted_frames=painted,wine_trace_sha256=sha(film/'wine.log')))
        (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        print('Observed actual MCI default exclusive end:',filename,observed,painted,flush=True)
    report['pass_']=True;(output/'report.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':main()
