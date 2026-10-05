#!/usr/bin/env python3
"""Compare one complete original intro PCM stream with actual port sinks.

Original accepted and virtual-device-played S16 PCM must both equal complete
native SDL and browser WebAudio submissions, with no trimming or fitting.
This proves this captured stream, not stable driver padding across runs,
shared presentation/audio clocks, other movies or physical DAC timing.
"""
import argparse
import hashlib
import json
from pathlib import Path
from artifacts import WORK, check_space, prepare_output
from verify_configuration_persistence import EXE_SHA256, require
from verify_movie_codec import compare


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('capture','native','browser','output'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();output=prepare_output(args.output)
    require(WORK in output.parents,'Use /tmp/wasm-dd2/');output.mkdir(parents=True,exist_ok=False)
    summary=json.loads((args.capture/'audio/summary.json').read_text())
    require(summary['exe_sha256']==EXE_SHA256 and summary['exe_modified'] is False and
            summary['movie_autoskip'] is False,'unmodified complete original movie required')
    streams=[r for r in summary['streams'] if r['format']=='S16_LE']
    played=[r for r in summary['played_streams'] if r['format']=='S16_LE']
    require(len(streams)==len(played)==1,'unique original movie lifetime required')
    stream,device=streams[0],played[0]
    require(stream['closed'] is True and device['closed'] is True and
            stream['rate']==device['rate']==22050 and stream['channels']==device['channels']==2,
            'complete original PCM16 stereo stream at 22050Hz required')
    expected=args.capture/'audio'/stream['file'];actual_played=args.capture/'audio'/device['file']
    require(sha(expected)==stream['sha256'] and sha(actual_played)==device['sha256'] and
            expected.stat().st_size==stream['accepted_frames']*4 and
            actual_played.stat().st_size==device['played_frames']*4,'changed original movie PCM')
    count=compare(expected,actual_played)
    events=[json.loads(s) for s in (args.native/'events.jsonl').read_text().splitlines()]
    opens=[(i,e) for i,e in enumerate(events) if e['event']=='open' and e['device'] and
           e['format']==0x8010 and e['rate']==22050 and e['channels']==2]
    require(len(opens)==1,'unique native PCM16 movie lifetime required');begin,opened=opens[0]
    end=next(i for i in range(begin+1,len(events)) if events[i]['event']=='close' and events[i]['device']==opened['device'])
    queues=[e for e in events[begin+1:end] if e['event']=='queue' and e['device']==opened['device']]
    require(queues and len({e['stream'] for e in queues})==1,'unique native movie submission stream required')
    offset=0
    for e in queues:
        require(e['result']==0 and e['offset']==offset,'native accepted PCM journal differs');offset+=e['bytes']
    native=args.native/f"stream{queues[0]['stream']}.pcm"
    require(native.stat().st_size==offset==events[end]['bytes']==count,'native PCM lifetime extent differs')
    require(compare(expected,native)==count,'native whole original PCM differs')
    browser_report=json.loads((args.browser/'report.json').read_text())
    cases=[r for r in browser_report['cases'] if r['file']=='Intro.avi' and r['skip'] is False]
    require(len(cases)==1,'unique full browser intro required');case=cases[0]
    require(case['audioBuffers']==1 and case['audioErrors']==0 and case['rates']==[22050] and
            case['pcmBytes']==count,'actual browser source PCM format/extent differs')
    browser=args.browser/case['pcmFile']
    require(sha(browser)==case['acceptedPcmSha256'] and compare(expected,browser)==count,
            'browser whole original PCM differs')
    changed=bytearray(native.read_bytes());changed[1000]^=1
    altered=output/'altered-native.pcm';altered.write_bytes(changed)
    try:compare(expected,altered)
    except RuntimeError:pass
    else:raise ValueError('accepted changed native PCM bit')
    altered.unlink()
    report=dict(scope=__doc__,pass_=True,original_exe_sha256=EXE_SHA256,whole_pcm_bytes=count,
                original_accepted_and_played_sha256=sha(expected),native_sdl_sha256=sha(native),
                browser_webaudio_sha256=sha(browser),changed_native_bit_rejected=True,
                native_events_sha256=sha(args.native/'events.jsonl'),browser_report_sha256=sha(args.browser/'report.json'))
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(output)
    print('All whole original accepted/played and native/browser PCM bytes exact:',count)


if __name__=='__main__':main()
