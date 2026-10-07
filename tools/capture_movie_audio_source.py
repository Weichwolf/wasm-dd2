#!/usr/bin/env python3
"""Capture actual Wine ACM source PCM from unchanged original movie packets.

This provides a reproducible source reference for live device comparisons.
It does not establish original game playback, device tails or A/V timing.
Keep the PCM only until its current comparison has been reported.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess

from artifacts import WORK, check_space, prepare_output
from verify_configuration_persistence import ROOT, require
from verify_movie_avi import original_metadata
from verify_sound_cursor import wine_probe


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--mingw', default=shutil.which('i686-w64-mingw32-gcc') or
                        str(ROOT / 'deps/mingw-sdk/usr/bin/i686-w64-mingw32-gcc-win32'))
    args = parser.parse_args();output = prepare_output(args.output)
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False)
    fixture = output/'movie_audio_test.c'
    fixture.write_bytes((ROOT/'tools/movie_audio_test.c').read_bytes())
    exe = output/'audio.exe'
    subprocess.run([args.mingw, '-O2', '-Wall', '-Wextra', '-Werror', str(fixture),
                    '-lmsacm32', '-o', str(exe)], check=True)
    report = dict(scope=__doc__, fixture_source_sha256=sha(fixture), fixture_exe_sha256=sha(exe),
                  capture_source_sha256=sha(Path(__file__)),
                  wine_version=subprocess.check_output(['wine', '--version'], text=True).strip(), films=[])
    env = {k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    for filename in ('Intro.avi', 'Outro.avi'):
        movie = ROOT/'DestructionDerby2'/filename
        metadata, _, format_, blocks = original_metadata(movie)
        film = output/movie.stem.lower();film.mkdir()
        bundle = film/'audio.packets';pcm = film/'wine.pcm'
        bundle.write_bytes(struct.pack('<II', len(format_), len(blocks))+format_+blocks)
        actual = wine_probe(exe, film, env,
                            arguments=tuple('Z:'+str(p).replace('/', '\\') for p in (bundle, pcm)))
        expected = dict(rate=metadata['pcm_rate'], channels=metadata['pcm_channels'],
                        frames=metadata['pcm_frames'], bytes=metadata['pcm_frames']*metadata['pcm_channels']*2)
        require(actual == expected and pcm.stat().st_size == expected['bytes'],
                'actual Wine ACM source extent differs from original AVI metadata')
        shutil.rmtree(film/'wine-prefix');bundle.unlink()
        report['films'].append(dict(file=filename, avi_sha256=sha(movie), metadata=metadata,
                                    pcm_sha256=sha(pcm), acm=actual))
        (output/'report.json').write_text(json.dumps(report, indent=2)+'\n');check_space(output)
        print('Actual Wine ACM source:', filename, actual['bytes'], 'PCM bytes', flush=True)


if __name__ == '__main__':
    main()
