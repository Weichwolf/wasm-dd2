#!/usr/bin/env python3
"""Losslessly pack closed racing images without modifying a running recorder.

The original indexed-zlib/palette files are decoded, packed in groups of at
most 64, and compared literally after decoding the new archive. Only then are
closed inputs removed. The immutable recorder still creates the same frames,
metadata and API streams. This is storage verification, not game/A/V parity.
"""
import argparse
import ctypes
import ctypes.util
import fcntl
from functools import lru_cache
import hashlib
import json
import math
from pathlib import Path
import re
import time
import zlib

from artifacts import WORK, check_space, open_files

FRAME = 307200
PALETTE = 1024
RECORD = FRAME+PALETTE
CHUNK = 64
MANIFEST = 'racing-archive.json'
FORMAT = 'indexed-palette-zstd-v1'


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


@lru_cache(maxsize=1)
def codec():
    lib = ctypes.CDLL(ctypes.util.find_library('zstd') or 'libzstd.so.1')
    signatures = {
        'ZSTD_compressBound': (ctypes.c_size_t, [ctypes.c_size_t]),
        'ZSTD_compress': (ctypes.c_size_t, [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int]),
        'ZSTD_decompress': (ctypes.c_size_t, [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t]),
        'ZSTD_isError': (ctypes.c_uint, [ctypes.c_size_t]),
        'ZSTD_getErrorName': (ctypes.c_char_p, [ctypes.c_size_t]),
        'ZSTD_getFrameContentSize': (ctypes.c_ulonglong, [ctypes.c_void_p, ctypes.c_size_t]),
        'ZSTD_findFrameCompressedSize': (ctypes.c_size_t, [ctypes.c_void_p, ctypes.c_size_t]),
        'ZSTD_versionString': (ctypes.c_char_p, []),
    }
    for name, (result, args) in signatures.items():
        function = getattr(lib, name); function.restype = result; function.argtypes = args
    return lib


def checked(value):
    lib = codec()
    require(not lib.ZSTD_isError(value), 'Zstandard: '+lib.ZSTD_getErrorName(value).decode())
    return value


def compress(data):
    lib = codec(); buffer = ctypes.create_string_buffer(lib.ZSTD_compressBound(len(data)))
    size = checked(lib.ZSTD_compress(buffer, len(buffer), data, len(data), 9))
    return buffer.raw[:size]


def decompress(packed, count):
    require(type(count) is int and 0 < count <= CHUNK, 'Invalid bounded archive extent')
    expected = count*RECORD
    require(0 < len(packed) <= expected+1024, 'Oversized or empty archive')
    lib = codec()
    require(lib.ZSTD_getFrameContentSize(packed, len(packed)) == expected,
            'Archive decoded extent differs')
    require(checked(lib.ZSTD_findFrameCompressedSize(packed, len(packed))) == len(packed),
            'Trailing archive bytes or extra compressed frame')
    buffer = ctypes.create_string_buffer(expected)
    require(checked(lib.ZSTD_decompress(buffer, expected, packed, len(packed))) == expected,
            'Incomplete decoded archive')
    return buffer.raw


def source_picture(path, size, compressed=False):
    raw = path.read_bytes()
    if compressed:
        require(len(raw) <= size+1024, 'Oversized source image')
        decoder = zlib.decompressobj()
        raw = decoder.decompress(raw, size+1)
        require(decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail,
                'Incomplete or trailing source image')
    require(len(raw) == size, 'Incomplete source image/palette')
    return raw


@lru_cache(maxsize=4)
def index(path, modified, size):
    document = json.loads(Path(path).read_text())
    require(document['format'] == FORMAT, 'Unsupported racing archive')
    mapping = {}
    for chunk in document['chunks']:
        require(re.fullmatch(r'pack[0-9]{5}\.zst', chunk['file']) and
                0 < len(chunk['frames']) <= CHUNK, 'Invalid racing archive chunk')
        for i, row in enumerate(chunk['frames']):
            require(re.fullmatch(r'race[0-9]{5}', row['prefix']) and row['prefix'] not in mapping,
                    'Invalid or repeated archived racing prefix')
            mapping[row['prefix']] = (chunk, i)
    return mapping


