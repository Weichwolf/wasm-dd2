#!/usr/bin/env python3
"""Capture actual Chromium ALSA movie output with a real-time virtual device.

Disable Playwright's default muting, isolate Pulse discovery, and negotiate
the same 22050-Hz stereo device as the original movie reference. Chromium's
ordinary backend performs its own conversion/buffering. No browser output is
shifted or replaced. This is a device diagnostic, not original parity proof.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

from artifacts import WORK, check_space, prepare_output, run_bounded
from reference.audio import build_audio, summarize_audio
from verify_configuration_persistence import ROOT, require


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,required=True)
    parser.add_argument('--movie',choices=('Intro.avi','Outro.avi'),default='Intro.avi')
    parser.add_argument('--skip',action='store_true')
    parser.add_argument('--clock-profile',action='store_true',help='bracket browser performance time with native monotonic time and observe actual movie clocks')
    parser.add_argument('--source-delay-ms',type=int,choices=range(201),default=0,metavar='0..200',
                        help='declared main-thread work before scheduling the unchanged AudioBuffer source')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();output=prepare_output(args.output)
    require(WORK in output.parents,'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=False);(output/'audio').mkdir()
    libraries=build_audio(output/'audio-libraries')
    if args.clock_profile:
        probe=output/'clock_probe.c'
        probe.write_text('#include <time.h>\n#include <stdio.h>\n#include <stdint.h>\n'
                         'int main(void){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return 1;'
                         'printf("%llu\\n",(unsigned long long)((uint64_t)t.tv_sec*1000000000+t.tv_nsec));return 0;}\n')
        subprocess.run(['gcc','-O2','-Wall','-Wextra','-Werror',str(probe),'-o',str(output/'clock-probe')],check=True)
    (output/'asound.conf').write_text(f'pcm_type.dd2clock {{ lib "{output}/audio-libraries/$LIB/dd2_clock.so" }}\n'
                                     'pcm.!default { type dd2clock }\n')
    env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    env.update(PULSE_SERVER='unix:'+str(output/'no-pulse'),ALSA_CONFIG_PATH=str(output/'asound.conf'),
               DD2_AUDIO_CAPTURE=str(output/'audio'),DD2_AUDIO_PROCESS='chromium',DD2_AUDIO_RATE='22050',
               LD_PRELOAD=str(libraries[1]/'dd2_audio.so'))
    command=['node',str(ROOT/'tools/browser/capture_movie_device.js'),str(args.build.resolve()),str(output),args.movie]
    if args.skip:command.append('--skip')
    if args.clock_profile:command.append('--clock-profile')
    if args.source_delay_ms:command.append('--source-delay-ms='+str(args.source_delay_ms))
    with (output/'run.log').open('w') as log:
        run_bounded(command,directory=output,env=env,timeout=180,check=True,stdout=log,stderr=subprocess.STDOUT)
    observed=json.loads((output/'browser.json').read_text())
    require(observed['observations_valid'] and observed['device_closed_before_browser_shutdown'] and
            observed['wasm_sha256']==sha(args.build/'index.wasm'),'browser/build observation binding differs')
    audio=summarize_audio(output/'audio',require_played=True)
    require(len(audio['streams'])==len(audio['played_streams'])==1 and
            all(r['closed'] and r['format']=='S16_LE' and r['rate']==22050 and r['channels']==2
                for r in [*audio['streams'],*audio['played_streams']]),'complete unique Chromium S16 device lifetime required')
    report=dict(scope=__doc__,observations_valid=True,original_port_parity='unproven',
                movie=args.movie,skip=args.skip,original_movie_sha256=sha(ROOT/'DestructionDerby2'/args.movie),
                browser_report_sha256=sha(output/'browser.json'),wasm_sha256=observed['wasm_sha256'],audio=audio,
                audio_observer_source_sha256=sha(ROOT/'tools/reference/wine_audio.c'),
                sample_clock_source_sha256=sha(ROOT/'tools/reference/alsa_clock.c'))
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(output)
    print('Actual closed Chromium movie device:',args.movie,'accepted',audio['streams'][0]['accepted_frames'],
          'played',audio['played_streams'][0]['played_frames'],'source/original comparison pending',flush=True)


if __name__=='__main__':main()
