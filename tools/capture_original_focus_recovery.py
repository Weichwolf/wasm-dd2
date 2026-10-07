#!/usr/bin/env python3
"""Diagnose unchanged Original recovery after real USER32 foreground changes.

This records bounded, non-atomic input/activation observations. Optional hardware
breakpoints read actual DirectDraw HRESULTs. A completed capture is distinct from
successful keyboard recovery; neither proves video, PCM or whole-game parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference')]
from reference import capture
from verify_configuration_persistence import original_args
from artifacts import check_space, open_files


def digest(path):
    with path.open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    observers = parser.add_mutually_exclusive_group()
    observers.add_argument('--observe-ddraw-returns', action='store_true')
    observers.add_argument('--observe-wine-restore', action='store_true',
                           help='read guarded Debian Wine 10.0 Restore format/error sites and original returns')
    parser.add_argument('--mingw', default=shutil.which('i686-w64-mingw32-gcc') or
                        str(ROOT / 'deps/mingw-sdk/usr/bin/i686-w64-mingw32-gcc-win32'))
    args = parser.parse_args()
    out = args.output.resolve()
    if Path('/tmp/wasm-dd2') not in out.parents or out.exists():
        parser.error('Use a fresh directory under /tmp/wasm-dd2/')
    out.mkdir(parents=True)
    capture.WORK = out / 'work'
    source_names = ['tools/capture_original_focus_recovery.py',
                    'tools/reference/window_focus_recovery_probe.c',
                    'tools/reference/capture.py']
    observe = args.observe_ddraw_returns or args.observe_wine_restore
    if observe:
        source_names.append('tools/original_focus_recovery_observer.py')
    report = dict(scope=__doc__.strip(), capture_complete=False, recovery_=False,
                  original_port_full_parity='unproven', engine_state_writes=False,
                  debugger=observe, wine_restore_inspection=args.observe_wine_restore, xvfb_depth=16,
                  wine_version=subprocess.check_output(['wine', '--version'], text=True).strip(),
                  exe_sha256=digest(ROOT / 'DestructionDerby2/dd2h.exe'),
                  sources={name: digest(ROOT / name) for name in source_names},
                  actions=[], samples=[])
    subprocess.run([args.mingw, '-Wall', '-Wextra', '-Werror',
                    str(ROOT / 'tools/reference/window_focus_recovery_probe.c'),
                    '-o', str(out / 'window.exe')], check=True)
    report['focus_probe_sha256'] = digest(out / 'window.exe')

    def driver(pid, output, env, deadline, rundir):
        report['original_process'] = dict(pid=pid, display=env['DISPLAY'],
            start_token=Path(f'/proc/{pid}/stat').read_text().split(') ', 1)[1].split()[19])
        libraries = {}
        for line in Path(f'/proc/{pid}/maps').read_text().splitlines():
            fields = line.split(maxsplit=5)
            if len(fields) == 6 and Path(fields[5]).name in ('ddraw.dll', 'wined3d.dll'):
                path = Path(fields[5])
                libraries[path.name] = dict(path=str(path), sha256=digest(path))
        assert set(libraries) == {'ddraw.dll', 'wined3d.dll'}, 'actual Wine graphics mappings missing'
        report['wine_backends'] = libraries
        memory = open(f'/proc/{pid}/mem', 'rb', buffering=0)
        debugger = sink = None
        observer_log = None
        serial = 0

        def sample(label):
            def word(address):
                return int.from_bytes(os.pread(memory.fileno(), 4, address), 'little')
            row = dict(label=label, host_ns=time.monotonic_ns(),
                       active=word(0x46042c), timer=word(0x460474),
                       timer_fires=word(0x460484), phase=word(0x4699cc),
                       left=os.pread(memory.fileno(), 1, 0x463045)[0],
                       poly_list=word(0x940010))
            report['samples'].append(row)
            return row

        def wait(test, seconds=3):
            until = time.monotonic() + seconds
            while time.monotonic() < until:
                if test():
                    return True
                check_space(out)
                time.sleep(.02)
            return False

        def key(action):
            subprocess.run(['xdotool', action, 'Left'], env=env, check=True,
                           timeout=5, stdout=subprocess.DEVNULL)
            report['actions'].append(dict(key='Left', action=action, host_ns=time.monotonic_ns()))

        def foreground(target):
            nonlocal serial
            serial += 1
            (out / 'focus-request.tmp').write_text(f'{serial} {target}\n')
            (out / 'focus-request.tmp').replace(out / 'focus-request')
            report['actions'].append(dict(foreground_target=target, host_ns=time.monotonic_ns()))

        try:
            if observe:
                script = out / 'observer.gdb'
                script.write_text('set pagination off\nset confirm off\nset auto-solib-add off\n' +
                    f'attach {pid}\npython\nimport sys\nsys.path.insert(0,{str(ROOT / "tools")!r})\n' +
                    f'from original_focus_recovery_observer import run\nrun({str(out)!r},inspect_wine={args.observe_wine_restore!r})\nend\ndetach\nquit\n')
                observer_log = (out / 'observer.log').open('wb')
                debugger = subprocess.Popen(['gdb', '--nx', '-q', '-batch', '-x', str(script)],
                    env=env, stdout=observer_log, stderr=observer_log)
                assert wait(lambda: (out / 'observer-ready').exists()), 'observer did not attach'
            with (out / 'sink.log').open('wb') as log:
                sink = subprocess.Popen(['wine', str(out / 'window.exe'),
                    'Z:' + str(out / 'focus-request').replace('/', '\\')],
                    env=env, stdout=log, stderr=log)
                assert wait(lambda: '"kind":"ready"' in (out / 'sink.log').read_text()), 'sink not ready'
                initial = sample('baseline')
                assert initial['active'] == 1 and initial['left'] == 0
                key('keydown')
                assert wait(lambda: sample('await-left')['left'] == 1), 'initial Left not delivered'
                foreground('sink')
                assert wait(lambda: sample('await-inactive')['active'] == 0), 'deactivation not delivered'
                time.sleep(.4)
                inactive = sample('inactive-held')
                assert inactive['active'] == inactive['timer'] == 0 and inactive['left'] == 1
                key('keyup')
                time.sleep(.1)
                outside = sample('outside-release')
                assert outside['left'] == 1 and outside['active'] == outside['timer'] == 0
                foreground('game')
                assert wait(lambda: sample('await-active')['active'] == 1), 'reactivation not delivered'
                sample('reactivated')
                def restored_foreground():
                    records = []
                    for line in (out / 'sink.log').read_text().splitlines():
                        try:
                            records.append(json.loads(line))
                        except json.JSONDecodeError:
                            continue  # The helper may still be writing its last record.
                    game = next(row['game'] for row in records if row['kind'] == 'ready')
                    return any(row['kind'] == 'activation-request' and row['target'] == game
                               and row['foreground'] == game and row['return'] == 1
                               and row['iconic'] == 0 for row in records)
                assert wait(restored_foreground), 'restored game is not the actual foreground window'
                # A fresh actual press/release produces a physical release even
                # though the held engine flag survived the earlier outside one.
                key('keydown')
                time.sleep(.02)
                key('keyup')
                released = wait(lambda: sample('await-active-release')['left'] == 0, seconds=.6)
                last = sample('recovery-end')
                report['active_release_processed'] = released
                report['recovery_'] = released and last['active'] == 1 and last['timer'] != 0
        finally:
            (out / 'observer-stop').touch()
            if debugger:
                if debugger.poll() is None:
                    debugger.send_signal(signal.SIGINT)
                debugger.wait(timeout=5)
                observer_log.close()
                if debugger.returncode != 0:
                    raise RuntimeError('Original HRESULT observer failed')
            if sink and sink.poll() is None:
                sink.terminate()
                sink.wait(timeout=5)
            memory.close()

    options = original_args()
    options.timeout = 120
    try:
        capture.run(ROOT / 'DestructionDerby2', out, options, on_menu=driver)
        report['capture_complete'] = True
    except BaseException as error:
        report['exception'] = repr(error)
        raise
    finally:
        log = out / 'wine.log'
        if log.exists():
            data = log.read_bytes()
            report['wine_log'] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
                primary_restore_messages=data.count(b'Had to Restore Primary Surface!'),
                excerpt=data[-512:].decode(errors='replace'))
        sink_log = out / 'sink.log'
        if sink_log.exists():
            report['foreground_observations'] = [json.loads(line) for line in
                sink_log.read_text().splitlines() if line.startswith('{')]
        returns = out / 'ddraw-returns.json'
        if returns.exists():
            report['ddraw_returns'] = json.loads(returns.read_text())
        (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        check_space(out)

    # Capture.run waited for all clients of this dedicated Wine server. Closed
    # raw output can now go; the bounded observations and file hashes remain.
    candidates = [p for p in capture.WORK.rglob('*') if p.is_file() and not p.is_symlink()]
    candidates.extend(out / name for name in
        ('wine.log', 'xvfb.log', 'sink.log', 'observer.log', 'ddraw-returns.json', 'window.exe')
        if (out / name).exists())
    opened = open_files()
    removed = []
    for path in candidates:
        stat = path.stat()
        if (stat.st_dev, stat.st_ino) in opened:
            raise RuntimeError('Capture output remains open: ' + str(path))
        removed.append(dict(path=str(path.relative_to(out)), bytes=stat.st_size, sha256=digest(path)))
    (out / 'cleanup.json').write_text(json.dumps(dict(
        report_sha256=digest(out / 'report.json'),
        removed_bytes=sum(item['bytes'] for item in removed), removed=removed), indent=2) + '\n')
    for path in candidates:
        if capture.WORK not in path.parents:
            path.unlink()
    shutil.rmtree(capture.WORK)
    print('Original focus recovery:', 'bounded key release observed' if report['recovery_']
          else 'FAIL; completed diagnosis retained, no recovery acceptance')
    return 0 if report['recovery_'] else 1


if __name__ == '__main__':
    sys.exit(main())
