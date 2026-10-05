#!/usr/bin/env python3
"""Compare unshifted native movie device PCM with actual Wine ACM source PCM.

Every source byte must occur at offset zero in accepted and device-consumed
streams. Remaining bytes are diagnosed, not trimmed for whole-output claims.
An optional original Intro/Outro capture additionally runs the strict whole-lifetime
comparison; a differing tail remains an explicit failure of original parity.
No synchronized A/V timing or physical DAC comparison is established.
"""
import argparse
import hashlib
import json
from pathlib import Path

from artifacts import WORK, check_space, prepare_output
from reference.audio import summarize_audio
from verify_configuration_persistence import ROOT, EXE_SHA256, require
from verify_movie_codec import compare


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def source_equal(expected, actual):
    require(len(actual) >= len(expected) and actual[:len(expected)] == expected,
            'actual device source PCM differs at offset zero')


def original_streams(directory,movie='Intro.avi'):
    summary = json.loads((directory/'audio/summary.json').read_text())
    outro=movie=='Outro.avi'
    if outro:
        provenance=json.loads((directory/'report.json').read_text())
        require(provenance['pass_'] and provenance['exe_sha256']==EXE_SHA256 and
                provenance['exe_modified'] is False and not provenance['engine_state_writes'] and
                provenance['original_process_exited'] and provenance['outro_started'] and
                provenance['card_unchanged'] and provenance['outro_frames']==1812 and
                provenance['observer_sha256']==sha(ROOT/'tools/capture_original_outro.py'),
                'completed real-input original Outro required')
    else:
        require(summary['exe_sha256'] == EXE_SHA256 and summary['exe_modified'] is False and
                summary['movie_autoskip'] is False, 'unmodified original with complete intro required')
    if outro:
        checked=summarize_audio(directory/'audio',require_played=True,write=False)
        require(all(summary[k]==checked[k] for k in ('streams','played_streams')),
                'original device journals differ from summary')
    result = {}
    for kind in ('streams', 'played_streams'):
        streams = [r for r in summary[kind] if r['format'] == 'S16_LE']
        require(len(streams) == (2 if outro else 1), 'original movie device count differs')
        row = streams[-1];path = directory/'audio'/row['file']
        if summary.get('reset_errors',False):
            faults=[json.loads(s) for s in (directory/'reset-fault.jsonl').read_text().splitlines()]
            require(summary['reset_faults']==faults and all(e['result']==-5 and e['pid']==row['pid'] for e in faults),
                    'changed original reset-fault observation')
        count = row['accepted_frames'] if kind == 'streams' else row['played_frames']
        require(row['closed'] and row['rate'] == 22050 and row['channels'] == 2 and
                path.stat().st_size == count*4 and sha(path) == row['sha256'], 'changed original PCM')
        result[kind] = path
    return result


