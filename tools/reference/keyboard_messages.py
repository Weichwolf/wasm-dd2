#!/usr/bin/env python3
"""Observe original handled keyboard messages without debugger stops.

Wine +msg records the unchanged engine's window procedure entry and return.
Every key edge retains its actual presentation/clock position, VK, message,
lParam and trace timestamp. Replay input covers six menu keys; no release
duration is inferred from sound. Timestamp precision and Wine instrumentation
do not establish physical OS timing, chronological video or full-game parity.
"""
import argparse
from decimal import Decimal
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'tools'))
from artifacts import WORK, check_space
from reference.game_clock import EXE, RETURN, SITES
from verify_original_race_audio import validate_capture

MESSAGE = re.compile(rb'([^:]+):([0-9a-f]+):(Call|Ret) +window proc 004132F0 '
    rb'\(hwnd=([0-9a-f]+),msg=(WM_KEYDOWN|WM_KEYUP|WM_SYSKEYDOWN|WM_SYSKEYUP),'
    rb'wp=([0-9a-f]+),lp=([0-9a-f]+)\)(?: retval=([0-9a-f]+))?$', re.I)
KEYS = {39: 'Right', 13: 'Return'}


def observe(capture, fixture):
    checkpoint = validate_capture(capture, fixture)
    clock = json.loads((capture/'game-clock/report.json').read_text())
    if not clock['pass_'] or clock['original_exe_sha256'] != EXE or clock['debugger']:
        raise ValueError('Verified unchanged original non-debugger clock required')
    pending = {}; edges = []; flips = clocks = 0; sha = hashlib.sha256()
    with (capture/'wine.log').open('rb') as lines:
        for number, raw in enumerate(lines, 1):
            sha.update(raw)
            if b'ddraw_surface1_Flip iface' in raw: flips += 1
            if b'.GetTickCount()' in raw:
                returned = RETURN.fullmatch(raw.decode(errors='replace').rstrip())
                if returned and int(returned[5], 16) in SITES: clocks += 1
            if b'window proc 004132F0' not in raw or b'msg=WM_' not in raw:
                continue
            m = MESSAGE.fullmatch(raw.rstrip())
            if not m:
                if any(b'msg='+name in raw for name in (b'WM_KEY', b'WM_SYSKEY')):
                    raise ValueError('Malformed original keyboard handler record')
                continue
            thread = m[2].decode().lower(); entry = m[3].lower() == b'call'
            row = dict(trace_line=number,time_ns=int(Decimal(m[1].decode())*1000000000),
                       thread=thread,hwnd=int(m[4],16),message=m[5].decode().upper(),
                       vk=int(m[6],16),lparam=int(m[7],16),flip=flips,clock_calls=clocks)
            if entry:
                if thread in pending or m[8] is not None:
                    raise ValueError('Nested/invalid original keyboard handler entry')
                pending[thread] = row
            else:
                called = pending.pop(thread, None)
                if (called is None or m[8] is None or int(m[8],16) != 0 or
                    any(called[k] != row[k] for k in ('thread','hwnd','message','vk','lparam','flip','clock_calls')) or
                    row['time_ns'] < called['time_ns']):
                    raise ValueError('Original keyboard handler entry/return differs')
                called.update(return_trace_line=number,return_time_ns=row['time_ns'],return_value=0)
                edges.append(called)
    if pending or sha.hexdigest() != clock['trace_sha256'] or clocks != clock['calls'] or flips != clock['completed_flips']:
        raise ValueError('Keyboard/clock trace is incomplete or has different provenance')
    selected = [e for e in edges if e['vk'] in KEYS]
    if len(selected) != 12:
        raise ValueError('Six genuine original replay key pairs required')
    if any(e['vk'] not in KEYS and e['trace_line'] > selected[0]['trace_line'] for e in edges):
        raise ValueError('Unexpected extra original replay keyboard input')
    schedule = []
    for i, (down, up) in enumerate(zip(selected[::2],selected[1::2])):
        if (down['vk'] != up['vk'] or down['message'] != 'WM_KEYDOWN' or up['message'] != 'WM_KEYUP' or
            down['lparam'] >> 31 or not up['lparam'] >> 31 or
            down['thread'] != clock['engine_thread'] or down['thread'] != up['thread'] or
            down['hwnd'] != up['hwnd'] or down['clock_calls'] or up['clock_calls'] or
            not down['flip'] < up['flip'] or up['time_ns'] < down['time_ns']):
            raise ValueError('Original replay key edge order/position differs')
        if schedule and down['flip'] <= schedule[-1]['release_flip']-1:
            raise ValueError('Original menu inputs overlap')
        schedule.append(dict(key=KEYS[down['vk']],down_flip=down['flip'],flip=down['flip']+1,
            clock_calls=down['clock_calls'],release_flip=up['flip']+1,release_clock_calls=up['clock_calls'],
            release_scope='original window-procedure entry/return',
            original_keydown_line=down['trace_line'],original_keyup_line=up['trace_line']))
    if [p['key'] for p in schedule] != checkpoint['input_keys'] or len({e['hwnd'] for e in selected}) != 1:
        raise ValueError('Observed original keyboard route differs')
    return dict(scope=__doc__,pass_=True,original_exe_sha256=EXE,trace_sha256=sha.hexdigest(),
        initial_save_sha256=checkpoint['initial_save_sha256'],game_clock_sha256=clock['ticks_sha256'],
        debugger=False,engine_state_writes=False,handler_address=0x4132f0,
        total_handler_key_edges=len(edges),paired_keyboard_records=24,edges=selected,schedule=schedule)


def load(capture, fixture, file):
    recorded = json.loads(file.read_text())
    if recorded != observe(capture, fixture):
        raise ValueError('Keyboard input differs from independently observed original messages')
    return recorded


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('capture','fixture','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    args = parser.parse_args(); out = args.output.resolve()
    if WORK not in out.parents or out.exists():
        parser.error('Fresh keyboard output must be under /tmp/wasm-dd2/')
    report = observe(args.capture,args.fixture)
    out.parent.mkdir(parents=True,exist_ok=True);check_space(args.capture)
    out.write_text(json.dumps(report,indent=2)+'\n')
    print('Actual original keyboard entry/return pairs exported:',report['paired_keyboard_records'])
