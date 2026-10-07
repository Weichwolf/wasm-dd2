#!/usr/bin/env python3
"""Compare typed single-player league rules with unmodified original x86.

All 6,240 explicit transfer, score-clear, sort and season-transition cases cover
valid driver permutations, ties, player divisions/ranks and original history and
unlock inputs. An additional original initial-grid case checks the constructor.
Compare only points, divisions, ranks and standing classifications. This does
not establish complete championships, their UI, history, persistence or races.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from verify_season_transition import EXE_SHA

CASES = 6241
RECORD = struct.Struct('<7I60H')


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def reference(data):
    if len(data) != CASES * RECORD.size:
        raise ValueError('Incomplete original league output')
    rows = []
    for values in RECORD.iter_unpack(data):
        rows.append(dict(case=list(values[:6]), standing=values[6],
                         drivers=[list(values[7 + index * 3:10 + index * 3]) for index in range(20)]))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-league-verification')
    parser.add_argument('--native-build', type=Path, default=WORK / 'rewrite-native')
    parser.add_argument('--wasm-build', type=Path, default=WORK / 'rewrite-wasm')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    exe = ROOT / 'DestructionDerby2/dd2h.exe'
    if digest(exe) != EXE_SHA:
        raise ValueError('Provision the supported unmodified original executable')
    sources = [ROOT / name for name in ('src/game/league.c', 'src/game/league.h',
               'tests/league_test.c', 'tests/league_export.c',
               'tools/reference/rewrite_league_fixture.c', 'tools/reference/pe_fixture.h',
               'CMakeLists.txt', 'Makefile')]
    sources.append(Path(__file__).resolve())
    source_sha256 = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    calls = []

    def run(command, label):
        log = output / (label + '.log')
        with log.open('wb') as stream:
            result = run_bounded(command, directory=output, cwd=ROOT, timeout=120,
                                 stdout=stream, stderr=subprocess.STDOUT)
        content = log.read_text(errors='replace')
        calls.append(dict(label=label, returncode=result.returncode, sha256=digest(log)))
        if result.returncode or 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(label + ' failed: ' + content[-3000:])
        return log

    original = output / 'original-x86'
    fixture = ROOT / 'tools/reference/rewrite_league_fixture.c'
    run(['gcc', '-m32', '-no-pie', '-O0', '-std=gnu99', '-w', str(fixture),
         '-o', str(original)], 'original-build')
    raw = output / 'original.bin'
    run([str(original), str(exe), str(raw)], 'original-run')
    expected = reference(raw.read_bytes())
    original_output_sha = digest(raw)
    flags = ['-std=c11', '-O1', '-g', '-I', str(ROOT / 'src'),
             '-Wall', '-Wextra', '-Wpedantic', '-Wno-unused-parameter', '-Wno-unused-function',
             '-fno-strict-aliasing', '-ffast-math', '-Werror', '-Wshadow', '-Wconversion',
             '-Wstrict-prototypes', '-Wmissing-prototypes', '-Wformat=2',
             '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    sanitized, synthetic = output / 'export-sanitized', output / 'test-sanitized'
    for name, binary in (('league_export', sanitized), ('league_test', synthetic)):
        run([tool('clang'), *flags, str(ROOT / 'src/game/league.c'),
             str(ROOT / ('tests/' + name + '.c')), '-o', str(binary)], name + '-build')
    run([str(synthetic)], 'synthetic-sanitized')
    targets = {}
    commands = {'native': [str(args.native_build / 'dd2_league_export')],
                'wasm': ['node', str(args.wasm_build / 'dd2_league_export.js')],
                'sanitized': [str(sanitized)]}
    binaries = [original, sanitized, synthetic, args.native_build / 'dd2_league_export',
                args.wasm_build / 'dd2_league_export.js',
                args.wasm_build / 'dd2_league_export.wasm']
    binary_sha256 = {str(path): digest(path) for path in binaries}
    for target, command in commands.items():
        path = run(command, target)
        rows = [json.loads(line) for line in path.read_text().splitlines()]
        if rows != expected:
            first = next((index for index, (left, right) in enumerate(zip(rows, expected))
                          if left != right), min(len(rows), len(expected)))
            raise ValueError(f'Original league differs on {target} at case {first}')
        targets[target] = dict(pass_=True, cases=len(rows), stdout_sha256=digest(path))
    if source_sha256 != {str(path.relative_to(ROOT)): digest(path) for path in sources}:
        raise ValueError('League verification sources changed during the run')
    if binary_sha256 != {str(path): digest(path) for path in binaries}:
        raise ValueError('League verification binaries changed during the run')
    report = dict(pass_=True, scope=__doc__.strip(), cases=CASES,
                  verified_at=datetime.now(timezone.utc).isoformat(), targets=targets,
                  original_exe_sha256=EXE_SHA, original_output_sha256=original_output_sha,
                  calls=calls, source_sha256=source_sha256, binary_sha256=binary_sha256)
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    for path in (raw, original, sanitized, synthetic):
        path.unlink()
    for call in calls:
        (output / (call['label'] + '.log')).unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, cases=CASES, report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
