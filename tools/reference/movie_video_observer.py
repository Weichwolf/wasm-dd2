"""Forward Wine's movie APIs and archive actual closed MCI window readbacks.

This observes dd2h.exe's StretchDIBits source and resulting 640x480 window RGB.
All backend sections and public exports are preserved. It does not establish
movie clocks, physical display output or chronological original/port parity.
"""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import threading
import zlib

from artifacts import WORK, check_space, open_files
from reference.timer_observer import KERNEL, pe_exports

SYSTEM = Path('/usr/lib/i386-linux-gnu/wine/i386-windows/msvfw32.dll')
RECORD_BYTES = 128 + 32768 + 320*192*3 + 640*480*4


def digest(data):
    return hashlib.sha256(data).hexdigest()


def build_backend(output, system, library, gdi=False):
    output = Path(output).resolve()
    if WORK not in output.parents:
        raise ValueError('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    original = system.read_bytes()
    exports, sections, pe = pe_exports(original)
    marker = b'Wine builtin DLL'
    at = original.find(marker)
    if at != 64 or at+len(marker) >= pe:
        raise ValueError('Expected non-executable Wine builtin marker')
    backend = original[:at] + b' '*len(marker) + original[at+len(marker):]
    if pe_exports(backend)[:2] != (exports, sections):
        raise ValueError('Movie backend sections or exports changed')
    (output/f'_{library}_real.dll').write_bytes(backend)
    observed = {'StretchDIBits': 52} if gdi else {'ICDecompress': None}
    definitions = [f'LIBRARY {library}.dll', 'EXPORTS']
    for e in exports:
        name, ordinal = e['name'], e['ordinal']
        if name in observed:
            symbol = f'_{name}@{observed[name]}' if observed[name] else name
            definitions.append(f'  {name}={symbol} @{ordinal}')
        elif name:
            definitions.append(f'  {name}={library}_real.{name} @{ordinal}')
        else:
            definitions.append(f'  ordinal_{ordinal}={library}_real.#{ordinal} @{ordinal} NONAME')
    additional=[]
    if not gdi:
        ordinal=max(e['ordinal'] for e in exports)+1
        definitions.append(f'  DD2MoviePacketCopy @{ordinal}')
        additional=[dict(name='DD2MoviePacketCopy',ordinal=ordinal,forwarder=None)]
    (output/f'{library}-exports.def').write_text('\n'.join(definitions)+'\n')
    imports = {'kernel32': {**KERNEL, 'CloseHandle': 4, 'MoveFileA': 8, 'GetModuleHandleA': 4},
               'gdi32': {'CreateCompatibleDC': 4, 'CreateDIBSection': 24,
                         'SelectObject': 8, 'BitBlt': 36, 'GdiFlush': 0},
               'user32': {'WindowFromDC': 4, 'GetClientRect': 8}}
    libraries = []
    for import_name, names in imports.items():
        definition = output/f'{import_name}.def'
        definition.write_text(f'LIBRARY {import_name}.dll\nEXPORTS\n' +
                              ''.join(f'  {name}@{size}\n' for name, size in names.items()))
        lib = output/f'{import_name}.lib'
        subprocess.run(['llvm-dlltool', '-m', 'i386', '-k', '-d', str(definition), '-l', str(lib)], check=True)
        libraries.append(str(lib))
    source = Path(__file__).with_suffix('.c')
    defines=['-DDD2_MOVIE_GDI_OBSERVER'] if gdi else []
    subprocess.run(['clang', '--target=i686-windows-gnu', '-ffreestanding', '-fno-builtin',
                    '-fno-stack-protector', '-O2', '-Wall', '-Wextra', '-Werror',
                    *defines,
                    '-c', str(source), '-o', str(output/'observer.obj')], check=True)
    subprocess.run(['lld-link', '/dll', '/nodefaultlib', '/timestamp:0', '/entry:DllMain@12',
                    '/machine:x86', '/def:'+str(output/f'{library}-exports.def'),
                    '/out:'+str(output/f'{library}.dll'), str(output/'observer.obj'), *libraries], check=True)
    actual = pe_exports((output/f'{library}.dll').read_bytes())[0]
    if [(e['name'], e['ordinal']) for e in actual] != [(e['name'], e['ordinal']) for e in exports+additional]:
        raise ValueError('Movie observer public ABI differs')
    for e in actual:
        expected = None if e['name'] in observed or e['name']=='DD2MoviePacketCopy' else f'_{library}_real.'+(e['name'] or '#'+str(e['ordinal']))
        if e['forwarder'] != expected:
            raise ValueError('Movie observer forwards to the wrong backend')
    report = dict(scope=__doc__.strip(), backend_path=str(system), backend_sha256=digest(original),
                  private_backend_sha256=digest(backend), unchanged_sections=sections,
                  modified_header_bytes=list(range(at, at+len(marker))), exports=exports,
                  observer_exports=actual, additional_observer_exports=additional,
                  observer_sha256=digest((output/f'{library}.dll').read_bytes()),
                  source_sha256={p.name: digest(p.read_bytes()) for p in
                                 (source, source.with_name('winmm_timer_observer.h'))},
                  record_bytes=RECORD_BYTES, maximum_frames=60000, original_window_address=0x46047c)
    (output/f'{library}-build.json').write_text(json.dumps(report, indent=2)+'\n')
    return report


def build(output, system=SYSTEM):
    report=build_backend(output,system,'msvfw32')
    report['gdi']=build_backend(output,SYSTEM.with_name('gdi32.dll'),'gdi32',gdi=True)
    (Path(output)/'build.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


def parse(raw, serial):
    if len(raw) != RECORD_BYTES:
        raise ValueError('Incomplete original movie readback')
    h = struct.unpack_from('<32I', raw)
    if (h[0] != 0x4d324444 or h[1] not in (1, 2) or h[2] != serial or not h[3] or not 0 < h[4] <= 32768 or
            h[9:14] != (320, 192, 32, 640, 480) or h[14] or h[15] != 192 or
            not h[16] or not h[18] or any(h[20:] if h[1] == 1 else h[30:])):
        raise ValueError('Unsupported original movie record')
    clock = {}
    if h[1] == 2:
        begin, end, readback, frequency = struct.unpack_from('<4Q', raw, 80)
        if not frequency or not 0 < begin <= end <= readback:
            raise ValueError('Reversed or missing original movie QPC observations')
        for ticks, qpc in ((h[28], begin), (h[29], end)):
            offset = (ticks-qpc*1000//frequency+0x80000000) % 0x100000000-0x80000000
            if abs(offset) > 2000:
                raise ValueError('Original movie QPC/tick domains differ')
        clock = dict(paint_begin_qpc=begin, paint_end_qpc=end,
                     readback_end_qpc=readback, frequency=frequency,
                     ticks_before=h[28], ticks_after=h[29])
    if any(raw[128+h[4]:128+32768]):
        raise ValueError('Nonzero compressed-packet padding')
    rect = list(struct.unpack_from('<4i', raw, 20))
    if rect not in ([0, 0, 320, 192], [0, 48, 640, 384]):
        raise ValueError('Unsupported actual movie destination')
    at = 128+32768
    return dict(serial=serial, record_version=h[1], clock=clock,
                decode_serial=h[3], packet=raw[128:128+h[4]],
                rectangle=rect, thread=h[16], observed_ms=h[17],
                private_mci_window_draws=h[19],
                source_rgb=raw[at:at+320*192*3], window_argb=raw[at+320*192*3:])


class Collector:
    """Archive only atomically renamed, closed, literal movie observations."""
    def __init__(self, directory):
        self.directory = Path(directory).resolve()
        if WORK not in self.directory.parents:
            raise ValueError('Use /tmp/wasm-dd2/')
        self.archive = self.directory/'movie-video-archive'
        self.archive.mkdir()
        self.records = []
        self.error = None
        self.stop = threading.Event()
        self.thread = threading.Thread(target=self.run, name='closed-movie-readbacks', daemon=True)

    def start(self):
        self.thread.start()
        return self

    def check(self):
        if self.error is not None:
            raise RuntimeError('Movie readback archival failed') from self.error

    def drain(self, final=False):
        while True:
            paths=[]
            for index in range(len(self.records),len(self.records)+8):
                path=self.directory/f'movie-video.bin.{index:06d}.raw'
                if not path.exists():break
                paths.append(path)
            if not paths or len(paths)<8 and not final:return
            # Scan descriptors around a bounded batch of eight CLOSED files.
            # Per-frame /proc scans cannot keep up with the real 25-fps writer.
            opened=open_files();pending=[]
            identity=lambda s:(s.st_dev,s.st_ino,s.st_size,s.st_mtime_ns)
            for index,path in enumerate(paths,len(self.records)):
                before=path.stat()
                if (before.st_dev,before.st_ino) in opened:
                    raise ValueError('Movie readback is still open')
                raw=path.read_bytes();row=parse(raw,index)
                if identity(path.stat())!=identity(before):raise ValueError('Movie readback changed')
                packed=zlib.compress(raw,1);target=self.archive/f'{index:06d}.zlib'
                with target.open('xb') as file:file.write(packed)
                if zlib.decompress(target.read_bytes())!=raw:raise ValueError('Lossless movie readback archive differs')
                entry=dict(serial=row['serial'],file=target.name,raw_sha256=digest(raw),
                    compressed_sha256=digest(packed),compressed_bytes=len(packed),
                    decode_serial=row['decode_serial'],rectangle=row['rectangle'],
                    private_mci_window_draws=row['private_mci_window_draws'],
                    packet_sha256=digest(row['packet']),source_rgb_sha256=digest(row['source_rgb']),
                    window_argb_sha256=digest(row['window_argb']))
                entry.update(record_version=row['record_version'], clock=row['clock'])
                pending.append((path,before,entry))
            check_space(self.directory);opened=open_files()
            for path,before,_ in pending:
                if identity(path.stat())!=identity(before) or (before.st_dev,before.st_ino) in opened:
                    raise ValueError('Movie readback reopened or changed before cleanup')
            for path,_,entry in pending:
                path.unlink();self.records.append(entry)

    def run(self):
        try:
            while not self.stop.wait(.05):
                self.drain()
        except BaseException as error:
            self.error = error

    def finish(self):
        self.stop.set()
        self.thread.join(timeout=60)
        if self.thread.is_alive():
            raise RuntimeError('Movie collector did not stop')
        self.check()
        self.drain(final=True)
        if list(self.directory.glob('movie-video.bin.*')) or not self.records:
            raise ValueError('Unclosed or missing movie readbacks')
        report = dict(scope=__doc__.strip(), pass_=True, record_bytes=RECORD_BYTES,
                      frames=len(self.records), records=self.records)
        (self.archive/'manifest.json').write_text(json.dumps(report, indent=2)+'\n')
        return report
