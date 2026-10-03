#!/usr/bin/env python3
"""Export independently observed menu-device progress at real original Flips.

The mixer/device report must already have validated the complete bounded PCM.
Clock entries use only original mixer block counts, presentation events and the
accepted device extent. No waveform search, source control substitution or
engine-state injection is used. The completed intro device is outside scope.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from artifacts import WORK,prepare_output
from verify_menu_startup_controls import original_controls


def export_clock(capture, mix_report, output):
    output=prepare_output(output)
    if WORK not in output.parents or output.exists():
        raise ValueError('Clock must be a fresh file under /tmp/wasm-dd2/')
    calls=original_controls(capture,mix_report)
    report=json.loads(mix_report.read_text())
    timeline=report['original_timeline']
    accepted=report['targets']['native']['accepted']['compared_frames']
    blocks=timeline['blocks']
    index=0
    mixed=timeline['probe_frames']
    entries=[]
    devices=[]
    for number,line in enumerate((capture/'wine.log').read_text().splitlines(),1):
        match=re.search(r'DSOUND_PrimaryOpen \(([0-9A-F]+)\)',line)
        if match and match[1] not in devices:
            devices.append(match[1])
        while index<len(blocks) and blocks[index]['line']<=number:
            block=blocks[index]
            mixed=block['offset_frames']+block['frames']+timeline['probe_frames']
            index+=1
        if 'ddraw_surface1_Flip iface' in line:
            entries.append(min(mixed,accepted))
    for call in calls:
        if entries[call['completed_flips']]!=call['sample_frames']:
            raise ValueError('Observed Flip clock does not locate the actual source start; no refitting')
    if not entries or entries[-1]!=accepted or any(b<a for a,b in zip(entries,entries[1:])):
        raise ValueError('Incomplete/non-monotonic observed device clock')
    epoch=devices.index(timeline['device'])+1
    raw=struct.pack('<8sIIII',b'DD2AC01\0',44100,epoch,len(entries),0)
    raw+=struct.pack('<'+'Q'*len(entries),*entries)
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_bytes(raw)
    metadata=dict(scope='Observed menu-device clock input for real port application startup; '
        'intro device, live scheduling and rendered video excluded',
        sha256=hashlib.sha256(raw).hexdigest(),rate=44100,epoch=epoch,entries=len(entries),
        final_frames=accepted,probe_frames=timeline['probe_frames'],source_anchors=calls,
        original_capture=str(capture),original_mix_report=str(mix_report),
        original_trace_sha256=report['original_api']['trace_sha256'],
        accepted_pcm_sha256=report['targets']['native']['accepted']['original_sha256'],
        played_pcm_sha256=report['targets']['native']['original_sha256'],
        played_frames=report['targets']['native']['compared_frames'])
    output.with_suffix(output.suffix+'.metadata.json').write_text(json.dumps(metadata,indent=2)+'\n')
    return metadata


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,required=True)
    parser.add_argument('--mix-report',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    print(json.dumps(export_clock(args.capture.resolve(),args.mix_report.resolve(),args.output),indent=2))
