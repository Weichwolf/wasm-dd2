#!/usr/bin/env python3
"""Check production movie QPC conversion and microsecond frame boundaries.

The conversion oracle follows Wine 10 currenttime_us signed x86 arithmetic;
the cadence oracle follows its snapshot clock and double next-frame deadline.
Native ASan/UBSan and WASM run the real AVI/MCI code with three unmodified
original packets and declared clocks. An old production movie source must
fail the same frame-boundary check. Rendering is replaced by a validating
counter here; this proves neither live pixel/audio output nor original A/V
synchronization. Actual Wine clock-provider checks are separate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

from artifacts import WORK, check_space, prepare_output
from verify_configuration_persistence import ROOT, require
from verify_movie_drain import short_avi


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def wine_us(ns):
    scaled=((ns//100)*1000000)&0xffffffffffffffff
    if scaled>=1<<63:
        scaled-=1<<64
    return (abs(scaled)//10000000)*(1 if scaled>=0 else -1)


def validate_clock(rows):
    require(len(rows)==30,'complete clock inputs required')
    for index,row in enumerate(rows):
        require(row['case']==index%15 and row['fallback']==index//15,
                'clock input identity differs')
        require(row['calls']==([4,1] if row['fallback'] else [4]),'clock provider differs')
        require(row['us']==wine_us(row['source_ns']),'QPC microsecond conversion differs')


def validate_frames(rows):
    epochs=(123456789000,123456789999,4294967295999999,4294967296000000)
    probes=(0,39000000,39211000,39999000,39999999,40000000,79999000,80000000)
    require(len(rows)==32,'complete frame boundary inputs required')
    for index,row in enumerate(rows):
        epoch=epochs[index//8];now=epoch+probes[index%8]
        require(row['epoch_ns']==epoch and row['now_ns']==now,'frame boundary input differs')
        expected=min(2,1+(now//1000-epoch//1000)//40000)
        require(row['frames']==expected,'microsecond frame deadline differs')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--before-movie',type=Path,required=True)
    args=parser.parse_args();output=prepare_output(args.output)
    require(WORK in output.parents,'Use /tmp/wasm-dd2/');output.mkdir(parents=True,exist_ok=False)
    snapshot=output/'production';snapshot.mkdir()
    for p in [*(ROOT/'build').glob('*.h'), *(ROOT/'build'/f'dd2_{n}.c' for n in ('movie','movie_platform','avi','cinepak','msadpcm'))]:
        (snapshot/p.name).write_bytes(p.read_bytes())
    source=snapshot/'movie_precision_test.c';source.write_bytes((ROOT/'tools/movie_precision_test.c').read_bytes())
    before=output/'before-movie.c';before.write_bytes(args.before_movie.read_bytes())
    movie=output/'short.avi';movie.write_bytes(short_avi(ROOT/'DestructionDerby2/Intro.avi',frames=3))
    base=['-O2','-std=gnu99','-ffunction-sections','-fdata-sections','-I'+str(snapshot),'-Wl,--gc-sections']
    sanitizer=['-m32','-no-pie','-fsanitize=address,undefined','-fno-sanitize-recover=all']
    clock=output/'clock'
    subprocess.run(['gcc',*sanitizer,*base,'-DDD2_PRECISION_CLOCK',str(source),
                    str(snapshot/'dd2_movie_platform.c'),'-Wl,--wrap=clock_gettime','-o',str(clock)],check=True)
    def run(command):
        result=subprocess.run(command,capture_output=True,text=True,check=True,timeout=30)
        require('runtime error:' not in result.stderr,'movie precision sanitizer error')
        return [json.loads(s) for s in result.stdout.splitlines()]
    clocks=run([str(clock)]);validate_clock(clocks)
    targets=[]
    units=[str(snapshot/f'dd2_{n}.c') for n in ('avi','cinepak','msadpcm')]
    for label,selected in (('production',snapshot/'dd2_movie.c'),('before',before)):
        for target in ('native','wasm'):
            binary=output/(label+'-'+target+('.js' if target=='wasm' else ''))
            command=['emcc',*base,'-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sINITIAL_MEMORY=67108864'] if target=='wasm' else ['gcc',*sanitizer,*base]
            subprocess.run([*command,*units,str(selected),str(source),'-o',str(binary)],check=True)
            rows=run((['node',str(binary)] if target=='wasm' else [str(binary)])+[str(movie)])
            if label=='before':
                try:validate_frames(rows)
                except RuntimeError as error:
                    require(str(error)=='microsecond frame deadline differs','unexpected old-source failure')
                else:raise RuntimeError('old millisecond source accepted microsecond frame boundary')
            else:validate_frames(rows)
            targets.append(dict(target=target,source=label,binary_sha256=sha(binary),rows=rows,
                                old_source_rejected=label=='before'))
    controls=[]
    for label in ('conversion','signed-wrap','clock-provider','missing-clock','early-frame','missing-frame'):
        rows=json.loads(json.dumps(clocks if label not in ('early-frame','missing-frame') else targets[0]['rows']))
        if label=='conversion':rows[4]['us']+=1
        elif label=='signed-wrap':rows[11]['us']=abs(rows[11]['us'])
        elif label=='clock-provider':rows[0]['calls']=[1]
        elif label in ('missing-clock','missing-frame'):rows.pop()
        else:rows[2]['frames']=2
        try:(validate_frames if label in ('early-frame','missing-frame') else validate_clock)(rows)
        except RuntimeError:controls.append(label)
        else:raise RuntimeError('accepted changed precision evidence: '+label)
    report=dict(scope=__doc__,pass_=True,original_port_av_parity='unproven',
                fixture_sha256=sha(source),verifier_sha256=sha(Path(__file__)),
                platform_sha256=sha(snapshot/'dd2_movie_platform.c'),movie_sha256=sha(snapshot/'dd2_movie.c'),
                original_avi_sha256=sha(ROOT/'DestructionDerby2/Intro.avi'),short_avi_sha256=sha(movie),
                clock_cases=clocks,targets=targets,negative_controls_rejected=controls)
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(output)
    print('PASS 30 QPC conversions, native/ASan/UBSan and WASM microsecond deadlines; old sources and six controls rejected')


if __name__=='__main__':
    main()
