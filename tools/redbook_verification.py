"""Build the patched CD backend and retain bounded component-test evidence."""
import hashlib
import json
from pathlib import Path
import subprocess

from artifacts import WORK, check_space, open_files, prepare_output

ROOT = Path(__file__).resolve().parents[1]


def add_backend_arguments(parser):
    parser.add_argument('--source-root', type=Path, default=ROOT / 'build',
                        help='patched production backend; run make patch first')
    parser.add_argument('--clean', action='store_true',
                        help='remove successful raw PCM after writing the report')


def output_directory(args, temporary):
    output = prepare_output(args.output) if args.output else temporary / 'results'
    if WORK not in output.parents:
        raise RuntimeError('Verification output must be inside /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False)
    return output


def compile_backends(args, temporary, fixture, game):
    source_root = args.source_root.resolve()
    required = ['dd2_cd.c', 'dd2h_stubs.c', 'dd2_cd.h', 'dd2_sound.h',
                'dd2_disc.h', 'dd2_sound_fir.h', 'dd2_sound_wide.h',
                'dd2_sound_mixwide.h', 'dd2_sound_gain.h', 'dd2_native.h',
                'dd2_movie.h', 'dd2_symbols.h', 'dd2_audio_service.h',
                'ghidra_compat.h']
    if not all((source_root / name).is_file() for name in required):
        raise RuntimeError('Missing patched backend; run make patch first')
    subprocess.run(['python3', str(ROOT / 'tools/generate_cd_toc.py'),
                    str(game / 'Redbook/disc.json'), str(temporary / 'dd2_disc.h')], check=True)
    if (source_root / 'dd2_disc.h').read_bytes() != (temporary / 'dd2_disc.h').read_bytes():
        raise RuntimeError('Patched backend TOC differs from provisioned disc')
    common = ['-O2', '-fno-strict-aliasing', '-std=gnu99', '-w',
              '-DDD2_NO_FOPEN_WRAP', '-ffunction-sections', '-fdata-sections',
              f'-I{temporary}', f'-I{source_root}', str(source_root / 'dd2_cd.c'),
              str(source_root / 'dd2h_stubs.c'), str(fixture), '-Wl,--gc-sections']
    native, asan, wasm = temporary / 'native', temporary / 'native-asan', temporary / 'wasm.js'
    subprocess.run(['gcc', '-m32', '-no-pie', *common, '-o', str(native)], check=True)
    subprocess.run(['gcc', '-m32', '-no-pie', '-fsanitize=address', *common, '-o', str(asan)], check=True)
    subprocess.run([args.emcc, *common, '-sNODERAWFS=1', '-sEXIT_RUNTIME=1',
                    '-sGLOBAL_BASE=10485760', '--pre-js', str(ROOT / 'tools/node_env.js'),
                    '-o', str(wasm)], check=True)
    targets = [('native', [str(native)], native), ('native-asan', [str(asan)], asan),
               ('wasm', [args.node, str(wasm)], wasm.with_suffix('.wasm'))]
    evidence = dict(source_root=str(source_root), optimization='O2',
                    source_sha256={name: hashlib.sha256((source_root / name).read_bytes()).hexdigest()
                                   for name in required},
                    disc_manifest_sha256=hashlib.sha256((game / 'Redbook/disc.json').read_bytes()).hexdigest(),
                    binary_sha256={name: hashlib.sha256(binary.read_bytes()).hexdigest()
                                   for name, _, binary in targets})
    return [(name, command) for name, command, _ in targets], evidence


def finish_report(output, report, clean):
    check_space(output)
    report['pass_'] = True
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    if clean:
        opened = open_files()
        for capture in output.rglob('*.pcm'):
            if capture.is_symlink():
                raise RuntimeError('Unexpected PCM symlink')
            stat = capture.stat()
            if (stat.st_dev, stat.st_ino) in opened:
                raise RuntimeError('Raw PCM is still open')
            capture.unlink()
