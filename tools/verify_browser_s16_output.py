#!/usr/bin/env python3
"""Measure every S16 value through the actual Chromium ALSA output path.

Compile the unchanged production movie import and two explicitly experimental
source encodings. Independently pinned Chromium C++ traits predict conversion;
real accepted and consumed device PCM must match that prediction at the source
position derived from the observed start call and initial device-buffer write.
An optional real PulseAudio client capture checks the same fixture's Float32
encoding using sample identifiers in its unchanged negative channel. This does
not establish daemon consumption or infer a common source/output clock.
No fitted waveform alignment, original parity or production codec fix is claimed.
"""
import argparse
import array
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import struct
from urllib.request import urlopen

from artifacts import WORK, check_space, open_files, prepare_output, run_bounded
from reference.audio import build_audio, summarize_audio
from reference.pulse import pulse_server, summarize_pulse as pulse_streams
from verify_configuration_persistence import ROOT, require

VERSION = '154.0.8037.92'
PINS = {
    'audio_sample_types.h': '96fee7d58629f2cb6749eba333c57cc171c4bab60c4638a37b9a5be2382ab547',
    'alsa_output.cc': '6ba7cda5591b1389cdb8c6ceef3cfaddcb99405720d481b9d973f8baeee9c3f5',
}
PULSE_PINS = {
    'pulse_output.cc': 'ec3f698eb92fd626b965823453742052f51d70a7692a2f83fa034a2a144b8530',
    'audio_manager_pulse.cc': '14e6761d39de3272df5df678ac72ab3fa1d652080440d952cfac860e01390da5',
}
SOURCE_PATHS = {
    'audio_sample_types.h': 'media/base/audio_sample_types.h',
    'alsa_output.cc': 'media/audio/alsa/alsa_output.cc',
    'pulse_output.cc': 'media/audio/pulse/pulse_output.cc',
    'audio_manager_pulse.cc': 'media/audio/pulse/audio_manager_pulse.cc',
}
LOOP = 'for(var i=0;i<frames;i++)samples[i]=HEAP16[(pcm>>1)+i*channels+ch]/32768;'
ENCODERS = {
    'chromium': 'return n<0 ? n/32768 : Math.fround(n*Math.fround(1/32767));',
    'inverse': 'return n<0 ? n/32768 : n/32767;',
}


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def fetch_sources(directory, pins):
    directory.mkdir()
    entries = {}
    for name, expected in pins.items():
        url = 'https://raw.githubusercontent.com/chromium/chromium/' + VERSION + '/' + SOURCE_PATHS[name]
        with urlopen(url, timeout=20) as response:
            data = response.read(1024**2 + 1)
        require(len(data) <= 1024**2 and hashlib.sha256(data).hexdigest() == expected,
                'downloaded primary source differs from exact pin: ' + name)
        (directory / name).write_bytes(data)
        entries[name] = dict(url=url, sha256=expected)
    (directory / 'sources.json').write_text(json.dumps(dict(chromium_version=VERSION, sources=entries), indent=2) + '\n')
    return directory