@lru_cache(maxsize=2)
def decoded_chunk(path, modified, size, sha256, raw_sha256, count):
    packed = Path(path).read_bytes()
    require(digest(packed) == sha256, 'Archived compressed hash differs')
    raw = decompress(packed, count)
    require(digest(raw) == raw_sha256, 'Archived literal hash differs')
    return raw


def archived_picture(root, prefix, suffix, size):
    require((suffix, size) in (('.bin', FRAME), ('.pal', PALETTE)), 'Unsupported archived picture')
    manifest = root/MANIFEST; stat = manifest.stat()
    entry = index(str(manifest), stat.st_mtime_ns, stat.st_size).get(prefix)
    require(entry is not None, 'Missing archived racing prefix: '+prefix)
    chunk, i = entry
    path = root/'racing-archive'/chunk['file']; stat = path.stat()
    raw = decoded_chunk(str(path), stat.st_mtime_ns, stat.st_size, chunk['sha256'],
                        chunk['literal_sha256'], len(chunk['frames']))
    start = i*RECORD+(FRAME if suffix == '.pal' else 0)
    data = raw[start:start+size]
    key = 'framebuffer_sha256' if suffix == '.bin' else 'palette_sha256'
    require(digest(data) == chunk['frames'][i][key], 'Archived picture/palette hash differs')
    return data


def clear_cache():
    index.cache_clear(); decoded_chunk.cache_clear()


def atomic_json(path, document):
    temp = path.with_suffix('.json.tmp')
    temp.write_text(json.dumps(document, indent=2)+'\n')
    temp.replace(path)


