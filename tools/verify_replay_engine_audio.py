#!/usr/bin/env python3
"""Compare actual replay-engine PCM with a bounded original capture.

The actual original-produced card is loaded through genuine X11/Playwright
keys. Ports execute their own controls and timer bodies; observed sound APIs,
positions, gains and statuses remain assertions. Key-down positions follow
original sound triggers. The first Enter release is an explicit diagnostic
hypothesis, not a recorded original OS event. Original OS input identity,
physical timing, chronological video and other scenarios remain unproved.
"""
import argparse
import copy
import json
from pathlib import Path

from artifacts import WORK, check_space, open_files, prepare_output
from capture_native_replay_audio import input_schedule
from reference.audio import summarize_audio
from reference.engine_audio_services import export
from verify_configuration_persistence import digest, require
from verify_original_race_audio import validate_capture
from verify_original_replay import fixture


def read(path):
    return json.loads(path.read_text())


def compare(expected, actual, label):
    if actual != expected:
        first = next((i for i, (a, b) in enumerate(zip(expected, actual)) if a != b),
                     min(len(expected), len(actual)))
        raise ValueError(f'{label} differs at byte {first}; actual {len(actual)}, original {len(expected)}')


def validate(target, original, source, clock, services, initial, planned, accepted, played, browser=False):
    require(target.get('pass_') is True and target.get('scenario') == 'original-replay' and
            target.get('engine_state_writes') is False and target['intro']['movie'] == 1,
            'completed actual replay engine required')
    require(target['initial_save_sha256'] == digest(initial) and
            target['game_clock_sha256'] == clock['ticks_sha256'] and
            target['audio_services_sha256'] == services['input_sha256'], 'replay input provenance differs')
    require(target['completion'] == source['completion'] and
            target['restored'] == source['start_state'], 'natural replay completion/restoration differs')
    for field in ('level', 'cf', 'movie', 'menu', 'cd'):
        require(target['end_state'][field] == original['end_state'][field],
                'original frontend endpoint differs: ' + field)
    complete = target['audio_services']
    require(complete['complete'] is True and complete['epoch'] == 2 and
            all(complete[field] == services[field] for field in ('events', 'frames', 'callbacks')) and
            complete['completed_flips'] == clock['completed_flips'], 'bounded audio service extent differs')
    schedule = target['schedule']
    require(len(schedule) == len(planned), 'six genuine replay inputs required')
    for actual, expected in zip(schedule, planned):
        require({k: actual[k] for k in expected} == expected, 'original sound trigger schedule differs')
    require(schedule[3].get('release_flip') and schedule[3].get('release_scope') ==
            'explicit diagnostic hypothesis; not observed original OS release',
            'diagnostic Enter release must be explicit')
    if browser:
        history = target['inputs']; events = history['events']
        require(not target['errors'] and history['index'] == 12 and len(events) == 12 and
                history['flips'] == complete['completed_flips'], 'browser input/output observation incomplete')
        for i, p in enumerate(schedule):
            key = 'ArrowRight' if p['key'] == 'Right' else 'Enter'
            for j, kind, flip in ((0, 'keydown', p['flip']-1),
                                 (1, 'keyup', p.get('release_flip', p['flip']+1)-1)):
                e = events[i*2+j]
                require(e['trusted'] is True and e['type'] == kind and e['code'] == key and
                        e['pending_flip'] == flip, 'trusted keyboard event position differs')
        sink = target['sink']
        require(sink['frames'] == services['frames'] and sink['buffers'] > 0 and
                not any(sink[k] for k in ('mismatches', 'missing', 'dropped')),
                'WebAudio delivery differs/drops buffers')
        require(target['accepted_sha256'] == digest(accepted), 'browser PCM digest differs')
    else:
        history = target['input_history']; terminal = history['terminal']
        require(history['pass_'] and history['hardware_only'] and not history['engine_state_writes'] and
                history['schedule'] == schedule and terminal['completed_flips'] == complete['completed_flips'] and
                terminal['clock_calls'] == clock['calls'] and terminal['services'] == services['events'],
                'read-only hardware endpoint differs')
        consumed = [e for e in history['inputs'] if e.get('pad_consumed')]
        require(len(consumed) == 6 and all(e['action'] == i and e['key'] == p['key'] and
                e['observation']['completed_flips'] == p['flip'] and
                e['observation']['held'] & (32 if p['key'] == 'Right' else 16384) and
                e['observation']['pressed'] & (32 if p['key'] == 'Right' else 16384)
                for i, (e, p) in enumerate(zip(consumed, schedule))), 'own pad consumption differs')
        edges = [e for e in history['inputs'] if 'down' in e]
        require(len(edges) == 12, 'complete genuine native key edges required')
        for i, p in enumerate(schedule):
            for j, down, flip in ((0, True, p['flip']-1),
                                  (1, False, p.get('release_flip', p['flip']+1)-1)):
                e = edges[i*2+j]
                require(e['action'] == i and e['key'] == p['key'] and e['down'] is down and
                        e['observation']['completed_flips'] == flip and
                        e['observation']['clock_calls'] == p['clock_calls'], 'genuine native keyboard edge differs')
        endpoint = history['endpoint']
        require(endpoint['scene'] == target['end_state'] and endpoint['settings'] == target['restored'] and
                endpoint['card_sha256'] == digest(initial) and
                endpoint['script_sha256'] == digest(initial[0x2012:0x3c12]) and
                endpoint['order_sha256'] == digest(initial[0x3c12:0x3c26]), 'actual stopped card/tape endpoint differs')
    return dict(pass_=True, accepted_bytes=len(accepted), accepted_sha256=digest(accepted),
                played_bytes=len(played), played_sha256=digest(played), services=complete,
                binary_sha256=target.get('binary_sha256', target.get('wasm_sha256')),
                webaudio=target.get('sink'), end_state=target['end_state'], schedule=schedule)


