#!/usr/bin/env python3
"""Observe an actual record update with a declared controlled save-file input.

Each target drives independently through real keyboard input. Selected name-menu
cycles may be compared with original; chronological racing A/V is not accepted.
One WORD in an authentic original-generated configuration is changed on disk:
the first saved best lap's minutes become two. This is a controlled file-input
case, not a best lap against the provisioned default record. No live engine
fields or machine code are changed. An actual completed lap must improve the
loaded time before the original's real Fastest Lap name dialog and saving run.
"""
import argparse
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from artifacts import check_space, run_bounded
from verify_championship_save import setup, execute, snapshot, pack
from verify_configuration_persistence import ROOT, EXE_SHA256, digest, require
from driver_name_input import name_actions
from verify_statistics_ui import fixture as statistics_fixture
from verify_configuration_card_ui import rendered_cycle


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--target',choices=['original','native'],default='native')
    parser.add_argument('--binary',type=Path,required=True)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--track', type=int, choices=[0], default=0)
    args = parser.parse_args()
    args.timeout=1100
    args.boot_label='Stock Car'
    source_fixture=args.fixture.resolve()
    source, source_state=statistics_fixture(source_fixture)
    initial=bytearray(source)
    edit_offset=0x4000+5948+args.track*80+10
    previous=bytes(initial[edit_offset:edit_offset+2])
    struct.pack_into('<H',initial,edit_offset,2)
    initial=bytes(initial)
    out, game = setup(args, initial)
    (out/'initial.card').write_bytes(initial)
    policy = out/'observer-source'
    policy.mkdir()
    for source_file in [ROOT/'tools/live_lap_record_driver.py', Path(__file__),
                   Path(__file__).with_name('live_lap_record_gdb.py')]:
        shutil.copy2(source_file, policy/source_file.name)
    report = dict(scope=__doc__, pass_=False, operation='generate', target=args.target,
                  binary_sha256=EXE_SHA256 if args.target=='original' else digest(args.binary.read_bytes()), engine_state_writes=False,
                  initial_card_sha256=digest(initial), input_keys=[],
                  initial_card_kind='controlled-file-input',
                  original_source_fixture=str(source_fixture),original_source_card_sha256=digest(source),
                  initial_file_edits=[dict(offset=edit_offset,field='fastest[track][0].minutes',
                      track=args.track,before=previous.hex(),after=initial[edit_offset:edit_offset+2].hex())],
                  observer_sources={p.name:digest(p.read_bytes()) for p in policy.iterdir()})

    def action(ui):
        def keys(codes):
            for code in codes:
                ui.key(code)
                report['input_keys'].append(code)

        keys(['Return', 'Return', 'Right', 'Right', 'Return'])
        require(ui.integer(0x4673f4) == 1 and ui.integer(0x46765c) == 1,
                'Actual single-car Time Trials selection required')
        # Use actual Amateur vehicle selection, independent of the loaded car.
        keys(['Right', 'Return'])
        for _ in range(3):
            if ui.integer(0x467400)==1:break
            keys(['Right'])
        keys(['Return'])
        require(ui.integer(0x467400) == 1, 'Actual Amateur vehicle required')
        if args.track:
            keys(['Right','Return','Right','Return'])
            require(ui.integer(0x4673fc) == args.track, 'Actual second road track selection required')
        report['before'] = snapshot(ui)
        keys((['Left'] if args.track else [])+['Down', 'Down', 'Return'])
        ui.wait(lambda: ui.integer(0x936ff4) == args.track+1 and ui.integer(0x7746c0) >= 8
                and ui.integer(0x7746ac) == 0, 60)
        script = out/'drive.gdb'
        script.write_text('\n'.join(['set pagination off','set confirm off','set auto-solib-add off',
            f'attach {ui.pid if args.target=="original" else ui.process.pid}', 'python', 'import sys',
            f'sys.path.insert(0,{str(ROOT/"tools")!r})', f'sys.path.insert(0,{str(policy)!r})',
            'from live_lap_record_gdb import drive', f'drive({str(out)!r},entry={0x420c9c if args.target=="original" else ui.table["Draw_All"]},target={args.target!r})',
            'end','detach','quit'])+'\n')
        with (out/'drive.log').open('wb') as log:
            run_bounded(['gdb','--nx','-q','-batch','-x',str(script)], directory=out,
                        env=ui.env, stdout=log, stderr=subprocess.STDOUT, timeout=900, check=True)
        report['drive'] = json.loads((out/'driving-report.json').read_text())
        require(report['drive']['pass_'], 'Actual better full lap required')
        keys(['Escape','Down','Down','Down','Return','Up','Return'])
        ui.wait(lambda: ui.integer(0x940010) == 0x469f70)
        require('Fastest Lap' in ui.text(0x46a024), 'Actual new lap record name dialog required')
        report['name_dialog'] = dict(menu=ui.integer(0x940010), title=ui.text(0x46a024),
                                   records=ui.read(0x4680c0,560).hex())
        report['name_dialog']['empty_cycle']=rendered_cycle(ui,dict(checkpoint='record-name-empty'),args.target=='original')
        actions=name_actions('D')
        for a in actions[:-1]:
            keys([a['key']])
            pointer=ui.integer(0x469fd4)
            require(list(struct.unpack('<hh',ui.read(0x469f34,4)))==a['cursor'] and
                    ui.read(pointer+8,10).split(b'\0')[0].decode('ascii')==a['entered'],
                    'Actual fastest-lap cursor/name differs')
        report['name_dialog']['typed_cycle']=rendered_cycle(ui,dict(checkpoint='record-name-typed'),args.target=='original')
        keys(['Return'])
        ui.wait(lambda: ui.integer(0x940010) == 0x4696b0 and ui.integer(0x936ff4) == 0)
        report['after'] = snapshot(ui)
        before, after = bytes.fromhex(report['before']['fastest']), bytes.fromhex(report['after']['fastest'])
        base = args.track*80
        require(after[base:base+2] == b'D\0' and after[base+16:base+80] == before[base:base+64] and
                after[:base] == before[:base] and after[base+80:] == before[base+80:],
                'Actual name/bank history update differs')
        expected_time = report['drive']['final']['runtime']
        require(list(struct.unpack_from('<HHH',after,base+10)) == expected_time,
                'Accepted record differs from actual completed lap')
        # Return from Go to Options, select actual Save Configuration.
        keys(['Up'])
        require('Configuration' in ui.text(0x46975c), 'Actual configuration action required')
        keys(['Return','Right','Right','Return'])
        ui.wait(lambda: ui.integer(0x93a318) == 3 and ui.integer(0x940010) == 0x4671ec)
        report['saved_state'] = snapshot(ui)
        expected = bytearray(pack(report['saved_state']))
        struct.pack_into('<H',expected,0,0x1010)
        require(ui.read(0x93a490,len(expected)) == expected, 'Actual complete configuration pack differs')
        keys(['Return','Right','Return','Left','Return'])
        ui.enter_name('B',replace=True)
        report['input_keys'] += ['Down','Down','Return','Up','Up','Right','Return','Down','Down','Right','Return']
        ui.wait(lambda: (ui.game/'SaveGames').read_bytes()[0x4000:0x597e] == expected)
        raw = (ui.game/'SaveGames').read_bytes()
        require(raw == ui.read(0x754460,len(raw)) and raw[0x4000:0x597e] == expected,
                'Actual saved record payload/card differs')
        require(raw[:0x200]==initial[:0x200] and raw[0x2000:0x4000]==initial[0x2000:0x4000],
                'Original championship slot changed during isolated configuration save')
        (out/'saved.card').write_bytes(raw)
        report.update(saved=True, card_sha256=digest(raw), payload_sha256=digest(expected),engine_matches_file=True)
    try:
        execute(args,out,game,action)
        # Reload in a fresh process, from the actual saved card, with normal boot.
        if args.target=='original':
            # Wine runs from an isolated copy; provision the actual saved output
            # as the new process's disk input, without editing its bytes.
            (game/'SaveGames').write_bytes((out/'saved.card').read_bytes())
        restart = out/'restart'
        restart.mkdir()
        args.boot_label = 'Wrecking'
        def restored(ui):
            state=snapshot(ui)
            require(state==report['saved_state'], 'Fresh process restored configuration differs')
            raw=(out/'saved.card').read_bytes()
            require((ui.game/'SaveGames').read_bytes()==raw and ui.read(0x754460,len(raw))==raw,
                    'Restart changed actual file/card RAM')
            report.update(reloaded_state=state,configuration_restored=True,pass_=True)
        execute(args,restart,game,restored)
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        check_space(out)


if __name__ == '__main__':
    main()
