"""Temporary verification output, bounded subprocesses and stale log cleanup."""
import argparse
import atexit
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
WORK = Path('/tmp/wasm-dd2')
MAX_BYTES = 2 * 1024**3
MIN_FREE = 1024**3


def temporary_output(path):
    path = Path(path).resolve()
    if Path('/tmp') not in path.parents:
        raise ValueError('Verification output must be inside /tmp (for example /tmp/wasm-dd2/run-name)')
    return path


def open_files():
    opened = set()
    for process in Path('/proc').glob('[0-9]*'):
        try:
            descriptors = list((process / 'fd').iterdir())
        except OSError:
            continue
        for descriptor in descriptors:
            try:
                stat = descriptor.stat()
                opened.add((stat.st_dev, stat.st_ino))
            except OSError:
                pass
    return opened


def cleanup_logs(roots=None, age=3600, keep=()):
    """Remove only old regular logs; never follow symlinks or delete open files."""
    roots = roots if roots is not None else (WORK, ROOT / 're_out')
    keep = [Path(path).resolve() for path in keep]
    opened = open_files()
    removed = total = 0
    cutoff = time.time() - age
    for root in roots:
        if Path(root).is_symlink():
            continue
        for directory, _, files in os.walk(root, followlinks=False):
            parent = Path(directory)
            if any(parent == path or path in parent.parents for path in keep):
                continue
            for name in files:
                if not (name.endswith('.log') or re.fullmatch(r'.+\.log\.(?:\d+|gz|\d+\.gz)', name) or name.endswith('log.txt')):
                    continue
                path = parent / name
                try:
                    stat = path.lstat()
                    if (path.is_symlink() or not path.is_file() or stat.st_mtime >= cutoff or
                            (stat.st_dev, stat.st_ino) in opened):
                        continue
                    path.unlink()
                    removed += 1
                    total += stat.st_size
                except FileNotFoundError:
                    pass
    return {'removed_logs': removed, 'removed_bytes': total}


def prepare_output(path):
    output = temporary_output(path)
    cleanup_logs(keep=(output,))
    atexit.register(cleanup_logs, keep=(output,))
    return output


def check_space(directory):
    size = 0
    for parent, _, names in os.walk(directory, followlinks=False):
        for name in names:
            path = Path(parent) / name
            if not path.is_symlink():
                try:
                    size += path.stat().st_size
                except FileNotFoundError:
                    pass
    if size > MAX_BYTES:
        raise RuntimeError('Verification output exceeded 2 GiB; stop and use smaller captures')
    if shutil.disk_usage(directory).free < MIN_FREE:
        raise RuntimeError('/tmp has less than 1 GiB free; remove completed captures before continuing')


def run_bounded(command, *, directory, timeout, check=False, **kwargs):
    """Stop only this subprocess group if its capture exceeds the space budget."""
    temporary_output(directory)
    check_space(directory)
    started = time.monotonic()
    with subprocess.Popen(command, start_new_session=True, **kwargs) as process:
        try:
            while process.poll() is None:
                check_space(directory)
                if time.monotonic() - started >= timeout:
                    raise subprocess.TimeoutExpired(command, timeout)
                time.sleep(0.25)
            check_space(directory)
        except BaseException:
            try:
                os.killpg(process.pid, signal.SIGTERM)
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
            except ProcessLookupError:
                process.wait()
            raise
        result = subprocess.CompletedProcess(command, process.returncode)
        if check:
            result.check_returncode()
        return result


def discard_frames(directory):
    """After successful checks, retain reports/logs instead of raw binary dumps."""
    for path in Path(directory).iterdir():
        if path.is_file() and not path.is_symlink() and path.suffix in ('.bin', '.pal', '.image', '.cars', '.pcm'):
            path.unlink()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--age-seconds', type=float, default=3600)
    args = parser.parse_args()
    if args.age_seconds < 0:
        parser.error('age must be nonnegative')
    print(cleanup_logs(age=args.age_seconds))
