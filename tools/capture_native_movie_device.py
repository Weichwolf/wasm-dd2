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
from reference.audio import build_audio, summarize_audio
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
    args = parser.parse_args()
    output = prepare_output(args.output)
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False);(output/'audio').mkdir()
    binary = args.binary.resolve();observer = output/'observer.so'
    subprocess.run(['gcc', '-m32', '-shared', '-fPIC', '-O2', '-Wall', '-Wextra', '-Werror',
                    *config('cflags'), str(ROOT/'tools/native_movie_observer.c'), *config('libs'),
                    '-ldl', '-o', str(observer)], check=True)
    libraries = build_audio(output/'audio-libraries')
    asound = output/'asound.conf'
    asound.write_text(f'pcm_type.dd2clock {{ lib "{output}/audio-libraries/$LIB/dd2_clock.so" }}\n'
                      'pcm.!default { type dd2clock }\n')
    process = display = None
    with (output/'run.log').open('wb') as log:
        try:
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
        finally:
            if process and process.poll() is None:
                process.terminate();process.wait(timeout=5)
            if display:
                display.terminate();display.wait(timeout=5)
    audio = summarize_audio(output/'audio', require_played=True)
    events = [json.loads(s) for s in (output/'events.jsonl').read_text().splitlines()]
    frames = [r for r in events if r['event'] == 'present']
    require(frames and [r['frame'] for r in frames] == list(range(len(frames))),
            'incomplete native movie frame journal')
    require(all(r['exact_pixels'] == 640*480 for r in frames), 'actual renderer/texture readback differs')
    accepted = [r for r in audio['streams'] if r['format'] == 'S16_LE']
    played = [r for r in audio['played_streams'] if r['format'] == 'S16_LE']
    require(len(accepted) == len(played) == 1 and accepted[0]['closed'] and played[0]['closed'],
            'unique complete S16 movie device lifetime required')
    report = dict(scope=__doc__, observations_valid=True, original_port_parity='unproven',
                  binary_sha256=sha(binary), original_movie_sha256=sha(ROOT/'DestructionDerby2'/args.movie),
                  movie=args.movie, frames=len(frames), exact_rendered_pixels=len(frames)*640*480,
                  skip=args.skip, key_up_retained=args.skip and control_done,
                  observer_source_sha256=sha(ROOT/'tools/native_movie_observer.c'), audio=audio)
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n');check_space(output)
    print('Actual native ALSA movie device recorded:', args.movie, len(frames),
          'frames,', accepted[0]['accepted_frames'], 'accepted and', played[0]['played_frames'],
          'played samples; original comparison remains separate', flush=True)


if __name__ == '__main__':
    main()
