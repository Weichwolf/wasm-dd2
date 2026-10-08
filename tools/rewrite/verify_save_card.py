#!/usr/bin/env python3
"""Check the owned Windows save container on Native, WASM and ASan/UBSan.

Default coverage reads the provisioned original image without changing it.
--original-cards additionally consumes bounded actual-original checkpoints from
an isolated reference run; it never runs reconstruction commands on master.
Opaque payload compatibility is separate from settings/game/replay codecs and
durable Native/browser storage adapters.
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

CARD_BYTES = 131072
HEADER_BYTES = 512
BLOCK_BYTES = 8192
SLOTS = 15
NAME_LIMIT = 8


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def entries(data):
    if len(data) != CARD_BYTES:
        raise ValueError('Original image extent differs')
    result = []
    for physical in range(SLOTS):
        header = physical * HEADER_BYTES
        occupied = struct.unpack_from('<I', data, header)[0]
        if occupied not in (0, 1):
            raise ValueError('Unsupported original occupancy')
        if not occupied:
            continue
        name = data[header+4:header+4+NAME_LIMIT+1]
        if b'\0' not in name:
            raise ValueError('Unterminated original filename')
        name = name.split(b'\0', 1)[0]
        result.append(dict(physical=physical,
                           name_hex=(name + bytes(NAME_LIMIT+1-len(name))).hex(),
                           magic=struct.unpack_from('<H', data, (physical+1)*BLOCK_BYTES)[0]))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK/'rewrite-save-card-verification')
    parser.add_argument('--original-cards', type=Path)
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    calls = []
    paths = [ROOT/'src/assets/save_card.c', ROOT/'src/assets/save_card.h',
             ROOT/'src/platform/file.c', ROOT/'tests/save_card_test.c',
             ROOT/'tests/save_card_export.c', Path(__file__), ROOT/'CMakeLists.txt']
    source = {str(path.relative_to(ROOT)): digest(path) for path in paths}

    def run(command, label):
        log = output/(label+'.log')
        with log.open('wb') as stream:
            result = run_bounded(command, directory=output, timeout=120, cwd=ROOT,
                                 stdout=stream, stderr=subprocess.STDOUT)
        text = log.read_text(errors='replace')
        calls.append(dict(label=label, command=command, returncode=result.returncode,
                          log_sha256=digest(log)))
        if result.returncode or 'Sanitizer:' in text or 'runtime error:' in text:
            raise RuntimeError(label+' failed: '+text[-6000:])
        return text

    flags = ['-std=c11', '-O1', '-g', '-I'+str(ROOT/'src'),
             '-Wall', '-Wextra', '-Wpedantic', '-Wno-unused-parameter', '-Wno-unused-function',
             '-fno-strict-aliasing', '-ffast-math', '-Werror', '-Wshadow', '-Wconversion',
             '-Wstrict-prototypes', '-Wmissing-prototypes', '-Wformat=2',
             '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    for name in ('test', 'export'):
        units = [str(ROOT/'src/assets/save_card.c'), str(ROOT/f'tests/save_card_{name}.c')]
        if name == 'export':
            units.append(str(ROOT/'src/platform/file.c'))
        run([tool('clang'), *flags, *units, '-o', str(output/(name+'-sanitized'))],
            'build-'+name+'-sanitized')
    tests = {'native': [str(WORK/'rewrite-native/dd2_save_card_test')],
             'wasm': ['node', str(WORK/'rewrite-wasm/dd2_save_card_test.js')],
             'sanitized': [str(output/'test-sanitized')]}
    exporters = {'native': [str(WORK/'rewrite-native/dd2_save_card_export')],
                 'wasm': ['node', str(WORK/'rewrite-wasm/dd2_save_card_export.js')],
                 'sanitized': [str(output/'export-sanitized')]}
    binaries = {str(Path(command[-1])): digest(Path(command[-1]))
                for command in [*tests.values(), *exporters.values()]}
    for path in (WORK/'rewrite-wasm').glob('dd2_save_card_*.wasm'):
        binaries[str(path)] = digest(path)
    image = ROOT/'DestructionDerby2/SaveGames'
    original_hash = digest(image)
    fixtures = [('provisioned', image)]
    if args.original_cards is not None:
        fixtures += [(path.stem, path) for path in sorted(args.original_cards.glob('*.card'))]
        needed = {'initial', 'saved-A', 'saved-A-B', 'deleted-A', 'loaded-B', 'saved-C-B'}
        if not needed.issubset({name for name, _ in fixtures}):
            raise ValueError('Supply all six original isolated multiple-card checkpoints')
    comparisons = []
    temporary = []
    for target, command in exporters.items():
        run(tests[target], 'synthetic-'+target)
        for label, fixture in fixtures:
            generated = output/(label+'-'+target+'.card')
            text = run([*command, str(fixture), str(generated), 'inspect', '0', '', ''],
                       label+'-'+target)
            actual = json.loads(text)
            if generated.read_bytes() != fixture.read_bytes() or actual != dict(
                    result=0, entries=entries(fixture.read_bytes())):
                raise ValueError('Original complete image or independent entries differ')
            comparisons.append(dict(target=target, case=label, operation='inspect',
                                    pass_=True, original_sha256=digest(fixture),
                                    output_sha256=digest(generated)))
            temporary.append(generated)
            for logical, entry in enumerate(entries(fixture.read_bytes())):
                generated = output/(label+'-block-'+str(logical)+'-'+target+'.bin')
                text = run([*command, str(fixture), str(generated), 'read', str(logical), '', ''],
                           label+'-read-'+str(logical)+'-'+target)
                physical = entry['physical']
                expected = fixture.read_bytes()[(physical+1)*BLOCK_BYTES:(physical+2)*BLOCK_BYTES]
                if generated.read_bytes() != expected or json.loads(text) != actual:
                    raise ValueError('Selected complete original physical payload differs')
                comparisons.append(dict(target=target, case=label, operation='read',
                                        logical=logical, physical=physical, pass_=True,
                                        original_sha256=hashlib.sha256(expected).hexdigest(),
                                        output_sha256=digest(generated)))
                temporary.append(generated)
        if args.original_cards is not None:
            for before, after, operation, slot, name, physical in (
                    ('initial', 'saved-A', 'put', 0, 'A', 0),
                    ('saved-A', 'saved-A-B', 'put', 1, 'B', 1),
                    ('saved-A-B', 'deleted-A', 'delete', 0, '', None),
                    ('loaded-B', 'saved-C-B', 'put', 1, 'C', 0)):
                first = args.original_cards/(before+'.card')
                final = args.original_cards/(after+'.card')
                payload = output/(after+'-payload.bin')
                generated = output/(after+'-updated-'+target+'.card')
                if operation == 'put':
                    payload.write_bytes(final.read_bytes()[(physical+1)*BLOCK_BYTES:
                                                          (physical+2)*BLOCK_BYTES])
                    temporary.append(payload)
                text = run([*command, str(first), str(generated), operation,
                            str(slot), name, str(payload)], after+'-mutation-'+target)
                if json.loads(text) != dict(result=0, entries=entries(final.read_bytes())) or \
                        generated.read_bytes() != final.read_bytes():
                    raise ValueError('Actual original complete mutation differs')
                comparisons.append(dict(target=target, case=before+' -> '+after,
                                        operation=operation, pass_=True,
                                        original_sha256=digest(final),
                                        output_sha256=digest(generated)))
                temporary.append(generated)
            # Correct the original's logical/physical duplicate exclusion bug.
            if (args.original_cards/'duplicate-B-B.card').exists():
                first = args.original_cards/'deleted-A.card'
                payload = output/'duplicate-payload.bin'
                payload.write_bytes((args.original_cards/'duplicate-B-B.card').read_bytes()[
                    BLOCK_BYTES:2*BLOCK_BYTES])
                generated = output/('duplicate-rejected-'+target+'.card')
                text = run([*command, str(first), str(generated), 'put', '1', 'B', str(payload)],
                           'compacted-duplicate-rejected-'+target)
                if json.loads(text) != dict(result=5, entries=entries(first.read_bytes())) or \
                        generated.read_bytes() != first.read_bytes():
                    raise ValueError('Compacted duplicate must preserve the entire original image')
                comparisons.append(dict(target=target, case='compacted duplicate',
                                        operation='rejected-put', pass_=True,
                                        original_sha256=digest(first), output_sha256=digest(generated)))
                temporary.extend([payload, generated])
    if digest(image) != original_hash or any(digest(ROOT/key) != value for key,value in source.items()):
        raise ValueError('Provisioned save or verification sources changed during the run')
    report = dict(pass_=True, time=datetime.now(timezone.utc).isoformat(),
                  scope='Owned opaque Windows card: independent image/entry reads, complete original mutations when supplied, and synthetic bounds/ownership/rollback on three targets; no typed settings/game/replay, durable adapter or frontend acceptance',
                  source_sha256=source, binaries_sha256=binaries, calls=calls,
                  comparisons=comparisons, original_save_unchanged=True,
                  actual_original_mutations=args.original_cards is not None)
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    for path in set(temporary):
        path.unlink(missing_ok=True)
    for path in output.glob('*.log'):
        path.unlink()
    (output/'test-sanitized').unlink()
    (output/'export-sanitized').unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, comparisons=len(comparisons),
                         actual_original_mutations=report['actual_original_mutations']), indent=2))


if __name__ == '__main__':
    main()
