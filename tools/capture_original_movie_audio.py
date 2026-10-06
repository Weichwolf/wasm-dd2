#!/usr/bin/env python3
"""Audit the actual unchanged Original's full Intro against fresh Wine ACM PCM.

Use an isolated Wine prefix and either the clocked ALSA device or an owned
Pulse server. Observe every accepted movie sample and its actual natural close;
Pulse client acceptance is distinct from daemon consumption. Record full source
differences literally, without shifting, trimming, or changing an engine/backend
return. Movie paint ordinals are observations, not framebuffer equality.
Neither a successful capture nor a source-only check proves original/port parity.
"""
import argparse
import array
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference')]
from artifacts import WORK, check_space, open_files, prepare_output
from reference import capture
from reference.audio import summarize_audio
from reference.pulse import pulse_server, summarize_pulse
from reference.wave_observer import summarize as summarize_wave, summarize_acm
from verify_configuration_persistence import original_args, require
from verify_movie_avi import original_metadata


def sha(path):
    with Path(path).open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def source_reference(directory):
    report = json.loads((directory / 'report.json').read_text())
    film = next(r for r in report['films'] if r['file'] == 'Intro.avi')
    pcm = directory / 'intro/wine.pcm'
    metadata, _, _, _ = original_metadata(ROOT / 'DestructionDerby2/Intro.avi')
    require(report['fixture_source_sha256'] == sha(ROOT / 'tools/movie_audio_test.c') and
            report['fixture_exe_sha256'] == sha(directory / 'audio.exe') and
            report['capture_source_sha256'] == sha(ROOT / 'tools/capture_movie_audio_source.py') and
            report['wine_version'] == subprocess.check_output(['wine', '--version'], text=True).strip() and
            film['metadata'] == metadata and film['avi_sha256'] == sha(ROOT / 'DestructionDerby2/Intro.avi') and
            film['pcm_sha256'] == sha(pcm) and pcm.stat().st_size == metadata['pcm_frames'] * 4 and
            film['acm'] == dict(rate=22050, channels=2, frames=metadata['pcm_frames'], bytes=pcm.stat().st_size),
            'fresh complete actual Wine ACM source/AVI/fixture binding required')
    return pcm.read_bytes(), dict(report_sha256=sha(directory / 'report.json'), pcm_sha256=sha(pcm),
                                  source_bytes=pcm.stat().st_size, pcm_frames=metadata['pcm_frames'],
                                  video_paint_ordinals=metadata['frames'] - 1)


