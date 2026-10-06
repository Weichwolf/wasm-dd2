"""Stream raw or lossless Zstandard Wine traces without loading whole logs."""
from contextlib import contextmanager
import hashlib
from pathlib import Path
import subprocess
import tempfile


@contextmanager
def read_trace(path):
    path = Path(path)
    compressed = path if path.suffix == '.zst' else Path(str(path) + '.zst')
    if path.suffix != '.zst' and path.exists():
        if compressed.exists():
            raise ValueError('Ambiguous raw and compressed trace: ' + str(path))
        with path.open('rb') as stream:
            yield stream
        return
    if not compressed.is_file():
        raise FileNotFoundError(compressed)
    with tempfile.TemporaryFile(dir=compressed.parent) as errors:
        process = subprocess.Popen(['zstd', '-q', '-d', '-c', str(compressed)],
                                   stdout=subprocess.PIPE, stderr=errors)
        try:
            yield process.stdout
        finally:
            # Validate the entire compressed stream even when a caller stops
            # after a header. Draining also avoids a blocked decoder at close.
            while process.stdout.read(65536):
                pass
            process.stdout.close()
            status = process.wait()
            if status:
                errors.seek(0)
                raise ValueError('Invalid compressed trace: ' +
                                 errors.read(2048).decode(errors='replace'))


def text_lines(path):
    with read_trace(path) as stream:
        for raw in stream:
            yield raw.decode(errors='replace').rstrip('\r\n')


def trace_digest(path):
    digest = hashlib.sha256()
    with read_trace(path) as stream:
        for chunk in iter(lambda: stream.read(65536), b''):
            digest.update(chunk)
    return digest.hexdigest()


@contextmanager
def write_trace(path, *, compressed=False):
    path = Path(path)
    if not compressed:
        with path.open('wb') as stream:
            yield stream
        return
    with Path(str(path) + '.zst').open('xb') as output, tempfile.TemporaryFile(dir=path.parent) as errors:
        process = subprocess.Popen(['zstd', '-q', '-1', '-c'], stdin=subprocess.PIPE,
                                   stdout=output, stderr=errors, start_new_session=True)
        try:
            yield process.stdin
        finally:
            process.stdin.close()
            try:
                status = process.wait(timeout=30)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
                raise RuntimeError('Trace compressor did not finish')
            if status:
                errors.seek(0)
                raise RuntimeError('Trace compression failed: ' +
                                   errors.read(2048).decode(errors='replace'))