def verify(args):
    out = prepare_output(args.output)
    require(WORK in out.parents and not out.exists(), 'fresh /tmp/wasm-dd2/ output required')
    out.mkdir(parents=True); check_space(out)
    initial, _, _ = fixture(args.fixture)
    original = validate_capture(args.original, args.fixture)
    source = read(args.original/'report.json'); clock = read(args.original/'game-clock/report.json')
    planned = input_schedule(args.original, args.services, args.fixture)
    services = export(args.original, args.mixer, out/'independent-input')
    require(digest((args.services/'services.bin').read_bytes()) == services['input_sha256'] and
            digest((args.original/'game-clock/ticks.bin').read_bytes()) == clock['ticks_sha256'],
            'inputs differ from independent original export')
    audio = summarize_audio(args.original/'audio', write=False, require_played=True)
    accepted = (args.original/'audio'/audio['streams'][-1]['file']).read_bytes()
    played = (args.original/'audio'/audio['played_streams'][-1]['file']).read_bytes()
    require(len(accepted) == services['frames']*8 and accepted[:len(played)] == played,
            'original accepted/played extent differs')
    targets = {}; inputs = {}
    for name, directory in (('native', args.native), ('native-asan', args.native_asan), ('browser', args.browser)):
        target = read(directory/'checkpoint.json'); inputs[name] = target
        require(read(directory/'mixed.pcm.json') == dict(format='FLOAT_LE', rate=44100, channels=2),
                'actual device format differs')
        pcm = (directory/'mixed.pcm').read_bytes()
        compare(accepted, pcm, name+' accepted PCM'); compare(played, pcm[:len(played)], name+' played PCM')
        targets[name] = validate(target, original, source, clock, services, initial, planned, accepted, played, name=='browser')
        require(target['schedule'] == inputs['native']['schedule'], 'cross-target keyboard schedule differs')
        print(f'PASS {name}: {len(pcm)} actual engine PCM bytes', flush=True)
    negative = {}
    for name, directory, offset in (('release-early', args.negative_early, -1), ('release-late', args.negative_late, 1)):
        failed = read(directory/'checkpoint.json'); history = read(directory/'input-failure.json')
        require(failed['pass_'] is False and history['pass_'] is False and history['action'] == 4 and
                failed['binary_sha256'] == inputs['native']['binary_sha256'] and
                failed['schedule'][3]['release_flip'] == inputs['native']['schedule'][3]['release_flip']+offset and
                failed['audio_services_sha256'] == services['input_sha256'] and
                failed['game_clock_sha256'] == clock['ticks_sha256'] and
                'engine API clock/presentation position differs' in (directory/'game-1.log').read_text(),
                'wrong input duration did not fail the actual engine timing assertion')
        negative[name] = dict(rejected=True, endpoint=history['last'], error=history['error'])
    # Exercise the comparator at interior stereo samples, endpoints and extent.
    for name, offset in (('first',0),('left',len(accepted)//16*8),
                         ('right',len(accepted)//16*8+4),('last',len(accepted)-1)):
        bad = bytearray(accepted); bad[offset] ^= 1
        try: compare(accepted,bad,name)
        except ValueError: negative['pcm-'+name] = dict(rejected=True)
        else: raise AssertionError('damaged PCM accepted: '+name)
    for name, bad in (('truncated',accepted[:-1]),('extra',accepted+b'\0')):
        try: compare(accepted,bad,name)
        except ValueError: negative['pcm-'+name] = dict(rejected=True)
        else: raise AssertionError('wrong PCM extent accepted: '+name)
    for name, field, replacement in (('callback-count','audio_services',{'callbacks':44}),
                                     ('endpoint','end_state',{'cf':original['end_state']['cf']+1})):
        bad = copy.deepcopy(inputs['native']); bad[field].update(replacement)
        try: validate(bad,original,source,clock,services,initial,planned,accepted,played)
        except RuntimeError: negative[name] = dict(rejected=True)
        else: raise AssertionError('damaged replay evidence accepted: '+name)
    for name in ('keyedge','card','pad'):
        bad = copy.deepcopy(inputs['native'])
        if name == 'keyedge': bad['input_history']['inputs'][0]['observation']['completed_flips'] += 1
        elif name == 'card': bad['input_history']['endpoint']['card_sha256'] = '0'*64
        else: next(e for e in bad['input_history']['inputs'] if e.get('pad_consumed'))['observation']['pressed'] = 0
        try: validate(bad,original,source,clock,services,initial,planned,accepted,played)
        except RuntimeError: negative[name] = dict(rejected=True)
        else: raise AssertionError('damaged '+name+' accepted')
    report = dict(scope=__doc__, pass_=True, original_exe_sha256=original['exe_sha256'],
                  initial_card_sha256=digest(initial), trace_sha256=services['trace_sha256'],
                  services_sha256=services['input_sha256'], game_clock_sha256=clock['ticks_sha256'],
                  targets=targets, negative_cases=negative)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    opened = open_files()
    for directory in (args.native,args.native_asan,args.browser):
        file = directory/'mixed.pcm'; st = file.stat()
        require((st.st_dev,st.st_ino) not in opened, 'successful PCM still in use'); file.unlink()
    for file in (out/'independent-input').glob('*.bin'): file.unlink()
    check_space(out)
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fixture','original','services','mixer','native','native-asan','browser',
                 'negative-early','negative-late','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    verify(parser.parse_args())
