#!/usr/bin/env python3
"""Compare actual port intro renderer/canvas bytes with the original window.

Every actual presentation is compared in order, without alignment, using a
bounded FIFO/HTTP readback. Original RGB is unchanged; only its
reserved BI_RGB byte uses opaque alpha. No reference pixels enter an engine.
This does not prove equal presentation clocks, audio endpoints or physical
display/DAC timing. The optional before binary must fail at the extra frame.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import select
import subprocess
import threading
import time

from artifacts import WORK, check_space, prepare_output, run_bounded
from verify_configuration_persistence import EXE_SHA256, ROOT, require
from verify_movie_codec import packets
from verify_movie_window import ARGB_BYTES, equal, unpack_record
from verify_native_sdl import config


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def original(capture):
    capture=capture.resolve();checkpoint=json.loads((capture/'checkpoint.json').read_text())
    require(checkpoint['exe_sha256']==EXE_SHA256 and checkpoint['exe_modified'] is False and
            checkpoint['end_state']['movie']==0,'completed unmodified original movie required')
    metadata=json.loads((capture/'movie-video-observer-build.json').read_text())
    for file,info in [('movie-video-observer.dll',metadata),('movie-gdi-observer.dll',metadata['gdi'])]:
        require(sha(capture/file)==info['observer_sha256'],'changed original movie observer')
    for name,expected in metadata['source_sha256'].items():
        require(sha(ROOT/'tools/reference'/name)==expected,'changed movie observer source')
    manifest=json.loads((capture/'movie-video-archive/manifest.json').read_text())
    require(manifest['pass_'] is True and manifest['record_bytes']==1446016 and
            manifest['frames']==len(manifest['records']),'complete source archive required')
    painted=[int(n) for n in re.findall(r'MCIAVI_PaintFrame Painting frame (\d+)',
                (capture/'wine.log').read_text(errors='replace'))]
    dimensions,compressed=packets(ROOT/'DestructionDerby2/Intro.avi')
    require(dimensions==(320,192) and painted==list(range(manifest['frames'])) and
            manifest['frames']==len(compressed)-1,'actual original exclusive full intro endpoint required')
    window_sha=hashlib.sha256()
    for index,entry in enumerate(manifest['records']):
        require(entry['serial']==index and entry['file']==f'{index:06d}.zlib','reordered source archive')
        _,row=unpack_record(capture,entry)
        require(row['packet']==compressed[index] and row['rectangle']==[0,48,640,384] and
                row['private_mci_window_draws']==0,'actual source packet/ordinal/rectangle differs')
        window_sha.update(row['window_argb'])
    return manifest,window_sha.hexdigest()


class NativeFrames:
    def __init__(self,case,capture,manifest):
        self.path=case/'video.pipe';os.mkfifo(self.path)
        self.fd=os.open(self.path,os.O_RDWR|os.O_NONBLOCK)
        self.capture=capture;self.manifest=manifest;self.frames=0;self.digest=hashlib.sha256()
        self.error=None;self.stop=threading.Event();self.thread=threading.Thread(target=self.read,daemon=True)
    def read(self):
        pending=bytearray()
        try:
            while True:
                ready=select.select([self.fd],[],[],.1)[0]
                if ready:
                    data=os.read(self.fd,ARGB_BYTES-len(pending))
                    if not data:continue
                    if self.frames==self.manifest['frames']:
                        raise ValueError('extra actual SDL movie frame after original endpoint')
                    pending.extend(data)
                    if len(pending)==ARGB_BYTES:
                        _,expected=unpack_record(self.capture,self.manifest['records'][self.frames])
                        actual=bytes(pending)
                        equal(expected['window_argb'],actual,f'actual SDL frame {self.frames}')
                        self.digest.update(actual);self.frames+=1;pending.clear()
                elif self.stop.is_set():break
            require(not pending and self.frames==self.manifest['frames'],'incomplete actual SDL movie stream')
        except BaseException as error:self.error=error
    def start(self):self.thread.start()
    def finish(self):
        self.stop.set();self.thread.join(timeout=10)
        require(not self.thread.is_alive(),'SDL movie FIFO consumer did not stop')
        os.close(self.fd);self.path.unlink()
        if self.error:raise self.error
        return dict(pass_=True,frames=self.frames,literal_window_argb_bytes=self.frames*ARGB_BYTES,
                    window_sha256=self.digest.hexdigest())


def native_run(binary,capture,manifest,observer,case):
    case.mkdir();frames=NativeFrames(case,capture,manifest);display=process=None;error=None
    try:
        with (case/'run.log').open('wb') as log:
            display=subprocess.Popen(['Xvfb','-displayfd','1','-screen','0','640x480x24'],stdout=subprocess.PIPE,stderr=log)
            number=display.stdout.readline().decode().strip();require(number,'native movie Xvfb failed')
            env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
            env.update(DISPLAY=':'+number,DD2_WINDOW='1',DD2_MOVIE='INTRO.AVI',SDL_AUDIODRIVER='dummy',
                       LD_PRELOAD=str(observer),DD2_NATIVE_MOVIE_OBSERVE=str(case),DD2_NATIVE_MOVIE_VIDEO_PIPE='1')
            frames.start();process=subprocess.Popen([str(binary)],cwd=ROOT/'DestructionDerby2',env=env,stdout=log,stderr=log)
            deadline=time.monotonic()+100
            while process.poll() is None:
                if frames.error:raise frames.error
                require(time.monotonic()<deadline,'native intro timed out')
                check_space(case);time.sleep(.05)
            require(process.returncode==0,'actual native movie process failed')
    except BaseException as caught:error=caught
    finally:
        if process and process.poll() is None:process.terminate();process.wait(timeout=5)
        if display:display.terminate();display.wait(timeout=5)
        try:result=frames.finish()
        except BaseException as caught:
            if error is None:error=caught
    if error:
        (case/'failure.json').write_text(json.dumps(dict(error=str(error),matched_original_frames=frames.frames),indent=2)+'\n')
        raise error
    events=[json.loads(s) for s in (case/'events.jsonl').read_text().splitlines()]
    presents=[e for e in events if e['event']=='present']
    require([e['frame'] for e in presents]==list(range(manifest['frames'])),'SDL journal/pipe frame count differs')
    result.update(binary_sha256=sha(binary),presentation_clock='actual observed wall clock; not synchronized to original')
    (case/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,required=True)
    parser.add_argument('--native',type=Path,required=True)
    parser.add_argument('--browser',type=Path,required=True)
    parser.add_argument('--before-native',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();output=prepare_output(args.output)
    require(WORK in output.parents,'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=False);capture=args.capture.resolve()
    manifest,window_sha=original(capture)
    observer=output/'observer.so'
    subprocess.run(['gcc','-m32','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror',*config('cflags'),
                    str(ROOT/'tools/native_movie_observer.c'),*config('libs'),'-ldl','-o',str(observer)],check=True)
    report=dict(scope=__doc__,pass_=False,original_exe_sha256=EXE_SHA256,
                original_manifest_sha256=sha(capture/'movie-video-archive/manifest.json'),
                observer_source_sha256=sha(ROOT/'tools/native_movie_observer.c'),
                browser_observer_source_sha256=sha(ROOT/'tools/browser/capture_movie_video.js'),
                original_frames=manifest['frames'],original_window_sha256=window_sha,targets={})
    if args.before_native:
        try:native_run(args.before_native.resolve(),capture,manifest,observer,output/'before-native')
        except ValueError as error:
            failure=json.loads((output/'before-native/failure.json').read_text())
            require(str(error)=='extra actual SDL movie frame after original endpoint' and
                    failure['matched_original_frames']==manifest['frames'],'before binary failed for another reason')
            report['before_native']=dict(rejected=True,binary_sha256=sha(args.before_native),**failure)
        else:raise ValueError('Accepted pre-876 extra final movie frame')
    report['targets']['native-sdl']=native_run(args.native.resolve(),capture,manifest,observer,output/'native')
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    run_bounded(['node',str(ROOT/'tools/browser/capture_movie_video.js'),str(args.browser.resolve()),
                 str(capture),str(output/'browser')],directory=output,check=True,timeout=480)
    report['targets']['browser-canvas']=json.loads((output/'browser/report.json').read_text())
    require(all(t['pass_'] is True and t['frames']==manifest['frames'] and t['window_sha256']==window_sha
                for t in report['targets'].values()),'original/port actual window streams differ')
    report['pass_']=True;(output/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(output)
    print('Every actual original intro window byte matches actual SDL and canvas:',manifest['frames'],flush=True)


if __name__=='__main__':main()
