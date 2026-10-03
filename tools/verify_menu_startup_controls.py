#!/usr/bin/env python3
"""Compare real original/native/browser frontend audio calls by presentation.

Both port captures must use normal application startup and real intro skip keys.
This checks source selection, controls and completed Flip counts, not live audio
sample timing or rendered video. Original source positions come from an existing
independent mixer/device report, never from searching output PCM.
"""
import argparse
import copy
import json
from pathlib import Path
import re

from artifacts import WORK, prepare_output
from verify_original_menu_audio import trace_sources
from reference.capture import EXE_SHA256


def original_controls(directory, mix_report):
    checkpoint=json.loads((directory/'checkpoint.json').read_text())
    report=json.loads(mix_report.read_text())
    api=trace_sources(directory/'wine.log')
    if (not report['pass_'] or report['original_capture']!=str(directory) or
            report['exe_sha256']!=EXE_SHA256 or checkpoint['exe_sha256']!=EXE_SHA256 or checkpoint['exe_modified'] or
            report['original_api']['trace_sha256']!=api['trace_sha256']):
        raise ValueError('Independent sample report does not describe this unmodified original')
    targets={api[name]['object']:name for name in ('effect','cd')}
    flips=0
    result=[]
    for line in (directory/'wine.log').read_text().splitlines():
        if 'ddraw_surface1_Flip iface' in line:
            if not re.search(r':0024:trace:ddraw:ddraw_surface1_Flip iface [0-9A-F]+, src 00000000, flags 0\.',line):
                raise ValueError('Unexpected original presentation path/thread')
            flips+=1
        match=re.search(r'IDirectSoundBufferImpl_Play \(([0-9A-F]+),',line)
        if match and match[1] in targets:
            name=targets[match[1]]
            call=next(play for play in api['plays'] if play['object']==match[1])
            result.append(dict(source=name,completed_flips=flips,
                bank_generations=api['bank_generations'],
                controls={key:call[key] for key in ('flags','position','pan','volume','frequency')},
                sample_frames=report['original_timeline']['starts'][name]))
    if [call['source'] for call in result]!=['effect','cd']:
        raise ValueError('Incomplete actual original startup calls')
    return result


def port_controls(directory):
    checkpoint=json.loads((directory/'checkpoint.json').read_text())
    if checkpoint.get('engine_state_writes') is not False or checkpoint['intro']['movie']!=1:
        raise ValueError('Actual unmodified-state application startup/intro required')
    for name in ('start_state','end_state'):
        state=checkpoint[name]
        if (state['movie']!=0 or state['level']!=0 or
                state['menu']!=dict(poly_list=0x4696b0,restart_cd_audio=0) or
                state['cd']!=dict(enabled=1,playing=1,**{'from':13,'to':14})):
            raise ValueError('Port did not stay in the actual main-menu/CD13 scope')
    buffers={}
    bank=[]
    plays=[]
    base=0
    for line in (directory/'sound.log').read_text().splitlines():
        prefix=re.fullmatch(r'cf(\d+) flip(-?\d+) sample(\d+) (.*)',line)
        if not prefix:
            raise ValueError('Missing complete port audio observation')
        cf,flip,sample,body=prefix.groups()
        cf,flip,sample=int(cf),int(flip),int(sample)
        if cf or flip<0:
            raise ValueError('Port did not use the frontend presentation counter')
        if body=='DirectSoundCreate -> OK':
            base=sample
            bank=[]
        match=re.fullmatch(r'DS CreateSoundBuffer flags=0x[0-9a-f]+ bytes=(\d+) freq=(\d+) channels=(\d+) bits=(\d+) -> (0x[0-9a-f]+)',body)
        if match:
            size,rate,channels,bits,obj=match.groups()
            buffers[obj]=dict(size=int(size),rate=int(rate),channels=int(channels),bits=int(bits),position=0)
            if int(size):
                bank.append(obj)
        match=re.fullmatch(r'Music Create frames=(\d+) -> (0x[0-9a-f]+)',body)
        if match:
            if len(bank)<45 or len(bank)%45:
                raise ValueError('Incomplete engine-loaded frontend bank')
            buffers[match[2]]=dict(music=True,frames=int(match[1]),position=0)
        match=re.fullmatch(r'DSB (0x[0-9a-f]+) SetCurrentPosition (\d+)',body)
        if match:
            buffers[match[1]]['position']=int(match[2])
        match=re.fullmatch(r'DSB (0x[0-9a-f]+) Play flags=(\d+) freq=(\d+) vol=(-?\d+) pan=(-?\d+)',body)
        if match:
            obj,flags,frequency,volume,pan=match.groups()
            if int(frequency)==0:
                continue # Primary-buffer transport, not a source.
            source=buffers[obj]
            if source.get('music'):
                name='cd'
            elif len(bank)>=45 and not len(bank)%45 and obj==bank[-45+42] and all(source[key]==value for key,value in
                    dict(size=6314,rate=11025,channels=1,bits=8).items()):
                name='effect'
            else:
                raise ValueError('Startup played a different engine source')
            plays.append(dict(source=name,completed_flips=flip,
                bank_generations=len(bank)//45,
                controls=dict(flags=int(flags),position=source['position'],pan=int(pan),
                              volume=int(volume),frequency=int(frequency)),sample_frames=sample-base))
    if [call['source'] for call in plays]!=['effect','cd']:
        raise ValueError('Missing, extra or reordered actual port startup plays')
    return plays


