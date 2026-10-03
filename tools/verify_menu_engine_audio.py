#!/usr/bin/env python3
"""Compare real port frontend PCM with a complete bounded original device window.

The engine runs normal application/intro/frontend initialization. Only observed
device progress is replayed; no source controls or engine state are supplied.
Checks all accepted bytes and the independently consumed prefix, with no fitting,
trimming or Python recomposition. Intro/video/live clocks/hardware remain open.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

from artifacts import WORK,prepare_output,check_space,open_files
from verify_menu_startup_controls import original_controls,port_controls,compare
from verify_original_menu_audio import compare_pcm
from reference.audio import summarize_audio

ROOT=Path(__file__).resolve().parents[1]


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def validate_target(directory, metadata, original, accepted, played):
    checkpoint=json.loads((directory/'checkpoint.json').read_text())
    clock=checkpoint.get('device_clock',{})
    if (checkpoint.get('device_clock_sha256')!=metadata['sha256'] or
            clock.get('complete') is not True or clock.get('epoch')!=metadata['epoch'] or
            clock.get('entries')!=metadata['entries'] or clock.get('frames')!=metadata['final_frames'] or
            clock.get('completed_flips')<metadata['entries']-1):
        raise ValueError('Actual application did not complete the independently observed device clock')
    controls=port_controls(directory)
    compare(original,controls)
    for wanted,actual in zip(original,controls):
        if wanted['sample_frames']!=actual['sample_frames']:
            raise ValueError('Actual engine source starts differ at the observed sample clock')
    fmt=json.loads((directory/'mixed.pcm.json').read_text())
    if fmt!=dict(format='FLOAT_LE',rate=44100,channels=2):
        raise ValueError('Unexpected actual engine output format')
    pcm=(directory/'mixed.pcm').read_bytes()
    compare_pcm(pcm,accepted)
    compare_pcm(pcm[:len(played)],played)
    return dict(accepted_bytes=len(pcm),accepted_sha256=digest(pcm),played_bytes=len(played),
                played_sha256=digest(pcm[:len(played)]),controls=controls,
                device_clock=clock,binary_sha256=checkpoint.get('binary_sha256',checkpoint.get('wasm_sha256')))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original',type=Path,required=True)
    parser.add_argument('--mix-report',type=Path,required=True)
    parser.add_argument('--clock',type=Path,required=True)
    parser.add_argument('--native',type=Path,required=True)
    parser.add_argument('--browser',type=Path,required=True)
    parser.add_argument('--binary',type=Path,default=Path('/tmp/dd2_native'))
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--negative-runs',action='store_true',help='run actual native startup with damaged clocks')
    args=parser.parse_args()
    output=prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('output must be under /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=False)
    original_dir=args.original.resolve()
    clock_file=args.clock.resolve()
    raw=clock_file.read_bytes()
    metadata=json.loads(clock_file.with_suffix(clock_file.suffix+'.metadata.json').read_text())
    if digest(raw)!=metadata['sha256']:
        raise ValueError('Recorded device clock changed')
    original=original_controls(original_dir,args.mix_report.resolve())
    audio=summarize_audio(original_dir/'audio',write=False,require_played=True)
    accepted=(original_dir/'audio'/audio['streams'][-1]['file']).read_bytes()
    played=(original_dir/'audio'/audio['played_streams'][-1]['file']).read_bytes()
    if (digest(accepted)!=metadata['accepted_pcm_sha256'] or digest(played)!=metadata['played_pcm_sha256'] or
            accepted[:len(played)]!=played):
        raise ValueError('Original accepted/consumed device bytes changed')
    targets={name:validate_target(directory.resolve(),metadata,original,accepted,played)
             for name,directory in (('native',args.native),('browser',args.browser))}
    negative=[]
    if args.negative_runs:
        magic,rate,epoch,count,reserved=struct.unpack_from('<8sIIII',raw)
        entries=list(struct.unpack_from('<'+'Q'*count,raw,24))
        damaged={
            'header':b'INVALID!'+raw[8:],
            'rate':raw[:8]+struct.pack('<I',22050)+raw[12:],
            'truncated':raw[:-1],
            'extra-record':raw+b'\0'*8,
            'non-monotonic':raw[:32]+struct.pack('<Q',entries[0]-1)+raw[40:],
        }
        for source in ('effect','cd','both'):
            shifted=list(entries)
            first=original[0 if source in ('effect','both') else 1]['completed_flips']
            last=original[1]['completed_flips'] if source=='effect' else count
            ceiling=entries[last] if last<count else entries[-1]
            for index in range(first,last):
                shifted[index]=min(shifted[index]+1,ceiling)
            damaged[source+'-one-sample-late']=raw[:24]+struct.pack('<'+'Q'*count,*shifted)
        for name,clock in damaged.items():
            path=output/(name+'.clock');path.write_bytes(clock)
            directory=output/name
            command=[sys.executable,str(ROOT/'tools/capture_native_menu_startup.py'),
                     '--binary',str(args.binary.resolve()),'--audio-clock',str(path),'--output',str(directory)]
            with (output/(name+'.log')).open('wb') as log:
                result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,timeout=90)
            if name.endswith('one-sample-late'):
                if result.returncode:
                    raise ValueError('Shifted actual engine run failed before PCM comparison: '+name)
                # Same extent, real engine, same presentation/control paths.
                # Compare actual bytes directly, not just clock provenance.
                actual=(directory/'mixed.pcm').read_bytes()
                if len(actual)!=len(accepted) or actual==accepted:
                    raise ValueError('Damaged timing did not produce a same-length PCM difference: '+name)
                try:compare_pcm(actual,accepted)
                except ValueError:pass
                else:raise ValueError('Actual shifted PCM accepted: '+name)
                negative.append(dict(case=name,kind='actual-engine-PCM-rejected',bytes=len(actual),sha256=digest(actual)))
            else:
                logtext=(directory/'run.log').read_text(errors='replace')
                if not result.returncode or 'DD2_AUDIO_FRAME_CLOCK:' not in logtext:
                    raise ValueError('Damaged clock did not fail closed in the production device: '+name)
                negative.append(dict(case=name,kind='production-device-input-rejected'))
            check_space(output)
    report=dict(pass_=True,scope='Real native/browser application frontend menu-device PCM '
        'at independently observed original device progress; intro device, rendered video, '
        'live scheduling and physical sinks remain open',
        original_capture=str(original_dir),clock=metadata,targets=targets,negative_runs=negative)
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    # The report now contains digests, extents, real controls and negative evidence.
    # Keep journals/reports, remove successful and diagnosed raw PCM only.
    opened=open_files()
    for directory in (args.native,args.browser,output):
        for path in directory.rglob('*.pcm'):
            if not path.is_symlink():
                stat=path.stat()
                if (stat.st_dev,stat.st_ino) not in opened:path.unlink()
    print(f'PASS actual native/browser engine: {len(accepted)} accepted and {len(played)} '
          f'consumed original menu-device bytes per target; {len(negative)} actual negative runs.')


if __name__=='__main__':
    main()
