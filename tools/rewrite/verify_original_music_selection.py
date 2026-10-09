#!/usr/bin/env python3
"""Recover original CD selection and transport, without live-menu or PCM claims.

Unmodified original routines run with a bounded imported-MCI observer. Protected
machine code cannot be written. Original caller disassembly supplies context IDs;
an independent PE data read checks the eleven-entry menu-to-level permutation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT
from verify_season_transition import EXE_SHA


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def level_map(executable):
    data = executable.read_bytes()
    header = struct.unpack_from('<I', data, 0x3c)[0]
    optional = header + 24
    sections = optional + struct.unpack_from('<H', data, header + 20)[0]
    for index in range(struct.unpack_from('<H', data, header + 6)[0]):
        _, address, raw_size, offset = struct.unpack_from('<4I', data, sections + index*40 + 8)
        flags = struct.unpack_from('<I', data, sections + index*40 + 36)[0]
        # Watcom leaves VirtualSize zero; read initialized file bytes only.
        if (not flags & 0x80 and address <= 0x67424 and
                0x67424 + 44 <= address + raw_size and offset + raw_size <= len(data)):
            return list(struct.unpack_from('<11I', data, offset + 0x67424-address))
    raise ValueError('Original menu level table is outside its data section')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK/'rewrite-original-music-selection')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    executable = ROOT/'DestructionDerby2/dd2h.exe'
    if digest(executable) != EXE_SHA:
        raise ValueError('Provision the supported unmodified original executable')
    files = [ROOT/'tools/reference/music_selection_fixture.c',
             ROOT/'tools/reference/pe_fixture.h', Path(__file__).resolve()]
    sources = {str(path.relative_to(ROOT)):digest(path) for path in files}
    calls = []

    def run(command, label):
        log = output/(label+'.log')
        with log.open('wb') as stream:
            result = run_bounded(command, directory=output, timeout=30, cwd=ROOT,
                                 stdout=stream, stderr=subprocess.STDOUT)
        calls.append(dict(label=label, command=command, exit_code=result.returncode,
                          log_sha256=digest(log)))
        if result.returncode:
            raise RuntimeError(label+' failed: '+log.read_text(errors='replace')[-3000:])
        return log.read_text()

    binary = output/'original-selector'
    run(['gcc', '-m32', '-no-pie', '-O0', '-std=gnu99', '-Wall', '-Wextra', '-Werror',
         '-Wno-unused-function', str(files[0]), '-o', str(binary)], 'build')
    rows = [json.loads(line) for line in run([str(binary), str(executable)], 'transport').splitlines()]
    if len(rows) != 49:
        raise ValueError('Incomplete original transport/menu-map inventory')
    for row in rows[:36]:
        if (row['case'] != 'select' or row['from'] != row['source_track']+1 or
                row['to'] != row['source_track']+2 or row['repeat'] != row['repeat_input'] or
                row['playing'] != 0 or row['commands'] != [0x830]):
            raise ValueError('Original selection contract differs: '+str(row))
    expected = {'start':(1,[0x806]), 'pause':(0,[0x808]), 'resume':(1,[0x806]),
                'loop-playing':(1,[0x814]), 'loop-ended':(1,[0x814,0x806]),
                'loop-paused':(0,[]), 'loop-no-repeat':(1,[]), 'failed-start':(0,[0x806]),
                'disabled':(0,[]), 'countdown-wait':(0,[]), 'countdown-held':(0,[]),
                'countdown-go':(1,[0x806])}
    if {row['case'] for row in rows[36:-1]} != set(expected):
        raise ValueError('Incomplete original transport transitions')
    for row in rows[36:-1]:
        if (row['playing'],row['commands']) != expected[row['case']]:
            raise ValueError('Original transport transition differs: '+str(row))
    mapping = level_map(executable)
    if (rows[-1] != dict(case='menu-level-map', levels=mapping) or
            sorted(mapping) != list(range(1,12))):
        raise ValueError('Original menu-to-level permutation differs')
    contexts = []
    for name,address,source in [('Front_End',0x4502a8,12), ('Play_Game',0x423b50,None),
                                ('Practice_Over',0x452c40,12), ('Race_Over',0x454d28,13),
                                ('Display_Season_Status',0x455518,13), ('End_Of_Season',0x453f08,14)]:
        lines = run(['objdump', '-d', '-Mintel', '--start-address='+hex(address),
                     '--stop-address='+hex(address+0x400), str(executable)], name).splitlines()
        index = next(i for i,line in enumerate(lines) if 'call' in line and '0x41617c' in line)
        snippet = lines[max(0,index-5):index+1]
        if 'push   0x1' not in '\n'.join(snippet):
            raise ValueError('Original context does not repeat')
        if source is not None and f'push   {hex(source)}' not in lines[index-1]:
            raise ValueError('Original context selects a different source track')
        if source is None and ('DWORD PTR ds:0x936ff4' not in lines[index-2] or
                               'push   edi' not in lines[index-1]):
            raise ValueError('Original race does not select its loaded level')
        contexts.append(dict(name=name, source_track=source,
                             physical_track=None if source is None else source+1,
                             repeat=True, instructions=snippet))
    if sources != {str(path.relative_to(ROOT)):digest(path) for path in files} or digest(executable) != EXE_SHA:
        raise ValueError('Source/original identity changed during verification')
    report = dict(pass_=True, scope=__doc__.strip(), original_sha256=EXE_SHA,
                  source_sha256=sources, binary_sha256=digest(binary), calls=calls,
                  checks=48, cases=rows, caller_contexts=contexts,
                  menu_to_asset_level=mapping,
                  menu_to_physical_race_track=[level+1 for level in mapping])
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    binary.unlink()
    for path in output.glob('*.log'):
        path.unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True,checks=48,menu_levels=len(mapping),report=str(output/'report.json'))))


if __name__ == '__main__':
    main()
