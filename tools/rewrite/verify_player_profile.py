#!/usr/bin/env python3
"""Verify player/car/audio profiles and actual Native/browser save dialogs.

Independent complete images and unmodified original driver rosters cover identity,
retained source bytes and persistence. This is not full configuration restoration,
a saved championship, a completed named season or original menu parity.
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
from rewrite.quality import ROOT
from rewrite.serve import BUILD, Handler
from rewrite.verify_preferences import digest, extension, preferences, BLOCK, SIZE, PREFIX
from rewrite.verify_save_store import put, delete
from rewrite.verify_window import NativeWindow, build_sanitized
from verify_season_transition import EXE_SHA

PLAYER = 5828
CAR = 6


def named(payload, name):
    result = bytearray(payload)
    encoded = name.encode('ascii')+b'\0'
    result[PLAYER:PLAYER+len(encoded)] = encoded
    return bytes(result)


def native_dialog(output, archive, binary, label, factory):
    folder = output/label
    folder.mkdir(exist_ok=True)
    ui = NativeWindow(folder, archive, binary, arguments=[str(archive)])
    ui.env.update(SDL_AUDIODRIVER='dummy', XDG_DATA_HOME=str(folder/'user-data'))
    card = folder/'user-data/Weichwolf/wasm-dd2/SaveGames'
    rows, cases = [], []
    observation_log = folder/'native.log'

    def records():
        nonlocal rows
        lines = observation_log.read_text(errors='replace').splitlines()
        rows = [json.loads(line) for line in lines if line.startswith('{')]
        return rows[-1] if rows else {}

    def state(phase, *, draft=None, name=None, slot=None, car=None):
        def observed():
            row = records()
            if row.get('phase') != phase:
                return None
            for key, value in [('draft_hex', draft), ('name_hex', name), ('slot', slot), ('car',car)]:
                if value is None:
                    continue
                wanted = value if key in ('slot','car') else value.encode('ascii').hex()
                if row.get(key) != wanted:
                    return None
            return row
        return ui.wait(observed)

    def key(*keys):
        ui.command('key', '--delay', '1', *keys)

    def edit(name):
        old = records()['draft_hex']
        if old:
            key(*(['BackSpace']*(len(old)//2)))
        state(records()['phase'], draft='')
        if name:
            ui.command('type', '--delay', '1', '--', name)
            state(records()['phase'], draft=name)

    def player(name):
        key('F2');state(1);edit(name);key('Return');state(0, name=name or 'PLAYER')

    def image(expected, label):
        ui.wait(lambda: card.exists() and card.read_bytes() == expected)
        cases.append(dict(label=label, bytes=len(expected), sha256=digest(card)))

    try:
        ui.start();state(0, name='PLAYER')
        key('Delete');state(11);key('Return');state(11)
        if card.exists():raise ValueError('Deleting from missing saves created an image')
        key('Escape');state(0)
        cases.append(dict(label='delete-opening-missing-saves-does-not-create-card'))
        key('F8');key('F2');first=state(1, draft='PLAYER')
        edit('Racer_7!');last=state(1, draft='Racer_7!')
        if (first['race_steps'],first['music_frame']) != (last['race_steps'],last['music_frame']):
            raise ValueError(label+' modal simulation/music advanced')
        ui.image().save(folder/'name-draft.png')
        key('Escape');state(0, name='PLAYER')
        cases.append(dict(label='draft-cancel-and-modal-clocks',before=first,after=last))
        player('Racer_7!')
        key('F3');state(4, slot=0)
        if card.exists():raise ValueError('Opening missing saves created an image')
        key(*(['Right']*14));state(4,slot=14)
        key(*(['Left']*14));state(4,slot=0)
        key('Return');state(6);edit('');key('Return');state(6,draft='')
        if card.exists():raise ValueError('Empty save name created an image')
        edit('A');key('Return');state(9)
        a=put(bytes(SIZE),0,'A',named(factory,'Racer_7!'))
        image(a,'first-durable-save-all-15-slots-empty-name-rejection')
        ui.image().save(folder/'save-completed.png')
        key('Escape');state(0)
        image(a,'Escape-dismisses-completion-and-retains-durable-save')
        player('LOCAL')
        key('F4');state(5,slot=0);key('Return');state(9,name='Racer_7!')
        image(a,'F4-restores-player-from-selected-profile')
        key('Return');state(0)
        key('F3');state(4);key('Return');state(6);edit('EDIT');key('Return');state(7)
        key('Escape');state(0)
        image(a,'occupied-entry-explicit-confirmation-cancel')
        key('F3');state(4);key('Return');state(6);key('Escape');state(0)
        image(a,'filename-dialog-cancel')
        bad=bytearray(factory);bad[PLAYER:PLAYER+5]=b'BAD\n\0'
        corrupt=put(a,1,'BAD',bytes(bad));card.write_bytes(corrupt)
        key('F4');state(5);key('Right');state(5,slot=1)
        key('Return');state(9,name='Racer_7!')
        image(corrupt,'invalid-player-load-keeps-live-name-and-complete-image')
        key('Return');state(0)
        # A real external writer changes the complete file while the old owner is
        # open. F4 must explicitly reload rather than silently using a stale card.
        legacy = named(factory,'LongName_11')
        external=put(a,1,'LEGACY',legacy);card.write_bytes(external)
        key('F4');state(5,slot=0);key('Right');state(5,slot=1)
        key('Return');state(9,name='LongName_11')
        image(external,'F4-reloads-external-file-and-retains-11-byte-identity')
        key('Return');state(0)
        key('F2');state(1,draft='LongName');key('Escape');state(0,name='LongName_11')
        cases.append(dict(label='legacy-editor-cancel-keeps-complete-name'))
        key('F3');state(4);key('Return');state(6);edit('EDIT');key('Return');state(7)
        changed=put(external,2,'EXTERNAL',named(factory,'OTHER'));card.write_bytes(changed)
        key('Return');state(9,name='LongName_11')
        image(changed,'external-writer-conflict-refuses-overwrite')
        key('Return');state(0)
        key('F4');state(5);key('Escape');state(0)
        external=changed
        key('Delete');state(11,slot=0)
        key(*(['Right']*14));state(11,slot=14);key('Return');state(11,slot=14)
        image(external,'delete-empty-slot-does-not-write')
        key(*(['Left']*14));state(11,slot=0);key('Return');state(12,slot=0)
        ui.image().save(folder/'delete-confirm.png')
        key('Escape');state(0,name='LongName_11')
        image(external,'delete-needs-separate-confirmation-and-cancel-keeps-image')
        key('Delete');state(11);key('Return');state(12)
        changed=put(external,3,'WRITER',named(factory,'WRITER'));card.write_bytes(changed)
        key('Return');state(9,name='LongName_11')
        image(changed,'delete-refuses-external-writer-conflict')
        key('Return');state(0)
        key('Delete');state(11);key('Right','Right');state(11,slot=2)
        key('Return');state(12,slot=2);key('Return');state(9,name='LongName_11')
        external=delete(changed,2);image(external,'delete-reloads-and-removes-middle-physical-entry')
        ui.image().save(folder/'delete-completed.png')
        key('Escape');state(0);image(external,'dismiss-delete-completion-retains-durable-image')
        key('Delete');state(11);key('Return');state(12);key('Return');state(9)
        external=delete(external,0);image(external,'delete-first-entry-compacts-logical-selection')
        key('Return');state(0)
        key('Escape')
        if ui.process.wait(timeout=15)!=0:raise ValueError(label+' actual window did not exit cleanly')
        if 'Sanitizer:' in (folder/'native.log').read_text() or 'runtime error:' in (folder/'native.log').read_text():
            raise ValueError(label+' sanitizer findings')
    finally:ui.close()
    # New process, same actual SDL user location. No C setter restores the name.
    ui=NativeWindow(folder,archive,binary,label='restart',arguments=[str(archive)])
    ui.env.update(SDL_AUDIODRIVER='dummy',XDG_DATA_HOME=str(folder/'user-data'))
    try:
        ui.start()
        # Observe the second process through its own read-only stdout.
        observation_log=folder/'restart.log'
        state(0,name='PLAYER');key('F4');state(5,slot=0)
        key('Return');state(9,name='LongName_11')
        image(external,'fresh-native-process-restores-full-legacy-identity')
        key('Return');state(0)
        # Deletion operates on card identity, including opaque GAME/REPLAY data
        # and duplicate source names; it must never decode/load the payload.
        inventory=bytes(SIZE)
        for logical in range(15):
            payload=bytearray(factory)
            struct.pack_into('<H',payload,0,0x3030 if logical%2 else 0x2020)
            payload[-1]=logical
            inventory=put(inventory,logical,'S'+str(logical),bytes(payload))
        duplicate=bytearray(inventory);duplicate[512+4:512+7]=b'S0\0';inventory=bytes(duplicate)
        card.write_bytes(inventory)
        key('Delete');state(11);key('Right');state(11,slot=1)
        key('Return');state(12,slot=1);key('Return');state(9,name='LongName_11')
        inventory=delete(inventory,1);image(inventory,'delete-duplicate-name-selected-physical-game-payload')
        key('Return');state(0)
        key('Delete');state(11);key(*(['Right']*13));state(11,slot=13)
        key('Return');state(12,slot=13);key('Return');state(9,name='LongName_11')
        inventory=delete(inventory,13);image(inventory,'delete-last-opaque-replay-keeps-all-reserved-bytes')
        key('Return');state(0)
        single=put(bytes(SIZE),0,'ONLY',named(factory,'OTHER'));card.write_bytes(single)
        key('Delete');state(11);key('Return');state(12);key('Return');state(9,name='LongName_11')
        image(delete(single,0),'delete-only-entry-keeps-live-identity')
        key('Return');state(0);key('Delete');state(11);key('Return');state(11)
        image(delete(single,0),'empty-card-delete-refuses-without-mutation')
        key('Escape');state(0)
        key('Tab')
        for car in range(3):
            payload=bytearray(named(factory,'LongName_11'));struct.pack_into('<h',payload,CAR,car)
            inventory=put(bytes(SIZE),0,'CLASS',bytes(payload));card.write_bytes(inventory)
            key('F4');state(5);key('Return');state(9,car=car);key('Return');state(0,car=car)
            key('F1');state(0,car=(car+1)%3)
            key('F3');state(4);key('Return');state(6);edit('CLASS');key('Return');state(7);key('Return');state(9)
            struct.pack_into('<h',payload,CAR,(car+1)%3);inventory=put(inventory,0,'CLASS',bytes(payload))
            image(inventory,'F1-F3-saves-class-'+str((car+1)%3))
            key('Return');state(0);key('F1');state(0,car=(car+2)%3)
            previous=ui.image().crop((0,336,640,480)).tobytes()
            key('F4');state(5);key('Return');state(9,car=(car+1)%3)
            image(inventory,'F4-restores-class-'+str((car+1)%3))
            def visible_modal():
                capture=ui.image()
                if any(pixel==(255,255,255) for pixel in capture.crop((40,150,150,185)).getdata()):
                    return capture
                return None
            modal=ui.wait(visible_modal);modal.save(folder/('class-profile-'+str((car+1)%3)+'.png'))
            pixels=modal.crop((0,336,640,480)).tobytes();key('Return');state(0)
            if pixels==previous:
                raise ValueError(label+' restored modal class paint/ratings are stale')
            ui.wait(lambda: pixels==ui.image().crop((0,336,640,480)).tobytes())
            cases.append(dict(label='restored-modal-class-pixels-'+str((car+1)%3),
                              rgb_sha256=hashlib.sha256(pixels).hexdigest(),
                              matches_closed_world=True,differs_from_previous_class=True))
        key('Escape')
        if ui.process.wait(timeout=15)!=0:raise ValueError(label+' restarted window did not exit')
    finally:ui.close()
    return dict(target=label,real_x11_keys=True,cases=cases,pass_=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=WORK/'rewrite-player-profile-verification')
    args=parser.parse_args();output=prepare_output(args.output)
    if WORK not in output.parents:parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=True);check_space(output)
    files=[p for p in (ROOT/'src').rglob('*') if p.suffix in ('.c','.h','.js','.html')]
    files += [ROOT/p for p in ('CMakeLists.txt','Makefile','tests/profile_application_export.c',
        'tests/profile_window_export.c','tests/configuration_test.c','tests/profile_menu_test.c','tests/profile_draw_test.c',
        'tests/window_input_test.c','tools/reference/player_identity_fixture.c',
        'tools/reference/configuration_defaults_fixture.c','tools/reference/save_profile_fixture.c',
        'tools/reference/pe_fixture.h','tools/rewrite/verify_window.py','tools/rewrite/verify_preferences.py',
        'tools/rewrite/verify_player_profile_browser.js','tools/rewrite/verify_save_store.py')]
    files.append(Path(__file__).resolve())
    sources={str(p.relative_to(ROOT)):digest(p) for p in files}
    calls,cases,binaries=[],[],{}
    def run(command,label,env=None,timeout=180):
        log=output/(label+'.log')
        with log.open('wb') as stream:
            result=run_bounded(command,directory=output,cwd=ROOT,timeout=timeout,env=env,stdout=stream,stderr=subprocess.STDOUT)
        text=log.read_text(errors='replace');calls.append(dict(label=label,command=command,returncode=result.returncode,sha256=digest(log)))
        if result.returncode or 'Sanitizer:' in text or 'runtime error:' in text:raise RuntimeError(label+' failed: '+text[-6000:])
        return text
    exe,save,archive=[ROOT/'DestructionDerby2'/name for name in ('dd2h.exe','SaveGames','Dirinfo')]
    original_save=digest(save)
    if digest(exe)!=EXE_SHA:raise ValueError('Supported original required')
    for stem in ('configuration_defaults','save_profile','player_identity'):
        binary=output/(stem+'-original')
        run(['gcc','-m32','-no-pie','-O0','-std=gnu99','-w',str(ROOT/f'tools/reference/{stem}_fixture.c'),'-o',str(binary)],stem+'-build')
        run([str(binary),str(exe),str(output/(stem+'.bin'))],stem+'-run')
    original=(output/'configuration_defaults.bin').read_bytes()
    factory=preferences(original,256,256)
    if len(factory)!=BLOCK:raise ValueError('Original pack extent')
    original_names=(output/'player_identity.bin').read_bytes()
    if len(original_names)!=960:raise ValueError('Original roster extent')
    rosters={name:original_names[i*320:(i+1)*320] for i,name in enumerate(('PLAYER','Racer_7!','LongName_11'))}
    def roster(name):return name.encode('ascii').ljust(16,b'\0')+rosters['PLAYER'][16:]
    for name,value in rosters.items():
        if roster(name)!=value:raise ValueError('Original player/NPC roster differs')
        cases.append(dict(label='original-roster-'+name,bytes=len(value),sha256=hashlib.sha256(value).hexdigest()))
    sanitized_dir=output/'sanitized-build';sanitized_dir.mkdir(exist_ok=True)
    sanitized=build_sanitized(sanitized_dir,ROOT/'tests/profile_application_export.c',('-Wl,--wrap=calloc',))
    window_dir=output/'window-sanitized-build';window_dir.mkdir(exist_ok=True)
    window_sanitized=build_sanitized(window_dir,ROOT/'tests/profile_window_export.c')
    commands={'native':WORK/'rewrite-native/dd2_profile_application_export','sanitized':sanitized}
    for path in [*commands.values(),window_sanitized,WORK/'rewrite-native/dd2_profile_window_export',
                 BUILD/'dd2_app.js',BUILD/'dd2_app.wasm',BUILD/'viewer.js',BUILD/'index.html',*BUILD.glob('dd2_app.worker.js')]:
        binaries[str(path)]=digest(path)
    environment=os.environ.copy();environment.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
    a=named(preferences(original,128,64),'Racer_7!')
    b=named(preferences(a,32,16),'PLAYER')
    ab=put(put(bytes(SIZE),0,'A',a),1,'B',b)
    # Normalize only consumed effects, car and player fields in the original
    # patterned payload; all dormant fields and reserved bytes remain exact.
    patterned=bytearray((output/'save_profile.bin').read_bytes()[:BLOCK]);struct.pack_into('<H',patterned,16,3681);struct.pack_into('<h',patterned,CAR,2)
    patterned=named(bytes(patterned),'LongName_11')
    variants=[('eight-ascii',a,True,'Racer_7!',128,64),('legacy-eleven',named(factory,'LongName_11'),True,'LongName_11',256,256),
              ('empty-name',factory,True,'PLAYER',256,256),('patterned-legacy',patterned,True,'LongName_11',230,16)]
    startup=bytearray(a);struct.pack_into('<H',startup,0,0x1020);variants.append(('startup',bytes(startup),True,'Racer_7!',128,64))
    for label,field in [('control-name',b'BAD\n\0'),('utf8-name',b'\xc3\xa4\0'),('unterminated-name',b'X'*12)]:
        payload=bytearray(a);payload[PLAYER:PLAYER+len(field)]=field;variants.append((label,bytes(payload),False,'LOCAL',32,16))
    for label,kind in [('game',0x3030),('replay',0x2020)]:
        payload=bytearray(a);struct.pack_into('<H',payload,0,kind);variants.append((label,bytes(payload),False,'LOCAL',32,16))
    payload=bytearray(a);payload[PREFIX+15]^=1;variants.append(('bad-music-extension',bytes(payload),False,'LOCAL',32,16))
    for invalid_car in (-32768,-1,3,32767):
        payload=bytearray(a);struct.pack_into('<h',payload,CAR,invalid_car)
        variants.append(('invalid-car-'+str(invalid_car),bytes(payload),False,'LOCAL',32,16))
    car_image=bytes(SIZE)
    for car in range(3):
        payload=bytearray(named(preferences(original,128,64),'C'+str(car)))
        struct.pack_into('<h',payload,CAR,car);car_image=put(car_image,car,'C'+str(car),bytes(payload))
    for target,binary in commands.items():
        directory=output/target;directory.mkdir()
        def action(label,operation,logical,image,expected,loaded,name,effects,music,folder=None,car=0):
            folder=folder or directory/label;folder.mkdir(exist_ok=True)
            if image is not None:(folder/'SaveGames').write_bytes(image)
            snapshot=folder/'accepted.card';names=folder/'roster.bin'
            row=json.loads(run([str(binary),str(archive),str(folder),str(snapshot),str(names),operation,str(logical)],target+'-'+label,env=environment))
            if row!=dict(loaded=int(loaded),effects=effects,music=music,car=car) or names.read_bytes()!=roster(name):raise ValueError(target+' '+label+' live identity/car/audio/roster differs')
            if snapshot.read_bytes()!=expected or (folder/'SaveGames').read_bytes()!=expected:raise ValueError(target+' '+label+' complete accepted/disk bytes differ')
            cases.append(dict(target=target,label=label,live=row,name=name,bytes=len(expected),sha256=digest(snapshot),roster_sha256=digest(names)))
            return folder
        folder=action('self-test','self-test',0,None,ab,True,'Racer_7!',128,64)
        action('restart','load',1,None,ab,True,'PLAYER',32,16,folder)
        cars=action('car-test','car-test',0,None,car_image,True,'C0',128,64)
        for car in range(3):
            action('car-restart-'+str(car),'load',car,None,car_image,True,'C'+str(car),128,64,cars,car)
        for label,payload,valid,name,effects,music in variants:
            image=put(bytes(SIZE),0,'SOURCE',payload);expected=image
            if valid:
                encoded=bytearray(named(payload,name));struct.pack_into('<H',encoded,0,0x1010);encoded[PREFIX:PREFIX+16]=extension(music)
                expected=put(image,0,'EDIT',bytes(encoded))
            action(label,'load-save',0,image,expected,valid,name,effects,music,car=struct.unpack_from('<h',payload,CAR)[0] if valid else 0)
        duplicate=bytearray(put(put(bytes(SIZE),0,'FIRST',a),1,'SECOND',b));duplicate[512+4:512+6]=b'F\0'
        # Duplicate source names still resolve the selected physical payload.
        duplicate[4:6]=b'F\0';duplicate=bytes(duplicate)
        action('duplicate-physical','load',1,duplicate,duplicate,True,'PLAYER',32,16)
    dialogs=[native_dialog(output,archive,WORK/'rewrite-native/dd2_profile_window_export','native-dialog',factory),
             native_dialog(output,archive,window_sanitized,'sanitized-dialog',factory)]
    browser_input=dict(factory=factory.hex(),a=a.hex(),b=b.hex(),ab=ab.hex(),empty=bytes(SIZE).hex(),legacy=named(factory,'LongName_11').hex(),patterned=patterned.hex())
    (output/'browser-input.json').write_text(json.dumps(browser_input))
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(BUILD)))
    thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    try:
        run(['node',str(ROOT/'tools/rewrite/verify_player_profile_browser.js'),f'http://127.0.0.1:{server.server_port}/',str(archive),str(output)],'browser',timeout=300)
    finally:server.shutdown();thread.join();server.server_close()
    browser=json.loads((output/'browser-report.json').read_text())
    if not browser['complete']:raise ValueError('Browser acceptance missing')
    memcheck=['valgrind','--error-exitcode=99','--leak-check=full','--show-leak-kinds=all','--errors-for-leak-kinds=definite,indirect','--track-fds=yes',
              str(commands['native']),str(archive),str(output/'native/self-test'),str(output/'memcheck.card'),str(output/'memcheck.roster'),'load','0']
    checked=run(['script','--quiet','--return','--command',shlex.join(memcheck),'/dev/null'],'memcheck',env=environment)
    if 'ERROR SUMMARY: 0 errors' not in checked:raise ValueError('Memcheck findings')
    if digest(save)!=original_save or digest(exe)!=EXE_SHA or {str(p.relative_to(ROOT)):digest(p) for p in files}!=sources:raise ValueError('Source/original changed')
    report=dict(scope=__doc__,source_sha256=sources,binary_sha256=binaries,original_exe_sha256=EXE_SHA,original_save_sha256=original_save,
                calls=calls,cases=cases,dialogs=dialogs,browser=browser,memcheck_zero_errors=True,
                memcheck_scope='Actual Native application, SDL dummy devices, controlling terminal, descriptor checks, no suppressions. No lost blocks; DBus retains reachable process globals.',complete=True)
    (output/'verification-report.json').write_text(json.dumps(report,indent=2)+'\n')
    opened=open_files();removed=[];retained=[]
    keep={output/'verification-report.json',output/'browser-report.json',output/'player-profile.png'}
    keep.update(output.rglob('*delete*.png'))
    keep.update(output.rglob('class-profile-*.png'))
    keep.update(output/path/'sanitizer-build.json' for path in ('sanitized-build','window-sanitized-build'))
    keep.update(output/path/name for path in ('native-dialog','sanitized-dialog') for name in ('name-draft.png','save-completed.png'))
    for path in sorted(output.rglob('*'),key=lambda p:len(p.parts),reverse=True):
        if path in keep:continue
        if path.is_symlink():path.unlink()
        elif path.is_file():
            stat=path.stat()
            if (stat.st_dev,stat.st_ino) in opened:retained.append(str(path));continue
            removed.append(dict(path=str(path),bytes=stat.st_size,sha256=digest(path)));path.unlink()
        elif path.is_dir():
            try:path.rmdir()
            except OSError:pass
    (output/'cleanup-report.json').write_text(json.dumps(dict(removed=removed,retained_open=retained),indent=2)+'\n')
    print(json.dumps(dict(complete=True,original_native_sanitized_cases=len(cases),native_dialog_cases=sum(len(row['cases']) for row in dialogs),browser_cases=len(browser['cases']))))


if __name__=='__main__':main()
