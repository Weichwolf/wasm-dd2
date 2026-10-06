"""Owned real-time Pulse server and read-only accepted-client PCM capture.

Client acceptance is distinct from daemon consumption and physical playback.
Keep stream lifetime/format differences explicit; never align or replace PCM.
"""
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(__file__).with_name('pulse_audio.c')


def require(condition, message):
    if not condition: raise RuntimeError(message)


def sha(path):
    with Path(path).open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def build_pulse(output, bits=(32, 64)):
    libraries = []
    for width in bits:
        directory = output / ('lib' + str(width)); directory.mkdir(parents=True, exist_ok=True)
        subprocess.run(['gcc', '-m' + str(width), '-shared', '-fPIC', '-O2', '-Wall', '-Wextra',
                        '-Werror', '-pthread', str(SOURCE), '-ldl', '-o', str(directory / 'dd2_pulse.so')], check=True)
        libraries.append(directory)
    return libraries


@contextmanager
def pulse_server(output, rate=22050):
    require(rate in (22050, 44100, 48000), 'unsupported declared Pulse server rate')
    directory = output / 'pulse-server'; directory.mkdir()
    runtime = directory / 'runtime'; runtime.mkdir(mode=0o700)
    config = directory / 'daemon.conf'
    config.write_text(f'default-sample-rate = {rate}\nalternate-sample-rate = {rate}\n'
                      'default-sample-format = float32le\n')
    socket = directory / 'socket'
    require(len(str(socket).encode()) < 108, 'owned Pulse socket path exceeds the UNIX socket limit')
    startup = directory / 'startup.conf'
    startup.write_text(f'load-module module-native-protocol-unix socket={socket} auth-anonymous=1\n'
                       f'load-module module-null-sink sink_name=dd2 rate={rate} channels=2 format=float32le\n'
                       'set-default-sink dd2\n')
    env = {k: v for k, v in os.environ.items() if not k.startswith(('DD2_', 'PULSE_'))}
    env.update(PULSE_CONFIG=str(config), PULSE_RUNTIME_PATH=str(runtime), PULSE_STATE_PATH=str(runtime),
               XDG_RUNTIME_DIR=str(runtime), DBUS_SESSION_BUS_ADDRESS='unix:path=' + str(directory / 'no-session-bus'),
               DBUS_SYSTEM_BUS_ADDRESS='unix:path=' + str(directory / 'no-system-bus'), LC_ALL='C')
    command = ['pulseaudio', '-n', '--daemonize=no', '--use-pid-file=no', '--exit-idle-time=-1',
               '--log-target=stderr', '-F', str(startup)]
    with (directory / 'daemon.log').open('w') as log:
        process = subprocess.Popen(command, env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            deadline = time.monotonic() + 10
            while not socket.exists():
                require(process.poll() is None and time.monotonic() < deadline, 'owned Pulse server did not start')
                time.sleep(.05)
            env['PULSE_SERVER'] = 'unix:' + str(socket)
            info = subprocess.check_output(['pactl', '--server=' + env['PULSE_SERVER'], 'info'],
                                           env=env, text=True, timeout=5)
            require(f'Default Sample Specification: float32le 2ch {rate}Hz' in info,
                    'Pulse server must declare the source rate, rather than silently resample it')
            (directory / 'server-info.txt').write_text(info)
            version = subprocess.check_output(['pulseaudio', '--version'], text=True).strip()
            metadata = dict(command=command, pid=process.pid, version=version, declared_rate=rate,
                            configuration_sha256=sha(config), startup_sha256=sha(startup),
                            server_info_sha256=sha(directory / 'server-info.txt'))
            yield env, metadata
        finally:
            if process.poll() is None:
                process.terminate()
                try: process.wait(timeout=5)
                except subprocess.TimeoutExpired: process.kill(); process.wait()
            (directory / 'terminal.json').write_text(json.dumps(dict(exit_code=process.returncode)) + '\n')


def summarize_pulse(directory, require_closed=False):
    require(not (directory / 'error.txt').exists(), 'Pulse observer reported a capture error')
    journals = sorted(directory.glob('pulse-*.jsonl'))
    require(journals and set(directory.glob('*.pcm')) == {p.with_suffix('.pcm') for p in journals},
            'complete actual Pulse accepted-stream/journal pairs required')
    streams = []
    for journal in journals:
        events = [json.loads(line) for line in journal.read_text().splitlines()]
        require(len(events) >= 2 and events[0]['event'] == 'format', 'missing actual Pulse format')
        info = events[0]
        widths = {'float32le': 4, 's16le': 2}
        require(info['format'] in widths and type(info['channels']) is int and 0 < info['channels'] <= 32 and
                info['frame_bytes'] == info['channels'] * widths[info['format']] and
                type(info['rate']) is int and 0 < info['rate'] <= 768000, 'invalid actual Pulse format')
        closed = events[-1]['event'] == 'close'
        require(closed or not require_closed, 'actual Pulse client did not close before capture termination')
        total = 0; last_time = info['time_ns']
        for event in events[1:-1] if closed else events[1:]:
            require(event['event'] == 'write' and event['result'] == 0 and
                    last_time <= event['call_begin_ns'] <= event['call_end_ns'] <= event['time_ns'] and
                    event['offset_frames'] == total and type(event['requested_bytes']) is int and
                    event['requested_bytes'] > 0 and event['requested_bytes'] % info['frame_bytes'] == 0,
                    'invalid or nonsequential real Pulse write')
            total += event['requested_bytes'] // info['frame_bytes']; last_time = event['time_ns']
        if closed:
            require(events[-1]['result'] == 0 and events[-1]['time_ns'] >= last_time and
                    events[-1]['frames'] == total, 'Pulse accepted extent/close differs')
        require(total > 0 and journal.with_suffix('.pcm').stat().st_size == total * info['frame_bytes'],
                'Pulse accepted PCM file size differs from actual write extents')
        library = Path(info['client_library']).resolve()
        streams.append(dict(scope='actual accepted Pulse client writes; daemon consumption and output timing unproven',
                            **{**info, 'client_library': str(library)}, accepted_frames=total, closed=closed,
                            last_time_ns=events[-1]['time_ns'], file=journal.with_suffix('.pcm').name,
                            events=journal.name, sha256=sha(journal.with_suffix('.pcm')),
                            client_library_sha256=sha(library)))
    return streams
