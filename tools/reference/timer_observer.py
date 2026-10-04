"""Build a reference-only PE observer around the installed Wine timer API.

The backend retains every original section byte. Only its DOS-stub builtin
marker is neutralized so Wine can load the same implementation under a private
alias; otherwise the loader redirects to a nonexistent builtin alias. Public
ordinals/names remain unchanged. No original game executable is modified.
"""
import hashlib
import json
from pathlib import Path
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[2]
SYSTEM_WINMM=Path('/usr/lib/i386-linux-gnu/wine/i386-windows/winmm.dll')
KERNEL={'GetProcAddress':8,'LoadLibraryA':4,'GetEnvironmentVariableA':12,
        'CreateFileA':28,'WriteFile':20,'GetProcessHeap':0,'HeapAlloc':12,
        'GetTickCount':0,'GetCurrentThreadId':0,'CreateMutexA':12,
        'WaitForSingleObject':8,'ReleaseMutex':4,'QueryPerformanceCounter':4,
        'QueryPerformanceFrequency':4,'ExitProcess':4,'DisableThreadLibraryCalls':4,
        'CreateEventA':16,'SetEvent':4,'InterlockedIncrement':4,'GetStdHandle':4,
        'InterlockedCompareExchange':12,'InterlockedExchange':8,'Sleep':4,'OutputDebugStringA':4}


def pe_exports(raw):
    pe=struct.unpack_from('<I',raw,0x3c)[0]
    if raw[pe:pe+4]!=b'PE\0\0' or struct.unpack_from('<H',raw,pe+4)[0]!=0x14c:
        raise ValueError('32-bit Wine PE required')
    count,opt=struct.unpack_from('<H',raw,pe+6)[0],struct.unpack_from('<H',raw,pe+20)[0]
    sections=[]
    for i in range(count):
        off=pe+24+opt+40*i
        va,size,pointer=struct.unpack_from('<III',raw,off+12)
        sections.append(dict(name=raw[off:off+8].rstrip(b'\0').decode(),va=va,size=size,pointer=pointer,
                             sha256=hashlib.sha256(raw[pointer:pointer+size]).hexdigest()))
    def offset(rva):
        for section in sections:
            if section['va']<=rva<section['va']+section['size']:
                return section['pointer']+rva-section['va']
        raise ValueError('PE RVA outside recorded sections')
    def text(rva):
        at=offset(rva);return raw[at:raw.index(b'\0',at)].decode('ascii')
    export_rva,export_size=struct.unpack_from('<II',raw,pe+24+96)
    at=offset(export_rva)
    base,functions,names,ft,nt,ot=struct.unpack_from('<IIIIII',raw,at+16)
    byindex={struct.unpack_from('<H',raw,offset(ot)+2*i)[0]:text(struct.unpack_from('<I',raw,offset(nt)+4*i)[0]) for i in range(names)}
    exports=[]
    for i in range(functions):
        rva=struct.unpack_from('<I',raw,offset(ft)+4*i)[0]
        if rva:
            exports.append(dict(ordinal=base+i,name=byindex.get(i),
                                forwarder=text(rva) if export_rva<=rva<export_rva+export_size else None))
    return exports,sections,pe


def verify_forwarders(actual,exports):
    identity=lambda entries:[(e['ordinal'],e['name']) for e in entries]
    if identity(actual)!=identity(exports):raise ValueError('Observer public ABI differs')
    for entry in actual:
        name,ordinal=entry['name'],entry['ordinal']
        target=None if name in ('timeSetEvent','timeKillEvent') else '_winmm_real.'+(name or '#'+str(ordinal))
        if entry['forwarder']!=target:
            raise ValueError('Observer export forwards to the wrong backend: '+str(name or ordinal))


def observer_digest(raw):
    # A previous capture can predate /timestamp:0. Ignore only the four-byte
    # COFF creation timestamp when proving that its probe code is identical.
    _,_,pe=pe_exports(raw)
    return hashlib.sha256(raw[:pe+8]+b'\0'*4+raw[pe+12:]).hexdigest()


