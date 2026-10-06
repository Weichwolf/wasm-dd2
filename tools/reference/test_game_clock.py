#!/usr/bin/env python3
"""Reject damaged relay inputs using a bounded excerpt of an actual capture."""
import argparse
import json
from pathlib import Path
import struct

from game_clock import CALL, RETURN, EXE, export_clock
from trace_log import text_lines


def verify(capture, executable, output):
    checkpoint = json.loads((capture/'checkpoint.json').read_text())
    if checkpoint.get('exe_modified') is not False or checkpoint.get('exe_sha256') != EXE or checkpoint.get('phase') != 'live engine; no debugger':
        raise ValueError('Unmodified original non-debugger capture required')
    output.mkdir(parents=True, exist_ok=False)
    flip = None; entries = {}; selected = []; values = []
    lines = text_lines(capture/'wine.log')
    try:
        for line in lines:
            text = line.rstrip('\r\n')
            if flip is None and 'ddraw_surface1_Flip iface' in text:
                flip = text
            entry, returned = CALL.fullmatch(text), RETURN.fullmatch(text)
            if entry and int(entry[4],16) in (0x423c2d, 0x424024):
                entries[entry[2]] = text
            if returned and int(returned[5],16) in (0x423c2d, 0x424024):
                selected.extend((entries.pop(returned[2]), text));values.append(int(returned[4],16))
                if len(values) == 2:break
    finally:
        lines.close()
    if flip is None or len(values) != 2:
        raise ValueError('Actual race clock and presentation excerpt required')
    original = [flip, *selected]
    source = output/'original-excerpt.txt'
    source.write_text('\n'.join(original)+'\n')
    positive = export_clock(source, executable, output/'positive')
    expected = struct.pack('<II', *values)
    if positive['calls'] != 2 or (output/'positive/ticks.bin').read_bytes() != expected:
        raise AssertionError('Actual DWORD clock returns changed')
    terminal_trace = output/'terminal-excerpt.txt'
    terminal_trace.write_text('\n'.join([*original, original[-2]])+'\n')
    terminal = export_clock(terminal_trace, executable, output/'terminated', allow_terminal_entry=True)
    if terminal['calls'] != 2 or terminal['terminal_unreturned_entry']['return_observed'] is not False or (output/'terminated/ticks.bin').read_bytes() != expected:
        raise AssertionError('Terminated tail introduced an invented clock return')
    thread = CALL.fullmatch(selected[0])[2]
    other_thread = f'{int(thread,16)+1:04x}'
    # Exercise late logging from another thread without treating it as an
    # engine return. This uses a modified actual trace excerpt as a control.
    other_thread_trace = output/'terminal-other-thread.txt'
    other_thread_trace.write_text('\n'.join([*original, original[-2],
                                            original[0].replace(':'+thread+':', ':'+other_thread+':')])+'\n')
    late = export_clock(other_thread_trace, executable, output/'terminated-other-thread', allow_terminal_entry=True)
    if (late['calls'] != 2 or late['terminal_unreturned_entry'].get('trailing_other_thread_lines') != 1 or
            late['terminal_unreturned_entry']['return_observed'] is not False or
            (output/'terminated-other-thread/ticks.bin').read_bytes() != expected):
        raise AssertionError('Other-thread tail introduced an invented clock return')
    cases = {}
    cases['truncated'] = original[:-1]
    cases['missing-entry'] = [original[0], *original[2:]]
    cases['return-caller'] = [*original[:-1], original[-1].replace('ret=00424024','ret=00424025')]
    cases['unknown-engine-caller'] = [row.replace('ret=00424024','ret=00424025') for row in original]
    cases['missing-presentation'] = original[1:]
    cases['return-thread'] = [*original[:-1], original[-1].replace(':'+thread+':',':'+other_thread+':')]
    cases['another-engine-thread'] = [*original[:3], *[row.replace(':'+thread+':',':'+other_thread+':') for row in original[3:]]]
    cases['backwards-clock'] = [*original[:-1], original[-1].replace(f'retval={values[-1]:08x}',f'retval={(values[0]-1)&0xffffffff:08x}')]
    cases['nested-entry'] = [*original[:2], original[1], *original[2:]]
    cases['reordered-return'] = [original[0], original[2], original[1], *original[3:]]
    cases['terminal-entry-without-termination-authorization'] = [*original, original[-2]]
    cases['interior-unreturned-entry'] = [*original, original[-2], 'unrelated trailing record']
    cases['later-engine-thread'] = [*original, original[-2], original[0]]
    rejected = {}
    for name, lines in cases.items():
        trace = output/(name+'.txt');trace.write_text('\n'.join(lines)+'\n')
        directory = output/name
        try:export_clock(trace, executable, directory, allow_terminal_entry=name in ('interior-unreturned-entry','later-engine-thread'))
        except ValueError as error:
            if (directory/'ticks.bin').exists() or (directory/'observations.bin').exists():
                raise AssertionError('Failed clock export retained partial replay input')
            rejected[name] = str(error)
        else:raise AssertionError('Damaged clock evidence accepted: '+name)
    report = dict(scope=__doc__.strip()+' Clock export validation only; port execution remains separate.',
                  pass_=True, capture=str(capture), actual_values=values,
                  terminated_tail_retains_only_observed_returns=True,
                  excerpt_trace_sha256=positive['trace_sha256'], negative_cases=rejected)
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    for directory in ('positive','terminated','terminated-other-thread'):
        for name in ('ticks.bin', 'observations.bin'):(output/directory/name).unlink()
    for path in output.glob('*.txt'):path.unlink()
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if Path('/tmp/wasm-dd2') not in args.output.resolve().parents:
        parser.error('Use /tmp/wasm-dd2/ for diagnostics')
    print(json.dumps(verify(args.capture.resolve(),args.executable,args.output.resolve()),indent=2))
