#!/usr/bin/env python3
"""Observe actual native movie rendering and ALSA accepted/played device output.

The real-time virtual ALSA device exposes literal sample extents and gaps.
This recorder does not align output, fit state or prove original/port parity.
No full video or engine memory images are captured.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import struct
import time

from artifacts import WORK, check_space, prepare_output
from reference.audio import build_audio, build_reset_fault, summarize_audio
from verify_configuration_persistence import ROOT, require
from verify_native_sdl import config


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--movie', choices=('Intro.avi', 'Outro.avi'), default='Intro.avi')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--skip', action='store_true', help='test real X11 key-up followed by cancellation key-down')
    parser.add_argument('--reset-errors', action='store_true', help='inject scoped ALSA reset errors after source playback')
    parser.add_argument('--clock-profile', action='store_true', help='observe movie-clock call sites in a frame-pointer native binary')
    parser.add_argument('--video-digest', action='store_true', help='hash every actual renderer frame through a bounded FIFO without retaining raw video')
    args = parser.parse_args()
    output = prepare_output(args.output)
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False);(output/'audio').mkdir()
    binary = args.binary.resolve();observer = output/'observer.so'
    subprocess.run(['gcc', '-m32', '-shared', '-fPIC', '-O2', '-Wall', '-Wextra', '-Werror',
                    *config('cflags'), str(ROOT/'tools/native_movie_observer.c'), *config('libs'),
                    '-ldl', '-o', str(observer)], check=True)
    libraries = build_audio(output/'audio-libraries')
    if args.reset_errors: build_reset_fault(libraries)
    clock_metadata = None
    if args.clock_profile:
        symbols = subprocess.check_output(['nm', '-S', str(binary)], text=True)
        matches = [s.split() for s in symbols.splitlines() if s.endswith(' dd2_movie_now_ms')]
        require(len(matches) == 1 and len(matches[0]) == 4, 'unique movie-clock symbol required')
        lower, size = (int(s, 16) for s in matches[0][:2])
        disassembly = subprocess.check_output(['objdump', '-d', '--disassemble=dd2_movie_now_ms', str(binary)], text=True)
        require('elf32-i386' in disassembly and 'push   %ebp' in disassembly and 'mov    %esp,%ebp' in disassembly,
                '32-bit frame-pointer movie-clock function required')
        (output/'movie-clock-disassembly.txt').write_text(disassembly)
        clock_observer = output/'clock-observer.so'
        subprocess.run(['gcc', '-m32', '-shared', '-fPIC', '-O2', '-fno-omit-frame-pointer',
                        '-Wall', '-Wextra', '-Werror', '-Wno-frame-address',
                        str(ROOT/'tools/native_movie_clock_observer.c'), '-ldl', '-o', str(clock_observer)], check=True)
        clock_metadata = dict(lower=lower, upper=lower+size,
                              source_sha256=sha(ROOT/'tools/native_movie_clock_observer.c'),
                              observer_sha256=sha(clock_observer), clock_domain='CLOCK_MONOTONIC')
    asound = output/'asound.conf'
    asound.write_text(f'pcm_type.dd2clock {{ lib "{output}/audio-libraries/$LIB/dd2_clock.so" }}\n'
                      'pcm.!default { type dd2clock }\n')
    process = display = video_reader = None
    video_digest = None
    with (output/'run.log').open('wb') as log:
        try:
            if args.video_digest:
                os.mkfifo(output/'video.pipe')
                reader = ('import hashlib,json,sys\n'
                          'h=hashlib.sha256();size=0\n'
                          'with open(sys.argv[1],"rb") as f:\n'
                          ' while block:=f.read(1048576):\n'
                          '  h.update(block);size+=len(block)\n'
                          'print(json.dumps(dict(bytes=size,sha256=h.hexdigest())))\n')
                video_reader = subprocess.Popen(['python3', '-c', reader, str(output/'video.pipe')],
                                                stdout=subprocess.PIPE, stderr=log)
            display = subprocess.Popen(['Xvfb', '-displayfd', '1', '-screen', '0', '640x480x24'],
                                       stdout=subprocess.PIPE, stderr=log)
            number = display.stdout.readline().decode().strip()
            require(number, 'native movie Xvfb failed')
            env = {k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
            env.update(DISPLAY=':'+number, DD2_WINDOW='1', DD2_MOVIE=args.movie.upper(),
                       SDL_AUDIODRIVER='alsa', ALSA_CONFIG_PATH=str(asound),
                       DD2_AUDIO_CAPTURE=str(output/'audio'), DD2_AUDIO_PROCESS=binary.name[:15],
                       DD2_AUDIO_RATE='22050', DD2_NATIVE_MOVIE_OBSERVE=str(output),
                       LD_PRELOAD=str(observer)+' dd2_audio.so', LD_LIBRARY_PATH=':'.join(map(str, libraries)))
            if args.reset_errors:
                env['LD_PRELOAD']='dd2_reset_fault.so '+env['LD_PRELOAD']
                env['DD2_RESET_FAULT_LOG']=str(output/'reset-fault.jsonl')
            if clock_metadata:
                env['LD_PRELOAD']=str(clock_observer)+' '+env['LD_PRELOAD']
                env.update(DD2_MOVIE_CLOCK_LOG=str(output/'movie-clock.jsonl'),
                           DD2_MOVIE_CLOCK_LOWER=format(lower,'x'), DD2_MOVIE_CLOCK_UPPER=format(lower+size,'x'))
            if args.video_digest:
                env['DD2_NATIVE_MOVIE_VIDEO_PIPE']='1'
            process = subprocess.Popen([str(binary)], cwd=ROOT/'DestructionDerby2',
                                       env=env, stdout=log, stderr=log)
            deadline = time.monotonic()+110
            control_done = False
            while process.poll() is None:
                require(time.monotonic() < deadline, 'native ALSA movie timed out')
                if args.skip and not control_done and (output/'events.jsonl').exists():
                    rows = [json.loads(s) for s in (output/'events.jsonl').read_text().splitlines()]
                    if sum(r['event'] == 'present' for r in rows) >= 26:
                        with open(f'/proc/{process.pid}/mem', 'rb', buffering=0) as memory:
                            def playing():
                                return struct.unpack('<I', os.pread(memory.fileno(), 4, 0x462cd4))[0]
                            require(playing() == 1, 'movie inactive before skip test')
                            control_env = {k:v for k,v in env.items() if k != 'LD_PRELOAD'}
                            subprocess.run(['xdotool', 'search', '--name', 'Destruction Derby 2',
                                            'windowfocus', 'keyup', 'Escape'], env=control_env, check=True, timeout=5)
                            time.sleep(.2)
                            require(process.poll() is None and playing() == 1, 'key-up skipped movie')
                            subprocess.run(['xdotool', 'search', '--name', 'Destruction Derby 2',
                                            'windowfocus', 'keydown', 'd'], env=control_env, check=True, timeout=5)
                        control_done = True
                check_space(output);time.sleep(.1)
            require(process.returncode == 0, 'native ALSA movie failed')
            require(not args.skip or control_done, 'movie exited before real skip keys')
            if video_reader:
                digest_output, _ = video_reader.communicate(timeout=10)
                require(video_reader.returncode == 0, 'actual video digest reader failed')
                video_digest = json.loads(digest_output)
        finally:
            if process and process.poll() is None:
                process.terminate();process.wait(timeout=5)
            if display:
                display.terminate();display.wait(timeout=5)
            if video_reader and video_reader.poll() is None:
                video_reader.terminate();video_reader.wait(timeout=5)
            if args.video_digest:
                (output/'video.pipe').unlink(missing_ok=True)
    audio = summarize_audio(output/'audio', require_played=True)
    events = [json.loads(s) for s in (output/'events.jsonl').read_text().splitlines()]
    frames = [r for r in events if r['event'] == 'present']
    require(frames and [r['frame'] for r in frames] == list(range(len(frames))),
            'incomplete native movie frame journal')
    require(all(r['exact_pixels'] == 640*480 for r in frames), 'actual renderer/texture readback differs')
    require(all(r.get('record_version') == 2 and r.get('clock_domain') == 'CLOCK_MONOTONIC' and
                r['readback_begin_ns'] <= r['readback_end_ns'] <= r['present_begin_ns'] <=
                r['present_end_ns'] <= r['time_ns'] for r in frames), 'actual presentation clock brackets missing')
    accepted = [r for r in audio['streams'] if r['format'] == 'S16_LE']
    played = [r for r in audio['played_streams'] if r['format'] == 'S16_LE']
    require(len(accepted) == len(played) == 1 and accepted[0]['closed'] and played[0]['closed'],
            'unique complete S16 movie device lifetime required')
    report = dict(scope=__doc__, observations_valid=True, original_port_parity='unproven',
                  binary_sha256=sha(binary), original_movie_sha256=sha(ROOT/'DestructionDerby2'/args.movie),
                  movie=args.movie, frames=len(frames), exact_rendered_pixels=len(frames)*640*480,
                  skip=args.skip, key_up_retained=args.skip and control_done,
                  observer_source_sha256=sha(ROOT/'tools/native_movie_observer.c'), audio=audio)
    if clock_metadata:
        clocks = [json.loads(s) for s in (output/'movie-clock.jsonl').read_text().splitlines()]
        require(len(clocks) >= len(frames) and all(e['clock_domain'] == 'CLOCK_MONOTONIC' for e in clocks),
                'movie-clock observations incomplete')
        report['clock_profile'] = dict(**clock_metadata, calls=len(clocks),
                                      journal_sha256=sha(output/'movie-clock.jsonl'))
    if video_digest:
        require(video_digest['bytes'] == len(frames)*640*480*4, 'actual video stream length differs')
        report['actual_video_digest'] = dict(**video_digest, frames=len(frames), format='opaque ARGB8888 little endian')
    if args.reset_errors:
        faults=[json.loads(s) for s in (output/'reset-fault.jsonl').read_text().splitlines()]
        require(faults and all(e['result']==-5 and e['pid']==accepted[0]['pid'] for e in faults),
                'scoped movie reset faults not observed')
        report.update(reset_errors=True,reset_faults=faults,
                      reset_fault_source_sha256=sha(ROOT/'tools/reference/alsa_reset_fault.c'))
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n');check_space(output)
    print('Actual native ALSA movie device recorded:', args.movie, len(frames),
          'frames,', accepted[0]['accepted_frames'], 'accepted and', played[0]['played_frames'],
          'played samples; original comparison remains separate', flush=True)


if __name__ == '__main__':
    main()