def build(output,system=SYSTEM_WINMM,*,observer_source=None):
    output=Path(output).resolve()
    if Path('/tmp/wasm-dd2') not in output.parents:raise ValueError('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=True)
    original=system.read_bytes();exports,sections,pe=pe_exports(original)
    marker=b'Wine builtin DLL';at=original.find(marker)
    if at!=64 or at+len(marker)>=pe:raise ValueError('Expected non-executable Wine DOS-stub marker')
    backend=original[:at]+b' '*len(marker)+original[at+len(marker):]
    if pe_exports(backend)[:2]!=(exports,sections):raise ValueError('Backend sections or exports changed')
    # lld's i386 DEF decoration prefixes the forwarder DLL with '_'. Use
    # that same private alias for explicit timer calls and every forwarder.
    (output/'_winmm_real.dll').write_bytes(backend)
    definitions=['LIBRARY winmm.dll','EXPORTS']
    for entry in exports:
        name,ordinal=entry['name'],entry['ordinal']
        if name in ('timeSetEvent','timeKillEvent'):
            arguments=20 if name=='timeSetEvent' else 4
            definitions.append(f'  {name}=_{name}@{arguments} @{ordinal}')
        elif name:definitions.append(f'  {name}=winmm_real.{name} @{ordinal}')
        else:definitions.append(f'  ordinal_{ordinal}=winmm_real.#{ordinal} @{ordinal} NONAME')
    (output/'winmm.def').write_text('\n'.join(definitions)+'\n')
    (output/'kernel32.def').write_text('LIBRARY kernel32.dll\nEXPORTS\n'+''.join(f'  {name}@{size}\n' for name,size in KERNEL.items()))
    subprocess.run(['llvm-dlltool','-m','i386','-k','-d',str(output/'kernel32.def'),'-l',str(output/'kernel32.lib')],check=True)
    flags=['--target=i686-windows-gnu','-ffreestanding','-fno-builtin','-fno-stack-protector','-O2','-Wall','-Wextra','-Werror']
    source=Path(observer_source) if observer_source else Path(__file__).with_name('winmm_timer_observer.c')
    subprocess.run(['clang',*flags,'-c',str(source),'-o',str(output/'proxy.obj')],check=True)
    subprocess.run(['lld-link','/dll','/nodefaultlib','/timestamp:0','/entry:DllMain@12','/machine:x86',
                    '/def:'+str(output/'winmm.def'),'/out:'+str(output/'winmm.dll'),
                    str(output/'proxy.obj'),str(output/'kernel32.lib')],check=True)
    actual=pe_exports((output/'winmm.dll').read_bytes())[0]
    verify_forwarders(actual,exports)
    report=dict(scope=__doc__.strip(),backend_path=str(system),backend_sha256=hashlib.sha256(original).hexdigest(),
                private_backend_sha256=hashlib.sha256(backend).hexdigest(),modified_header_bytes=list(range(at,at+len(marker))),
                unchanged_sections=sections,exports=exports,observer_exports=actual,
                source_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [source,source.with_suffix('.h')]},
                observer_sha256=hashlib.sha256((output/'winmm.dll').read_bytes()).hexdigest(),
                observer_normalized_sha256=observer_digest((output/'winmm.dll').read_bytes()),record_bytes=72)
    (output/'build.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


def build_probe(output):
    (output/'winmm-import.def').write_text('LIBRARY winmm.dll\nEXPORTS\n  timeSetEvent@20\n  timeKillEvent@4\n')
    subprocess.run(['llvm-dlltool','-m','i386','-k','-d',str(output/'winmm-import.def'),'-l',str(output/'winmm-import.lib')],check=True)
    source=Path(__file__).with_name('winmm_timer_probe.c')
    subprocess.run(['clang','--target=i686-windows-gnu','-ffreestanding','-fno-builtin','-fno-stack-protector','-O2',
                    '-Wall','-Wextra','-Werror','-c',str(source),'-o',str(output/'probe.obj')],check=True)
    subprocess.run(['lld-link','/nodefaultlib','/entry:start','/machine:x86','/subsystem:console',
                    '/out:'+str(output/'probe.exe'),str(output/'probe.obj'),str(output/'kernel32.lib'),str(output/'winmm-import.lib')],check=True)


if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();report=build(args.output);build_probe(args.output)
    print('Built timer observer; preserved',len(report['exports']),'exports and all',len(report['unchanged_sections']),'original sections')