def validate(actual, prediction, browser, device, first_write):
    require(browser['frames'] == 65536 and browser['channels'] == device['channels'] == 2 and
            browser['sample_rate'] == device['rate'] == 22050 and device['format'] == 'S16_LE',
            'complete exhaustive stereo fixture and exact device format required')
    events = browser['events']
    require([e['event'] for e in events] == ['start', 'ended', 'close'] and browser['closed'],
            'natural source completion and closed context required')
    require(all(a['performance_ms'] <= b['performance_ms'] and a['context_time'] <= b['context_time']
                for a, b in zip(events, events[1:])), 'reversed browser clock')
    scheduled = events[0]['scheduled_time']
    position = round(scheduled * 22050)
    require(scheduled >= 0 and abs(scheduled * 22050 - position) < 1e-6 and
            events[0]['sample_rate'] == 22050, 'source start is not an observed sample boundary')
    startup = device['buffer_frames']
    require(first_write['offset_frames'] == 0 and first_write['accepted'] == startup and
            not any(actual[:startup * 4]), 'independent initial device-buffer write differs')
    offset = startup + position
    require(len(actual) % 4 == 0 and len(prediction) == 65536 * 4 and
            len(actual) >= offset * 4 + len(prediction) and not any(actual[:offset * 4]),
            'complete independently positioned source extent is missing')
    body = actual[offset * 4:]
    require(body[:len(prediction)] == prediction and not any(body[len(prediction):]),
            'actual converter/source/tail differs from independent pinned-header prediction')
    return dict(startup_silence_frames=startup, scheduled_source_start_frames=position,
                predicted_source_offset_frames=offset, source_frames_compared=65536,
                trailing_silence_frames=(len(body) - len(prediction)) // 4)


def mismatches(actual, expected):
    a = array.array('h'); a.frombytes(actual)
    b = array.array('h'); b.frombytes(expected)
    require(len(a) == len(b), 'sample comparison extents differ')
    return sum(x != y for x, y in zip(a, b))


def summarize_pulse(directory):
    streams = pulse_streams(directory, require_closed=True)
    require(len(streams) == 1 and streams[0]['rate'] == 22050 and streams[0]['channels'] == 2 and
            streams[0]['format'] == 'float32le' and streams[0]['frame_bytes'] == 8,
            'complete Pulse Float32 22050-Hz stereo lifetime required')
    return streams[0]


def validate_pulse(actual, expected_source, canonical_source):
    require(len(actual) % 8 == 0 and len(expected_source) == len(canonical_source) == 65536 * 8,
            'complete Float32 stereo fixture required')
    markers = []; positions = []; changes = 0
    for position in range(len(actual) // 8):
        pair = actual[position * 8:position * 8 + 8]
        if pair == b'\0' * 8: continue
        left, right = struct.unpack('<ff', pair)
        # Each caller pair sums to -1 in S16 units. At least one channel is
        # negative; its canonical Float32 is an exact, unmodified sample ID.
        negative = left if left < 0 else right
        require(-1 <= negative < 0 and (negative * 32768).is_integer(),
                'unchanged negative-channel fixture identifier is missing')
        nleft = int(negative * 32768) if left < 0 else -1 - int(negative * 32768)
        marker = nleft + 32768
        require(0 <= marker < 65536 and pair == expected_source[marker * 8:marker * 8 + 8],
                'actual Pulse Float32 pair differs from pinned source encoding')
        changes += sum(pair[ch * 4:ch * 4 + 4] != canonical_source[marker * 8 + ch * 4:marker * 8 + ch * 4 + 4]
                       for ch in range(2))
        markers.append(marker); positions.append(position)
    require(markers == list(range(65536)) and positions == list(range(positions[0], positions[0] + 65536)),
            'complete ordered source identifiers were dropped, repeated or interrupted')
    return dict(source_frames_observed=65536, canonical_float_sample_changes=changes,
                source_values_identified_by='unchanged negative-channel synthetic sample identifiers; no clock alignment',
                observed_source_begin_frame=positions[0], observed_source_end_frame=positions[-1] + 1,
                complete_accepted_frames=len(actual) // 8, whole_fixture_float_equal=actual == canonical_source)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--chromium-sources', type=Path,
                        help='reuse pinned source bundle; otherwise download exact guarded sources into the output directory')
    parser.add_argument('--pulse', action='store_true', help='also check actual Float32 Pulse client output')
    parser.add_argument('--pulse-sources', type=Path,
                        help='also measure real Float32 Pulse client writes on an owned 22050-Hz server')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = prepare_output(args.output)
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    require(os.sys.byteorder == 'little', 'fixture serialization requires a little-endian host')
    output.mkdir(parents=True, exist_ok=False)
    immutable = {path: sha(path) for path in (
        Path(__file__), ROOT / 'tools/browser/qa_s16_output.js', ROOT / 'tools/reference/browser_s16_reference.cc',
        ROOT / 'tools/reference/pulse_audio.c', ROOT / 'tools/reference/wine_audio.c', ROOT / 'tools/reference/alsa_clock.c',
        ROOT / 'tools/reference/pulse.py',
    )}
    pulse_enabled = args.pulse or args.pulse_sources is not None
    if args.chromium_sources is None:
        args.chromium_sources = fetch_sources(output / 'downloaded-chromium', PINS)
    if pulse_enabled and args.pulse_sources is None:
        args.pulse_sources = fetch_sources(output / 'downloaded-pulse', PULSE_PINS)
    provenance = json.loads((args.chromium_sources / 'sources.json').read_text())
    require(provenance['chromium_version'] == VERSION, 'unsupported Chromium source version')
    reference = output / 'reference'; reference.mkdir()
    for name, expected_sha in PINS.items():
        entry = provenance['sources'][name]
        require(entry['sha256'] == expected_sha == sha(args.chromium_sources / name) and
                '/' + VERSION + '/' in entry['url'], 'pinned primary-source binding differs: ' + name)
        shutil.copy2(args.chromium_sources / name, reference / name)
    helper_source = ROOT / 'tools/reference/browser_s16_reference.cc'
    shutil.copy2(helper_source, reference / helper_source.name)
    subprocess.run(['g++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(reference), str(reference / helper_source.name),
                    '-o', str(reference / 'converter')], check=True)
    snapshot = output / 'production'; snapshot.mkdir()
    for header in (ROOT / 'build').glob('*.h'):
        shutil.copy2(header, snapshot / header.name)
    original = (ROOT / 'build/dd2_movie_platform.c').read_text()
    require(original.count(LOOP) == 1 and original.count('    var buffer;') == 1,
            'production source encoding changed; diagnose before rebuilding candidates')
    (snapshot / 'dd2_movie_platform.c').write_text(original)
    libraries = build_audio(output / 'audio-libraries')
    asound = output / 'asound.conf'
    asound.write_text(f'pcm_type.dd2clock {{ lib "{output}/audio-libraries/$LIB/dd2_clock.so" }}\n'
                      'pcm.!default { type dd2clock }\n')
    env = {k: v for k, v in os.environ.items() if not k.startswith('DD2_')}
    env.update(PULSE_SERVER='unix:' + str(output / 'no-pulse'), ALSA_CONFIG_PATH=str(asound),
               DD2_AUDIO_PROCESS='chromium', DD2_AUDIO_RATE='22050',
               LD_PRELOAD=str(libraries[1] / 'dd2_audio.so'))
    cases = []; negative = []
    for label in ('canonical', 'chromium', 'inverse'):
        case = output / label; case.mkdir(); (case / 'audio').mkdir()
        source = original
        if label != 'canonical':
            source = source.replace('    var buffer;',
                                    '    var buffer;\n'
                                    '    function encodeSample(n){' + ENCODERS[label] + '}')
            source = source.replace(LOOP, 'for(var i=0;i<frames;i++)'
                                    'samples[i]=encodeSample(HEAP16[(pcm>>1)+i*channels+ch]);')
        unit = case / 'platform.c'; unit.write_text(source)
        subprocess.run(['emcc', '-DDD2_BROWSER', '-I' + str(snapshot), str(unit), '--no-entry',
                        '-sASYNCIFY', '-sEXPORTED_FUNCTIONS=["_malloc","_free",'
                        '"_dd2_movie_audio_start","_dd2_movie_audio_done","_dd2_movie_audio_stop"]',
                        '-sEXIT_RUNTIME=0', '-o', str(case / 'fixture.js')], check=True)
        (case / 'index.html').write_text('<script>var Module={onRuntimeInitialized(){window.__startS16Fixture();}};</script>'
                                        '<script src="fixture.js"></script>')
        predicted = json.loads(subprocess.check_output([str(reference / 'converter'), label, str(case)], text=True))
        current_env = {**env, 'DD2_AUDIO_CAPTURE': str(case / 'audio')}
        with (case / 'run.log').open('w') as log:
            run_bounded(['node', str(ROOT / 'tools/browser/qa_s16_output.js'), str(case)],
                        directory=output, env=current_env, timeout=45, check=True,
                        stdout=log, stderr=subprocess.STDOUT)
        browser = json.loads((case / 'browser.json').read_text())
        require(browser['observations_valid'] and browser['device_closed_before_browser_shutdown'] and
                browser['chromium_version'] == VERSION and browser['wasm_sha256'] == sha(case / 'fixture.wasm') and
                browser['observer_source_sha256'] == sha(ROOT / 'tools/browser/qa_s16_output.js') and
                browser['source_sha256'] == sha(case / 'actual-source.f32') == sha(case / 'expected-source.f32'),
                'actual browser/source/model provenance differs')
        audio = summarize_audio(case / 'audio', require_played=True)
        require(len(audio['streams']) == len(audio['played_streams']) == 1 and
                all(r['closed'] for kind in ('streams', 'played_streams') for r in audio[kind]),
                'one actual naturally closed accepted/consumed device lifetime required')
        writes = [json.loads(line) for line in (case / 'audio' / audio['streams'][0]['events']).read_text().splitlines()]
        first = next(e for e in writes if e['event'] == 'write')
        input_pcm = (case / 'input.pcm').read_bytes()
        prediction = (case / 'expected-device.pcm').read_bytes()
        require(mismatches(prediction, input_pcm) == predicted['s16_mismatches'], 'pinned model statistics differ')
        results = []
        for kind in ('streams', 'played_streams'):
            device = audio[kind][0]; raw = case / 'audio' / device['file']; actual = raw.read_bytes()
            details = validate(actual, prediction, browser, audio['played_streams'][0], first)
            offset = details['predicted_source_offset_frames'] * 4
            interval = actual[offset:offset + len(input_pcm)]
            changed = mismatches(interval, input_pcm)
            require(changed == predicted['s16_mismatches'], 'real device differs from pinned model mismatch count')
            results.append(dict(kind=kind, **details, actual_frames=len(actual) // 4, actual_sha256=sha(raw),
                                source_interval_s16_mismatches=changed, source_interval_s16_equal=not changed,
                                whole_fixture_input_equal=actual == input_pcm))
            swapped = b''.join(actual[i + 2:i + 4] + actual[i:i + 2] for i in range(0, len(actual), 4))
            for name, mutated in (
                ('source-bit', actual[:offset] + bytes([actual[offset] ^ 1]) + actual[offset + 1:]),
                ('extra-leading-silence', b'\0' * 4 + actual), ('muted-device', b'\0' * len(actual)),
                ('missing-source', actual[:offset]), ('swapped-channels', swapped),
            ):
                try: validate(mutated, prediction, browser, audio['played_streams'][0], first)
                except RuntimeError: negative.append(dict(encoding=label, kind=kind, mutation=name))
                else: raise RuntimeError('accepted changed actual device data: ' + name)
            altered = copy.deepcopy(browser); altered['events'][0]['scheduled_time'] += 1 / 22050
            try: validate(actual, prediction, altered, audio['played_streams'][0], first)
            except RuntimeError: negative.append(dict(encoding=label, kind=kind, mutation='source-clock'))
            else: raise RuntimeError('accepted changed source clock')
        require((label == 'inverse') == all(r['source_interval_s16_equal'] for r in results),
                'canonical/Chromium roundtrip controls or inverse encoding did not behave as measured')
        cases.append(dict(encoding=label, experimental=label != 'canonical', platform_sha256=sha(unit),
                          browser_report_sha256=sha(case / 'browser.json'), wasm_sha256=browser['wasm_sha256'],
                          source_sha256=sha(case / 'actual-source.f32'), fixture_input_sha256=sha(case / 'input.pcm'),
                          predicted_device_sha256=sha(case / 'expected-device.pcm'), reference_model=predicted,
                          browser_events=browser['events'], device_pcm=results, audio=audio))
        print(label, 'actual accepted/consumed S16 mismatches:', results[0]['source_interval_s16_mismatches'], flush=True)
        check_space(output)
    pulse_cases = []; pulse_metadata = None; pulse_sources = {}
    if pulse_enabled:
        pulse_provenance = json.loads((args.pulse_sources / 'sources.json').read_text())
        require(pulse_provenance['chromium_version'] == VERSION, 'unsupported Pulse Chromium source version')
        for name, expected_sha in PULSE_PINS.items():
            entry = pulse_provenance['sources'][name]
            require(entry['sha256'] == expected_sha == sha(args.pulse_sources / name) and
                    '/' + VERSION + '/' in entry['url'], 'pinned Pulse primary-source binding differs: ' + name)
            shutil.copy2(args.pulse_sources / name, reference / name)
            pulse_sources[name] = entry
        pulse_observer = ROOT / 'tools/reference/pulse_audio.c'
        shutil.copy2(pulse_observer, reference / pulse_observer.name)
        subprocess.run(['gcc', '-shared', '-fPIC', '-O2', '-Wall', '-Wextra', '-Werror', '-pthread',
                        str(pulse_observer), '-ldl', '-o', str(reference / 'pulse_audio.so')], check=True)
        canonical_source = (output / 'canonical/expected-source.f32').read_bytes()
        with pulse_server(output) as (pulse_env, pulse_metadata):
            pulse_env.update(DD2_AUDIO_PROCESS='chromium', LD_PRELOAD=str(reference / 'pulse_audio.so'))
            for label in ('canonical', 'inverse'):
                case = output / ('pulse-' + label); case.mkdir(); (case / 'audio').mkdir()
                for name in ('fixture.js', 'fixture.wasm', 'index.html', 'platform.c', 'expected-source.f32'):
                    shutil.copy2(output / label / name, case / name)
                pulse_env['DD2_PULSE_CAPTURE'] = str(case / 'audio')
                with (case / 'run.log').open('w') as log:
                    run_bounded(['node', str(ROOT / 'tools/browser/qa_s16_output.js'), str(case), '--pulse'],
                                directory=output, env=pulse_env, timeout=45, check=True,
                                stdout=log, stderr=subprocess.STDOUT)
                browser = json.loads((case / 'browser.json').read_text())
                require(browser['observations_valid'] and browser['backend'] == 'pulse' and
                        browser['accepted_stream_closed_before_browser_shutdown'] and browser['closed'] and
                        browser['chromium_version'] == VERSION and browser['sample_rate'] == 22050 and
                        browser['frames'] == 65536 and browser['channels'] == 2 and
                        browser['observer_source_sha256'] == sha(ROOT / 'tools/browser/qa_s16_output.js') and
                        browser['wasm_sha256'] == sha(case / 'fixture.wasm') and
                        browser['source_sha256'] == sha(case / 'actual-source.f32') == sha(case / 'expected-source.f32'),
                        'complete bound actual Pulse browser/source observation required')
                audio = summarize_pulse(case / 'audio')
                actual = (case / 'audio' / audio['file']).read_bytes()
                expected = (case / 'expected-source.f32').read_bytes()
                result = validate_pulse(actual, expected, canonical_source)
                require(result['canonical_float_sample_changes'] == (0 if label == 'canonical' else 65534),
                        'actual Float32 canonical encoding disagreement differs')
                begin = result['observed_source_begin_frame'] * 8
                altered_order = actual[:begin] + actual[begin + 8:begin + 16] + actual[begin:begin + 8] + actual[begin + 16:]
                swapped = b''.join(actual[i + 4:i + 8] + actual[i:i + 4] for i in range(0, len(actual), 8))
                wrong = (output / ('inverse' if label == 'canonical' else 'canonical') / 'expected-source.f32').read_bytes()
                for name, changed, model in (
                    ('source-bit', actual[:begin + 4] + bytes([actual[begin + 4] ^ 1]) + actual[begin + 5:], expected),
                    ('muted-device', b'\0' * len(actual), expected),
                    ('truncated-source', actual[:begin + 65535 * 8], expected),
                    ('reordered-frames', altered_order, expected), ('swapped-channels', swapped, expected),
                    ('wrong-encoding', actual, wrong),
                ):
                    try: validate_pulse(changed, model, canonical_source)
                    except RuntimeError: negative.append(dict(encoding='pulse-' + label, kind='accepted', mutation=name))
                    else: raise RuntimeError('accepted changed Pulse encoding/coverage: ' + name)
                pulse_cases.append(dict(encoding=label, experimental=label != 'canonical', **result,
                                        platform_sha256=sha(case / 'platform.c'), wasm_sha256=browser['wasm_sha256'],
                                        browser_report_sha256=sha(case / 'browser.json'),
                                        source_sha256=sha(case / 'actual-source.f32'), audio=audio))
                print('pulse-' + label, 'actual canonical Float32 sample changes:', result['canonical_float_sample_changes'], flush=True)
                check_space(output)
    require(all(sha(path) == digest for path, digest in immutable.items()),
            'verification sources changed during the actual run; refuse a mismatched report')
    report = dict(scope=__doc__, pass_=True, original_port_parity='unproven',
                  production_changed=False, supported_chromium_version=VERSION,
                  production_platform_sha256=sha(snapshot / 'dd2_movie_platform.c'),
                  primary_sources={name: provenance['sources'][name] for name in PINS},
                  reference_helper_sha256=sha(helper_source),
                  verifier_sha256=sha(Path(__file__)),
                  browser_observer_sha256=sha(ROOT / 'tools/browser/qa_s16_output.js'),
                  audio_observer_sha256=sha(ROOT / 'tools/reference/wine_audio.c'),
                  sample_clock_sha256=sha(ROOT / 'tools/reference/alsa_clock.c'),
                  cases=cases, pulse_cases=pulse_cases, pulse_server=pulse_metadata,
                  pulse_primary_sources=pulse_sources,
                  pulse_observer_sha256=sha(ROOT / 'tools/reference/pulse_audio.c') if pulse_enabled else None,
                  pulse_capture_helper_sha256=sha(ROOT / 'tools/reference/pulse.py') if pulse_enabled else None,
                  negative_controls=negative)
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    # Only completed fixture raw output is removed; preserve hashes and reports.
    opened = open_files(); removed = []
    labels = ['canonical', 'chromium', 'inverse'] + (['pulse-canonical', 'pulse-inverse'] if pulse_enabled else [])
    cleanup_files = []
    for case in (output / label for label in labels):
        cleanup_files.extend([*case.glob('*.pcm'), *case.glob('*.f32'), *case.glob('*.log'),
                              *(case / 'audio').glob('*.pcm'), *(case / 'audio').glob('*.jsonl')])
    if pulse_enabled: cleanup_files.append(output / 'pulse-server/daemon.log')
    for raw in cleanup_files:
        stat = raw.stat()
        require((stat.st_dev, stat.st_ino) not in opened and not raw.is_symlink(),
                'completed fixture raw output is still open: ' + str(raw))
        removed.append(dict(file=str(raw.relative_to(output)), bytes=stat.st_size, sha256=sha(raw)))
        raw.unlink()
    (output / 'cleanup.json').write_text(json.dumps(dict(removed=removed), indent=2) + '\n')
    check_space(output)
    print('PASS exhaustive actual Chromium S16 transport diagnosis;', len(negative), 'damaged data/clock/encoding cases rejected; '
          'inverse is experimental and original parity remains unproven', flush=True)


if __name__ == '__main__':
    main()
