#!/usr/bin/env python3
"""Locate read-only RNG telemetry in the actual WASM's literal LCG instructions.

No binary/heap writes or exported replacement RNG. The browser checks seed 1,
counter 0 at boot and the computed LCG seed at every observed counter value.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

from artifacts import WORK, prepare_output, run_bounded


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--wasm', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = prepare_output(args.output)
    if WORK not in out.parents or out.exists():
        parser.error('Use a fresh directory under /tmp/wasm-dd2/')
    out.mkdir(parents=True)
    wat = out / 'module.wat'
    run_bounded(['wasm-dis', str(args.wasm.resolve()), '-o', str(wat)], directory=out, timeout=60, check=True)
    text = wat.read_text()
    positions = [match.start() for match in re.finditer(r'\(i32.const 1103515245\)', text)]
    if len(positions) != 1:
        raise ValueError('Unique literal Watcom LCG required')
    position = positions[0]
    start = text.rfind('\n (func ', 0, position)
    end = text.find('\n (func ', position)
    if start < 0 or end < 0:
        raise ValueError('Bounded actual RNG function required')
    body = text[start:end]
    pivot = position - start
    if '(i32.const 12345)' not in body[pivot:pivot + 2500]:
        raise ValueError('Literal LCG increment missing')
    loads = [int(n) for n in re.findall(r'\(i32.load offset=(\d+)', body[:pivot]) if int(n) >= 10485760]
    stores = [(match.start(), int(match.group(1))) for match in re.finditer(r'\(i32.store offset=(\d+)', body[pivot:]) if int(match.group(1)) >= 10485760]
    if not loads or len(stores) < 2 or stores[0][1] != loads[-1]:
        raise ValueError('Actual LCG seed load/store not identified')
    seed = loads[-1]
    counter = stores[1][1]
    section = body[pivot + stores[0][0]:pivot + stores[1][0]]
    if f'(i32.load offset={counter}' not in section or '(i32.const 1)' not in section or '(i32.add' not in section:
        raise ValueError('Actual RNG call-counter increment not identified')
    report = dict(scope=__doc__.strip(), wasm_sha256=hashlib.sha256(args.wasm.read_bytes()).hexdigest(),
                  seed_address=seed, counter_address=counter, function=body.splitlines()[1].strip(),
                  multiplier=1103515245, increment=12345, output_mask=32767)
    ordinal = int(re.match(r'\(func \$(\d+)\b', report['function'])[1])
    report['imported_function_count'] = len(re.findall(r'^ \(import .*\(func ', text, re.M))
    definitions = list(re.finditer(r'\n \(func \$(\d+)\b', text))
    steam = []
    for i, definition in enumerate(definitions):
        candidate = text[definition.start():definitions[i + 1].start() if i + 1 < len(definitions) else len(text)]
        if (len(re.findall(r'\(param ', candidate.splitlines()[1])) == 17
                and len(re.findall(r'\(call \$' + str(ordinal) + r'\b', candidate)) == 2
                and all(f'(i32.const {n})' in candidate for n in (64, 96, 100, 80, 88, 63))):
            steam.append(int(definition[1]))
    if len(steam) != 1:
        raise ValueError('Unique literal Steam signature/particle lifetime/coordinates/RNG calls required')
    report['steam_function_ordinal'] = steam[0]
    def function(index):
        begin = text.index(f'\n (func ${index} ')
        finish = text.index('\n (func ', begin + 1)
        return text[begin:finish]
    # These adjacent getters are in the same handwritten stubs compilation
    # unit. Validate literal load/store/increment pairs before using addresses.
    for name, getter, implementation in [('clock', ordinal - 5, ordinal - 4),
                                          ('random_replay', ordinal - 2, ordinal)]:
        getter_body = function(getter)
        addresses = re.findall(r'\(i32.load offset=(\d+)', getter_body)
        if len(addresses) != 1 or '(i32.store' in getter_body or '(call ' in getter_body:
            raise ValueError('Actual API replay counter getter not identified')
        address = int(addresses[0])
        impl = function(implementation)
        store = impl.find(f'(i32.store offset={address}')
        load = impl.rfind(f'(i32.load offset={address}', 0, store)
        increment = impl[load:store]
        if load < 0 or store < 0 or '(i32.const 1)' not in increment or '(i32.add' not in increment:
            raise ValueError('Actual API replay counter increment not identified')
        report[name + '_counter_address'] = address
        report[name + '_function_ordinal'] = implementation
    (out / 'layout.json').write_text(json.dumps(report, indent=2) + '\n')
    wat.unlink()
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
