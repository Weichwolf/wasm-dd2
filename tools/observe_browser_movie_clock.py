#!/usr/bin/env python3
"""Observe actual browser movie presentation against consumed device samples.

RPC brackets independently relate performance.now to native CLOCK_MONOTONIC;
their intersection includes Chromium's documented 100-us coarse precision.
Native clock probes verify the host hrtime domain before and after playback.
Use complete device sample intervals and the separately observed prefill/start
to report source position, video lead and output-timestamp estimation error.
No clocks are fitted to PCM or images, and no bytes are shifted or trimmed.
This is a timing diagnosis, not original/port PCM or whole-game parity proof.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import statistics

from artifacts import WORK, check_space, prepare_output
from observe_browser_movie_device import predict, validate as validate_pcm
from reference.audio import summarize_audio
from verify_configuration_persistence import ROOT, require
from verify_movie_avi import original_metadata


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file,'sha256').hexdigest()


def clock_bounds(browser):
    pairs=browser['clock_bounds'];host=browser['host_clock_bounds']
    require(len(pairs)==16 and len(host)==6,'playing/finished RPC and native clock probes required')
    require([p['phase'] for p in pairs]==['playing']*8+['finished']*8,'clock phases differ')
    origin=pairs[0]['time_origin_ms'];lower=[];upper=[];last=0
    for pair in pairs:
        begin,end=int(pair['begin_ns']),int(pair['end_ns']);sample=pair['performance_ms']
        require(0<last<=begin<=end if last else 0<begin<=end,'reversed host RPC clock')
        require(math.isfinite(sample) and sample>=0 and pair['time_origin_ms']==origin,
                'invalid browser performance clock or changed document epoch')
        # The pinned non-isolated Chromium TimeClamper rounds within 100 us.
        lower.append(begin-round(sample*1e6)-100000)
        upper.append(end-round(sample*1e6)+100000);last=end
    for pair in host:
        require(0<int(pair['begin_ns'])<=int(pair['native_monotonic_ns'])<=int(pair['end_ns']),
                'native CLOCK_MONOTONIC does not lie inside the host hrtime bracket')
    lo,hi=max(lower),min(upper)
    require(lo<=hi,'independent clock brackets have no common offset')
    return lo,hi


def validate(browser,device,metadata):
    lo,hi=clock_bounds(browser);rate=device['rate'];source=browser['source']
    require(rate==source['rate']==source['context_rate']==metadata['pcm_rate'] and
            device['channels']==source['channels']==metadata['pcm_channels'] and
            source['frames']==metadata['pcm_frames'],'source/device format or extent differs')
    events=browser['events'];start=events[0]
    require(start['event']=='source-start' and events[-1]['event']=='context-close' and
            [e['performance_ms'] for e in events]==sorted(e['performance_ms'] for e in events),
            'movie source/event chronology differs')
    scheduled=start['scheduled_time'];position=round(scheduled*rate)
    require(scheduled>=0 and abs(scheduled*rate-position)<1e-6,'non-sample source start')
    prefix=device['buffer_frames']+position;segments=device['segments']
    require(segments and device['closed'],'complete consumed device intervals required')
    def consumed(ns):
        return sum(min(s['frames'],max(0,(ns-s['begin_ns'])*rate//1000000000)) for s in segments)
    def times(begin,end):
        return [lo+round(begin*1e6)-100000,hi+round(end*1e6)+100000]
    def source_times(bounds):
        return [(consumed(ns)-prefix)*1000/rate for ns in bounds]
    precise=browser.get('movie_clock_unit')=='microseconds'
    require(browser.get('movie_clock_unit','milliseconds') in ('milliseconds','microseconds'),'unknown movie clock unit')
    frames=browser['frames'];rows=[];last=-1;last_clock=browser['initial_movie_clock_us'] if precise else browser['initial_movie_clock_ms']
    deadline_us=float(last_clock)
    duration_us=(metadata['video_scale']/metadata['video_rate'])*1000000
    require(browser['clock_calls']>len(frames) and frames and
            (26<=len(frames)<metadata['frames']-1 if browser['skip'] else len(frames)==metadata['frames']-1),
            'movie presentation/clock extent differs')
    for i,frame in enumerate(frames):
        point=frame['before_clock'];begin=frame['begin_performance_ms'];end=frame['end_performance_ms']
        require(frame['frame']==i and last<=begin<=point['begin_performance_ms']<=point['end_performance_ms']<=end,
                'actual presentation brackets are missing or reordered')
        last=end;clock=frame['movie_clock_ms'];pts=i*metadata['video_scale']*1000/metadata['video_rate']
        if precise:
            observed_us=frame['movie_clock_us']
            require(type(observed_us) is int and clock==observed_us/1000 and
                    observed_us>=last_clock and observed_us>=deadline_us,
                    'movie frame precedes its observed elapsed-clock deadline')
            last_clock=observed_us;deadline_us+=duration_us
        else:
            require(clock>=last_clock and clock-browser['initial_movie_clock_ms']>=pts,
                    'movie frame precedes its observed elapsed-clock deadline')
            last_clock=clock
        require(0<=point['output_context_time']<=point['context_time'] and
                0<=point['output_performance_ms']<=point['end_performance_ms']+0.1,
                'output timestamp is outside rendered/time bounds')
        bounds=times(point['begin_performance_ms'],point['end_performance_ms']);source_ms=source_times(bounds)
        row=dict(frame=i,pts_ms=pts,movie_clock_ms=clock,monotonic_begin_bounds_ns=bounds,
                 consumed_source_time_bounds_ms=source_ms,
                 video_ahead_source_bounds_ms=[pts-source_ms[1],pts-source_ms[0]],
                 render_context_time_ms=point['context_time']*1000)
        # A clamped zero output position before the first source reaches the
        # device is not a usable estimate of the negative prefill interval.
        row['output_position_below_movie_clock_resolution']=point['output_context_time']*1000<1
        if point['output_context_time']>0 and point['output_performance_ms']>0:
            estimate=point['output_context_time']*1000+point['begin_performance_ms']-point['output_performance_ms']
            row['extrapolated_output_error_bounds_ms']=[estimate-scheduled*1000-source_ms[1],
                                                       estimate-scheduled*1000-source_ms[0]]
        rows.append(row)
    endpoints=[]
    for event in events[1:]:
        source_ms=source_times(times(event['performance_ms'],event['performance_ms']))
        endpoints.append(dict(event=event['event'],consumed_source_time_bounds_ms=source_ms,
                              source_frames_remaining_bounds=[source['frames']-math.floor(v*rate/1000)
                                                              for v in reversed(source_ms)]))
    return dict(independent_performance_to_monotonic_offset_bounds_ns=[lo,hi],
                offset_interval_width_ms=(hi-lo)/1e6,frames=rows,endpoints=endpoints)


def summarize(rows,key):
    values=[r[key] for r in rows if key in r]
    return {name:[fn(v[i] for v in values) for i in (0,1)]
            for name,fn in [('min',min),('median',statistics.median),('max',max)]} if values else None


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,nargs='+',required=True)
    parser.add_argument('--clock-sources',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();output=prepare_output(args.output)
    require(WORK in output.parents,'Use /tmp/wasm-dd2/');output.mkdir(parents=True,exist_ok=False)
    sources=json.loads((args.clock_sources/'sources.json').read_text())
    for source in sources['sources']:
        require(sha(args.clock_sources/Path(source['path']).name)==source['sha256'] and
                '/'+sources['chromium_version']+'/' in source['url'],'changed pinned Chromium clock source')
    require('kCoarseResolutionMicroseconds = 100;' in (args.clock_sources/'time_clamper.h').read_text(),
            'pinned performance-clock coarse precision required')
    cases=[];negative=[]
    for capture in args.capture:
        report=json.loads((capture/'report.json').read_text());browser=json.loads((capture/'browser.json').read_text())
        require(report['observations_valid'] and browser['clock_profile'] and
                report['browser_report_sha256']==sha(capture/'browser.json') and
                browser['observer_source_sha256']==sha(capture/'observer-source.js') and
                browser['host_clock_probe_sha256']==sha(capture/'clock_probe.c') and
                browser['chromium_version']==sources['chromium_version'],'changed actual browser/clock provenance')
        require(report['sample_clock_source_sha256']==sha(ROOT/'tools/reference/alsa_clock.c'),
                'changed actual device clock observer')
        audio=summarize_audio(capture/'audio',require_played=True)
        require(audio['streams']==report['audio']['streams'] and audio['played_streams']==report['audio']['played_streams'],
                'changed actual device PCM or transport intervals')
        movie=ROOT/'DestructionDerby2'/report['movie'];metadata,*_=original_metadata(movie)
        require(sha(movie)==report['original_movie_sha256'] and len(audio['played_streams'])==1,
                'changed original AVI or multiple device clocks')
        pcm=capture/browser['source']['pcm_file'];source=pcm.read_bytes()
        require(sha(pcm)==browser['source']['pcm_sha256'] and len(source)==metadata['pcm_frames']*4 and
                browser['source']['canonical_encoding_errors']==0,'changed complete actual AudioBuffer PCM')
        converted=predict(source);device=audio['played_streams'][0]
        writes=[json.loads(s) for s in (capture/'audio'/audio['streams'][0]['events']).read_text().splitlines()]
        first_write=next(e for e in writes if e['event']=='write')
        for kind in ('streams','played_streams'):
            validate_pcm((capture/'audio'/audio[kind][0]['file']).read_bytes(),converted,browser,device,first_write)
        details=validate(browser,audio['played_streams'][0],metadata)
        (output/(capture.name+'-frames.json')).write_text(json.dumps(details)+'\n')
        for mutation in ['reversed-rpc','native-clock-domain','shifted-performance','document-epoch',
                         'frame-before-clock','output-after-render','reversed-source-events']:
            changed=copy.deepcopy(browser)
            if mutation=='reversed-rpc':changed['clock_bounds'][0]['begin_ns']=str(int(changed['clock_bounds'][0]['end_ns'])+1)
            elif mutation=='native-clock-domain':changed['host_clock_bounds'][0]['native_monotonic_ns']='0'
            elif mutation=='shifted-performance':changed['clock_bounds'][0]['performance_ms']+=1000
            elif mutation=='document-epoch':changed['clock_bounds'][1]['time_origin_ms']+=1000
            elif mutation=='frame-before-clock':changed['frames'][10]['movie_clock_ms']=changed['initial_movie_clock_ms']
            elif mutation=='output-after-render':changed['frames'][10]['before_clock']['output_context_time']+=1000
            else:changed['events'][-1]['performance_ms']=changed['events'][0]['performance_ms']-1
            try:validate(changed,audio['played_streams'][0],metadata)
            except RuntimeError:negative.append(dict(capture=str(capture),mutation=mutation))
            else:raise RuntimeError('accepted altered clock observation: '+mutation)
        cases.append(dict(capture=str(capture),movie=report['movie'],skip=browser['skip'],
                          declared_source_schedule_delay_ms=browser.get('source_schedule_delay_ms',0),
                          scheduled_source_start_ms=browser['events'][0]['scheduled_time']*1000,
                          wasm_sha256=browser['wasm_sha256'],frames=len(details['frames']),
                          movie_clock_unit=browser.get('movie_clock_unit','milliseconds'),
                          independent_offset_bounds_ns=details['independent_performance_to_monotonic_offset_bounds_ns'],
                          offset_interval_width_ms=details['offset_interval_width_ms'],endpoints=details['endpoints'],
                          video_ahead_source_ms=summarize(details['frames'],'video_ahead_source_bounds_ms'),
                          extrapolated_output_error_ms=summarize(details['frames'],'extrapolated_output_error_bounds_ms'),
                          sub_millisecond_output_frames=[r['frame'] for r in details['frames']
                                                         if r['output_position_below_movie_clock_resolution']],
                          browser_report_sha256=sha(capture/'browser.json'),capture_report_sha256=sha(capture/'report.json')))
    report=dict(scope=__doc__,observations_valid=True,original_port_parity='unproven',
                coarse_performance_precision_us=100,primary_clock_sources=sources,cases=cases,negative_controls=negative)
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(output)
    print('PASS independent browser/device clock observations:',len(cases),'cases;',len(negative),'changed clock controls rejected',flush=True)


if __name__=='__main__':main()
