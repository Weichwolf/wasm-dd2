#!/usr/bin/env python3
"""Validate forwarded SDL presentation brackets against consumed audio samples.

Both journals use CLOCK_MONOTONIC. Timing-only captures skip pixel readback
and export and cannot serve as pixel-equality evidence. Readback and export work precede the
forwarded SDL_RenderPresent call and are reported separately. Observations
include their scheduling cost; these are API/device measurements, not
physical display/DAC timestamps or synchronized original/port parity.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path

from artifacts import WORK, check_space, prepare_output
from reference.audio import summarize_audio
from verify_configuration_persistence import ROOT, require
from verify_movie_avi import original_metadata


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def validate_frames(frames, timing_only=False):
    require(frames, 'presentation records missing')
    previous = 0
    for index, row in enumerate(frames):
        require(row['frame'] == index and row['exact_pixels'] == (0 if timing_only else 640*480) and
                row.get('record_version') == (3 if timing_only else 2) and row.get('clock_domain') == 'CLOCK_MONOTONIC',
                'actual presentation sequence or clock domain differs')
        if timing_only:
            require('readback_begin_ns' not in row and 'readback_end_ns' not in row, 'timing-only observation claims pixel readback')
        keys = ('present_begin_ns','present_end_ns','time_ns') if timing_only else ('readback_begin_ns','readback_end_ns','present_begin_ns','present_end_ns','time_ns')
        times = [row[key] for key in keys]
        require(all(type(t) is int and t > 0 for t in times) and previous <= times[0] and
                times == sorted(times), 'reversed or overlapping forwarded presentation brackets')
        previous = times[-1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args();output = prepare_output(args.output)
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False);capture = args.capture.resolve()
    observation = json.loads((capture/'report.json').read_text())
    require(observation['observations_valid'] and not observation['skip'], 'complete native movie required')
    require(observation['observer_source_sha256'] == sha(ROOT/'tools/native_movie_observer.c'),
            'capture used a different SDL observer')
    movie = ROOT/'DestructionDerby2'/observation['movie']
    require(sha(movie) == observation['original_movie_sha256'], 'changed original movie')
    metadata, _, _, _ = original_metadata(movie)
    events = [json.loads(s) for s in (capture/'events.jsonl').read_text().splitlines()]
    mode = observation.get('pixel_readback') is False
    frames = [r for r in events if r['event'] == 'present'];validate_frames(frames,mode)
    require(len(frames) == observation['frames'] == metadata['frames']-1, 'incomplete presentation sequence')
    updates = [r for r in events if r['event'] == 'texture_update']
    copies = [r for r in events if r['event'] == 'render_copy']
    threads = [r for r in events if r['event'] == 'movie_thread']
    require(len(updates) == len(copies) == len(frames) and len(threads) == 1 and threads[0]['success'],
            'complete movie start/render API observations required')
    for index, (update, copied, presented) in enumerate(zip(updates, copies, frames)):
        require(update['frame'] == copied['frame'] == index and
                update['call_begin_ns'] <= update['call_end_ns'] <= update['time_ns'] <=
                copied['call_begin_ns'] <= copied['call_end_ns'] <= copied['time_ns'] <=
                presented['present_begin_ns' if mode else 'readback_begin_ns'], 'reordered texture/copy/readback API intervals')
    thread = threads[0]
    require(thread['call_begin_ns'] <= thread['call_end_ns'] <= thread['time_ns'] <= updates[0]['call_begin_ns'],
            'thread startup does not precede movie rendering')
    audio = summarize_audio(capture/'audio', require_played=True)
    devices = [r for r in audio['played_streams'] if r['format'] == 'S16_LE']
    require(len(devices) == 1, 'unique movie playback device required')
    device = devices[0];segments = device['segments'];rate = device['rate']
    require(device['closed'] and segments and rate == metadata['pcm_rate'] and
            device['channels'] == metadata['pcm_channels'] == 2 and
            device['played_frames'] >= metadata['pcm_frames'], 'complete played source format required')
    def consumed(ns):
        return sum(min(s['frames'], max(0, (ns-s['begin_ns'])*rate//1_000_000_000)) for s in segments)
    for row in frames:
        row['played_audio_frames_during_present'] = [consumed(row['present_begin_ns']),
                                                     consumed(row['present_end_ns'])]
        row['readback_duration_ns'] = None if mode else row['readback_end_ns']-row['readback_begin_ns']
        row['export_before_present_ns'] = None if mode else row['present_begin_ns']-row['readback_end_ns']
    source_end_ns = None
    for segment in segments:
        if segment['offset_frames'] < metadata['pcm_frames'] <= segment['offset_frames']+segment['frames']:
            source_end_ns = segment['begin_ns']+(metadata['pcm_frames']-segment['offset_frames'])*1_000_000_000//rate
            break
    require(source_end_ns is not None, 'played source endpoint missing')
    negative = []
    for label in ('clock-domain', 'reversed-present', 'unexpected-readback' if mode else 'readback-after-present', 'frame-order'):
        altered = copy.deepcopy(frames)
        row = altered[0]
        if label == 'clock-domain': row['clock_domain'] = 'CLOCK_MONOTONIC_RAW'
        elif label == 'reversed-present': row['present_begin_ns'] = row['present_end_ns']+1
        elif label == 'unexpected-readback': row['readback_begin_ns'] = row['present_begin_ns']
        elif label == 'readback-after-present': row['readback_end_ns'] = row['present_end_ns']+1
        else: row['frame'] = 1
        try: validate_frames(altered,mode)
        except RuntimeError: negative.append(label)
        else: raise RuntimeError('accepted altered actual presentation clocks: '+label)
    frame_file = output/'frames.json';frame_file.write_text(json.dumps(frames)+'\n')
    first, last = frames[0], frames[-1]
    report = dict(scope=__doc__, observations_valid=True, original_port_parity='unproven',
                  movie=observation['movie'], binary_sha256=observation['binary_sha256'],
                  native_observer_source_sha256=observation['observer_source_sha256'],
                  capture_report_sha256=sha(capture/'report.json'), events_sha256=sha(capture/'events.jsonl'),
                  frames_sha256=sha(frame_file), frames=len(frames), pixel_readback=not mode, clock_domain='CLOCK_MONOTONIC',
                  sample_clock_source_sha256=sha(ROOT/'tools/reference/alsa_clock.c'),
                  played_journal_sha256=sha(capture/'audio'/device['events']),
                  played_pcm_sha256=device['sha256'], played_segments=len(segments),
                  source_pcm_frames=metadata['pcm_frames'], played_pcm_frames=device['played_frames'],
                  first_present_after_first_played_sample_ms=[(first[k]-segments[0]['begin_ns'])/1e6
                                                              for k in ('present_begin_ns','present_end_ns')],
                  last_present_after_source_endpoint_ms=[(last[k]-source_end_ns)/1e6
                                                         for k in ('present_begin_ns','present_end_ns')],
                  last_present_after_last_played_sample_ms=[(last[k]-segments[-1]['end_ns'])/1e6
                                                            for k in ('present_begin_ns','present_end_ns')],
                  first_present_played_audio_frames=first['played_audio_frames_during_present'],
                  last_present_played_audio_frames=last['played_audio_frames_during_present'],
                  observed_present_begin_span_ms=(last['present_begin_ns']-first['present_begin_ns'])/1e6,
                  max_readback_duration_ms=None if mode else max(r['readback_duration_ns'] for r in frames)/1e6,
                  max_export_before_present_ms=None if mode else max(r['export_before_present_ns'] for r in frames)/1e6,
                  movie_thread_creation_ms=(thread['call_end_ns']-thread['call_begin_ns'])/1e6,
                  first_texture_after_first_played_sample_ms=(updates[0]['call_begin_ns']-segments[0]['begin_ns'])/1e6,
                  max_texture_update_ms=max((r['call_end_ns']-r['call_begin_ns'])/1e6 for r in updates),
                  max_render_copy_ms=max((r['call_end_ns']-r['call_begin_ns'])/1e6 for r in copies),
                  negative_controls_rejected=negative)
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n');check_space(output)
    print(json.dumps(report, indent=2), flush=True)


if __name__ == '__main__':
    main()
