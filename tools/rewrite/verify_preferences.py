#!/usr/bin/env python3
"""Verify actual audio-preference application actions and retained source profiles.

Original factory packs, complete independently predicted Native/IndexedDB images,
live engine/music gains and browser PCM/restart cover preference integration.
This does not establish full configuration or playable saved-game restoration.
"""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import hashlib
import json
import os
from pathlib import Path
import shlex
import struct
import subprocess
import sys
import threading

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, open_files, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.serve import BUILD, Handler
from rewrite.verify_save_store import put, delete
from rewrite.verify_window import build_sanitized
from verify_season_transition import EXE_SHA

BLOCK, SIZE, PREFIX = 8192, 131072, 6526


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def extension(music, version=1, flags=0, length=16):
    record = b'D2CF'+struct.pack('<4H', version, length, music, flags)
    check = 2166136261
    for value in record:
        check = ((check ^ value)*16777619) & 0xffffffff
    return record+struct.pack('<I', check)


def preferences(original, effects, music):
    result = bytearray(original)
    struct.pack_into('<H', result, 0, 0x1010)
    struct.pack_into('<H', result, 16, (effects*4090+128)//256)
    result[PREFIX:PREFIX+16] = extension(music)
    return bytes(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK/'rewrite-preferences-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    files = [path for path in (ROOT/'src').rglob('*') if path.suffix in ('.c','.h','.js','.html')]
    files += [ROOT/name for name in ('tests/configuration_test.c',
              'tests/configuration_application_export.c','tools/reference/configuration_defaults_fixture.c',
              'tools/reference/save_profile_fixture.c','tools/reference/pe_fixture.h',
              'tools/rewrite/verify_window.py','tools/rewrite/verify_save_store.py',
              'tools/rewrite/verify_preferences_browser.js','CMakeLists.txt','Makefile')]
    files.append(Path(__file__).resolve())
    sources = {str(path.relative_to(ROOT)):digest(path) for path in files}
    calls, cases, binaries = [], [], {}

    def run(command, label, timeout=180, env=None):
        log=output/(label+'.log')
        with log.open('wb') as stream:
            result=run_bounded(command,directory=output,cwd=ROOT,timeout=timeout,
                               stdout=stream,stderr=subprocess.STDOUT,env=env)
        text=log.read_text(errors='replace')
        calls.append(dict(label=label,command=command,returncode=result.returncode,sha256=digest(log)))
        if result.returncode or 'Sanitizer:' in text or 'runtime error:' in text:
            raise RuntimeError(label+' failed: '+text[-5000:])
        return text

    exe, save, archive = [ROOT/'DestructionDerby2'/name for name in ('dd2h.exe','SaveGames','Dirinfo')]
    original_save = digest(save)
    if digest(exe) != EXE_SHA:
        raise ValueError('Provision the supported original')
    for stem in ('configuration_defaults','save_profile'):
        binary=output/(stem+'-original')
        run(['gcc','-m32','-no-pie','-O0','-std=gnu99','-w',str(ROOT/f'tools/reference/{stem}_fixture.c'),'-o',str(binary)],stem+'-build')
        run([str(binary),str(exe),str(output/(stem+'.bin'))],stem+'-run')
    original=(output/'configuration_defaults.bin').read_bytes()
    if len(original)!=BLOCK:
        raise ValueError('Original factory pack extent differs')
    factory=preferences(original,256,256)
    a=preferences(original,128,64);b=preferences(original,32,16)
    empty=bytes(SIZE);ab=put(put(empty,0,'A',a),1,'B',b);remaining=delete(ab,0)
    sanitized_unit=output/'unit-sanitized'
    flags=['-std=c11','-O1','-g','-I'+str(ROOT/'src'),'-Wall','-Wextra','-Wpedantic',
           '-Wno-unused-parameter','-Wno-unused-function','-fno-strict-aliasing','-ffast-math',
           '-Werror','-Wshadow','-Wconversion','-Wstrict-prototypes','-Wmissing-prototypes',
           '-Wformat=2','-fsanitize=address,undefined','-fno-omit-frame-pointer']
    run([tool('clang'),*flags,*[str(ROOT/name) for name in ('src/game/configuration.c',
         'src/assets/save_profile.c','tests/configuration_test.c')],'-o',str(sanitized_unit)],'unit-sanitized-build')
    for target,command in [('native',[str(WORK/'rewrite-native/dd2_configuration_test')]),
                           ('wasm',['node',str(BUILD/'dd2_configuration_test.js')]),
                           ('sanitized',[str(sanitized_unit)])]:
        packed=output/(target+'-factory.block');run([*command,str(packed)],'factory-'+target)
        if packed.read_bytes()!=factory:
            raise ValueError(target+' factory/original prefix or extension differs')
        cases.append(dict(label='factory-'+target,bytes=BLOCK,original_prefix_bytes=PREFIX,
                          retained_suffix_bytes=BLOCK-PREFIX-16,sha256=digest(packed),pass_=True))
    sanitized=build_sanitized(output,ROOT/'tests/configuration_application_export.c')
    commands={'native':[str(WORK/'rewrite-native/dd2_configuration_application_export')],
              'sanitized':[str(sanitized)]}
    for path in [Path(command[0]) for command in commands.values()]+[sanitized_unit,
                 WORK/'rewrite-native/dd2_configuration_test',BUILD/'dd2_configuration_test.js',
                 BUILD/'dd2_configuration_test.wasm',BUILD/'dd2_app.js',BUILD/'dd2_app.wasm',
                 BUILD/'viewer.js',BUILD/'index.html',*BUILD.glob('dd2_app.worker.js')]:
        binaries[str(path)]=digest(path)
    environment=os.environ.copy();environment.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
    legacy=bytearray(original);struct.pack_into('<H',legacy,16,3681);legacy=bytes(legacy)
    patterned=(output/'save_profile.bin').read_bytes()[:BLOCK]
    patterned=bytearray(patterned);struct.pack_into('<H',patterned,16,3681);patterned=bytes(patterned)
    variants=[('factory',factory,True),('legacy',legacy,True),('patterned-legacy',patterned,True)]
    for label,kind in [('startup',0x1020),('game',0x3030),('replay',0x2020)]:
        value=bytearray(legacy);struct.pack_into('<H',value,0,kind);variants.append((label,bytes(value),kind==0x1020))
    for label,volume in [('negative-effects',65535),('excess-effects',4091)]:
        value=bytearray(factory);struct.pack_into('<H',value,16,volume);variants.append((label,bytes(value),False))
    value=bytearray(factory);value[5828:5840]=b'X'*12;variants.append(('invalid-player-text',bytes(value),False))
    for label,record in [('excess-music',extension(257)),('future-version',extension(64,version=2)),
                         ('unknown-flags',extension(64,flags=1)),('bad-length',extension(64,length=15)),
                         ('bad-checksum',extension(64)[:-1]+bytes([extension(64)[-1]^1]))]:
        value=bytearray(factory);value[PREFIX:PREFIX+16]=record;variants.append((label,bytes(value),False))
    for target,command in commands.items():
        directory=output/target;directory.mkdir(exist_ok=True)
        def action(label,operation,logical,image,expected,loaded,effects,music):
            folder=directory/label;folder.mkdir(exist_ok=True)
            if image is not None:(folder/'SaveGames').write_bytes(image)
            snapshot=output/(target+'-'+label+'.card')
            row=json.loads(run([*command,str(archive),str(folder),str(snapshot),operation,str(logical)],target+'-'+label,env=environment))
            if row['loaded']!=int(loaded) or row['effects']!=effects or row['music']!=music:
                raise ValueError(target+' '+label+' live preference values differ: '+str(row))
            if snapshot.read_bytes()!=expected or (folder/'SaveGames').read_bytes()!=expected:
                raise ValueError(target+' '+label+' complete memory/disk image differs')
            cases.append(dict(target=target,label=label,live=row,sha256=digest(snapshot),pass_=True))
            return folder
        self_folder=action('self-test','self-test',0,None,remaining,True,32,16)
        restart=output/(target+'-restart.card')
        row=json.loads(run([*command,str(archive),str(self_folder),str(restart),'restore','0'],target+'-restart',env=environment))
        if row['loaded']!=1 or row['effects']!=32 or row['music']!=16 or restart.read_bytes()!=remaining:
            raise ValueError(target+' fresh process did not restore saved preferences')
        cases.append(dict(target=target,label='fresh-process-restore',live=row,sha256=digest(restart),pass_=True))
        for label,payload,valid in variants:
            image=put(empty,0,'SOURCE',payload);expected=image
            effects,music=32,16
            if valid:
                volume=struct.unpack_from('<H',payload,16)[0];effects=(volume*256+2045)//4090
                music=256 if label=='factory' else 16
                encoded=bytearray(payload);struct.pack_into('<H',encoded,0,0x1010);encoded[PREFIX:PREFIX+16]=extension(music)
                expected=put(image,0,'EDIT',bytes(encoded))
            action(label,'load-save',0,image,expected,valid,effects,music)
        duplicate=put(empty,0,'B',a);duplicate=bytearray(put(duplicate,1,'SECOND',b));duplicate[512+4:512+6]=b'B\0';duplicate=bytes(duplicate)
        action('duplicate-physical-selection','load',1,duplicate,duplicate,True,32,16)
    (output/'browser-input.json').write_text(json.dumps(dict(factory=factory.hex(),legacy=legacy.hex(),
        patterned=patterned.hex(),a=a.hex(),b=b.hex(),empty=empty.hex(),ab=ab.hex(),remaining=remaining.hex())))
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(BUILD)))
    thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    try:
        run(['node',str(ROOT/'tools/rewrite/verify_preferences_browser.js'),f'http://127.0.0.1:{server.server_port}/',
             str(archive),str(ROOT/'DestructionDerby2/Redbook/track02.cdda'),str(output)],'browser',timeout=300)
    finally:server.shutdown();thread.join();server.server_close()
    browser=json.loads((output/'browser-report.json').read_text())
    if not browser['complete']:raise ValueError('Browser acceptance missing')
    # Give SDL's Linux keyboard probe a controlling terminal. Without one the
    # installed SDL2 closes -1 during video initialization. Keep all descriptor
    # checks enabled and use no suppressions; script propagates Valgrind's status.
    memcheck_command=['valgrind','--error-exitcode=99','--leak-check=full','--show-leak-kinds=all',
                      '--errors-for-leak-kinds=definite,indirect','--track-fds=yes',*commands['native'],
                      str(archive),str(output/'native/self-test'),str(output/'memcheck.card'),'restore','0']
    memcheck=run(['script','--quiet','--return','--command',shlex.join(memcheck_command),'/dev/null'],
                 'memcheck',env=environment)
    if 'ERROR SUMMARY: 0 errors' not in memcheck:raise ValueError('Native application Memcheck findings')
    if digest(save)!=original_save or digest(exe)!=EXE_SHA or {str(p.relative_to(ROOT)):digest(p) for p in files}!=sources:
        raise ValueError('Source/original identity changed')
    report=dict(scope=__doc__,source_sha256=sources,binary_sha256=binaries,original_exe_sha256=EXE_SHA,
                original_save_sha256=original_save,original_factory_sha256=hashlib.sha256(original).hexdigest(),
                calls=calls,cases=cases,browser=browser,memcheck_zero_errors=True,
                memcheck_scope='Actual Native application with SDL dummy devices and a controlling terminal; descriptor checks enabled, no suppressions. No lost blocks; system DBus retains reachable process globals.',
                complete=True)
    (output/'verification-report.json').write_text(json.dumps(report,indent=2)+'\n')
    opened=open_files();removed=[];retained=[]
    keep={output/name for name in ('verification-report.json','browser-report.json','sanitizer-build.json','preferences-loaded.png')}
    for path in sorted(output.rglob('*'),key=lambda p:len(p.parts),reverse=True):
        if path in keep:continue
        if path.is_symlink():path.unlink()
        elif path.is_file():
            st=path.stat()
            if (st.st_dev,st.st_ino) in opened:retained.append(str(path));continue
            removed.append(dict(path=str(path),bytes=st.st_size,sha256=digest(path)));path.unlink()
        elif path.is_dir():
            try:path.rmdir()
            except OSError:pass
    (output/'cleanup-report.json').write_text(json.dumps(dict(removed=removed,retained_open=retained),indent=2)+'\n')
    print(json.dumps(dict(complete=True,original_and_native_cases=len(cases),browser_cases=len(browser['cases']))))


if __name__=='__main__':main()
