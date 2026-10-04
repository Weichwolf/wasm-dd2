"""Lossless, bounded storage of closed DirectDraw observation blocks.

This preserves every record byte. Storage verification establishes neither
original game behavior nor original/port parity. Active files are never removed.
"""
import bisect
import hashlib
import json
from pathlib import Path
import threading
import zlib

from artifacts import WORK, check_space, open_files

RECORD_BYTES = 128 + 307200 + 2048
MAX_FRAMES = 60000
MAX_CHUNK_FRAMES = 128


def digest(data):
    return hashlib.sha256(data).hexdigest()


def closed_bytes(path):
    before = path.stat()
    if (before.st_dev, before.st_ino) in open_files():
        raise ValueError('Video block is still open: ' + str(path))
    raw = path.read_bytes()
    after = path.stat()
    if (before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns) != (
            after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns):
        raise ValueError('Video block changed while reading')
    if not raw or len(raw) % RECORD_BYTES or len(raw) > RECORD_BYTES * MAX_CHUNK_FRAMES:
        raise ValueError('Incomplete or oversized video block')
    return raw, after


class Collector:
    """Consume only renamed closed blocks; finish after all writers have exited."""
    def __init__(self, directory, chunk_frames=MAX_CHUNK_FRAMES):
        self.directory = Path(directory).resolve()
        if WORK not in self.directory.parents or not 1 <= chunk_frames <= MAX_CHUNK_FRAMES:
            raise ValueError('Bounded /tmp/wasm-dd2/ video archive required')
        self.chunk_frames = chunk_frames
        self.archive = self.directory / 'video-archive'
        self.archive.mkdir()
        self.chunks = []
        self.frames = 0
        self.sha = hashlib.sha256()
        self.error = None
        self.stop = threading.Event()
        self.thread = threading.Thread(target=self.run, name='closed-video-archive', daemon=True)

    def start(self):
        self.thread.start()
        return self

    def check(self):
        if self.error is not None:
            raise RuntimeError('Video archival failed') from self.error

    def consume(self, path, final=False):
        number = len(self.chunks)
        expected = self.directory / f'video.bin.{number:06d}.{"part" if final else "raw"}'
        if path != expected:
            raise ValueError('Missing or reordered video block')
        raw, stat = closed_bytes(path)
        frames = len(raw) // RECORD_BYTES
        if frames > self.chunk_frames or (not final and frames != self.chunk_frames):
            raise ValueError('Unexpected closed video block extent')
        if self.frames + frames > MAX_FRAMES:
            raise ValueError('Video frame budget exceeded')
        packed = zlib.compress(raw, 6)
        if zlib.decompress(packed) != raw:
            raise ValueError('Lossless video roundtrip failed')
        name = f'{number:06d}.zlib'
        target = self.archive / name
        with target.open('xb') as file:
            file.write(packed)
        # Validate the written artifact literally before discarding source bytes.
        if zlib.decompress(target.read_bytes()) != raw:
            raise ValueError('Written video block differs')
        check_space(self.directory)
        current = path.stat()
        if current != stat or (stat.st_dev, stat.st_ino) in open_files():
            raise ValueError('Video block reopened or changed before removal')
        self.chunks.append(dict(file=name, first=self.frames, frames=frames,
            raw_bytes=len(raw), compressed_bytes=len(packed), raw_sha256=digest(raw),
            compressed_sha256=digest(packed)))
        self.sha.update(raw)
        self.frames += frames
        path.unlink()

    def drain(self):
        while (path := self.directory / f'video.bin.{len(self.chunks):06d}.raw').exists():
            self.consume(path)

    def run(self):
        try:
            while not self.stop.wait(.05):
                self.drain()
        except BaseException as error:
            self.error = error

    def finish(self):
        self.stop.set()
        if self.thread.ident is not None:
            self.thread.join(timeout=60)
        if self.thread.is_alive():
            raise RuntimeError('Video collector did not stop')
        self.check()
        self.drain()
        pending = sorted(self.directory.glob('video.bin.*.part'))
        if len(pending) > 1:
            raise ValueError('Multiple unfinished video blocks')
        if pending:
            self.consume(pending[0], final=True)
        if list(self.directory.glob('video.bin.*.raw')) or not self.frames:
            raise ValueError('Missing, reordered or empty video archive')
        report = dict(scope=__doc__.strip(), pass_=True, version=1, codec='zlib',
            record_bytes=RECORD_BYTES, frame_count=self.frames, chunk_frames=self.chunk_frames,
            raw_bytes=self.frames * RECORD_BYTES, raw_sha256=self.sha.hexdigest(),
            compressed_bytes=sum(c['compressed_bytes'] for c in self.chunks), chunks=self.chunks)
        (self.archive / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
        check_space(self.directory)
        return report


class Records:
    """Read literal records from legacy raw video or a sealed lossless archive."""
    def __init__(self, directory):
        self.directory = Path(directory)
        self.raw = None
        self.cached = -1
        self.data = None
        manifest = self.directory / 'video-archive/manifest.json'
        if manifest.exists():
            if (self.directory / 'video.bin').exists():
                raise ValueError('Ambiguous raw and archived video')
            self.manifest = json.loads(manifest.read_text())
            m = self.manifest
            if (m.get('pass_') is not True or m.get('version') != 1 or m.get('codec') != 'zlib' or
                    m.get('record_bytes') != RECORD_BYTES or not 0 < m['frame_count'] <= MAX_FRAMES or
                    not 1 <= m['chunk_frames'] <= MAX_CHUNK_FRAMES):
                raise ValueError('Unsupported video archive')
            total = 0
            for i, c in enumerate(m['chunks']):
                if (c['file'] != f'{i:06d}.zlib' or c['first'] != total or
                        not 0 < c['frames'] <= m['chunk_frames'] or
                        (i < len(m['chunks'])-1 and c['frames'] != m['chunk_frames']) or
                        c['raw_bytes'] != c['frames'] * RECORD_BYTES):
                    raise ValueError('Malformed video chunk index')
                total += c['frames']
            if total != m['frame_count'] or m['raw_bytes'] != total * RECORD_BYTES:
                raise ValueError('Incomplete video archive')
            self.count = total
            self.starts = [c['first'] for c in m['chunks']]
        else:
            self.manifest = None
            path = self.directory / 'video.bin'
            size = path.stat().st_size
            if not size or size % RECORD_BYTES or size > 4096 * RECORD_BYTES:
                raise ValueError('Incomplete or unbounded legacy video')
            self.count = size // RECORD_BYTES
            self.raw = path.open('rb')

    def __len__(self):
        return self.count

    def __getitem__(self, index):
        if not isinstance(index, int) or not 0 <= index < self.count:
            raise IndexError(index)
        if self.raw:
            self.raw.seek(index * RECORD_BYTES)
            result = self.raw.read(RECORD_BYTES)
        else:
            number = bisect.bisect_right(self.starts, index)-1
            c = self.manifest['chunks'][number]
            if self.cached != number:
                packed = (self.directory / 'video-archive' / c['file']).read_bytes()
                if len(packed) != c['compressed_bytes'] or digest(packed) != c['compressed_sha256']:
                    raise ValueError('Compressed video block differs')
                inflater = zlib.decompressobj()
                raw = inflater.decompress(packed, c['raw_bytes']+1)
                if (not inflater.eof or inflater.unused_data or inflater.unconsumed_tail or
                        len(raw) != c['raw_bytes'] or digest(raw) != c['raw_sha256']):
                    raise ValueError('Decoded video block differs')
                self.cached, self.data = number, raw
            offset = (index-c['first']) * RECORD_BYTES
            result = self.data[offset:offset+RECORD_BYTES]
        if len(result) != RECORD_BYTES:
            raise ValueError('Truncated video record')
        return result

    def __iter__(self):
        for index in range(self.count):
            yield self[index]

    def __enter__(self):
        return self

    def __exit__(self, *args):
        if self.raw:
            self.raw.close()
