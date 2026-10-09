#!/usr/bin/env python3
"""Verify prepared rendering and normal original-free game launches."""
import argparse
from datetime import datetime, timezone
from functools import partial
import hashlib
from http.server import ThreadingHTTPServer
import json
import os
from pathlib import Path
import subprocess
import sys
import threading
import time

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from assets.verify_render import image_compare
from assets.verify_tracks import compare
from rewrite.quality import ROOT
from rewrite.serve import BUILD, Handler
from rewrite.verify_window import NativeWindow

CODES = '123456789AB'
MODES = ('world', 'car', 'start', 'drive')


def digest(data):
    return hashlib.sha256(data).hexdigest()


def source_hash():
    files = [*ROOT.joinpath('src').rglob('*'), *ROOT.joinpath('tests').glob('*'),
             ROOT/'CMakeLists.txt', ROOT/'Makefile', Path(__file__),
             ROOT/'tools/assets/verify_game_browser.js', ROOT/'tools/rewrite/serve.py']
    return {str(p.relative_to(ROOT)): digest(p.read_bytes()) for p in sorted(files) if p.is_file()}


def native_checks(output, root, binary, label):
    ui = NativeWindow(output, root, binary, label=label, arguments=['--assets', str(root), '1'])
    cases = []
    target = 'sanitized' if label == 'sanitized' else 'native'
    def match(code, mode):
        expected = Image.open(output/f'{target}-{code}-{mode}.ppm').convert('RGB')
        cases.append(ui.match(expected, code+'-'+mode))
    try:
        ui.start()
        for code in CODES:
            match(code, 'world')
            ui.command('key', 'Tab'); match(code, 'car')
            ui.command('key', 'Tab')
            ui.command('key', 'Return')
            # Pause is retained across leaving driving and selecting another track.
            if code == '1':
                ui.command('key', 'p')
            ui.command('key', 'r')
            match(code, 'start')
            ui.command('key', 'Return')
            ui.command('key', 'Prior')
        match('1', 'world')
        ui.command('key', 'Return'); ui.command('key', 'r')
        baseline = ui.stable()
        ui.command('key', 'p'); ui.command('keydown', 'w')
        try:
            ui.wait(lambda: ui.image().tobytes() != baseline)
            time.sleep(.3)
        finally:
            ui.command('keyup', 'w')
        ui.command('key', 'p')
        moved = ui.stable()
        if moved == baseline:
            raise ValueError('Prepared real Native driving did not move')
        ui.image().save(output/(label+'-moving.png'))
        ui.command('key', 'r'); match('1', 'start')
        ui.command('key', 'Return')
        ui.command('key', 'Escape')
        if ui.process.wait(timeout=10) != 0:
            raise ValueError('Prepared normal Native close failed')
        return dict(pass_=True, comparisons=cases, real_input=True, moved_sha256=digest(moved),
                    paused_sha256=digest(baseline), all_eleven_tracks=True,
                    deterministic_reset=True, no_original_arguments=True)
    finally:
        ui.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT/'assets/runtime')
    parser.add_argument('--sanitized-build', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=WORK/'prepared-game-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents or WORK not in args.sanitized_build.resolve().parents:
        parser.error('Keep verification output/builds under /tmp/wasm-dd2/')
    root = args.runtime.resolve()
    output.mkdir(parents=True, exist_ok=True)
    initial = source_hash()
    report = dict(pass_=False, scope='Prepared game presentation and short original-free launches',
                  started=datetime.now(timezone.utc).isoformat(), cases=[], comparisons=[],
                  limitations=['New audio owner, liveries, body deformation, cockpit and sky pending',
                               'No full-game, quality-superiority or 60-FPS acceptance'], source_sha256=initial)
    environment = dict(os.environ, SDL_VIDEODRIVER='dummy',
                       ASAN_OPTIONS='detect_leaks=1:halt_on_error=1:exitcode=86',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1:exitcode=87')
    server = thread = None
    def run(command, name, timeout=120):
        check_space(output)
        log = output/(name+'.log')
        with log.open('wb') as stream:
            result = run_bounded(list(map(str,command)), directory=output, timeout=timeout,
                                 cwd=output, stdout=stream, stderr=subprocess.STDOUT, env=environment)
        raw=log.read_bytes()
        report.setdefault('commands',[]).append(dict(name=name, returncode=result.returncode, sha256=digest(raw)))
        if result.returncode or b'Sanitizer:' in raw or b'runtime error:' in raw:
            raise ValueError(name+': '+raw.decode(errors='replace'))
        log.unlink()
        return raw
    try:
        commands = {'native':[WORK/'rewrite-native/dd2_prepared_game_preview'],
                    'wasm':['node',WORK/'rewrite-wasm/dd2_prepared_game_preview.js'],
                    'sanitized':[args.sanitized_build/'dd2_prepared_game_preview']}
        for target in ('native','sanitized'):
            build = WORK/'rewrite-native' if target == 'native' else args.sanitized_build
            run([build/'dd2_track_draw_test'],target+'-pose-cache')
            run([build/'dd2_world_draw_test'],target+'-shared-world-cache')
            run([build/'dd2_prepared_application_test',root],target+'-application',timeout=180)
        run(['valgrind','--error-exitcode=86','--leak-check=full',WORK/'rewrite-native/dd2_track_draw_test'], 'memcheck-track-draw')
        for code in CODES:
            for mode in MODES:
                rows = {}
                for target,command in commands.items():
                    name=target+'-'+code+'-'+mode
                    raw=run([*command,root,output/(name+'.ppm'),code,mode],name)
                    rows[target]=[json.loads(line) for line in raw.splitlines()]
                # Geometry/state use exact counts; visible pixel counts belong to the image oracle.
                state_indices=(0,1,3)
                if any(not compare([rows['native'][i] for i in state_indices],
                                   [row[i] for i in state_indices]) for row in rows.values()):
                    raise ValueError('Cross-target prepared state/submission mismatch: '+code+' '+mode)
                stats=rows['native'][-1]
                if mode in ('start','drive') and (stats['world_triangles']==0 or stats['vehicle_models']==0 or
                                                  stats['body_lods'][0]==0 or stats['vehicle_triangles']==0):
                    raise ValueError('Empty prepared world/vehicle draw')
                report['cases'].append(dict(level=code,mode=mode,rows=rows))
                for target in ('wasm','sanitized'):
                    report['comparisons'].append(dict(level=code,mode=mode,target=target,
                        **image_compare(output/f'native-{code}-{mode}.ppm',output/f'{target}-{code}-{mode}.ppm')))
        report['native']=native_checks(output,root,WORK/'rewrite-native/dd2_app','native')
        report['sanitized']=native_checks(output,root,args.sanitized_build/'dd2_app','sanitized')
        server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(BUILD)))
        thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
        raw=run(['node',ROOT/'tools/assets/verify_game_browser.js',f'http://127.0.0.1:{server.server_port}/',output],
                'browser',timeout=420)
        report['browser']=json.loads((output/'browser.json').read_text())
        if not report['browser']['pass_']:
            raise ValueError('Actual prepared browser did not pass')
        if source_hash()!=initial:
            raise ValueError('Source changed during prepared game verification')
        for log in output.glob('*.log'):
            raw=log.read_text(errors='replace')
            if 'Sanitizer:' in raw or 'runtime error:' in raw:
                raise ValueError('Sanitizer finding in '+log.name)
        report['pass_']=True
    finally:
        if server is not None:
            server.shutdown();server.server_close();thread.join(timeout=5)
        report['finished']=datetime.now(timezone.utc).isoformat()
        (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(pass_=report['pass_'],cases=len(report['cases']),comparisons=len(report['comparisons']))))


if __name__=='__main__':
    main()
