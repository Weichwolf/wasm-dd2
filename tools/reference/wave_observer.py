"""Forward Wine WinMM APIs and capture original compressed source unchanged.

Keep WinMM submission, Pulse client acceptance and physical playback separate.
"""
import hashlib
import json
from pathlib import Path
import struct
import subprocess

from artifacts import WORK
from reference.timer_observer import KERNEL, SYSTEM_WINMM, pe_exports

SOURCE = Path(__file__).with_name('winmm_wave_observer.c')
OBSERVED = {'waveOutOpen': 24, 'waveOutWrite': 12, 'waveOutReset': 4,
            'waveOutClose': 4, 'waveOutPrepareHeader': 12, 'waveOutUnprepareHeader': 12}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    with Path(path).open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def build_backend(output, library, observed, source, record_bytes):
    output = Path(output).resolve()
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    system = SYSTEM_WINMM.with_name(library + '.dll')
    original = system.read_bytes()
    exports, sections, pe = pe_exports(original)
    marker = b'Wine builtin DLL'
    at = original.find(marker)
    require(at == 64 and at + len(marker) < pe, 'Expected non-executable Wine builtin marker')
    backend = original[:at] + b' ' * len(marker) + original[at + len(marker):]
    require(pe_exports(backend)[:2] == (exports, sections), 'WinMM backend sections or exports changed')
    (output / ('_' + library + '_real.dll')).write_bytes(backend)
    definitions = ['LIBRARY ' + library + '.dll', 'EXPORTS']
    for entry in exports:
        name, ordinal = entry['name'], entry['ordinal']
        if name in observed:
            definitions.append(f'  {name}=_{name}@{observed[name]} @{ordinal}')
        elif name:
            definitions.append(f'  {name}={library}_real.{name} @{ordinal}')
        else:
            definitions.append(f'  ordinal_{ordinal}={library}_real.#{ordinal} @{ordinal} NONAME')
    (output / (library + '.def')).write_text('\n'.join(definitions) + '\n')
    imports = {**KERNEL, 'GetLastError': 0, 'SetLastError': 4, 'CloseHandle': 4,
               'HeapFree': 12, 'GetModuleFileNameA': 12, 'GetCurrentProcessId': 0}
    (output / 'kernel32.def').write_text('LIBRARY kernel32.dll\nEXPORTS\n' +
                                      ''.join(f'  {name}@{size}\n' for name, size in imports.items()))
    subprocess.run(['llvm-dlltool', '-m', 'i386', '-k', '-d', str(output / 'kernel32.def'),
                    '-l', str(output / 'kernel32.lib')], check=True)
    subprocess.run(['clang', '--target=i686-windows-gnu', '-ffreestanding', '-fno-builtin',
                    '-fno-stack-protector', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-c', str(source), '-o', str(output / (library + '.obj'))], check=True)
    subprocess.run(['lld-link', '/dll', '/nodefaultlib', '/timestamp:0', '/entry:DllMain@12',
                    '/machine:x86', '/def:' + str(output / (library + '.def')), '/out:' + str(output / (library + '.dll')),
                    str(output / (library + '.obj')), str(output / 'kernel32.lib')], check=True)
    actual = pe_exports((output / (library + '.dll')).read_bytes())[0]
    require([(e['ordinal'], e['name']) for e in actual] == [(e['ordinal'], e['name']) for e in exports],
            'WinMM observer public ABI differs')
    for entry in actual:
        target = None if entry['name'] in observed else '_' + library + '_real.' + (entry['name'] or '#' + str(entry['ordinal']))
        require(entry['forwarder'] == target, 'WinMM export forwards to wrong backend')
    report = dict(scope=__doc__, backend_path=str(system), backend_sha256=sha(system),
                  private_backend_sha256=sha(output / ('_' + library + '_real.dll')), unchanged_sections=sections,
                  modified_header_bytes=list(range(at, at + len(marker))), exports=exports, observer_exports=actual,
                  observer_sha256=sha(output / (library + '.dll')), record_bytes=record_bytes,
                  source_sha256={p.name: sha(p) for p in (source, source.with_name('winmm_timer_observer.h'))})
    (output / (library + '-build.json')).write_text(json.dumps(report, indent=2) + '\n')
    return report


def build(output):
    report = build_backend(output, 'winmm', OBSERVED, SOURCE, 160)
    report['acm'] = build_backend(output, 'msacm32', {'acmStreamOpen': 32, 'acmStreamConvert': 12,
                                                    'acmStreamClose': 8}, SOURCE.with_name('winmm_acm_observer.c'), 264)
    (Path(output) / 'build.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def summarize(directory, source_format):
    journals = sorted(directory.glob('winmm-*.bin'))
    require(len(journals) == 1 and set(directory.glob('winmm-*.adpcm')) == {journals[0].with_suffix('.adpcm')},
            'one original WinMM process journal and MS-ADPCM source required')
    raw = journals[0].read_bytes()
    require(raw and len(raw) % 160 == 0, 'incomplete WinMM observations')
    events, streams, total = [], {}, 0
    fields = ('magic', 'version', 'serial', 'kind', 'pid', 'thread', 'caller', 'handle', 'result', 'last_error',
              'argument_bytes', 'bytes', 'header', 'data', 'user', 'flags_before', 'flags_after', 'loops',
              'next', 'reserved', 'callback', 'instance', 'device', 'unused', 'offset', 'begin', 'end', 'frequency')
    pcm = journals[0].with_suffix('.adpcm').read_bytes()
    for at in range(0, len(raw), 160):
        row = dict(zip(fields, struct.unpack_from('<24I4Q', raw, at)))
        require(row['magic'] == 0x57443244 and row['version'] == 1 and row['serial'] == len(events) + 1 and
                row['kind'] in range(1, 7) and row['pid'] and row['thread'] and row['frequency'] and
                0 < row['begin'] <= row['end'] and row['offset'] == total and not row['unused'] and
                not any(raw[at + 146:at + 160]), 'invalid WinMM record extent/clock/header')
        if row['kind'] == 1:
            require(row['result'] == 0 and row['handle'] and row['handle'] not in streams,
                    'failed, query-only or reused WinMM open')
            format_ = struct.unpack_from('<HHIIHHH', raw, at + 128)
            require(raw[at + 128:at + 146] == source_format[:18] and
                    format_[:3] == (2, 2, 22050), 'original movie must open its actual AVI MS-ADPCM format')
            streams[row['handle']] = dict(handle=row['handle'], format=list(format_),
                                         open_serial=row['serial'], closed=False, writes=[], resets=[])
        else:
            require(not any(raw[at + 128:at + 146]) and row['handle'] in streams,
                    'unexpected WinMM format or unknown device')
            stream = streams[row['handle']]
            require(not stream['closed'] and row['result'] == 0, 'failed call or call after WinMM close')
            if row['kind'] in (2, 5, 6):
                require(row['argument_bytes'] == 32 and row['header'] and row['bytes'] and row['data'],
                        'unsupported original wave header')
            if row['kind'] == 2:
                require(row['bytes'] % format_[4] == 0 and row['flags_before'] & 2 and row['flags_after'] & 2 and
                        row['flags_after'] & 16 and not row['loops'], 'unsupported source write or queue state')
                stream['writes'].append(dict(serial=row['serial'], offset=total, bytes=row['bytes'],
                                             begin=row['begin'], end=row['end'], caller=row['caller']))
                total += row['bytes']
                require(total <= len(pcm), 'WinMM accepted source file truncated')
            elif row['kind'] == 3:
                stream['resets'].append(row['serial'])
            elif row['kind'] == 4:
                stream.update(closed=True, close_serial=row['serial'])
        events.append(row)
    require(total == len(pcm) and all(s['closed'] and s['writes'] for s in streams.values()),
            'incomplete WinMM source extent or natural close')
    for stream in streams.values():
        data = b''.join(pcm[w['offset']:w['offset'] + w['bytes']] for w in stream['writes'])
        stream.update(accepted_bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
    require(len(streams) == 1, 'one actual original movie source stream required')
    return dict(scope=__doc__, journal_sha256=sha(journals[0]), source_sha256=sha(journals[0].with_suffix('.adpcm')),
                source_bytes=total, streams=list(streams.values()), events=events), pcm


def summarize_acm(directory, source_format):
    journals = sorted(directory.glob('acm-*.bin'))
    require(len(journals) == 1 and set(directory.glob('acm-*.adpcm')) == {journals[0].with_suffix('.adpcm')} and
            set(directory.glob('acm-*.pcm')) == {journals[0].with_suffix('.pcm')}, 'one complete original ACM journal/input/output required')
    raw = journals[0].read_bytes()
    require(raw and len(raw) % 264 == 0, 'incomplete actual ACM observations')
    source, pcm = journals[0].with_suffix('.adpcm').read_bytes(), journals[0].with_suffix('.pcm').read_bytes()
    fields = ('magic', 'version', 'serial', 'kind', 'pid', 'thread', 'caller', 'handle', 'result', 'last_error',
              'flags', 'header', 'header_size', 'status_before', 'status_after', 'source_requested', 'source_used',
              'destination_requested', 'destination_used', 'source_pointer', 'destination_pointer',
              'source_format_bytes', 'destination_format_bytes', 'unused', 'source_offset', 'destination_offset',
              'begin', 'end', 'frequency')
    events, streams, source_total, destination_total = [], {}, 0, 0
    for at in range(0, len(raw), 264):
        row = dict(zip(fields, struct.unpack_from('<24I5Q', raw, at)))
        require(row['magic'] == 0x41443244 and row['version'] == 1 and row['serial'] == len(events) + 1 and
                row['kind'] in (1, 2, 3) and row['pid'] and row['thread'] and row['frequency'] and
                0 < row['begin'] <= row['end'] and row['source_offset'] == source_total and
                row['destination_offset'] == destination_total and not row['unused'] and row['result'] == 0,
                'invalid actual ACM record/error/clock/extent')
        if row['kind'] == 1:
            source_bytes, destination_bytes = row['source_format_bytes'], row['destination_format_bytes']
            require(source_bytes == len(source_format) and destination_bytes == 18 and
                    raw[at + 136:at + 136 + source_bytes] == source_format and
                    not any(raw[at + 136 + source_bytes:at + 200]) and
                    struct.unpack_from('<HHIIHHH', raw, at + 200) == (1, 2, 22050, 88200, 4, 16, 0) and
                    not any(raw[at + 218:at + 264]) and not row['flags'] & 2,
                    'actual ACM source/destination format or async mode differs')
            query = bool(row['flags'] & 1)
            row['query_only'] = query
            if not query:
                require(row['handle'] and row['handle'] not in streams, 'unknown/reused actual ACM lifetime')
                streams[row['handle']] = dict(handle=row['handle'], open_serial=row['serial'], closed=False, conversions=[])
        else:
            require(not any(raw[at + 136:at + 264]) and row['handle'] in streams,
                    'unexpected ACM format or unknown stream')
            stream = streams[row['handle']]
            require(not stream['closed'], 'actual ACM call after close')
            if row['kind'] == 2:
                require(row['header_size'] == 84 and row['header'] and
                        0 <= row['source_used'] <= row['source_requested'] and
                        0 <= row['destination_used'] <= row['destination_requested'] and row['destination_used'] % 4 == 0 and
                        row['status_before'] & 0x20000 and row['status_after'] & 0x10000,
                        'unsupported actual synchronous ACM conversion/header status')
                stream['conversions'].append(dict(serial=row['serial'], source_offset=source_total,
                                                  destination_offset=destination_total, source_bytes=row['source_used'],
                                                  pcm_bytes=row['destination_used'], begin=row['begin'], end=row['end']))
                source_total += row['source_used']; destination_total += row['destination_used']
            else:
                stream.update(closed=True, close_serial=row['serial'])
        events.append(row)
    require(source_total == len(source) and destination_total == len(pcm) and len(streams) == 1 and
            all(s['closed'] and s['conversions'] for s in streams.values()), 'incomplete actual ACM source/output lifetime')
    return dict(scope='Actual original WinMM ACM consumed compressed input and synchronous decoded PCM; no playback or port parity claim',
                journal_sha256=sha(journals[0]), compressed_sha256=sha(journals[0].with_suffix('.adpcm')),
                pcm_sha256=sha(journals[0].with_suffix('.pcm')), source_bytes=source_total, pcm_bytes=destination_total,
                streams=list(streams.values()), events=events), source, pcm
