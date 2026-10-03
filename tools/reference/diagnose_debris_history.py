#!/usr/bin/env python3
"""Controlled native-only diagnosis of inherited debris vertex state.

This is NOT original-game acceptance: GDB changes one native debris slot's
initial vertices, and the direct harness initializes the observed RNG/blink
states. The original executable and captures remain untouched. An unchanged
native control must fail, while copying just the inactive slot's 24 retained
vertex bytes must remove every complete-loop frame/palette/state difference.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

from compare_race_stream import EXE_SHA, ROOT, compare


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--native', type=Path, default=Path('/tmp/dd2_native'))
    parser.add_argument('--slot', type=int, required=True)
    parser.add_argument('--timeout', type=float, default=180)
    args = parser.parse_args()
    if not 0 <= args.slot < 128:
        parser.error('--slot must be in 0..127')
    reference_root = args.capture.resolve() / 'race'
    reference = json.loads((reference_root / 'race.json').read_text())
    if (reference.get('exe_modified') is not False or
            reference.get('exe_sha256') != EXE_SHA or
            hashlib.sha256((ROOT / 'DestructionDerby2/dd2h.exe').read_bytes()).hexdigest() != EXE_SHA or
            not reference.get('complete_racing_loop') or
            reference['first_state']['ticks'] != 0 or
            reference['level'] not in range(1, 11)):
        raise RuntimeError('complete supported unmodified original capture required')
    image = (reference_root / 'frame00000.image').read_bytes()
    if len(image) != 0x580400:
        raise RuntimeError('complete first original engine image required')
    slot = 0x7748e0 + args.slot * 124
    if image[slot - 0x400000 + 0x78]:
        raise RuntimeError('source slot must be inactive at the first presentation')
    address = slot + 0x40
    vertices = image[address - 0x400000:address - 0x400000 + 24]
    if not any(vertices):
        raise RuntimeError('original retained vertices must be nonzero')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    reference['_root'] = reference_root
    report = {'scope': __doc__, 'original_exe_sha256': EXE_SHA,
              'native_sha256': hashlib.sha256(args.native.read_bytes()).hexdigest(),
              'level': reference['level'], 'slot': args.slot,
              'original_vertex_sha256': hashlib.sha256(vertices).hexdigest(),
              'targets': []}
    env = {key: value for key, value in os.environ.items() if not key.startswith('DD2_')}
    for copy in (False, True):
        directory = output / ('inherited' if copy else 'unchanged')
        directory.mkdir()
        (directory / 'vertices.bin').write_bytes(vertices)
        # The only inferior write is to the native port, never to dd2h.exe.
        script = f'''set pagination off
set confirm off
break GetTickCount
run
python
import gdb
from pathlib import Path
inferior = gdb.selected_inferior()
directory = Path({str(directory)!r})
assert int.from_bytes(inferior.read_memory(0x936ff4, 4).tobytes(), 'little') == {reference['level']}
assert int.from_bytes(inferior.read_memory(0x7746c0, 4).tobytes(), 'little') == 0
assert inferior.read_memory({slot + 0x78}, 1).tobytes() == b'\\x00'
before = inferior.read_memory({address}, 24).tobytes()
assert before == bytes(24)
directory.joinpath('before.bin').write_bytes(before)
if {copy!r}:
    inferior.write_memory({address}, directory.joinpath('vertices.bin').read_bytes())
end
disable 1
continue
quit
'''
        (directory / 'controlled.gdb').write_text(script)
        image_counters = {reference['frames'][i]['cf'] for i in reference.get('diagnostic_image_frames', [])}
        experiment_env = dict(env, DD2_LEVEL=str(reference['level']), DD2_SOUND='1',
                              DD2_RANDOM_REFERENCE=str(reference_root / 'random.bin'),
                              DD2_RANDOM_LEVEL=str(reference['level']),
                              DD2_RACE_FLASH_INITIAL=str(reference['first_state']['demo_flash']),
                              DD2_TICK_REPLAY=str(reference_root / 'ticks.bin'),
                              DD2_RACE_STREAM=str(directory / 'race.jsonl'),
                              DD2_FRAMEDIR=str(directory), DD2_PALDUMP='1')
        if image_counters:
            experiment_env.update(DD2_IMGDUMP=','.join(map(str, sorted(image_counters))), DD2_IMGDUMP_FLIP='1')
        with (directory / 'run.log').open('w') as log:
            run = subprocess.run(['gdb', '-q', '-batch', '-x', str(directory / 'controlled.gdb'),
                                  str(args.native.resolve())], cwd=ROOT / 'DestructionDerby2',
                                 env=experiment_env, stdout=log, stderr=subprocess.STDOUT,
                                 timeout=args.timeout)
        text = (directory / 'run.log').read_text()
        if (run.returncode or 'exited normally' not in text or
                f'[clock-replay] consumed={reference["clock_calls"]} complete' not in text or
                f'[random-reference] consumed={reference["rng_calls"]} complete' not in text or
                'Traceback' in text):
            raise RuntimeError(f'incomplete controlled native run: {directory}')
        rows = [json.loads(line) for line in (directory / 'race.jsonl').read_text().splitlines()]
        failures = compare(reference, directory, rows)
        differences = []
        for failure in failures:
            if failure['error'] == 'bin bytes differ':
                index = failure['index']
                original = (reference_root / f'{reference["frames"][index]["prefix"]}.bin').read_bytes()
                port = (directory / f'f{rows[index]["flip"]:05d}.bin').read_bytes()
                differences.append({'index': index, 'pixels': sum(a != b for a, b in zip(original, port))})
        result = {'copied_initial_bytes': 24 if copy else 0, 'frames': len(rows),
                  'literal_original_match': not failures, 'failures': failures,
                  'differing_pixels': differences}
        report['targets'].append(result)
        (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        if copy and failures:
            raise RuntimeError('copying the retained slot does not fully explain this stream')
        if not copy and (not differences or any(f['error'] != 'bin bytes differ' for f in failures)):
            raise RuntimeError('unchanged control must isolate framebuffer differences only')
        print(f'{directory.name}: {len(rows)} frames, {len(failures)} differences', flush=True)
    report['controlled_initial_vertices_explain_all_differences'] = True
    report['naturally_calculated_history_acceptance'] = False
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