def compact(root, max_frames=None, minimum_age=2, complete_chunks_only=False):
    """May run beside the known recorder: its write-once prefixes are immutable.

    Neither the recorder nor port replay reads back these racing files. Keep
    recent pairs and all open files, verify each pack before committing its
    manifest, then recheck open descriptors immediately before deleting.
    """
    root = root.resolve()
    require(WORK in root.parents, 'Use /tmp/wasm-dd2/')
    storage = root/'racing-archive'; storage.mkdir(exist_ok=True)
    added = 0
    with (storage/'lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        path = root/MANIFEST
        document = json.loads(path.read_text()) if path.exists() else dict(
            format=FORMAT, scope=__doc__, original_port_parity='unproven', chunks=[],
            zstandard_version=codec().ZSTD_versionString().decode(), level=9,
            source_image_format='indexed-zlib', recorder_modified=False)
        require(document['format'] == FORMAT, 'Unsupported existing archive')
        archived = {row['prefix'] for chunk in document['chunks'] for row in chunk['frames']}
        opened = open_files(); cutoff = time.time()-minimum_age
        candidates = []
        prefixes = {p.name[:-6] for p in root.glob('race*.bin.z')} | {p.stem for p in root.glob('race*.pal')}
        for prefix in sorted(prefixes):
            frame = root/(prefix+'.bin.z'); palette = root/(prefix+'.pal')
            if not re.fullmatch(r'race[0-9]{5}', prefix): continue
            if prefix in archived:
                # Recover an interrupted cleanup after the manifest was
                # committed. Existing source pairs must still match literally;
                # retain the whole pair if either existing file is open.
                present = [p for p in (frame, palette) if p.exists()]
                stats = [p.stat() for p in present]
                if any((s.st_dev,s.st_ino) in opened or s.st_mtime > cutoff for s in stats): continue
                for source in present:
                    suffix = '.bin' if source == frame else '.pal'
                    size = FRAME if source == frame else PALETTE
                    require(source_picture(source,size,source == frame) == archived_picture(root,prefix,suffix,size),
                            'Remaining source differs from committed archive')
                refreshed = open_files()
                for source, expected in zip(present, stats):
                    current = source.stat()
                    require((current.st_dev,current.st_ino,current.st_size,current.st_mtime_ns) ==
                            (expected.st_dev,expected.st_ino,expected.st_size,expected.st_mtime_ns),
                            'Remaining source changed while checking archive')
                    if (current.st_dev,current.st_ino) not in refreshed: source.unlink()
                continue
            if not frame.is_file() or not palette.is_file(): continue
            stats = [frame.stat(), palette.stat()]
            if any((s.st_dev, s.st_ino) in opened or s.st_mtime > cutoff for s in stats): continue
            candidates.append((frame, palette, prefix, stats))
        if max_frames is not None: candidates = candidates[:max_frames]
        if complete_chunks_only: candidates = candidates[:len(candidates)//CHUNK*CHUNK]
        for begin in range(0, len(candidates), CHUNK):
            group = candidates[begin:begin+CHUNK]
            raw_parts = []; rows = []
            for frame, palette, prefix, stats in group:
                image = source_picture(frame, FRAME, True); colors = source_picture(palette, PALETTE)
                raw_parts.append(image+colors)
                rows.append(dict(prefix=prefix, framebuffer_sha256=digest(image), palette_sha256=digest(colors)))
            raw = b''.join(raw_parts); packed = compress(raw)
            require(decompress(packed, len(rows)) == raw, 'Pack literal roundtrip failed')
            filename = f'pack{len(document["chunks"]):05d}.zst'
            destination = storage/filename
            require(not destination.exists(), 'Uncommitted archive already exists: '+str(destination))
            temp = destination.with_suffix('.partial'); temp.write_bytes(packed)
            require(decompress(temp.read_bytes(), len(rows)) == raw, 'Written archive differs')
            temp.replace(destination)
            chunk = dict(file=filename, frames=rows, sha256=digest(packed), literal_sha256=digest(raw),
                         source_bytes=sum(s.st_size for _, _, _, stats in group for s in stats),
                         archived_bytes=len(packed), literal_roundtrip=True)
            document['chunks'].append(chunk)
            atomic_json(path, document)
            clear_cache()
            for i, (_, _, prefix, _) in enumerate(group):
                require(archived_picture(root, prefix, '.bin', FRAME) == raw_parts[i][:FRAME] and
                        archived_picture(root, prefix, '.pal', PALETTE) == raw_parts[i][FRAME:],
                        'Public archive reader differs')
            opened = open_files()
            for frame, palette, prefix, stats in group:
                for source, expected in zip((frame, palette), stats):
                    current = source.stat()
                    require((current.st_dev, current.st_ino, current.st_size, current.st_mtime_ns) ==
                            (expected.st_dev, expected.st_ino, expected.st_size, expected.st_mtime_ns),
                            'Source changed while packing: '+str(source))
                    if (current.st_dev, current.st_ino) not in opened: source.unlink()
            added += len(group)
            check_space(root.parent)
    return added


def identity(pid):
    try:
        fields = Path(f'/proc/{pid}/stat').read_text().rsplit(')', 1)[1].split()
        return fields[19] if fields[0] != 'Z' else None
    except FileNotFoundError:
        return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True, help='history directory containing race*.bin.z and race*.pal')
    parser.add_argument('--follow-pid', type=int, help='keep packing until this specific recorder process exits')
    parser.add_argument('--follow-timeout-seconds', type=float, default=7200,
                        help='positive finite follower lifetime; default 7200 seconds')
    args = parser.parse_args()
    if not math.isfinite(args.follow_timeout_seconds) or args.follow_timeout_seconds <= 0:
        parser.error('--follow-timeout-seconds must be positive and finite')
    token = identity(args.follow_pid) if args.follow_pid else None
    if args.follow_pid: require(token is not None, 'Recorder is not live')
    start = time.monotonic()
    while True:
        count = compact(args.capture, complete_chunks_only=bool(token))
        if count: print('Archived', count, 'closed racing frames; literal bytes retained', flush=True)
        if not token or identity(args.follow_pid) != token:
            compact(args.capture, minimum_age=0)
            break
        require(time.monotonic()-start < args.follow_timeout_seconds,
                'Archive follower exceeded its declared lifetime of '+str(args.follow_timeout_seconds)+' seconds')
        time.sleep(2)


if __name__ == '__main__':
    main()