def compare(original, port):
    if len(original)!=2 or len(port)!=2:
        raise ValueError('Incomplete startup sequence')
    for wanted,actual in zip(original,port):
        if actual['source']!=wanted['source'] or actual['completed_flips']!=wanted['completed_flips']:
            raise ValueError('Startup audio call occurs at a different presentation')
        if actual['bank_generations']!=wanted['bank_generations']:
            raise ValueError('Port loaded a different number of original sound banks')
        # Wine MCI loops a finite streaming ring (flags=1). The port has the
        # complete finite CD range (flags=0), so its storage transport differs.
        # Both actual flags are checked, not reported as equal API controls.
        controls={**wanted['controls'],'flags':0}
        if actual['controls']!=controls or wanted['controls']['flags']!=(1 if wanted['source']=='cd' else 0):
            raise ValueError('Startup audio source controls differ')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original',type=Path,required=True)
    parser.add_argument('--original-mix-report',type=Path,required=True)
    parser.add_argument('--native',type=Path,required=True)
    parser.add_argument('--browser',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    output=prepare_output(args.report)
    if WORK not in output.parents or output.exists():
        parser.error('report must be a fresh file under /tmp/wasm-dd2/')
    original=original_controls(args.original.resolve(),args.original_mix_report.resolve())
    ports={name:port_controls(directory.resolve()) for name,directory in
           (('native',args.native),('browser',args.browser))}
    rejected=[]
    for name,calls in ports.items():
        compare(original,calls)
        mutations=[]
        for index in (0,1):
            bad=copy.deepcopy(calls);bad[index]['completed_flips']+=1
            mutations.append((f'{name}-{index}-presentation',bad))
        for key in ('frequency','volume','pan','position','flags'):
            bad=copy.deepcopy(calls);bad[0]['controls'][key]+=1
            mutations.append((f'{name}-effect-{key}',bad))
        bad=copy.deepcopy(calls);bad[1]['controls']['flags']=1
        mutations.append((f'{name}-cd-looping-whole-range',bad))
        mutations.extend(((f'{name}-reordered',list(reversed(calls))),
                          (f'{name}-truncated',calls[:1])))
        for label,bad in mutations:
            try:
                compare(original,bad)
            except ValueError:
                rejected.append(label)
            else:
                raise AssertionError('Damaged startup accepted: '+label)
    report=dict(pass_=True,scope='Actual frontend source selection, controls and '
        'presentation counts; full PCM, video and live scheduling equivalence remain open',
        original=original,ports=ports,negative_cases=rejected,
        cd_transport='Original loops the MCI streaming ring; port consumes the finite complete track range',
        sample_offsets_compared=False)
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS actual original/native/browser startup: effect after '
          f"{original[0]['completed_flips']} flips, CD after {original[1]['completed_flips']} flips; "
          f'{len(rejected)} damaged control sequences rejected. Sample timing remains open.')


if __name__=='__main__':
    main()
