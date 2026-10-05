"""Build a reference-only forwarding DirectDraw1 presentation observer.

The installed Wine backend keeps every PE section and public export. Only its
non-executable DOS-stub builtin marker is neutralized for loading under a private
alias, as with the existing timer observer. Original game bytes are unchanged.
The observer covers the supported game's 640x480 indexed DirectDraw1 route,
not arbitrary COM clients, AVI output, physical timing or port parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))

from reference.timer_observer import KERNEL,pe_exports
from artifacts import WORK,check_space

SYSTEM_DDRAW=Path('/usr/lib/i386-linux-gnu/wine/i386-windows/ddraw.dll')


def build(output,system=SYSTEM_DDRAW):
    output=Path(output).resolve()
    if WORK not in output.parents:raise ValueError('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=True)
    original=system.read_bytes();exports,sections,pe=pe_exports(original)
    marker=b'Wine builtin DLL';at=original.find(marker)
    if at!=64 or at+len(marker)>=pe:raise ValueError('Expected non-executable Wine DOS-stub marker')
    backend=original[:at]+b' '*len(marker)+original[at+len(marker):]
    if pe_exports(backend)[:2]!=(exports,sections):raise ValueError('Backend sections or exports changed')
    (output/'_ddraw_real.dll').write_bytes(backend)
    definitions=['LIBRARY ddraw.dll','EXPORTS']
    for e in exports:
        name,ordinal=e['name'],e['ordinal']
        if name=='DirectDrawCreate':definitions.append(f'  {name}=_{name}@12 @{ordinal}')
        elif name:definitions.append(f'  {name}=ddraw_real.{name} @{ordinal}')
        else:definitions.append(f'  ordinal_{ordinal}=ddraw_real.#{ordinal} @{ordinal} NONAME')
    (output/'ddraw.def').write_text('\n'.join(definitions)+'\n')
    kernel={**KERNEL,'CloseHandle':4,'MoveFileA':8}
    (output/'kernel32.def').write_text('LIBRARY kernel32.dll\nEXPORTS\n'+''.join(f'  {name}@{size}\n' for name,size in kernel.items()))
    subprocess.run(['llvm-dlltool','-m','i386','-k','-d',str(output/'kernel32.def'),'-l',str(output/'kernel32.lib')],check=True)
    assembly=['.def @feat.00; .scl 3; .type 0; .endef','.set @feat.00, 1','.text'];declarations=[];initializers=[]
    for kind,count in [('draw',23),('surface',36)]:
        for slot in range(count):
            name=f'{kind}_forward{slot}'
            assembly.extend([f'.globl _{name}',f'_{name}:','movl 4(%esp), %eax','movl 4(%eax), %eax',
                             'movl %eax, 4(%esp)','movl (%eax), %eax',f'jmp *{slot*4}(%eax)'])
            declarations.append(f'extern void {name}(void);')
            initializers.append(f'    {kind}_table[{slot}]=(void*){name};')
    (output/'forwarders.S').write_text('\n'.join(assembly)+'\n')
    (output/'ddraw_video_forwarders.h').write_text('\n'.join(declarations)+
        '\nstatic void initialize_tables(void){\n'+'\n'.join(initializers)+'\n}\n')
    source=Path(__file__).with_suffix('.c')
    flags=['--target=i686-windows-gnu','-ffreestanding','-fno-builtin','-fno-stack-protector','-O2','-Wall','-Wextra','-Werror']
    subprocess.run(['clang',*flags,'-I',str(output),'-c',str(source),'-o',str(output/'observer.obj')],check=True)
    subprocess.run(['clang','--target=i686-windows-gnu','-c',str(output/'forwarders.S'),'-o',str(output/'forwarders.obj')],check=True)
    subprocess.run(['lld-link','/dll','/nodefaultlib','/timestamp:0','/entry:DllMain@12','/machine:x86',
                    '/def:'+str(output/'ddraw.def'),'/out:'+str(output/'ddraw.dll'),
                    str(output/'observer.obj'),str(output/'forwarders.obj'),str(output/'kernel32.lib')],check=True)
    actual=pe_exports((output/'ddraw.dll').read_bytes())[0]
    if [(e['name'],e['ordinal']) for e in actual]!=[(e['name'],e['ordinal']) for e in exports]:
        raise ValueError('Observer public ABI differs')
    for e in actual:
        expected=None if e['name']=='DirectDrawCreate' else '_ddraw_real.'+(e['name'] or '#'+str(e['ordinal']))
        if e['forwarder']!=expected:raise ValueError('Observer export forwards to wrong backend')
    inputs=[source,source.with_name('winmm_timer_observer.h'),output/'forwarders.S',output/'ddraw_video_forwarders.h']
    report=dict(scope=__doc__.strip(),backend_path=str(system),backend_sha256=hashlib.sha256(original).hexdigest(),
                private_backend_sha256=hashlib.sha256(backend).hexdigest(),modified_header_bytes=list(range(at,at+len(marker))),
                unchanged_sections=sections,exports=exports,observer_exports=actual,
                source_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
                observer_sha256=hashlib.sha256((output/'ddraw.dll').read_bytes()).hexdigest(),
                record_bytes=128+307200+1024+1024,maximum_frames=4096,
                record_versions=[1,2,3],rng_record_version=3,rng_getter_address=0x4571fb,
                rng_getter_bytes='a17c099400c3',rng_thread_pointer_address=0x94097c,rng_seed_offset=12,
                archive_maximum_frames=60000,archive_maximum_chunk_frames=128)
    (output/'build.json').write_text(json.dumps(report,indent=2)+'\n');check_space(output)
    return report


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();r=build(args.output)
    print('Built DirectDraw observer; preserved',len(r['exports']),'exports and all',len(r['unchanged_sections']),'original sections')
