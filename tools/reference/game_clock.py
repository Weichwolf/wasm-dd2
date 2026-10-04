"""Export original engine clock returns from Wine relay, without debugger stops.

The five call sites are verified against the unchanged executable. Every return
must have its matching entry on the same thread. DLL-internal calls are excluded
by caller address, not by their values. Timestamp precision is that of Wine's
trace; these records do not prove physical timing or port audio/video parity.
"""
from collections import Counter
from decimal import Decimal
import hashlib
import json
from pathlib import Path
import re
import struct

SITES = (0x423c2d, 0x423ecd, 0x424007, 0x424024, 0x424040)
EXE = '0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'
CALL = re.compile(r'([^:]+):([0-9a-f]+):Call (KERNEL32|kernelbase)\.GetTickCount\(\) ret=([0-9a-f]+)$', re.I)
RETURN = re.compile(r'([^:]+):([0-9a-f]+):Ret  (KERNEL32|kernelbase)\.GetTickCount\(\) retval=([0-9a-f]+) ret=([0-9a-f]+)$', re.I)


def verified_sites(executable):
    raw = executable.read_bytes()
    if hashlib.sha256(raw).hexdigest() != EXE:
        raise ValueError('Unmodified supported original executable required')
    pe = struct.unpack_from('<I', raw, 0x3c)[0]
    sections, optional = struct.unpack_from('<H', raw, pe+6)[0], struct.unpack_from('<H', raw, pe+20)[0]
    base = struct.unpack_from('<I', raw, pe+24+28)[0]
    sites = set()
    for i in range(sections):
        offset = pe+24+optional+i*40
        va, size, file_offset, flags = (struct.unpack_from('<I', raw, offset+n)[0] for n in (12,16,20,36))
        if not flags & 0x20000000:
            continue
        code = raw[file_offset:file_offset+size]
        instruction = b'\x2e\xff\x15\xc0\x00\x95\x00'
        at = 0
        while (at := code.find(instruction, at)) >= 0:
            sites.add(base+va+at+len(instruction));at += len(instruction)
    if sites != set(SITES):
        raise ValueError('Original GetTickCount import call sites differ')
    return sites


def export_clock(trace, executable, output, *, allow_terminal_entry=False):
    sites = verified_sites(executable)
    output.mkdir(parents=True, exist_ok=False)
    pending = {}; counts = Counter(); threads = set()
    total = flips = ignored = 0
    first = last = previous = None
    sha = hashlib.sha256()
    try:
        with trace.open('rb') as lines, (output/'ticks.bin').open('wb') as ticks, (output/'observations.bin').open('wb') as observations:
            # observations: eight-byte magic, u32 version and record width;
            # each record has entry/return trace timestamps, entry/return line,
            # completed original Flip count and original return address.
            observations.write(struct.pack('<8sII', b'DD2GC01\0', 1, 32))
            for number, raw_line in enumerate(lines, 1):
                sha.update(raw_line)
                line = raw_line.decode('utf-8', errors='replace').rstrip('\r\n')
                if 'ddraw_surface1_Flip iface' in line:
                    flips += 1
                if '.GetTickCount()' not in line:
                    continue
                entry, returned = CALL.fullmatch(line), RETURN.fullmatch(line)
                match = entry or returned
                if not match:
                    raise ValueError('Malformed GetTickCount relay record')
                time_ns = int(Decimal(match[1])*1000000000)
                thread = match[2].lower(); module = match[3].lower()
                caller = int(match[4 if entry else 5], 16)
                if entry:
                    if thread in pending:
                        raise ValueError('Nested/unreturned clock call on the same thread')
                    pending[thread] = (caller, module, time_ns, number, flips)
                    continue
                called = pending.pop(thread, None)
                if called is None or called[:2] != (caller, module) or time_ns < called[2]:
                    raise ValueError('Clock return has no matching ordered entry')
                if caller not in sites:
                    if 0x400000 <= caller < 0x458000:
                        raise ValueError('Unknown original engine clock call site')
                    ignored += 1
                    continue
                value = int(match[4], 16)
                if not 0 <= value <= 0xffffffff:
                    raise ValueError('Clock return is not a DWORD')
                if previous is not None and ((value-previous)&0xffffffff) > 60000:
                    raise ValueError('Original engine clock moved backwards or exceeded the capture bound')
                if threads and thread not in threads:
                    raise ValueError('Engine clock calls moved to another thread')
                ticks.write(struct.pack('<I', value))
                observations.write(struct.pack('<QQIIII', called[2], time_ns, called[3], number, called[4], caller))
                total += 1;counts[hex(caller)] += 1;threads.add(thread)
                if first is None:first = dict(value=value, line=number, completed_flips=called[4])
                last = dict(value=value, line=number, completed_flips=called[4]);previous = value
                if total > 10000000:
                    raise ValueError('Clock trace exceeds ten million bounded calls')
            terminal_entry = None
            if pending and allow_terminal_entry and len(pending) == 1:
                thread, call = next(iter(pending.items()))
                # A normally terminated capture can stop between TRACE entry
                # and return. Retain only completed calls and expose the tail.
                # Interior gaps, other callers/threads and nonterminal entries
                # still fail; no return value is manufactured.
                if thread in threads and call[0] in sites and call[3] == number:
                    terminal_entry = dict(thread=thread, caller=hex(call[0]), entry_time_ns=call[2],
                                          entry_line=call[3], completed_flips=call[4], return_observed=False)
                    pending.clear()
            if pending or not total or not flips or counts[hex(SITES[0])] < 1 or counts[hex(SITES[3])] < 1:
                raise ValueError('Incomplete original engine clock/presentation trace')
        def digest(path):
            with path.open('rb') as file:return hashlib.file_digest(file, 'sha256').hexdigest()
        result = dict(scope=__doc__.strip(), pass_=True, original_exe_sha256=EXE,
                      trace_sha256=sha.hexdigest(), calls=total, call_sites=dict(counts),
                      engine_thread=next(iter(threads)), ignored_dll_calls=ignored,
                      completed_flips=flips, first=first, last=last,
                      ticks_sha256=digest(output/'ticks.bin'), observations_sha256=digest(output/'observations.bin'),
                      observation_record_bytes=32, debugger=False,
                      terminal_unreturned_entry=terminal_entry,
                      port_comparison='pending; recorded clock inputs and presentation observations only')
        (output/'report.json').write_text(json.dumps(result, indent=2)+'\n')
        return result
    except BaseException:
        # Partial clock data must never look like reusable replay input.
        for name in ('ticks.bin', 'observations.bin'):
            (output/name).unlink(missing_ok=True)
        raise


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--trace', type=Path, required=True)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if Path('/tmp/wasm-dd2') not in args.output.resolve().parents:
        parser.error('Clock diagnostics must be under /tmp/wasm-dd2/')
    print(json.dumps(export_clock(args.trace, args.executable, args.output), indent=2))
