#!/usr/bin/env python3
"""Exercise incremental comparison and safe cleanup using copied source records.

These transport fixtures are copied from a verified original capture. They are
not output from a native or WASM engine, and establish no port parity.
"""
import argparse
import json
from pathlib import Path
import shutil
import time

from artifacts import WORK, check_space, open_files
from reference.video_archive import Records
from replay_video_stream import Comparison, FIELDS


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('original','reference','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();out=args.output.resolve()
    if WORK not in out.parents or out.exists():parser.error('fresh /tmp/wasm-dd2/ output required')
    out.mkdir();source=json.loads(args.reference.read_text());assert source['pass_'] and source['frame_count']>=2
    with Records(args.original) as records:raw=[records[i] for i in range(2)]
    cases={}
    for case in ('exact','pixel','palette','truncated','index','state','open-file'):
        directory=out/case;directory.mkdir();opened=None;stopped=[]
        comparison=Comparison(directory,args.original,source,2,lambda:stopped.append(True)).start()
        try:
            with (directory/'presentations.jsonl').open('w') as journal:
                for i in range(2):
                    row=dict(index=i,**{k:source['frames'][i][k] for k in FIELDS})
                    pixels=bytearray(raw[i][128:128+307200]);palette=bytearray(raw[i][128+307200:128+308224])
                    if i==0:
                        if case=='pixel':pixels[42]^=1
                        elif case=='palette':palette[42]^=1
                        elif case=='truncated':pixels=pixels[:-1]
                        elif case=='index':row['index']=1
                        elif case=='state':row['cf']+=1
                    (directory/f'f{i:05d}.bin').write_bytes(pixels)
                    (directory/f'f{i:05d}.pal').write_bytes(palette)
                    if case=='open-file' and i==0:opened=(directory/f'f{i:05d}.bin').open('rb')
                    journal.write(json.dumps(row)+'\n');journal.flush()
            deadline=time.monotonic()+10
            while len(comparison.rows)<2 and comparison.error is None and time.monotonic()<deadline:time.sleep(.01)
            try:proof=comparison.finish()
            except (ValueError,RuntimeError):
                assert case!='exact';proof=json.loads((directory/'comparison.json').read_text());assert not proof['pass_']
                assert (directory/'f00000.bin').exists(),'failed/open frame discarded'
                cases[case]=dict(rejected=True,compared=proof.get('compared_frames'),writer_stop_requested=bool(stopped))
            else:
                assert case=='exact' and proof['pass_'] and len(proof['frames'])==2
                assert not list(directory.glob('f*.bin')) and not list(directory.glob('f*.pal'))
                cases[case]=dict(pass_=True,frames=2,literal_bytes=2*(307200+1024),raw_removed=True)
        finally:
            if opened:opened.close()
    report=dict(scope=__doc__.strip(),pass_=True,source_video_sha256=source['video_sha256'],cases=cases)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    opened=open_files()
    for directory in [p for p in out.iterdir() if p.is_dir()]:
        for path in directory.rglob('*'):
            if path.is_file():
                stat=path.stat();assert (stat.st_dev,stat.st_ino) not in opened
        shutil.rmtree(directory)
    check_space(out);print('Verified incremental transport and six rejection cases; no port parity claim')


if __name__=='__main__':main()
