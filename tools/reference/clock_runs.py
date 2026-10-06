"""Lossless runs of actual original GetTickCount DWORD returns.

DD2TKR1 stores little-endian (value, count) pairs. Decoding preserves every
logical call, including equal consecutive values and DWORD wrap. Storage
equivalence is separate from engine, audio/video and physical timing parity.
Production ports select this format with DD2_TICK_REPLAY_FORMAT=DD2TKR1.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct


MAGIC = b'DD2TKR1\0'
DWORD = struct.Struct('<I')
RUN = struct.Struct('<II')
MAX_CALLS = 0xffffffff


class Writer:
    def __init__(self, stream):
        self.stream = stream
        self.value = None
        self.count = self.calls = self.runs = 0
        self.logical_hash = hashlib.sha256()
        stream.write(MAGIC)

    def write(self, value):
        if not 0 <= value <= 0xffffffff or self.calls == MAX_CALLS:
            raise ValueError('Clock value/call count exceeds DWORD')
        raw = DWORD.pack(value)
        self.logical_hash.update(raw)
        self.calls += 1
        if self.value == value:
            self.count += 1
        else:
            self.flush()
            self.value, self.count = value, 1

    def flush(self):
        if self.count:
            self.stream.write(RUN.pack(self.value, self.count))
            self.runs += 1
            self.count = 0


def values(stream):
    if stream.read(len(MAGIC)) != MAGIC:
        raise ValueError('Clock run header differs')
    total = 0
    while True:
        raw = stream.read(RUN.size)
        if not raw:
            break
        if len(raw) != RUN.size:
            raise ValueError('Partial clock run')
        value, count = RUN.unpack(raw)
        if not count or count > MAX_CALLS - total:
            raise ValueError('Invalid clock run count')
        total += count
        for _ in range(count):
            yield value
    if not total:
        raise ValueError('Empty clock run input')


def pack(source, output):
    if output.exists() or source.resolve() == output.resolve():
        raise ValueError('Fresh clock run output required')
    original_hash = hashlib.sha256()
    try:
        with source.open('rb') as original, output.open('xb') as encoded:
            writer = Writer(encoded)
            while block := original.read(65536):
                if len(block) % DWORD.size:
                    raise ValueError('Partial original clock DWORD')
                original_hash.update(block)
                for (value,) in struct.iter_unpack('<I', block):
                    writer.write(value)
            writer.flush()
            if not writer.calls:
                raise ValueError('Empty original clock input')
        # Independently decode every value; never accept solely an encoder hash.
        decoded_hash = hashlib.sha256()
        decoded_calls = 0
        with output.open('rb') as encoded:
            for value in values(encoded):
                decoded_hash.update(DWORD.pack(value))
                decoded_calls += 1
        if decoded_calls != writer.calls or decoded_hash.digest() != original_hash.digest():
            raise ValueError('Decoded clock calls differ from actual source DWORDs')
        with output.open('rb') as encoded:
            encoded_hash = hashlib.file_digest(encoded, 'sha256').hexdigest()
        return dict(scope=__doc__.strip(), pass_=True, encoding='DD2TKR1',
                    calls=writer.calls, runs=writer.runs,
                    original_bytes=source.stat().st_size, encoded_bytes=output.stat().st_size,
                    logical_ticks_sha256=original_hash.hexdigest(), encoded_sha256=encoded_hash,
                    port_comparison='pending; exact storage equivalence only')
    except BaseException:
        output.unlink(missing_ok=True)
        raise


if __name__ == '__main__':
    import sys
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    from artifacts import WORK, check_space
    if any(WORK not in p.resolve().parents for p in (args.output, args.report)):
        parser.error('Clock inputs and reports must remain under /tmp/wasm-dd2/')
    if args.report.exists():
        parser.error('Fresh report required')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    proof = pack(args.source, args.output)
    args.report.write_text(json.dumps(proof, indent=2) + '\n')
    check_space(args.output.parent)
    print(json.dumps(proof, indent=2))
