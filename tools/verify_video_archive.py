#!/usr/bin/env python3
"""Verify lossless video storage through actual Wine DirectDraw calls.

A separate synthetic executable supplies known pixels, palettes and counters.
This verifies the forwarding observer and archive transport, not dd2h.exe game
behavior, frame timing, audio, or original/native/WASM parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess

from artifacts import WORK, check_space, open_files, run_bounded
from reference.ddraw_video_observer import build
from reference.video_archive import Collector, Records, RECORD_BYTES

ROOT = Path(__file__).resolve().parents[1]
COMPILER = ROOT / 'third_party/mingw-sdk/usr/bin/i686-w64-mingw32-gcc-win32'
NODE_READER = ROOT / 'tools/browser/video_record_reader.js'


def node_read(directory, negative=False):
    script = '''const {VideoRecords}=require(process.argv[1]);
const crypto=require('crypto');let records;
try { records=new VideoRecords(process.argv[2]);const sha=crypto.createHash('sha256');
 for(let i=0;i<records.count;i++)sha.update(records.record(i));
 console.log(JSON.stringify({frames:records.count,sha256:sha.digest('hex')}));
} finally {if(records)records.close();}'''
    result = subprocess.run(['node','-e',script,str(NODE_READER),str(directory)],
        stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,timeout=120)
    if negative:
        assert result.returncode != 0,'Node accepted damaged archive'
        return dict(rejected=True)
    result.check_returncode()
    return json.loads(result.stdout)


def verify(directory, frames):
    base = bytes((x//16+y//16)&255 for y in range(480) for x in range(640))
    sha = hashlib.sha256()
    with Records(directory) as records:
        assert len(records) == frames
        for i, raw in enumerate(records):
            header = struct.unpack_from('<32I', raw)
            assert header[:4] == (0x32564444,1,i+1,i+1)
            assert header[9] == i and header[13] == i*2
            assert header[17:24] == (640,480,640,8,307200,1024,1024)
            pixels = base.translate(bytes((p+i)&255 for p in range(256)))
            palette = bytes(value for p in range(256) for value in
                            ((p+i)&255,(p*3+i)&255,(p*7+i)&255,0))
            assert raw[128:128+307200] == pixels
            assert raw[128+307200:128+308224] == palette
            assert header[24] == 0 and raw[-1024:] == palette
            sha.update(raw)
        if records.manifest:
            assert records.manifest['raw_sha256'] == sha.hexdigest()
    return dict(frames=frames, literal_bytes=frames*RECORD_BYTES, sha256=sha.hexdigest())


def negatives(output, directory):
    cases = {}
    for kind in ('one-bit','truncated','reordered','raw-hash','oversized','extra-zlib-data'):
        broken = output / ('negative-'+kind)
        shutil.copytree(directory/'video-archive',broken/'video-archive')
        manifest = broken/'video-archive/manifest.json'
        m = json.loads(manifest.read_text())
        chunk = broken/'video-archive'/m['chunks'][0]['file']
        if kind in ('one-bit','truncated','extra-zlib-data'):
            raw = bytearray(chunk.read_bytes())
            if kind == 'one-bit':raw[len(raw)//2] ^= 1
            elif kind == 'truncated':raw = raw[:-1]
            else:
                raw += b'extra'
                m['chunks'][0]['compressed_bytes'] = len(raw)
                m['chunks'][0]['compressed_sha256'] = hashlib.sha256(raw).hexdigest()
            chunk.write_bytes(raw)
        elif kind == 'reordered':m['chunks'][0]['first'] = 1
        elif kind == 'raw-hash':m['chunks'][0]['raw_sha256'] = '0'*64
        else:m['chunks'][0]['frames'] = 129
        manifest.write_text(json.dumps(m))
        try:
            with Records(broken) as records:records[0]
        except (ValueError,IndexError):cases[kind] = dict(rejected=True)
        else:raise AssertionError('Damaged archive accepted: '+kind)
        cases[kind]['node'] = node_read(broken,negative=True)
        shutil.rmtree(broken)
    # Guard deletion of an actively open block, and a truncated closed tail.
    for kind in ('active-block','partial-record'):
        broken = output / ('negative-'+kind);broken.mkdir()
        collector = Collector(broken,1)
        path = broken/'video.bin.000000.part'
        with path.open('wb') as file:
            file.write(bytes(RECORD_BYTES if kind=='active-block' else 17));file.flush()
            if kind == 'active-block':
                try:collector.consume(path,final=True)
                except ValueError:cases[kind] = dict(rejected=True)
                else:raise AssertionError('Open block accepted')
                assert path.exists()
        if kind == 'partial-record':
            try:collector.finish()
            except ValueError:cases[kind] = dict(rejected=True)
            else:raise AssertionError('Truncated block accepted')
        assert path.exists()
        shutil.rmtree(broken)
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--frames',type=int,default=4101)
    args = parser.parse_args();output = args.output.resolve()
    if WORK not in output.parents or output.exists() or not 4096 < args.frames <= 60000:
        parser.error('Fresh /tmp/wasm-dd2 output and 4097..60000 frames required')
    output.mkdir(parents=True);observer = output/'observer';metadata = build(observer)
    source = ROOT/'tools/reference/ddraw_video_probe.c'
    executable = observer/'probe.exe'
    subprocess.run([str(COMPILER),'-O2','-Wall','-Wextra','-Werror',str(source),
        '-Wl,--image-base,0x400000','-lddraw','-o',str(executable)],check=True)
    env = {**os.environ,'WINEPREFIX':str(output/'wine-prefix'),'WINEARCH':'win32','WINEDEBUG':'-all'}
    report = dict(scope=__doc__.strip(),pass_=False,observer=metadata,
        probe_source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        probe_executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest())
    display = None
    try:
        with (output/'wine.log').open('wb') as log:
            display = subprocess.Popen(['Xvfb','-displayfd','1','-screen','0','640x480x16'],
                stdout=subprocess.PIPE,stderr=log)
            number = display.stdout.readline().decode().strip()
            if not number:raise RuntimeError('Xvfb failed')
            env['DISPLAY'] = ':'+number
            subprocess.run(['wineboot','--init'],env=env,stdout=log,stderr=log,check=True,timeout=60)
            results = {}
            for kind,frames in [('legacy',17),('archive',args.frames)]:
                directory = output/kind;directory.mkdir()
                for name in ('ddraw.dll','_ddraw_real.dll','probe.exe'):
                    shutil.copyfile(observer/name,directory/name)
                capture_env = {**env,'WINEDLLOVERRIDES':'ddraw=n;_ddraw_real=n',
                    'DD2_VIDEO_CAPTURE':'Z:'+str(directory/'video.bin').replace('/','\\')}
                collector = None
                if kind == 'archive':
                    capture_env.update(DD2_VIDEO_CHUNK_FRAMES='128',DD2_VIDEO_MAX_FRAMES=str(frames))
                    collector = Collector(directory).start()
                try:
                    with (directory/'probe.json').open('w') as proof:
                        run_bounded(['wine',str(directory/'probe.exe'),str(frames)],
                            directory=output,timeout=300,check=True,env=capture_env,cwd=directory,
                            stdout=proof,stderr=log)
                    assert json.loads((directory/'probe.json').read_text())['frames'] == frames
                finally:
                    subprocess.run(['wineserver','-k'],env=env,stdout=log,stderr=log,timeout=10,check=True)
                    subprocess.run(['wineserver','-w'],env=env,stdout=log,stderr=log,timeout=10,check=True)
                    if collector:report['archive'] = collector.finish()
                results[kind] = verify(directory,frames)
                node = node_read(directory)
                assert node['frames'] == frames and node['sha256'] == results[kind]['sha256']
                results[kind]['node'] = node
            report.update(pass_=True,targets=results,negative_cases=negatives(output,output/'archive'))
    finally:
        if display:display.terminate();display.wait(timeout=5)
        (output/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(output)
    # The durable report precedes raw cleanup; do not delete any live handles.
    opened = open_files()
    for directory in (output/'legacy',output/'archive',output/'wine-prefix'):
        for path in directory.rglob('*'):
            if path.is_file():
                stat = path.stat()
                assert (stat.st_dev,stat.st_ino) not in opened,str(path)
        shutil.rmtree(directory)
    print('Verified actual Wine video storage:',args.frames,'frames; no game parity claim')


if __name__ == '__main__':main()
