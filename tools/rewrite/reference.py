#!/usr/bin/env python3
"""Provision the immutable reconstruction under /tmp, outside the rewrite tree."""
import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import sys
import tarfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, open_files

ROOT = Path(__file__).resolve().parents[2]
REVISION = 'b1111bd588a1afe12d7ba1663096b70ef6688af9'
DESTINATION = WORK / 'reference' / REVISION


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def blob_id(data):
    return hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()


def prepare_snapshot():
    actual = git('rev-parse', 'reconstruction-baseline^{commit}').decode().strip()
    if actual != REVISION:
        raise ValueError('The immutable reconstruction-baseline tag differs from the pinned reference')
    DESTINATION.mkdir(parents=True, exist_ok=True)
    check_space(DESTINATION)
    tree = {}
    for entry in git('ls-tree', '-rz', REVISION).split(b'\0'):
        if not entry:
            continue
        metadata, name = entry.split(b'\t', 1)
        mode, kind, object_id = metadata.decode().split()
        if kind == 'blob':
            if mode not in ('100644', '100755'):
                raise ValueError('Unsupported frozen reference file mode: ' + name.decode())
            tree[name.decode()] = (mode, object_id)
    opened = open_files()
    manifest = {}
    archive = git('archive', '--format=tar', REVISION)
    with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
        for entry in stream:
            if entry.isdir():
                continue
            relative = PurePosixPath(entry.name)
            if not entry.isfile() or relative.is_absolute() or '..' in relative.parts:
                raise ValueError('Unsafe reference archive entry: ' + entry.name)
            data = stream.extractfile(entry).read()
            mode, expected = tree.pop(entry.name)
            if blob_id(data) != expected:
                raise ValueError('Reference Git blob differs: ' + entry.name)
            path = DESTINATION / entry.name
            if path.is_symlink():
                raise ValueError('Frozen source cannot be a symlink: ' + str(path))
            if not path.exists() or path.read_bytes() != data:
                if path.exists() and (path.stat().st_dev, path.stat().st_ino) in opened:
                    raise RuntimeError('Changed frozen source is still in use: ' + str(path))
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
            path.chmod(0o755 if mode == '100755' else 0o644)
            manifest[entry.name] = dict(git_blob=expected, sha256=hashlib.sha256(data).hexdigest())
    if tree:
        raise ValueError('Incomplete frozen source archive')
    (DESTINATION / 'reference-identity.json').write_text(json.dumps(
        dict(commit=REVISION, tag='reconstruction-baseline', files=manifest), indent=2) + '\n')
    check_space(DESTINATION)
    return DESTINATION


def reference_file(name):
    relative = PurePosixPath(name)
    if relative.is_absolute() or '..' in relative.parts:
        raise ValueError('Use a relative tracked reference file')
    root = prepare_snapshot()
    manifest = json.loads((root / 'reference-identity.json').read_text())['files']
    if name not in manifest:
        raise ValueError('File is absent from the pinned reconstruction: ' + name)
    return root / relative


def prepare_game(directory):
    source = ROOT / 'DestructionDerby2'
    destination = directory / 'DestructionDerby2'
    destination.mkdir(exist_ok=True)
    # Shared immutable originals; the reference has its own mutable save card.
    for name in ('dd2h.exe', 'dd2.exe', 'Dirinfo', 'dd2_image.bin', 'Redbook'):
        original = source / name
        link = destination / name
        if original.exists() and not link.exists() and not link.is_symlink():
            link.symlink_to(original.resolve(), target_is_directory=original.is_dir())
    card = destination / 'SaveGames'
    if card.is_symlink():
        raise ValueError('Reference SaveGames must not share mutable original state')
    if not card.exists() and (source / 'SaveGames').is_file():
        shutil.copyfile(source / 'SaveGames', card)
    image = directory / 're_out/dd2_image.bin'
    if not image.exists() and (source / 'dd2_image.bin').is_file():
        shutil.copyfile(source / 'dd2_image.bin', image)
    # Existing reusable reference build dependencies can be borrowed without
    # copying them into the rewrite or downloading another SDK.
    dependencies = directory / 'third_party'
    dependencies.mkdir(exist_ok=True)
    for name in ('sdl2-sdk', 'mingw-sdk', 'mingw-reference'):
        original = ROOT / 'deps' / name
        link = dependencies / name
        if original.exists() and not link.exists() and not link.is_symlink():
            link.symlink_to(original.resolve(), target_is_directory=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--file', help='Print the verified path to a tracked reference file')
    parser.add_argument('--make', metavar='TARGET', help='Run a frozen reference Makefile target')
    args = parser.parse_args()
    if args.file:
        print(reference_file(args.file))
        return
    directory = prepare_snapshot()
    prepare_game(directory)
    if args.make:
        subprocess.run(['make', '-C', str(directory), args.make,
                        'NATIVE=' + str(directory / 'dd2_native'),
                        'OUTJS=' + str(directory / 'dd2run.js')], check=True)
    else:
        print(directory)


if __name__ == '__main__':
    main()
