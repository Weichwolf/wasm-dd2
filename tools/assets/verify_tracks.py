#!/usr/bin/env python3
"""Verify owned track providers, moving fields and static provider rendering."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT
from assets.verify_assets import HEADER
from assets.prepare_scenes import compile_scene
from assets.verify_render import image_compare

MODEL_NAMES = ('car-close', 'car-medium', 'car-distant', 'wheel-primary', 'wheel-secondary', 'detached-trunk')


def digest(data):
    return hashlib.sha256(data).hexdigest()


def compare(left, right):
    if isinstance(left, dict):
        return left.keys() == right.keys() and all(compare(left[k], right[k]) for k in left)
    if isinstance(left, list):
        return len(left) == len(right) and all(compare(a, b) for a, b in zip(left, right))
    if isinstance(left, float) or isinstance(right, float):
        return math.isfinite(left) and math.isfinite(right) and math.isclose(left, right, abs_tol=1e-5, rel_tol=1e-8)
    return type(left) is type(right) and left == right


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT / 'assets/runtime')
    parser.add_argument('--sanitized-build', type=Path, required=True)
    parser.add_argument('--reference-archive', type=Path)
    parser.add_argument('--output', type=Path, default=WORK / 'content-provider-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents or WORK not in args.sanitized_build.resolve().parents:
        parser.error('Keep diagnostics/builds under /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    runtime = args.runtime.resolve()
    manifests = json.loads((runtime / 'reference/manifest.json').read_text())
    commands = {'native':[WORK/'rewrite-native/dd2_track_content_export'],
                'wasm':['node',WORK/'rewrite-wasm/dd2_track_content_export.js'],
                'sanitized':[args.sanitized_build/'dd2_track_content_export']}
    previews = {'native':[WORK/'rewrite-native/dd2_prepared_preview'],
                'wasm':['node',WORK/'rewrite-wasm/dd2_prepared_preview.js'],
                'sanitized':[args.sanitized_build/'dd2_prepared_preview']}
    environment = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1:exitcode=86',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1:exitcode=87')
    calls, levels, captures, comparisons = [], [], [], []

    def run(command, name, expected=0, export=False):
        log = output/(name+'.log')
        with log.open('wb') as stream:
            result = run_bounded(list(map(str, command)), directory=output, timeout=120,
                                 stdout=stream, stderr=subprocess.STDOUT, cwd=output, env=environment)
        raw=log.read_bytes()
        calls.append(dict(name=name, returncode=result.returncode, sha256=digest(raw)))
        if result.returncode != expected or b'Sanitizer:' in raw or b'runtime error:' in raw:
            raise RuntimeError(name+': '+raw.decode(errors='replace'))
        if expected and raw:
            raise ValueError('Rejected provider emitted partial output')
        value=json.loads(raw) if export else None
        log.unlink()
        return value

    run([args.sanitized_build/'dd2_track_content_test'],'sanitized-ownership')
    run(['valgrind','--error-exitcode=86','--leak-check=full',WORK/'rewrite-native/dd2_track_content_test'],'memcheck-ownership')
    for level in manifests['levels']:
        code=level['level']
        scene=json.loads((runtime/level['scene']).read_text())
        expected_models=[HEADER.unpack_from((runtime/scene['templates'][name]['model']).read_bytes())[-1]//3 for name in MODEL_NAMES]
        expected_resources=len({item['model'] for item in [*scene['objects'],*scene['templates'].values()]})
        rows={}
        for platform, command in commands.items():
            row=run([*command,'prepared',runtime,code],platform+'-field-'+code,export=True)
            if (not row['valid'] or row['resources']!=expected_resources or row['instances']!=len(scene['objects'])
                or row['models']!=expected_models or len(row['snapshots'])!=2):
                raise ValueError('Incomplete provider inventory/field')
            for index,snapshot in enumerate(row['snapshots']):
                if len(snapshot['vehicles'])!=20 or any(v['steps']!=200+index*120 for v in snapshot['vehicles']):
                    raise ValueError('Actual twenty-car field did not advance the specified prefix')
            rows[platform]=row
        if any(not compare(rows['native'],row) for row in rows.values()):
            raise ValueError('Cross-target provider/moving-field mismatch')
        optional=False
        if args.reference_archive:
            reference=run([*commands['native'],'reference',args.reference_archive.resolve(),code], 'reference-field-'+code, export=True)
            if reference['snapshots']!=rows['native']['snapshots']:
                raise ValueError('Prepared and original-decoded Native field prefixes differ')
            optional=True
        levels.append(dict(level=code,resources=expected_resources,instances=len(scene['objects']),models=expected_models,
                           vehicles=20,settling_steps=200,fixed_steps=120,optional_reference_exact=optional,
                           field_sha256={k:digest(json.dumps(v['snapshots'],sort_keys=True).encode()) for k,v in rows.items()}))
        for mode in ('full','crop'):
            images,stats={},{}
            for platform,command in previews.items():
                image=output/(platform+'-'+code+'-'+mode+'.ppm')
                stats[platform]=run([*command,runtime,code,image,*(['crop'] if mode=='crop' else [])],platform+'-provider-image-'+code+'-'+mode,export=True)
                images[platform]=image
                captures.append(dict(level=code,mode=mode,platform=platform,sha256=digest(image.read_bytes()),stats=stats[platform]))
            if any(v!=stats['native'] for v in stats.values()):raise ValueError('Provider render counters differ')
            for platform in ('wasm','sanitized'):
                comparisons.append(dict(level=code,mode=mode,platform=platform,**image_compare(images['native'],images[platform])))
            old=output/('direct-'+code+'-'+mode+'.ppm')
            direct=run([*previews['native'],runtime,level['compiled_scene'],old,*(['crop'] if mode=='crop' else [])],'direct-scene-'+code+'-'+mode,export=True)
            if direct!=stats['native'] or old.read_bytes()!=images['native'].read_bytes():raise ValueError('Provider changes static scene output')
            if code in ('1','8'):
                from PIL import Image
                with Image.open(images['native']) as image:image.save(output/(code+'-'+mode+'.png'))
            for image in [old,*images.values()]:image.unlink()
        print(json.dumps(dict(level=code,pass_=True)),flush=True)
    fixture=output/'missing-content'
    (fixture/'roads').mkdir(parents=True,exist_ok=True)
    (fixture/'reference/scenes').mkdir(parents=True,exist_ok=True)
    (fixture/'reference/models').mkdir(parents=True,exist_ok=True)
    first=manifests['levels'][0]
    scene=json.loads((runtime/first['scene']).read_text())
    resources={item['model'] for item in [*scene['objects'],*scene['templates'].values()]}
    for resource in resources:(fixture/resource).symlink_to(runtime/resource)
    road_path=fixture/'roads/level-1.dd2road';scene_path=fixture/first['compiled_scene']
    road_bytes=(runtime/'roads/level-1.dd2road').read_bytes();scene_bytes=(runtime/first['compiled_scene']).read_bytes()
    cases=('missing-road','missing-scene','invalid-road','invalid-scene','missing-body','missing-required-template','long-root')
    body=fixture/scene['templates']['car-close']['model']
    for case in cases:
        road_path.write_bytes(road_bytes);scene_path.write_bytes(scene_bytes)
        if case=='missing-road':road_path.unlink()
        if case=='missing-scene':scene_path.unlink()
        if case=='invalid-road':road_path.write_bytes(b'!'+road_bytes[1:])
        if case=='invalid-scene':scene_path.write_bytes(b'!'+scene_bytes[1:])
        if case=='missing-body':body.unlink()
        if case=='missing-required-template':
            altered=json.loads(json.dumps(scene));del altered['templates']['car-close'];scene_path.write_bytes(compile_scene(altered))
        for platform,command in commands.items():run([*command,'prepared','x'*4096 if case=='long-root' else fixture,'1'],platform+'-reject-'+case,expected=1)
        if case=='missing-body':body.symlink_to(runtime/scene['templates']['car-close']['model'])
    for file in sorted(fixture.rglob('*'),key=lambda v:len(v.parts),reverse=True):
        if file.is_file() or file.is_symlink():file.unlink()
        elif file.is_dir():file.rmdir()
    fixture.rmdir()
    sources=['src/assets/track.c','src/assets/track.h','src/platform/content.c','src/platform/content.h',
             'src/game/championship_session.c','src/game/championship_session.h','src/game/application.c',
             'tests/track_content_test.c','tests/track_content_export.c','tests/prepared_preview.c','tools/assets/verify_tracks.py']
    report=dict(pass_=True,verified_at=datetime.now(timezone.utc).isoformat(),
                scope='Prepared track ownership, required templates, short physical prefixes, transactional championship restarts and static provider scenes; no completed championships, normal prepared-app startup, original parity or 60-FPS claim',
                original_inputs_required=False,optional_reference_requested=bool(args.reference_archive),
                sanitizer_scope='Fresh O1 ASan/UBSan instruments reached rewrite C and pinned SoftGL sources; system SDL2 uninstrumented',
                levels=levels,captures=captures,image_comparisons=comparisons,rejection_cases=list(cases),calls=calls,
                source_sha256={name:digest((ROOT/name).read_bytes()) for name in sources})
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(output)
    print(json.dumps(dict(pass_=True,report=str(output/'report.json'))))


if __name__=='__main__':main()
