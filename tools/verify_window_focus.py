#!/usr/bin/env python3
"""Check recorded focus-loss semantics; mismatching ports return a failing gate.

This compares activation, stored timer cancellation and one held input flag.
It does not accept resumption, framebuffer, PCM, timing or whole-game parity.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
LABELS=('inactive-start','inactive-held','released-in-sink')


def require(value,reason):
    if not value:raise ValueError(reason)


def samples(report):
    rows={row['label']:row for row in report['samples']}
    require(all(label in rows for label in ('baseline','key-down-active',*LABELS)),
            'missing focus observation')
    return rows


def original(report):
    require(report['pass_'] and report['operation']=='original-window-focus-capture' and
            report['engine_state_writes'] is False,'completed read-only Original capture required')
    require(report['exe_sha256']==hashlib.sha256((ROOT/'DestructionDerby2/dd2h.exe').read_bytes()).hexdigest(),
            'Original executable differs')
    for name,sha in report['sources'].items():
        require(hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==sha,'capture source differs: '+name)
    require(set(report['sources'])=={'tools/capture_window_focus.py','tools/window_focus_observer.py',
                                   'tools/reference/window_focus_probe.c','tools/reference/capture.py'},
            'complete capture source identity required')
    rows=samples(report)
    require(rows['baseline']['active']==1 and rows['baseline']['timer']!=0 and
            rows['baseline']['flags'][12:14]=='00','wrong initial state')
    require(rows['key-down-active']['active']==1 and rows['key-down-active']['flags'][12:14]=='01',
            'actual held Left flag missing')
    for label in LABELS:
        require(rows[label]['active']==rows[label]['timer']==0 and rows[label]['flags'][12:14]=='01',
                'Original deactivation/cancellation/retained input differs: '+label)
    require(rows['inactive-held']['timer_fires']>rows['inactive-start']['timer_fires'],
            'surviving timer callback not observed')
    events=report['observer_events']
    require(any(row['kind']=='message' and row.get('message')==0x100 and row['wparam']==0x25
                and row['pc']==0x4132f0 for row in events),'original keyboard entry missing')
    require(any(row['kind']=='activation-return' and row['pc']==0x41344d and
                row['requested_active']==row['active']==row['timer']==0 and
                row['input_flags'][12:14]=='01' for row in events),'actual deactivation return missing')
    require(any(row['kind']=='wait-enter' and row['pc']==0x4130a3 and row['active']==0
                for row in events),'actual message wait missing')
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original',type=Path,required=True)
    parser.add_argument('--native',type=Path)
    parser.add_argument('--browser',type=Path)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--negative-controls',action='store_true')
    args=parser.parse_args()
    output=args.report.resolve()
    require(Path('/tmp/wasm-dd2') in output.parents and not output.exists(),'fresh report under /tmp/wasm-dd2 required')
    source=json.loads(args.original.read_text());reference=original(source)
    report=dict(scope=__doc__.strip(),original_port_full_parity='unproven',pass_=True,targets=[],
                original_report_sha256=hashlib.sha256(args.original.read_bytes()).hexdigest(),negative_controls=[])
    if args.negative_controls:
        cases=[('reset-held-flag',lambda r:r['samples'][next(i for i,x in enumerate(r['samples']) if x['label']=='inactive-held')].update(flags='00'*17)),
               ('lost-deactivation-return',lambda r:r.update(observer_events=[x for x in r['observer_events'] if x['kind']!='activation-return'])),
               ('lost-message-wait',lambda r:r.update(observer_events=[x for x in r['observer_events'] if x['kind']!='wait-enter'])),
               ('lost-actual-key',lambda r:r.update(observer_events=[x for x in r['observer_events'] if x.get('message')!=0x100])),
               ('incomplete-capture',lambda r:r.update(pass_=False)),
               ('state-writing-capture',lambda r:r.update(engine_state_writes=True)),
               ('lost-source-identity',lambda r:r.update(sources={})),
               ('different-executable',lambda r:r.update(exe_sha256='0'*64))]
        for name,mutate in cases:
            damaged=copy.deepcopy(source);mutate(damaged)
            try:original(damaged)
            except ValueError:report['negative_controls'].append(dict(case=name,rejected=True))
            else:raise AssertionError('Damaged Original capture accepted: '+name)
    for target,path in [('native',args.native),('browser',args.browser)]:
        if path is None:continue
        captured=json.loads(path.read_text())
        require(captured['pass_'] and captured['target']==target and captured['operation']=='window-focus-capture' and
                captured['engine_state_writes'] is False,'completed read-only '+target+' capture required')
        for name,sha in captured['sources'].items():
            require(hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==sha,'target source differs: '+name)
        rows=samples(captured)
        require(rows['key-down-active']['flags'][12:14]=='01','target never held Left: '+target)
        if target=='browser':
            require(captured['playwright_focus_emulation_disabled_requests']>0 and
                    any(row['type']=='blur' and row['trusted'] for row in captured['events']),
                    'real browser focus loss not observed')
        differences=[]
        for label in LABELS:
            for field in ('active','timer','flags'):
                a,b=reference[label][field],rows[label][field]
                if field=='flags':a,b=a[12:14],b[12:14]
                if a!=b:differences.append(dict(label=label,field=field,original=a,actual=b))
        report['targets'].append(dict(target=target,report_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                                      match_=not differences,differences=differences))
        if differences:report['pass_']=False
    require(report['targets'],'at least one port capture required')
    output.parent.mkdir(parents=True,exist_ok=True);output.write_text(json.dumps(report,indent=2)+'\n')
    print('Recorded focus semantics:', 'PASS' if report['pass_'] else 'FAIL; actual port differences recorded')
    return 0 if report['pass_'] else 1


if __name__=='__main__':raise SystemExit(main())
