#!/usr/bin/env python3
"""Verify durable production save storage and delayed C ownership separately.

Compare complete Native/Chromium card images independently, including failed
writes, actual process interruption, browser abort/stale owners and restart.
This establishes storage, not settings application or gameplay continuation.
"""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import hashlib
import json
import os
from pathlib import Path
import select
import signal
import struct
import subprocess
import sys
import threading
import time

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, open_files, prepare_output, run_bounded
from rewrite.build_wasm import cache_environment
from rewrite.quality import ROOT, tool
from rewrite.serve import Handler, FILES
from verify_season_transition import EXE_SHA

SIZE, BLOCK, SLOTS = 131072, 8192, 15


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def put(before, logical, name, payload):
    data = bytearray(before)
    occupied = [slot for slot in range(SLOTS) if struct.unpack_from('<I', data, slot*512)[0]]
    replaced = occupied[logical] if logical < len(occupied) else None
    if replaced is not None:
        data[replaced*512:replaced*512+5] = bytes(5)
    target = next(slot for slot in range(SLOTS) if struct.unpack_from('<I', data, slot*512)[0] == 0)
    struct.pack_into('<I', data, target*512, 1)
    data[target*512+4:target*512+5+len(name)] = name.encode()+b'\0'
    data[(target+1)*BLOCK:(target+2)*BLOCK] = payload
    return bytes(data)


