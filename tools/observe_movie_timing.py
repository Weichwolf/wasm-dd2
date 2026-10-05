#!/usr/bin/env python3
"""Validate original movie clock observations and report their measured relation.

QPC brackets the actual forwarded StretchDIBits call. Paired Linux RAW and
MONOTONIC readings expose the clock offset independently of audio/video data.
The sampled offset envelope is diagnostic, not a bound on every unsampled
instant or proof of original/port timing or physical output parity.
"""
import argparse
import hashlib
import json
from pathlib import Path

from artifacts import WORK, check_space, prepare_output
from reference.audio import summarize_audio
from verify_configuration_persistence import require
from verify_movie_video import original
from verify_movie_window import unpack_record


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = prepare_output(args.output)
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False)
    capture = args.capture.resolve()
    manifest, video_sha = original(capture)
    anchors = [json.loads(s) for s in (capture/'audio/engine.jsonl').read_text().splitlines()]
    require(len(anchors) >= 2, 'paired Linux clock observations missing')
    offsets = []
    previous = None
    for anchor in anchors:
        require(anchor.get('qpc_clock') == 'CLOCK_MONOTONIC_RAW', 'QPC clock domain missing')
        begin, end = anchor['read_begin_raw_ns'], anchor['read_end_raw_ns']
        lower = anchor['read_begin_ns']-begin
        upper = anchor['read_end_ns']-end
        require(begin <= end and lower <= upper and
                (previous is None or previous <= begin), 'reversed paired clock observations')
        previous = end
        offsets.append((lower, upper))
    lower = min(v[0] for v in offsets)
    upper = max(v[1] for v in offsets)
    frames = []
    frequency = previous = None
    for entry in manifest['records']:
        _, row = unpack_record(capture, entry)
        clock = row['clock']
        require(row['record_version'] == 2 and clock == entry['clock'], 'authenticated QPC record required')
        require(frequency is None or frequency == clock['frequency'], 'movie QPC frequency changed')
        frequency = clock['frequency']
        begin = clock['paint_begin_qpc']*1_000_000_000//frequency
        end = clock['paint_end_qpc']*1_000_000_000//frequency
        require(anchors[0]['read_begin_raw_ns'] <= begin <= end <= anchors[-1]['read_end_raw_ns']
                and (previous is None or previous <= begin), 'unbracketed/reordered actual movie clock')
        previous = end
        frames.append(dict(serial=row['serial'], paint_begin_raw_ns=begin, paint_end_raw_ns=end,
                           readback_end_raw_ns=clock['readback_end_qpc']*1_000_000_000//frequency,
                           sampled_monotonic_begin_ns=[begin+lower, begin+upper],
                           sampled_monotonic_end_ns=[end+lower, end+upper]))
    audio = summarize_audio(capture/'audio', write=False, require_played=True)
    accepted = [r for r in audio['streams'] if r['format'] == 'S16_LE']
    played = [r for r in audio['played_streams'] if r['format'] == 'S16_LE']
    require(len(accepted) == len(played) == 1, 'unique actual movie PCM lifetime required')
    accepted, played = accepted[0], played[0]
    require(accepted['closed'] and played['closed'] and
            accepted['rate'] == played['rate'] == 22050 and
            accepted['channels'] == played['channels'] == 2, 'complete movie PCM format required')
    require(played['segments'], 'played sample time intervals missing')
    segments = played['segments']
    def consumed(ns):
        return sum(min(s['frames'], max(0, (ns-s['begin_ns'])*played['rate']//1_000_000_000))
                   for s in segments)
    for frame in frames:
        frame['sampled_played_audio_frames_at_paint_begin'] = [
            consumed(ns) for ns in frame['sampled_monotonic_begin_ns']]
    report = dict(scope=__doc__, observations_valid=True, original_port_parity='unproven',
                  original_manifest_sha256=sha(capture/'movie-video-archive/manifest.json'),
                  linux_anchor_journal_sha256=sha(capture/'audio/engine.jsonl'),
                  original_window_sha256=video_sha, frames=len(frames), qpc_frequency=frequency,
                  paired_clock_observations=len(anchors), sampled_raw_to_monotonic_offset_ns=[lower, upper],
                  observed_paint_span_ms=(frames[-1]['paint_begin_raw_ns']-frames[0]['paint_begin_raw_ns'])/1e6,
                  accepted_pcm_bytes=accepted['accepted_frames']*4,
                  played_pcm_bytes=played['played_frames']*4,
                  accepted_pcm_sha256=accepted['sha256'], played_pcm_sha256=played['sha256'],
                  accepted_and_played_equal=(accepted['sha256'] == played['sha256'] and
                                            accepted['accepted_frames'] == played['played_frames']),
                  played_segments=len(segments),
                  first_paint_after_first_played_sample_ms=[
                      (ns-segments[0]['begin_ns'])/1e6 for ns in frames[0]['sampled_monotonic_begin_ns']],
                  last_paint_after_last_played_sample_ms=[
                      (ns-segments[-1]['end_ns'])/1e6 for ns in frames[-1]['sampled_monotonic_begin_ns']],
                  first_paint_played_frames=frames[0]['sampled_played_audio_frames_at_paint_begin'],
                  last_paint_played_frames=frames[-1]['sampled_played_audio_frames_at_paint_begin'])
    (output/'frames.json').write_text(json.dumps(frames)+'\n')
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    check_space(output)
    print(json.dumps(report, indent=2), flush=True)


if __name__ == '__main__':
    main()
