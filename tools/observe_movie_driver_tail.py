#!/usr/bin/env python3
"""Observe movie source completion and subsequent virtual-device silence.

Write call brackets and consumed sample intervals use CLOCK_MONOTONIC.
A call overlapping the source endpoint does not prove which came first.
This reports actual driver output; it does not establish full original/port
PCM or clock parity, or physical DAC timing.
"""
import argparse
import hashlib
import json
from pathlib import Path

from artifacts import WORK, check_space, prepare_output
from reference.audio import summarize_audio
from verify_configuration_persistence import ROOT, EXE_SHA256, require
from verify_movie_avi import original_metadata


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--movie', choices=('Intro.avi', 'Outro.avi'), default='Intro.avi')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args();output = prepare_output(args.output)
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False)
    capture = args.capture.resolve()
    metadata, _, _, _ = original_metadata(ROOT/'DestructionDerby2'/args.movie)
    count = metadata['pcm_frames']
    audio = summarize_audio(capture/'audio', write=False, require_played=True)
    accepted = [r for r in audio['streams'] if r['format']=='S16_LE']
    played = [r for r in audio['played_streams'] if r['format']=='S16_LE']
    require(len(accepted)==len(played)==1, 'unique movie device required')
    accepted, played = accepted[0], played[0]
    require(accepted['closed'] and played['closed'] and
            accepted['rate']==played['rate']==metadata['pcm_rate']==22050 and
            accepted['channels']==played['channels']==2 and played['played_frames']>=count,
            'closed complete source format required')
    end = next(s['begin_ns']+((count-s['offset_frames'])*1_000_000_000+played['rate']-1)//played['rate']
               for s in played['segments'] if s['offset_frames']<count<=s['offset_frames']+s['frames'])
    events = [json.loads(s) for s in (capture/'audio'/accepted['events']).read_text().splitlines()]
    zeros = [e for e in events if e['event']=='write' and e['accepted']>0 and
             e['offset_frames']+e['accepted']>count]
    for stream in (accepted, played):
        with (capture/'audio'/stream['file']).open('rb') as file:
            file.seek(count*4);require(not any(file.read()), 'nonzero output after declared source')
    first = zeros[0] if zeros else None
    period = played['period_frames']
    report = dict(scope=__doc__, observations_valid=True, original_port_parity='unproven',
                  movie=args.movie, original_movie_sha256=sha(ROOT/'DestructionDerby2'/args.movie),
                  accepted_journal_sha256=sha(capture/'audio'/accepted['events']),
                  played_journal_sha256=sha(capture/'audio'/played['events']),
                  source_frames=count, source_played_end_ns=end, negotiated_period_frames=period,
                  accepted_zero_tail_frames=accepted['accepted_frames']-count,
                  played_zero_tail_frames=played['played_frames']-count,
                  accepted_pcm_sha256=accepted['sha256'], played_pcm_sha256=played['sha256'],
                  source_and_silence_writes_separate=all(e['offset_frames']>=count for e in zeros),
                  driver_silence_requests_within_period=all(e['requested']<=period for e in zeros),
                  silence_calls_begin_after_source=all(e['call_begin_ns']>=end for e in zeros),
                  silence_calls_end_after_source=all(e['call_end_ns']>=end for e in zeros),
                  first_silence_call_after_source_end_ms=[(first[k]-end)/1e6 for k in
                                                        ('call_begin_ns','call_end_ns')] if first else None,
                  played_segments=played['segments'])
    if (capture/'report.json').exists():
        observed=json.loads((capture/'report.json').read_text())
        require(observed['observations_valid'] and observed['movie']==args.movie, 'changed native observation')
        report['native_binary_sha256']=observed['binary_sha256']
    else:
        original=json.loads((capture/'audio/summary.json').read_text())
        require(args.movie=='Intro.avi' and original['exe_sha256']==EXE_SHA256 and
                original['exe_modified'] is False and original['movie_autoskip'] is False,
                'unmodified full original intro required')
        report['original_exe_sha256']=EXE_SHA256
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(output)
    print(args.movie, 'accepted/played tail frames:', report['accepted_zero_tail_frames'],
          report['played_zero_tail_frames'], 'period:', period,
          'first silence call relative to source end (ms):',report['first_silence_call_after_source_end_ms'],flush=True)


if __name__=='__main__':
    main()