def source_metadata(film):
    if 'metadata' in film:return film['metadata'],film['pcm_sha256']
    # verify_movie_audio.py proves the complete ACM source and both decoders.
    # Its "frames" counts PCM frames; obtain video count from the actual AVI.
    from verify_movie_codec import packets
    require(film['targets']==['native-asan-ubsan','wasm'], 'matched ACM source required')
    dimensions,video=packets(ROOT/'DestructionDerby2'/film['file'])
    require(dimensions==(320,192) and film['bytes']==film['frames']*film['channels']*2,
            'original source metadata differs')
    return dict(frames=len(video),pcm_frames=film['frames'],pcm_rate=film['rate'],
                pcm_channels=film['channels']),film['source_pcm_sha256']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--capture', type=Path, nargs='+', required=True)
    parser.add_argument('--original-intro', type=Path)
    parser.add_argument('--original-outro', type=Path)
    parser.add_argument('--before-intro', type=Path, help='require a captured pre-fix intro to fail the unshifted source comparison')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args();output = prepare_output(args.output)
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False)
    reference = json.loads((args.source/'report.json').read_text())
    originals={};original_reset_errors={}
    for movie,directory in [('Intro.avi',args.original_intro),('Outro.avi',args.original_outro)]:
        if directory:
            originals[movie]=original_streams(directory,movie)
            original_reset_errors[movie]=json.loads((directory/'audio/summary.json').read_text()).get('reset_errors',False)
    original_source_checks = []
    for movie,streams in originals.items():
        pcm=args.source/Path(movie).stem.lower()/'wine.pcm';expected=pcm.read_bytes()
        film=next(r for r in reference['films'] if r['file']==movie)
        _,source_sha=source_metadata(film)
        require(sha(pcm)==source_sha, 'changed original source reference')
        for kind,path in streams.items():
            actual=path.read_bytes()
            exact=len(actual)>=len(expected) and actual[:len(expected)]==expected
            mismatch=next((i for i,(a,b) in enumerate(zip(expected,actual)) if a!=b),
                          min(len(expected),len(actual))) if not exact else None
            original_source_checks.append(dict(movie=movie,kind=kind,source_pcm_bytes=len(expected),
                                               source_interval_exact_at_offset_zero=exact,
                                               first_mismatch_byte=mismatch))
        require(original_source_checks[-2]['source_interval_exact_at_offset_zero'],
                'original accepted source differs at offset zero')
    cases = [];negative = []
    before = None
    if args.before_intro:
        observed = json.loads((args.before_intro/'report.json').read_text())
        require(observed['observations_valid'], 'valid pre-fix live observation required')
        expected = (args.source/'intro/wine.pcm').read_bytes()
        film = next(r for r in reference['films'] if r['file'] == 'Intro.avi')
        require(hashlib.sha256(expected).hexdigest() == source_metadata(film)[1], 'changed pre-fix source reference')
        audio = summarize_audio(args.before_intro/'audio', require_played=True)
        failed = []
        for kind in ('streams', 'played_streams'):
            streams = [r for r in audio[kind] if r['format'] == 'S16_LE']
            require(len(streams) == 1 and streams[0]['closed'], 'complete pre-fix intro device required')
            stream = streams[0];actual = (args.before_intro/'audio'/stream['file']).read_bytes()
            try: source_equal(expected, actual)
            except RuntimeError:
                mismatch = next((i for i, (a,b) in enumerate(zip(expected, actual)) if a != b),
                                min(len(expected), len(actual)))
                failed.append(dict(kind=kind, actual_complete_sha256=stream['sha256'],
                                   unshifted_source_rejected=True, first_mismatch_byte=mismatch))
            else: raise RuntimeError('pre-fix output unexpectedly passes source comparison')
        before = dict(binary_sha256=observed['binary_sha256'], device_pcm=failed)
    for capture in args.capture:
        observation = json.loads((capture/'report.json').read_text())
        require(observation['observations_valid'] is True, 'valid live native observation required')
        name = observation['movie'];film = next(r for r in reference['films'] if r['file'] == name)
        require(observation['original_movie_sha256'] == film['avi_sha256'] ==
                sha(ROOT/'DestructionDerby2'/name), 'changed original movie')
        pcm = args.source/Path(name).stem.lower()/'wine.pcm'
        expected = pcm.read_bytes();metadata,source_sha = source_metadata(film)
        skip = observation.get('skip', False)
        reset_errors = observation.get('reset_errors', False)
        if reset_errors:
            require(observation['reset_faults']==[json.loads(s) for s in (capture/'reset-fault.jsonl').read_text().splitlines()] and
                    all(e['result']==-5 for e in observation['reset_faults']), 'changed reset-fault observation')
        require(sha(pcm) == source_sha and len(expected) == metadata['pcm_frames']*4,
                'changed source PCM')
        require((26 <= observation['frames'] < metadata['frames']-1 and observation['key_up_retained'])
                if skip else observation['frames'] == metadata['frames']-1, 'wrong movie endpoint or skip behavior')
        audio = summarize_audio(capture/'audio', require_played=True)
        rows = []
        for kind in ('streams', 'played_streams'):
            streams = [r for r in audio[kind] if r['format'] == 'S16_LE']
            require(len(streams) == 1, 'unique native movie stream required')
            stream = streams[0];path = capture/'audio'/stream['file'];actual = path.read_bytes()
            require(stream['closed'] and stream['rate'] == metadata['pcm_rate'] == 22050 and
                    stream['channels'] == metadata['pcm_channels'] == 2, 'complete source format required')
            if skip:
                require(actual and len(actual) < len(expected) and actual == expected[:len(actual)],
                        'cancelled device PCM differs from literal source prefix')
            else:
                source_equal(expected, actual)
                require(not any(actual[len(expected):]), 'nonzero samples beyond source PCM')
            row = dict(kind=kind, source_interval_exact_at_offset_zero=True,
                       complete_source_played=not skip,
                       source_pcm_bytes=len(expected), source_pcm_sha256=sha(pcm),
                       actual_complete_bytes=len(actual), actual_complete_sha256=sha(path),
                       actual_zero_tail_bytes=max(0, len(actual)-len(expected)))
            if name in originals and not skip:
                original=originals[name][kind]
                try: compare(original, path)
                except RuntimeError as error:
                    row.update(whole_original_equal=False, whole_original_difference=str(error))
                else: row['whole_original_equal'] = True
                row.update(original_complete_bytes=original.stat().st_size,
                           original_complete_sha256=sha(original),original_reset_errors=original_reset_errors[name],
                           same_reset_error_profile=reset_errors==original_reset_errors[name])
            for label, altered in ([] if skip else [('changed-first-byte', bytes([actual[0]^1])+actual[1:]),
                                   ('prepended-silence', b'\0'*4+actual),
                                   ('truncated-source', actual[:len(expected)-4])]):
                try: source_equal(expected, altered)
                except RuntimeError: negative.append(dict(movie=name, kind=kind, mutation=label))
                else: raise RuntimeError('accepted altered source interval: '+label)
            rows.append(row)
        cases.append(dict(capture=str(capture.resolve()), movie=name, frames=observation['frames'],
                          skip=skip,
                          reset_errors=reset_errors,
                          binary_sha256=observation['binary_sha256'], device_pcm=rows))
    report = dict(scope=__doc__, source_pcm_checks_passed=True, original_port_parity='unproven',
                  source_report_sha256=sha(args.source/'report.json'), cases=cases,
                  original_source_checks=original_source_checks,
                  before_intro=before, negative_controls=negative)
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n');check_space(output)
    print('PASS unshifted source PCM:', len(cases), 'movies,', len(negative),
          'changed PCM controls rejected; whole original/timing parity remains separate', flush=True)


if __name__ == '__main__':
    main()