def delete(before, logical):
    data = bytearray(before)
    occupied = [slot for slot in range(SLOTS) if struct.unpack_from('<I', data, slot*512)[0]]
    target = occupied[logical]
    data[target*512:target*512+5] = bytes(5)
    return bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK/'rewrite-save-store-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    files = [ROOT/name for name in ('src/platform/save_store.c', 'src/platform/save_store.h',
             'src/platform/save_backend.c', 'src/platform/save_backend.h',
             'src/platform/web/save_store.js', 'src/assets/save_card.c', 'src/assets/save_card.h',
             'src/platform/file.c', 'tests/save_store_test.c', 'tests/save_store_export.c',
             'tests/save_store_probe.c', 'tools/reference/save_profile_fixture.c',
             'tools/reference/pe_fixture.h', 'tools/rewrite/verify_save_store_browser.js',
             'CMakeLists.txt', 'Makefile')]
    files.append(Path(__file__).resolve())
    sources = {str(path.relative_to(ROOT)):digest(path) for path in files}
    calls, cases = [], []

    def run(command, label, timeout=120):
        log = output/(label+'.log')
        with log.open('wb') as stream:
            result = run_bounded(command, directory=output, cwd=ROOT, timeout=timeout,
                                 stdout=stream, stderr=subprocess.STDOUT)
        text = log.read_text(errors='replace')
        calls.append(dict(label=label, command=command, returncode=result.returncode, sha256=digest(log)))
        if result.returncode or 'Sanitizer:' in text or 'runtime error:' in text:
            raise RuntimeError(label+' failed: '+text[-5000:])
        return text

    exe = ROOT/'DestructionDerby2/dd2h.exe'
    save = ROOT/'DestructionDerby2/SaveGames'
    before_save = digest(save)
    if digest(exe) != EXE_SHA:
        raise ValueError('Provision the supported original')
    original = output/'original-packer'
    packs = output/'original-packs.bin'
    run(['gcc','-m32','-no-pie','-O0','-std=gnu99','-w',
         str(ROOT/'tools/reference/save_profile_fixture.c'),'-o',str(original)],'original-build')
    run([str(original),str(exe),str(packs)],'original-run')
    packed = packs.read_bytes()
    if len(packed) != 48*BLOCK:
        raise ValueError('Original pack extent differs')
    payloads = [packed[i*BLOCK:(i+1)*BLOCK] for i in range(4)]
    inputs = []
    for i, payload in enumerate(payloads):
        path = output/f'payload-{i}.block'
        path.write_bytes(payload)
        inputs.append(path)
    short = output/'short.block';short.write_bytes(payloads[0][1:])
    flags = ['-std=c11','-O1','-g','-I'+str(ROOT/'src'),'-D_POSIX_C_SOURCE=200809L',
             '-Wall','-Wextra','-Wpedantic','-Wno-unused-parameter','-Wno-unused-function',
             '-fno-strict-aliasing','-ffast-math','-Werror','-Wshadow','-Wconversion',
             '-Wstrict-prototypes','-Wmissing-prototypes','-Wformat=2',
             '-fsanitize=address,undefined','-fno-omit-frame-pointer']
    wasm_flags = [flag for flag in flags if not flag.startswith('-fsanitize=')]
    run([tool('clang-tidy'),'--warnings-as-errors=*',
         str(ROOT/'src/platform/save_backend.c'),'--',*wasm_flags,
         '--target=wasm32-unknown-emscripten','-D__EMSCRIPTEN__',
         '--sysroot='+str(Path(cache_environment()['EM_CACHE'])/'sysroot')],
        'active-wasm-tidy')
    common = [str(ROOT/'src/platform/save_store.c'),str(ROOT/'src/assets/save_card.c')]
    sanitized = output/'export-sanitized'
    run([tool('clang'),*flags,*common,str(ROOT/'src/platform/save_backend.c'),
         str(ROOT/'src/platform/file.c'),str(ROOT/'tests/save_store_export.c'),
         '-Wl,--wrap=write,--wrap=read,--wrap=fsync,--wrap=close,--wrap=renameat',
         '-o',str(sanitized)],'build-sanitized-export')
    unit = output/'unit-sanitized'
    run([tool('clang'),*flags,*common,str(ROOT/'tests/save_store_test.c'),'-o',str(unit)],'build-sanitized-unit')
    for label, command in [('native',[str(WORK/'rewrite-native/dd2_save_store_test')]),
                           ('wasm',['node',str(WORK/'rewrite-wasm/dd2_save_store_test.js')]),
                           ('sanitized',[str(unit)])]:
        run(command,'delayed-owner-'+label)
    commands = {'native':[str(WORK/'rewrite-native/dd2_save_store_export')],
                'sanitized':[str(sanitized)]}
    binaries = {str(Path(command[0])):digest(Path(command[0])) for command in commands.values()}
    for path in [WORK/'rewrite-native/dd2_save_store_test',WORK/'rewrite-wasm/dd2_save_store_test.js',
                 WORK/'rewrite-wasm/dd2_save_store_test.wasm',WORK/'rewrite-wasm/dd2_save_store_probe.js',
                 WORK/'rewrite-wasm/dd2_save_store_probe.wasm',unit]:
        binaries[str(path)] = digest(path)
    for target, command in commands.items():
        directory = output/target;directory.mkdir(exist_ok=True)
        disk = directory/'SaveGames'
        image = bytes(SIZE)
        number = 0
        def action(operation, logical=0, name='', payload=0, fault=0, result=2, phase=2,
                   expected_memory=None, expected_disk=None):
            nonlocal number
            number += 1
            label = f'{target}-{number:02d}-{operation}-{fault}'
            snapshot = output/(label+'.card')
            selected = inputs[payload] if isinstance(payload,int) else payload
            row = json.loads(run([*command,str(directory),str(snapshot),operation,str(logical),
                                  name,str(selected),str(fault)],label))
            if row['result'] != result or row['phase'] != phase:
                raise ValueError(label+' result/phase differs: '+str(row))
            if expected_memory is not None and (not snapshot.exists() or snapshot.read_bytes()!=expected_memory):
                raise ValueError(label+' accepted complete memory differs')
            if expected_disk is not None and (not disk.exists() or disk.read_bytes()!=expected_disk):
                raise ValueError(label+' complete persisted disk differs')
            if (directory/'.SaveGames.pending').exists():
                raise ValueError(label+' left a completed pending file')
            cases.append(dict(target=target,label=label,state=row,
                              memory_sha256=digest(snapshot) if snapshot.exists() else None,
                              disk_sha256=digest(disk) if disk.is_file() and not disk.is_symlink() else None,pass_=True))
            return row
        action('inspect',expected_memory=image)
        if disk.exists():raise ValueError('Read created a card')
        for logical,name,payload in [(0,'A',0),(1,'B',1)]:
            image=put(image,logical,name,payloads[payload]);action('put',logical,name,payload,expected_memory=image,expected_disk=image)
        image=delete(image,0);action('delete',expected_memory=image,expected_disk=image)
        image=put(image,1,'C',payloads[2]);action('put',1,'C',2,expected_memory=image,expected_disk=image)
        action('put',2,'B',3,result=9,expected_memory=image,expected_disk=image)
        action('put',2,'BAD',short,result=3,expected_memory=image,expected_disk=image)
        action('delete',2,result=10,expected_memory=image,expected_disk=image)
        action('put',2,'ABCDEFGHI',0,result=3,expected_memory=image,expected_disk=image)
        for logical,name in [(2,'ABCDEFGH'),(3,'')]+[(i,'S'+str(i)) for i in range(4,15)]:
            image=put(image,logical,name,payloads[logical%4]);action('put',logical,name,logical%4,expected_memory=image,expected_disk=image)
        action('put',15,'FULL',result=8,expected_memory=image,expected_disk=image)
        action('lock',expected_memory=image,expected_disk=image)
        for fault in [2,3,4,5,7,12]:
            action('put',0,'FAILED',1,fault,result=5,expected_memory=image,expected_disk=image)
        for fault in [1,8]:
            image=put(image,0,'TRANS',payloads[1]);action('put',0,'TRANS',1,fault,expected_memory=image,expected_disk=image)
        prior=image;image=put(image,0,'AMBIG',payloads[2])
        row=action('put',0,'AMBIG',2,6,result=7,phase=5,expected_memory=prior,expected_disk=image)
        if row['first']!=7:raise ValueError('Directory sync ambiguity lost')
        action('inspect',expected_memory=image,expected_disk=image)
        image=put(image,0,'RECOVER',payloads[3]);row=action('recover',0,'RECOVER',3,6,expected_memory=image,expected_disk=image)
        if row['first']!=7:raise ValueError('Recovery fabricated save success')
        prior=image;external=bytearray(image);external[-1]^=1;image=bytes(external)
        action('stale-put',0,'STALE',0,result=6,phase=5,expected_memory=prior,expected_disk=image)
        action('inspect',expected_memory=image,expected_disk=image)
        if target=='native':
            for fault in [9,10]:
                candidate=put(image,0,'CRASH',payloads[0])
                args_command=[*command,str(directory),str(output/f'crash-{fault}.card'),'put','0','CRASH',str(inputs[0]),str(fault)]
                process=subprocess.Popen(args_command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,cwd=ROOT,start_new_session=True)
                try:
                    ready,_,_=select.select([process.stdout],[],[],20)
                    if not ready or process.stdout.readline()!=b'checkpoint\n':raise ValueError('Crash checkpoint not observed')
                    for _ in range(100):
                        state=Path(f'/proc/{process.pid}/status').read_text()
                        if 'State:\tT' in state:break
                        time.sleep(.01)
                    else:raise ValueError('Writer not authoritatively stopped')
                    denied=json.loads(run([*command,str(directory),str(output/'live-denied.card'),'inspect','0','',str(inputs[0]),'0'],f'live-lease-{fault}'))
                    if denied['result']!=6:raise ValueError('Live writer lease was not respected')
                    visible=image if fault==9 else candidate
                    if disk.read_bytes()!=visible:raise ValueError('Crash boundary partial publication')
                    process.kill();stdout,stderr=process.communicate(timeout=10)
                    if process.returncode!=-signal.SIGKILL:raise ValueError('Writer did not terminate')
                    image=visible
                    cases.append(dict(target=target,label=f'process-interruption-{fault}',pid=process.pid,signal='SIGKILL',boundary='before-file-sync' if fault==9 else 'after-rename-before-directory-sync',disk_sha256=digest(disk),pass_=True))
                finally:
                    if process.poll() is None:process.kill();process.wait(timeout=10)
                action('inspect',expected_memory=image,expected_disk=image)
        for label,raw in [('truncated',image[:-1]),('extra',image+b'X'),('header',b'\x02'+image[1:]),('name',image[:4]+b'X'*9+image[13:])]:
            disk.write_bytes(raw);action('inspect',result=3,phase=0,expected_disk=raw)
        disk.unlink();target_file=directory/'symlink-target';target_file.write_bytes(image);disk.symlink_to(target_file)
        action('inspect',result=3,phase=0)
        if target_file.read_bytes()!=image:raise ValueError('Symlink target was touched')
        disk.unlink();os.mkfifo(disk)
        action('inspect',result=3,phase=0)
        disk.unlink();disk.write_bytes(image)
        action('inspect',expected_memory=image,expected_disk=image)
    empty=bytes(SIZE);a=put(empty,0,'A',payloads[0]);ab=put(a,1,'B',payloads[1]);b=delete(ab,0);cb=put(b,1,'C',payloads[2])
    (output/'browser-input.json').write_text(json.dumps(dict(database='wasm-dd2-verification-0057',images=[row.hex() for row in [empty,a,ab,b,cb,image,put(image,7,'FULLNEW',payloads[0])]],payloads=[p.hex() for p in payloads])))
    html='''<!doctype html><html lang="en"><meta charset="utf-8"><title>Save store verification</title><script>var Module={noInitialRun:true,onRuntimeInitialized(){window.ready=true;}};</script><script src="/dd2_save_store_probe.js"></script></html>'''
    server_directory=output/'server';server_directory.mkdir(exist_ok=True)
    (server_directory/'index.html').write_text(html)
    for name in ['dd2_save_store_probe.js','dd2_save_store_probe.wasm']:
        (server_directory/name).symlink_to(WORK/'rewrite-wasm'/name)
    FILES.update({'/dd2_save_store_probe.js','/dd2_save_store_probe.wasm'})
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(server_directory)))
    thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    try:
        run(['node',str(ROOT/'tools/rewrite/verify_save_store_browser.js'),f'http://127.0.0.1:{server.server_port}/',str(output)],'browser',timeout=240)
    finally:
        server.shutdown();thread.join();server.server_close()
    browser=json.loads((output/'browser-report.json').read_text())
    if not browser['pass_']:raise ValueError('Browser acceptance missing')
    memcheck=run(['valgrind','--error-exitcode=99','--leak-check=full','--show-leak-kinds=all',
                  '--errors-for-leak-kinds=all','--track-fds=yes',*commands['native'],str(output/'native'),str(output/'memcheck.card'),'inspect','0','',str(inputs[0]),'0'],'memcheck')
    if 'ERROR SUMMARY: 0 errors' not in memcheck or 'no leaks are possible' not in memcheck:raise ValueError('Memcheck errors/leaks')
    if digest(save)!=before_save or digest(exe)!=EXE_SHA or {str(p.relative_to(ROOT)):digest(p) for p in files}!=sources:raise ValueError('Source/original identity changed')
    report=dict(scope=__doc__,source_sha256=sources,binary_sha256=binaries,original_save_sha256=before_save,original_pack_sha256=digest(packs),calls=calls,cases=cases,browser=browser,complete=True)
    (output/'verification-report.json').write_text(json.dumps(report,indent=2)+'\n')
    # All subprocesses, the browser and the HTTP server have completed. Remove
    # only this verifier's raw output, retaining hashes and acceptance reports.
    opened = open_files()
    removed, retained_open = [], []
    generated = [original,packs,sanitized,unit,short,*inputs,output/'browser-input.json',
                 *output.glob('*.card'),*(output/(call['label']+'.log') for call in calls)]
    for directory in [output/name for name in ('native','sanitized','server','chromium-profile')]:
        generated.extend(directory.rglob('*'))
        generated.append(directory)
    for path in sorted(set(generated),key=lambda p:len(p.parts),reverse=True):
        if path.is_symlink():
            removed.append(dict(path=str(path),symlink=str(path.readlink())))
            path.unlink()
        elif path.is_file():
            metadata=path.stat()
            if (metadata.st_dev,metadata.st_ino) in opened:
                retained_open.append(str(path))
                continue
            removed.append(dict(path=str(path),bytes=metadata.st_size,sha256=digest(path)))
            path.unlink()
        elif path.is_dir():
            try:path.rmdir()
            except OSError:pass # Keep directories containing still-open files.
    (output/'cleanup-report.json').write_text(json.dumps(dict(removed=removed,retained_open=retained_open),indent=2)+'\n')
    print(json.dumps(dict(complete=True,native_cases=len(cases),browser_cases=len(browser['cases']))))


if __name__=='__main__':main()
