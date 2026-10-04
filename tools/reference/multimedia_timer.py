"""Observe original startup timer lifetimes without debugger or game-state writes.

Wine relay identifies the two original call sites, returned IDs and nesting
inside ShowWindow. This is registration evidence, not a recording of every
callback or proof of whole-game audio/video timing.
"""
from decimal import Decimal
import hashlib
import json
from pathlib import Path
import re

from game_clock import EXE, verified_sites

API = re.compile(r'([^:]+):([0-9a-f]+):(Call |Ret  )(winmm|user32)\.(\w+)\((.*?)\)(?: retval=([0-9a-f]+))? ret=([0-9a-f]+)$', re.I)
TIMER_SITES = {0x4133d8, 0x412a9d}


def observe(trace, executable):
    verified_sites(executable)
    pending = {}; records = []; digest = hashlib.sha256()
    with Path(trace).open('rb') as lines:
        for number, raw in enumerate(lines, 1):
            digest.update(raw)
            line = raw.decode(errors='replace').rstrip('\r\n')
            match = API.fullmatch(line)
            if not match:
                if ':Call winmm.timeSetEvent(' in line or ':Ret  winmm.timeSetEvent(' in line:
                    raise ValueError('Malformed timer relay record')
                continue
            caller = int(match[8], 16)
            if not 0x410000 <= caller < 0x460000:
                continue
            module, function, thread = match[4].lower(), match[5], match[2].lower()
            timestamp = int(Decimal(match[1])*1000)
            stack = pending.setdefault(thread, [])
            if match[3] == 'Call ':
                if function == 'timeSetEvent' and caller not in TIMER_SITES:
                    raise ValueError('Unknown original timer call site')
                record = dict(module=module, function=function, caller=caller, thread=thread,
                              arguments=match[6], entry_ms=timestamp, entry_line=number,
                              parents=[entry['function'] for entry in stack])
                stack.append(record)
            else:
                if not stack:
                    raise ValueError('Return without original API entry')
                entry = stack.pop()
                if (entry['module'],entry['function'],entry['caller']) != (module,function,caller) or timestamp < entry['entry_ms']:
                    raise ValueError('Mismatched original API return')
                if match[7] is None:
                    raise ValueError('Missing API return value')
                entry.update(return_ms=timestamp, return_line=number, result=int(match[7],16))
                records.append(entry)
    if any(pending.values()):
        raise ValueError('Unreturned original platform API call')
    records.sort(key=lambda entry:entry['entry_line'])
    timers = [entry for entry in records if entry['function']=='timeSetEvent']
    if len(timers) != 2 or {entry['caller'] for entry in timers} != TIMER_SITES:
        raise ValueError('Both original startup timer registrations required')
    for timer in timers:
        if timer['arguments'].lower() != '00000190,0000000a,0041345c,00000000,00000001' or not timer['result']:
            raise ValueError('Original timer arguments or successful ID differ')
    if timers[0]['caller'] != 0x4133d8 or 'ShowWindow' not in timers[0]['parents'] or timers[1]['parents']:
        raise ValueError('Original activation/initialization timer order differs')
    if timers[0]['thread'] != timers[1]['thread'] or timers[0]['result'] == timers[1]['result']:
        raise ValueError('Original timer registrations are not independent')
    gap = timers[1]['entry_ms']-timers[0]['entry_ms']
    if not 0 <= gap < 5000:
        raise ValueError('Unbounded original initialization interval')
    return dict(scope=__doc__.strip(),pass_=True,original_exe_sha256=EXE,
                trace_sha256=digest.hexdigest(),timers=timers,registration_gap_ms=gap,
                platform_calls=records,callbacks='not completely observed')


if __name__ == '__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--trace',type=Path,required=True)
    parser.add_argument('--exe',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    from sys import path
    path.insert(0,str(Path(__file__).resolve().parents[1]))
    from artifacts import temporary_output
    temporary_output(args.report)
    report=observe(args.trace,args.exe)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print('Original independent timers:',[timer['result'] for timer in report['timers']],
          'registration gap:',report['registration_gap_ms'],'ms')