def compare_source(actual, expected):
    extent = len(actual) >= len(expected)
    exact = extent and actual[:len(expected)] == expected
    first = None if exact else next((i for i, (a, b) in enumerate(zip(actual, expected)) if a != b),
                                   min(len(actual), len(expected)))
    result = dict(source_interval_exact_at_offset_zero=exact, complete_source_extent_available=extent,
                  whole_source_bytes_equal=actual == expected, actual_bytes=len(actual), source_bytes=len(expected),
                  source_extent_shortfall_bytes=max(0, len(expected) - len(actual)), first_mismatch_byte=first,
                  extra_tail_bytes=max(0, len(actual) - len(expected)),
                  extra_tail_has_nonzero_samples=any(actual[len(expected):]),
                  source_check_passed=exact and not any(actual[len(expected):]))
    if first is not None:
        begin = max(0, first // 4 - 16); end = begin + 64
        samples = {}
        for label, data in (('actual', actual), ('source', expected)):
            values = array.array('h'); values.frombytes(data[begin * 4:end * 4])
            samples[label] = values.tolist()
        result['bounded_mismatch_checkpoint'] = dict(begin_frame=begin, max_frames=64, samples=samples)
    return result


def cleanup(output):
    require(capture.original_pid(output / 'work/prefix') is None,
            'Original is still alive in the owned prefix; leave its files intact')
    opened = open_files(); paths = []
    for directory in (output / 'work', output / 'audio', output / 'winmm-source'):
        for file in directory.rglob('*'):
            if file.is_file() and not file.is_symlink(): paths.append(file)
    for file in [*output.glob('*.log'), output / 'pulse-server/daemon.log']:
        if file.exists(): paths.append(file)
    removed = []
    for file in paths:
        stat = file.stat()
        require((stat.st_dev, stat.st_ino) not in opened, 'capture file is still open: ' + str(file))
        removed.append(dict(file=str(file.relative_to(output)), bytes=stat.st_size, sha256=sha(file)))
    (output / 'cleanup.json').write_text(json.dumps(dict(removed=removed, original_pid_absent=True), indent=2) + '\n')
    for file in paths:
        if output / 'work' not in file.parents: file.unlink()
    shutil.rmtree(output / 'work')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True, help='fresh capture_movie_audio_source.py output')
    parser.add_argument('--backend', choices=('alsa', 'pulse'), default='pulse')
    parser.add_argument('--trace-winmm', action='store_true', help='also capture actual WinMM MS-ADPCM submissions and ACM decoded PCM')
    parser.add_argument('--require-exact-source', action='store_true',
                        help='exit 1 after reporting/cleanup if the actual source check fails')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    require(sys.byteorder == 'little', 'checkpoint sample decoding requires little endian')
    output = prepare_output(args.output)
    require(WORK in output.parents and not output.exists(), 'Use a fresh directory under /tmp/wasm-dd2/')
    expected, reference = source_reference(args.source.resolve())
    output.mkdir(parents=True)
    sources = {name: sha(ROOT / name) for name in (
        'tools/capture_original_movie_audio.py', 'tools/reference/capture.py', 'tools/reference/pulse.py',
        'tools/reference/pulse_audio.c', 'tools/reference/audio.py', 'tools/reference/wine_audio.c',
        'tools/reference/alsa_clock.c',
        'tools/reference/wave_observer.py', 'tools/reference/winmm_wave_observer.c',
        'tools/reference/winmm_timer_observer.h', 'tools/reference/timer_observer.py',
        'tools/reference/winmm_acm_observer.c',
    )}
    capture.WORK = output / 'work'; capture.WORK.mkdir()
    options = original_args()
    options.mode = 'audio'; options.audio = True; options.audio_backend = args.backend
    options.audio_rate = 22050; options.audio_device = 'clock'; options.audio_tail = .2
    options.reset_errors = False; options.keep_movie = True; options.timeout = 140
    options.trace_movie_wave = args.trace_winmm
    options.wine_debug = '-all,+iccvid,+mciavi'
    report = dict(scope=__doc__, observations_valid=False, capture_complete=False,
                  original_port_parity='unproven', engine_state_writes=False,
                  backend=args.backend, trace_winmm=args.trace_winmm, keep_movie=True, exe_sha256=capture.EXE_SHA256, exe_modified=False,
                  wine_version=subprocess.check_output(['wine', '--version'], text=True).strip(),
                  original_movie_sha256=sha(ROOT / 'DestructionDerby2/Intro.avi'), source_reference=reference,
                  sources=sources, declared_device_rate=22050, pulse_server=None)
    initial_save = sha(ROOT / 'DestructionDerby2/SaveGames')

    def driver(pid, directory, env, deadline, rundir):
        report['menu_endpoint'] = capture.state(pid)
        require(capture.menu_ready(report['menu_endpoint']), 'full Intro did not return naturally to Main Menu')
        report['actual_process'] = dict(pid=pid,
            start_token=Path(f'/proc/{pid}/stat').read_text().split(') ', 1)[1].split()[19])
        report['save_unchanged'] = initial_save == sha(rundir / 'SaveGames')
        require(report['save_unchanged'], 'no-input original capture changed its card')
        libraries = {}
        for line in Path(f'/proc/{pid}/maps').read_text().splitlines():
            fields = line.split(maxsplit=5)
            if len(fields) == 6 and Path(fields[5]).name in (
                    'winepulse.drv', 'winepulse.so', 'winealsa.drv', 'winealsa.so', 'dd2_pulse.so', 'dd2_audio.so',
                    'winmm.dll', '_winmm_real.dll', 'mciavi32.dll', 'msacm32.dll', '_msacm32_real.dll'):
                path = Path(fields[5]); libraries[str(path)] = dict(sha256=sha(path))
        require(any(Path(name).name == 'wine' + args.backend + '.drv' for name in libraries),
                'declared actual Wine audio driver is not mapped')
        report['mapped_audio_libraries'] = libraries
        if args.trace_winmm:
            observer = json.loads((output / 'wave-observer-build.json').read_text())
            require(any(Path(p).name == 'winmm.dll' and v['sha256'] == observer['observer_sha256']
                        for p, v in libraries.items()) and
                    any(Path(p).name == '_winmm_real.dll' and v['sha256'] == observer['private_backend_sha256']
                        for p, v in libraries.items()), 'actual forwarded WinMM proxy/backend must be mapped')
            require(any(Path(p).name == 'msacm32.dll' and v['sha256'] == observer['acm']['observer_sha256']
                        for p, v in libraries.items()) and
                    any(Path(p).name == '_msacm32_real.dll' and v['sha256'] == observer['acm']['private_backend_sha256']
                        for p, v in libraries.items()), 'actual forwarded ACM proxy/backend must be mapped')

    if args.backend == 'pulse':
        with pulse_server(output) as (env, server):
            report['pulse_server'] = server
            os.environ.update(env)
            capture.run(ROOT / 'DestructionDerby2', output, options, on_menu=driver)
        streams = summarize_pulse(output / 'audio')
        movie = [r for r in streams if r['format'] == 's16le']
        require(len(movie) == 1 and movie[0]['closed'] and movie[0]['rate'] == 22050 and movie[0]['channels'] == 2,
                'one actual naturally closed S16 Pulse movie lifetime required')
        report['client_streams'] = streams
        report['pcm_checks'] = [dict(kind='accepted-client', **compare_source(
            (output / 'audio' / movie[0]['file']).read_bytes(), expected))]
        report['daemon_consumption_observed'] = False
    else:
        capture.run(ROOT / 'DestructionDerby2', output, options, on_menu=driver)
        audio = summarize_audio(output / 'audio', require_played=True)
        report['audio'] = audio; report['pcm_checks'] = []
        for kind in ('streams', 'played_streams'):
            movie = [r for r in audio[kind] if r['format'] == 'S16_LE']
            require(len(movie) == 1 and movie[0]['closed'] and movie[0]['rate'] == 22050 and movie[0]['channels'] == 2,
                    'one actual naturally closed S16 ALSA movie lifetime required')
            report['pcm_checks'].append(dict(kind=kind, **compare_source(
                (output / 'audio' / movie[0]['file']).read_bytes(), expected)))
    painted = [int(n) for n in re.findall(r'MCIAVI_PaintFrame Painting frame (\d+)',
                                         (output / 'wine.log').read_text(errors='replace'))]
    if args.trace_winmm:
        _, _, format_, blocks = original_metadata(ROOT / 'DestructionDerby2/Intro.avi')
        report['winmm_source'], submitted = summarize_wave(output / 'winmm-source', format_)
        report['actual_acm'], consumed, pcm = summarize_acm(output / 'winmm-source', format_)
        report['compressed_checks'] = [dict(kind=kind, source_check_passed=data == blocks,
            actual_bytes=len(data), source_bytes=len(blocks), actual_sha256=hashlib.sha256(data).hexdigest(),
            source_sha256=hashlib.sha256(blocks).hexdigest()) for kind, data in
            (('winmm-submitted-msadpcm', submitted), ('actual-acm-consumed-msadpcm', consumed))]
        report['pcm_checks'].append(dict(kind='actual-winmm-acm-decoded', **compare_source(pcm, expected)))
    require(painted == list(range(reference['video_paint_ordinals'])), 'complete ordered original movie paint calls required')
    require(all(sha(ROOT / name) == digest for name, digest in sources.items()), 'capture sources changed during the run')
    report.update(observations_valid=True, capture_complete=True, paint_calls=len(painted),
                  source_check_passed=all(r['source_check_passed'] for r in report['pcm_checks'] + report.get('compressed_checks', [])))
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    check_space(output)
    cleanup(output)
    check_space(output)
    print('Original', args.backend, 'Intro capture complete;', len(painted), 'paint calls; source check:',
          report['source_check_passed'], '; original/port parity remains unproven', flush=True)
    if args.require_exact_source and not report['source_check_passed']: raise SystemExit(1)


if __name__ == '__main__': main()
