#!/usr/bin/env python3
"""Exercise the native/WASM MCI backend and compare its entire PCM output to CDDA.

This verifies source playback with a matched clock, not the Windows hardware
mixer or complete game behavior.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

from artifacts import WORK, check_space, open_files, prepare_output, run_bounded

ROOT = Path(__file__).resolve().parent.parent


def compare_stream(actual, source, count, label):
    """Compare the entire externally specified extent without loading it in RAM."""
    remaining = count
    digest = hashlib.sha256()
    while remaining:
        size = min(1024 * 1024, remaining)
        expected = source.read(size)
        observed = actual.read(size)
        if len(expected) != size or observed != expected:
            raise RuntimeError(f'{label}: full CDDA output differs with {remaining} bytes remaining')
        digest.update(observed)
        remaining -= size
    return digest.hexdigest()


def compare_output(output, game, tracks):
    complete = []
    with output.open('rb') as actual:
        for index, track in enumerate(tracks):
            start = index * 977
            first = (start + 101) * 44100 // 1000 - start * 44100 // 1000
            resumed = (start + 977) * 44100 // 1000 - (start + 878) * 44100 // 1000
            with (game / 'Redbook' / track['file']).open('rb') as source:
                compare_stream(actual, source, first * 4, f"track {track['number']} prefix")
                # STOP/TO-only PLAY repeats the current fractional CD sector.
                source.seek(first // 588 * 588 * 4)
                compare_stream(actual, source, resumed * 4, f"track {track['number']} restart")
        for track in tracks:
            count = track['sample_frames'] * 4
            with (game / 'Redbook' / track['file']).open('rb') as source:
                observed = compare_stream(actual, source, count, f"complete track {track['number']}")
                if source.read(1) or observed != track['sha256']:
                    raise RuntimeError('Provisioned complete CDDA differs from disc manifest')
            complete.append(dict(track=track['number'], bytes=count, sha256=observed))
        began = 18 * 977 + 18 * 500000
        resumed = began + 73 + 911
        paused = ((began + 73) * 44100 // 1000 - began * 44100 // 1000 +
                  (resumed + 27) * 44100 // 1000 - resumed * 44100 // 1000)
        with (game / 'Redbook' / tracks[1]['file']).open('rb') as source:
            compare_stream(actual, source, paused * 4, 'MCI pause/resume')
        with (game / 'Redbook' / tracks[0]['file']).open('rb') as source:
            source.seek(-2 * 2352, os.SEEK_END)
            compare_stream(actual, source, 2 * 2352, 'track02 boundary suffix')
        with (game / 'Redbook' / tracks[1]['file']).open('rb') as source:
            compare_stream(actual, source, 2 * 2352, 'track03 boundary prefix')
        if actual.read(1):
            raise RuntimeError('Extra samples beyond the final controlled interval')
    return complete


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emcc", default="emcc")
    parser.add_argument("--node", default="node")
    parser.add_argument('--source-root', type=Path, default=ROOT / 'build',
                        help='patched production backend directory; run make patch first')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--clean', action='store_true',
                        help='remove successful raw PCM after writing the report')
    args = parser.parse_args()
    source_root = args.source_root.resolve()
    sources = ['dd2_cd.c', 'dd2h_stubs.c', 'dd2_cd.h', 'dd2_sound.h', 'dd2_sound_fir.h', 'dd2_sound_wide.h']
    if not all((source_root / name).is_file() for name in sources):
        raise RuntimeError('Missing patched backend; run make patch first')
    WORK.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="redbook-build-", dir=WORK) as tmp:
        directory = Path(tmp)
        output = prepare_output(args.output) if args.output else directory / 'captures'
        if WORK not in output.parents:
            raise RuntimeError('Verification output must be inside /tmp/wasm-dd2/')
        output.mkdir(parents=True, exist_ok=False)
        game = ROOT / "DestructionDerby2"
        subprocess.run(["python3", str(ROOT / "tools/generate_cd_toc.py"),
                        str(game / "Redbook/disc.json"), str(directory / "dd2_disc.h")], check=True)
        common = ["-std=gnu99", "-w", "-DDD2_NO_FOPEN_WRAP", "-ffunction-sections", "-fdata-sections", f"-I{directory}",
                  f"-I{source_root}", str(source_root / "dd2_cd.c"),
                  str(source_root / "dd2h_stubs.c"),str(ROOT / "tools/redbook_test.c"),"-Wl,--gc-sections"]
        native, asan, wasm = directory / "native", directory / 'native-asan', directory / "wasm.js"
        subprocess.run(["gcc", "-m32", "-no-pie", '-O2', '-fno-strict-aliasing', *common, "-o", str(native)], check=True)
        subprocess.run(["gcc", "-m32", "-no-pie", '-O2', '-fno-strict-aliasing', '-fsanitize=address', *common, "-o", str(asan)], check=True)
        subprocess.run([args.emcc, '-O2', '-fno-strict-aliasing', *common, "-sNODERAWFS=1", "-sEXIT_RUNTIME=1",
                        '-sGLOBAL_BASE=10485760', '--pre-js', str(ROOT / 'tools/node_env.js'),
                        "-o", str(wasm)], check=True)
        env = {key: value for key, value in os.environ.items() if not key.startswith("DD2_")}
        env["DD2_CD_ROOT"] = str(game / "Redbook")
        tracks = json.loads((game / "Redbook/disc.json").read_text())["tracks"][1:]
        if len(tracks) != 18 or [t['number'] for t in tracks] != list(range(2, 20)):
            raise RuntimeError('All eighteen original physical audio tracks required')
        report = dict(scope=__doc__, pass_=False, source_root=str(source_root),
                      source_sha256={name: hashlib.sha256((source_root / name).read_bytes()).hexdigest() for name in sources},
                      fixture_sha256=hashlib.sha256((ROOT / 'tools/redbook_test.c').read_bytes()).hexdigest(),
                      disc_manifest_sha256=hashlib.sha256((game / 'Redbook/disc.json').read_bytes()).hexdigest(),
                      targets={})
        captures = []
        for name, command, binary in (("native", [str(native)], native),
                                      ('native-asan', [str(asan)], asan),
                                      ("wasm", [args.node, str(wasm)], wasm.with_suffix('.wasm'))):
            capture = output / f'{name}.pcm'
            with (output / f'{name}.log').open('wb') as log:
                run_bounded([*command, str(capture)], directory=output, env=env, cwd=game,
                            stdout=log, stderr=subprocess.STDOUT, check=True, timeout=240)
            log = (output / f'{name}.log').read_text(errors='replace')
            if 'AddressSanitizer' in log or 'runtime error:' in log:
                raise RuntimeError(f'{name}: sanitizer diagnostic')
            complete = compare_output(capture, game, tracks)
            report['targets'][name] = dict(pass_=True, complete_tracks=complete,
                pcm_bytes=capture.stat().st_size, binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest())
            captures.append(capture)
            check_space(output)
            print(f'PASS {name}: all 18 complete tracks and transport intervals, {capture.stat().st_size} exact PCM bytes', flush=True)
        report['pass_'] = True
        (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        if args.clean:
            opened = open_files()
            for capture in captures:
                stat = capture.stat()
                if capture.is_symlink() or (stat.st_dev, stat.st_ino) in opened:
                    raise RuntimeError('Raw capture remains in use')
                capture.unlink()


if __name__ == "__main__":
    main()
