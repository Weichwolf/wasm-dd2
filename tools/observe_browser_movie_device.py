#!/usr/bin/env python3
"""Diagnose actual Chromium movie PCM without claiming original parity.

Whole device streams are compared literally with the retained original.
Separately, Chromium's observed startup write and scheduled source time predict
its source interval. This independent prediction diagnoses its S16 converter;
it never trims, shifts or replaces bytes for a whole-output equality claim.
Browser context/performance clocks remain separate from ALSA CLOCK_MONOTONIC.
"""
import argparse
import array
import copy
import hashlib
import json
from pathlib import Path

from artifacts import WORK, check_space, prepare_output
from compare_native_movie_device import original_streams
from reference.audio import summarize_audio
from verify_configuration_persistence import ROOT, require
from verify_movie_avi import original_metadata


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def predict(source):
    samples=array.array('h');samples.frombytes(source)
    # Canonical n/32768 Float32 is exact. Chromium's S16 converter scales
    # negative values by 32768, positive values by 32767, then truncates.
    return array.array('h',(n-(n>0) for n in samples)).tobytes()


def validate(actual, converted, browser, device, first_write):
    source=browser['source'];start=browser['events'][0]
    require(start['event']=='source-start' and source['mismatches']==0 and
            source['rate']==source['context_rate']==device['rate']==22050 and
            source['channels']==device['channels']==2, 'canonical source/device metadata differs')
    scheduled=start['scheduled_time'];position=round(scheduled*22050)
    require(scheduled>=0 and abs(scheduled*22050-position)<1e-6, 'source start is not on an observed sample boundary')
    startup=device['buffer_frames']
    require(first_write['offset_frames']==0 and first_write['accepted']==startup and
            not any(actual[:startup*4]), 'independent initial device-buffer silence write differs')
    offset=startup+position
    require(len(actual)%4==0 and len(actual)>offset*4 and not any(actual[:offset*4]),
            'source-start/device-prefill prediction does not describe initial silence')
    body=actual[offset*4:]
    if browser['skip']:
        require(0<len(body)<len(converted) and body==converted[:len(body)],
                'cancelled Chromium converter/source-prefix prediction differs')
    else:
        require(len(body)>=len(converted) and body[:len(converted)]==converted and
                not any(body[len(converted):]), 'complete Chromium converter/source/tail prediction differs')
    return dict(startup_silence_frames=startup,scheduled_source_start_frames=position,
                predicted_source_offset_frames=offset,predicted_source_bytes_compared=min(len(body),len(converted)),
                complete_source_extent_observed=not browser['skip'],
                actual_zero_tail_frames=max(0,(len(body)-len(converted))//4))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,nargs='+',required=True)
    parser.add_argument('--original-intro',type=Path,required=True)
    parser.add_argument('--chromium-sources',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();output=prepare_output(args.output)
    require(WORK in output.parents,'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=False)
    originals=original_streams(args.original_intro)
    metadata,_,_,_=original_metadata(ROOT/'DestructionDerby2/Intro.avi')
    source=originals['streams'].read_bytes()[:metadata['pcm_frames']*4]
    require(originals['played_streams'].read_bytes()[:len(source)]==source,'original complete source intervals differ')
    converted=predict(source)
    provenance=json.loads((args.chromium_sources/'sources.json').read_text())
    for name,entry in provenance['sources'].items():
        require(sha(args.chromium_sources/name)==entry['sha256'] and
                '/'+provenance['chromium_version']+'/' in entry['url'],'Chromium primary-source binding differs')
    require('SND_PCM_FORMAT_S16' in (args.chromium_sources/'alsa_output.cc').read_text() and
            'ScalingFactors<FloatType>::kForPositiveInput' in (args.chromium_sources/'audio_sample_types.h').read_text(),
            'recorded Chromium S16 converter source required')
    cases=[];negative=[]
    for capture in args.capture:
        observed=json.loads((capture/'report.json').read_text())
        browser=json.loads((capture/'browser.json').read_text())
        require(observed['observations_valid'] and observed['movie']=='Intro.avi' and
                observed['browser_report_sha256']==sha(capture/'browser.json') and
                observed['original_movie_sha256']==sha(ROOT/'DestructionDerby2/Intro.avi') and
                browser['observer_source_sha256']==sha(ROOT/'tools/browser/capture_movie_device.js') and
                browser['chromium_version']==provenance['chromium_version'] and
                browser['device_closed_before_browser_shutdown'] and
                browser['source']['frames']==metadata['pcm_frames'],'actual supported complete browser observation required')
        frames=browser['frames']
        require([r['frame'] for r in frames]==list(range(len(frames))) and
                (26<=len(frames)<metadata['frames']-1 if browser['skip'] else len(frames)==metadata['frames']-1),
                'browser presentation/cancellation extent differs')
        previous=0
        for event in browser['events']:
            require(event['performance_ms']>=previous,'reversed browser performance clock')
            previous=event['performance_ms']
        audio=summarize_audio(capture/'audio',require_played=True)
        require(audio==observed['audio'],'device observation/journal binding differs')
        require(len(audio['streams'])==len(audio['played_streams'])==1,'unique Chromium device required')
        device=audio['played_streams'][0]
        writes=[json.loads(s) for s in (capture/'audio'/audio['streams'][0]['events']).read_text().splitlines()]
        first_write=next(e for e in writes if e['event']=='write')
        streams=[]
        for kind in ('streams','played_streams'):
            row=audio[kind][0];actual=(capture/'audio'/row['file']).read_bytes()
            require(row['closed'] and row['format']=='S16_LE','closed actual S16 lifetime required')
            diagnosis=validate(actual,converted,browser,device,first_write)
            original=originals[kind].read_bytes()
            interval=actual[diagnosis['predicted_source_offset_frames']*4:]
            compared=min(len(interval),len(source))
            streams.append(dict(kind=kind,**diagnosis,
                                source_interval_pcm_equal=interval[:compared]==source[:compared],
                                whole_original_equal=None if browser['skip'] else actual==original,
                                whole_original_comparison_scope=('not compared: full original has different cancellation inputs'
                                                                 if browser['skip'] else 'literal complete full-intro device streams'),
                                actual_bytes=len(actual),
                                actual_sha256=sha(capture/'audio'/row['file']),original_bytes=len(original),
                                original_sha256=sha(originals[kind])))
            offset=diagnosis['predicted_source_offset_frames']*4
            for label,mutated in [('source-bit',actual[:offset]+bytes([actual[offset]^1])+actual[offset+1:]),
                                  ('extra-leading-silence',b'\0'*4+actual),('muted-device',b'\0'*len(actual)),
                                  ('missing-source',actual[:offset])]:
                try:validate(mutated,converted,browser,device,first_write)
                except RuntimeError:negative.append(dict(capture=str(capture),kind=kind,mutation=label))
                else:raise RuntimeError('accepted changed device data: '+label)
            altered=copy.deepcopy(browser);altered['events'][0]['scheduled_time']+=1/22050
            try:validate(actual,converted,altered,device,first_write)
            except RuntimeError:negative.append(dict(capture=str(capture),kind=kind,mutation='source-clock'))
            else:raise RuntimeError('accepted changed source clock')
        cases.append(dict(capture=str(capture),capture_report_sha256=sha(capture/'report.json'),
                          wasm_sha256=observed['wasm_sha256'],skip=browser['skip'],frames=len(frames),
                          source_events=browser['events'],device_pcm=streams))
    report=dict(scope=__doc__,observations_valid=True,original_port_parity='unproven',
                source_pcm_sha256=hashlib.sha256(source).hexdigest(),
                chromium_converter_prediction_sha256=hashlib.sha256(converted).hexdigest(),
                primary_sources=provenance,cases=cases,negative_controls=negative)
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(output)
    print('Observed Chromium startup/source-clock/converter predictions:',len(cases),'cases;',len(negative),
          'changed-data/clock controls rejected; original PCM equality reported independently',flush=True)


if __name__=='__main__':main()
