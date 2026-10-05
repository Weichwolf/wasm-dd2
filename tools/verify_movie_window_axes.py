#!/usr/bin/env python3
"""Compare every old/current window byte for complete original movies and transforms.

Native ASan/UBSan and WASM use the production AVI decoder and window renderer.
The pre-axis implementation is reconstructed from the current production
source and the reverse patch, not a reference image injected into an engine.
Recorded elapsed rendering times are diagnostics, not timing acceptance.
Unchanged pixels do not establish common live original/port audio clocks.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

from artifacts import WORK, check_space, prepare_output, run_bounded
from verify_configuration_persistence import ROOT, require
from verify_movie_avi import original_metadata


def sha(path):
    with path.open('rb') as file: return hashlib.file_digest(file, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args();output = prepare_output(args.output)
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False)
    production = output/'production';production.mkdir()
    originals = [ROOT/'build'/f'dd2_{name}.c' for name in ('movie_surface','avi','cinepak','msadpcm')]
    # Compile one immutable source snapshot; another build may recreate build/.
    for path in [*originals, *sorted((ROOT/'build').glob('*.h'))]:
        shutil.copy2(path,production/path.name)
    surface = production/'dd2_movie_surface.c';before = output/'dd2_movie_surface.c'
    patch = ROOT/'patches/879-movie-window-axis-transform.diff'
    before.write_bytes(surface.read_bytes())
    subprocess.run(['patch', '--batch', '--fuzz=0', '-R', '-p1', '-i', str(patch)], cwd=output, check=True)
    fixture = ROOT/'tools/movie_window_axes_test.c'
    codecs = [production/f'dd2_{name}.c' for name in ('avi','cinepak','msadpcm')]
    common = ['-std=gnu99','-O2','-Wall','-Wextra','-Werror','-I'+str(production)]
    renames = ['-Ddd2_movie_render_window=dd2_movie_render_window_before', '-Ddd2_movie_render=dd2_movie_render_before']
    targets = []
    for name, compiler, options, runner, suffix in [
            ('native-asan-ubsan', 'gcc', ['-m32','-no-pie','-fsanitize=address,undefined','-fno-sanitize-recover=all'], [], ''),
            ('wasm', 'emcc', [], ['node'], '.js')]:
        old = output/(name+'-before.o');binary = output/(name+suffix)
        subprocess.run([compiler,*common,*options,*renames,'-c',str(before),'-o',str(old)],check=True)
        wasm_options = ['-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sINITIAL_MEMORY=67108864',
                        '-sALLOW_MEMORY_GROWTH=1','-sMAXIMUM_MEMORY=268435456'] if name == 'wasm' else []
        subprocess.run([compiler,*common,*options,str(old),str(surface),*map(str,codecs),str(fixture),
                        *wasm_options,'-o',str(binary)],check=True)
        targets.append((name,[*runner,str(binary)]))
    report = dict(scope=__doc__, original_port_live_parity='unproven', before_source_sha256=sha(before),
                  sources={str(p.relative_to(ROOT)):sha(production/p.name) for p in originals},
                  fixture_source_sha256=sha(fixture),patch_sha256=sha(patch),cases=[])
    env = {k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    for filename in ('Intro.avi','Outro.avi'):
        movie=ROOT/'DestructionDerby2'/filename;metadata,_,_,_=original_metadata(movie)
        for name,command in targets:
            log=output/(Path(filename).stem.lower()+'-'+name+'.log')
            with log.open('w') as file:
                run_bounded([*command,str(movie)],directory=output,timeout=600,check=True,
                            env=env,stdout=file,stderr=subprocess.STDOUT)
            lines=log.read_text().splitlines();require(len(lines)==1,'sanitizer or unexpected fixture diagnostics')
            observed=json.loads(lines[0])
            require(observed['avi_frames']==metadata['frames'] and observed['synthetic_cases']==74 and
                    observed['exact_frames']==metadata['frames']+74 and
                    observed['exact_argb_bytes']==(metadata['frames']+74)*640*480*4 and
                    observed['changed_bit_rejected'] and observed['invalid_rectangles_rejected'],
                    'complete movie axes comparison failed')
            report['cases'].append(dict(movie=filename,avi_sha256=sha(movie),target=name,actual=observed))
            (output/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(output)
            print('PASS old/current window bytes:',filename,name,observed,flush=True)
    report['pass_']=True;(output/'report.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__ == '__main__': main()
