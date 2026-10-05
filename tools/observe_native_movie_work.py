#!/usr/bin/env python3
"""Observe movie elapsed-clock to texture work in an unchanged native binary.

Clock call sites are bound to actual binary disassembly. The epoch is the
observed MCI start clock, without fitting to video or audio. Elapsed-to-texture
intervals include decoder/filter work, observation cost and OS scheduling.
RAW clocks retain independent MONOTONIC brackets; costs are bounded in the
texture clock domain without offset or rate fitting;
they are not CPU-only, physical presentation or original/port parity evidence.
"""
import argparse
import bisect
import copy
import hashlib
import json
from pathlib import Path
import re
import statistics
import subprocess

from artifacts import WORK, check_space, prepare_output
from observe_native_movie_timing import validate_frames
from verify_configuration_persistence import ROOT, require
from verify_movie_avi import original_metadata


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def call_sites(binary, function):
    text = subprocess.check_output(['objdump', '-d', '--disassemble='+function, str(binary)], text=True)
    sites = re.findall(r'call[^\n]*<dd2_movie_now_(?:ms|us)>\n\s*([0-9a-f]+):', text)
    return [int(s, 16) for s in sites], text


def validate(clocks, updates, sites, epoch_site, metadata, precise=False):
    require(clocks and clocks[0]['site'] == epoch_site and
            sum(c['site'] == epoch_site for c in clocks) == 1, 'unique observed MCI epoch required')
    domain=clocks[0]['clock_domain']
    require(domain in ('CLOCK_MONOTONIC','CLOCK_MONOTONIC_RAW'), 'unsupported movie clock domain')
    previous = previous_mono = 0
    for c in clocks:
        require(c['clock_domain'] == domain and c['event'] == 'movie_clock' and
                c['site'] in [epoch_site, *sites] and type(c['time_ns']) is int and c['time_ns'] > previous,
                'reordered clock, unknown call site or changed clock domain')
        previous = c['time_ns']
        if domain=='CLOCK_MONOTONIC_RAW':
            require(type(c.get('monotonic_begin_ns')) is int and type(c.get('monotonic_end_ns')) is int and
                    previous_mono<=c['monotonic_begin_ns']<=c['monotonic_end_ns'],
                    'paired monotonic clock bracket missing or reversed')
            previous_mono=c['monotonic_end_ns']
    def movie_us(ns):
        ticks=ns//100
        scaled=(ticks*1000000)&0xffffffffffffffff
        if scaled>=1<<63:scaled-=1<<64
        return (abs(scaled)//10000000)*(1 if scaled>=0 else -1)
    epoch = clocks[0]['time_ns']
    deadline_us=float(movie_us(epoch))
    duration_us=(metadata['video_scale']/metadata['video_rate'])*1000000
    pump_records = [c for c in clocks if c['site'] == sites[0]]
    pumps = [c['monotonic_end_ns'] if domain=='CLOCK_MONOTONIC_RAW' else c['time_ns'] for c in pump_records]
    require(pumps and len(updates) == metadata['frames']-1, 'complete movie pump/texture history required')
    rows = []
    previous_end = 0
    for index, u in enumerate(updates):
        require(u['frame'] == index and previous_end <= u['call_begin_ns'] <= u['call_end_ns'] <= u['time_ns'],
                'texture sequence or interval differs')
        previous_end = u['time_ns']
        position = bisect.bisect_right(pumps, u['call_begin_ns'])-1
        require(position >= 0, 'texture precedes first elapsed-clock observation')
        observed = pump_records[position]
        t = observed['time_ns']
        mono_begin=observed['monotonic_begin_ns'] if domain=='CLOCK_MONOTONIC_RAW' else t
        mono_end=pumps[position]
        elapsed = (t//1_000_000 - epoch//1_000_000) & 0xffffffff
        require(movie_us(t)>=deadline_us if precise else index*metadata['video_scale']*1000 <= elapsed*metadata['video_rate'],
                'texture drawn before its actual elapsed-clock deadline')
        deadline_us+=duration_us
        nominal = index*metadata['video_scale']*1_000_000_000/metadata['video_rate']
        rows.append(dict(frame=index, pump_time_ns=t, pump_clock_domain=domain,
                         pump_monotonic_bracket_ns=[mono_begin,mono_end], texture_begin_ns=u['call_begin_ns'],
                         elapsed_to_texture_ns=u['call_begin_ns']-mono_end,
                         elapsed_to_texture_upper_ns=u['call_begin_ns']-mono_begin,
                         pump_after_nominal_ns=(movie_us(t)-movie_us(epoch))*1000-nominal if precise else t-epoch-nominal))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args();output = prepare_output(args.output)
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False);capture = args.capture.resolve()
    observed = json.loads((capture/'report.json').read_text());binding = observed['clock_profile']
    require(observed['observations_valid'] and not observed['skip'] and
            sha(args.binary) == observed['binary_sha256'], 'unchanged complete native movie binary required')
    require(binding['source_sha256'] == sha(ROOT/'tools/native_movie_clock_observer.c') and
            binding['observer_sha256'] == sha(capture/'clock-observer.so') and
            binding['journal_sha256'] == sha(capture/'movie-clock.jsonl'), 'clock observer binding differs')
    movie = ROOT/'DestructionDerby2'/observed['movie']
    require(sha(movie) == observed['original_movie_sha256'], 'changed original movie')
    metadata, _, _, _ = original_metadata(movie)
    sites, pump_disassembly = call_sites(args.binary, 'dd2_movie_pump')
    epochs, start_disassembly = call_sites(args.binary, 'dd2_movie_mci_send')
    require(len(sites) == 3 and len(epochs) == 1, 'unsupported movie clock call graph')
    precise = binding.get('microsecond_clock', False)
    require(precise == bool(re.search(r'call[^\n]*<dd2_movie_now_us>', start_disassembly)) and
            precise == bool(re.search(r'call[^\n]*<dd2_movie_now_us>', pump_disassembly)),
            'declared clock precision differs from actual MCI/pump calls')
    clocks = [json.loads(s) for s in (capture/'movie-clock.jsonl').read_text().splitlines()]
    require(len(clocks) == binding['calls'] and clocks and
            all(c['clock_domain'] == binding['clock_domain'] for c in clocks),
            'clock observation count or declared domain differs')
    events = [json.loads(s) for s in (capture/'events.jsonl').read_text().splitlines()]
    validate_frames([r for r in events if r['event'] == 'present'], observed.get('pixel_readback') is False)
    updates = [r for r in events if r['event'] == 'texture_update']
    rows = validate(clocks, updates, sites, epochs[0], metadata, precise)
    negative = []
    for label in ('clock-domain', 'unknown-site', 'reversed-clock', 'texture-before-clock', *(['reversed-monotonic'] if clocks[0]['clock_domain']=='CLOCK_MONOTONIC_RAW' else [])):
        changed = copy.deepcopy(clocks);textures = copy.deepcopy(updates)
        if label == 'clock-domain': changed[1]['clock_domain'] = 'CLOCK_MONOTONIC' if clocks[0]['clock_domain']=='CLOCK_MONOTONIC_RAW' else 'CLOCK_MONOTONIC_RAW'
        elif label == 'unknown-site': changed[1]['site'] = 0
        elif label == 'reversed-clock': changed[1]['time_ns'] = changed[0]['time_ns']-1
        elif label == 'reversed-monotonic': changed[1]['monotonic_end_ns'] = changed[1]['monotonic_begin_ns']-1
        else: textures[0]['call_begin_ns'] = changed[0].get('monotonic_begin_ns',changed[0]['time_ns'])-1
        try: validate(changed, textures, sites, epochs[0], metadata, precise)
        except RuntimeError: negative.append(label)
        else: raise RuntimeError('accepted changed work history: '+label)
    (output/'frames.json').write_text(json.dumps(rows)+'\n')
    (output/'call-sites.txt').write_text(pump_disassembly+start_disassembly)
    def summary(key):
        values = [r[key]/1e6 for r in rows]
        return dict(first=values[0], last=values[-1], median=statistics.median(values),
                    maximum=max(values), minimum=min(values))
    report = dict(scope=__doc__, observations_valid=True, original_port_parity='unproven',
                  capture_report_sha256=sha(capture/'report.json'), binary_sha256=sha(args.binary),
                  events_sha256=sha(capture/'events.jsonl'), clocks_sha256=binding['journal_sha256'],
                  frames_sha256=sha(output/'frames.json'), call_sites_sha256=sha(output/'call-sites.txt'),
                  frames=len(rows), clock_calls=len(clocks), epoch_site=epochs[0], pump_sites=sites,
                  movie_clock_domain=clocks[0]['clock_domain'],
                  microsecond_clock=precise,
                  elapsed_to_texture_ms=summary('elapsed_to_texture_ns'),
                  elapsed_to_texture_upper_ms=summary('elapsed_to_texture_upper_ns'),
                  pump_after_nominal_ms=summary('pump_after_nominal_ns'),
                  negative_controls_rejected=negative)
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n');check_space(output)
    print(json.dumps(report, indent=2), flush=True)


if __name__ == '__main__':
    main()
