"""Compare bounded video from the same original/port runs as replay audio.

Every presentation, indexed byte and palette byte is checked without realignment.
Browser records additionally prove actual canvas pixels and per-frame clock
positions. Original unattached palettes, intro, physical display/timing and other
scenarios remain outside the proof. No captured frame/state is applied to a port.
"""
import copy
import hashlib
import json
from pathlib import Path

from PIL import Image
from reference.video_frames import observe,BYTES,HEADER
from reference.video_archive import Records
from verify_configuration_persistence import require

FIELDS=('flip','level','cf','movie','poly_list','restart_cd_audio','ticks','replay','quit','script_cursor','clock_calls')


def equal_bytes(original,actual,label):
    require(original==actual,label+' bytes differ')


def match_browser(original,actual,rgba_sha256):
    require(all(actual[k]==original[k] for k in FIELDS),'browser presentation state/clock differs')
    require(actual['framebuffer_sha256']==original['framebuffer_sha256'] and
            actual['palette_sha256']==original['palette_sha256'] and actual['original_bytes_compared'] is True,
            'browser original indexed/palette byte comparison differs')
    require(actual['canvas_rgba_sha256']==rgba_sha256,'actual browser canvas pixels differ')


def verify(original,reference,targets,services):
    source=observe(original)
    require(json.loads(reference.read_text())==source,'original video differs from independent trace observation')
    require(source['trace_sha256']==services['trace_sha256'] and source['game_clock_sha256']==services['clock_sha256'],
            'video and audio must come from the same original trace')
    end=services['completion_position']['flip'];require(0<end<=source['frame_count'],'complete bounded original video required')
    video={};results={};negative={}
    for name,(directory,checkpoint) in targets.items():
        if name=='browser':
            proof=checkpoint['video']
            require(proof['pass_'] and proof['original_video_sha256']==source['video_sha256'] and
                    proof['original_trace_sha256']==source['trace_sha256'],'browser video provenance differs')
            frames=proof['frames']
        else:frames=[json.loads(line) for line in (directory/'video/presentations.jsonl').read_text().splitlines()]
        require(len(frames)>=end and all(row['index']==i for i,row in enumerate(frames)),
                'missing, duplicated or reordered port presentation')
        if name=='browser':require(len(frames)==end,'browser bounded video extent differs')
        video[name]=frames
        results[name]=dict(pass_=True,frames=end,bytes_per_frame=307200,palette_bytes_per_frame=1024,
            actual_canvas=name=='browser',per_frame_clock_observed=name=='browser',excluded_after_audio_endpoint=len(frames)-end,
            frame_hashes=[])
    with Records(original) as raw:
        for index in range(end):
            record=raw[index];require(len(record)==BYTES,'original video truncated')
            pixels=record[HEADER.size:HEADER.size+307200];palette=record[HEADER.size+307200:HEADER.size+308224]
            expected=source['frames'][index]
            image=Image.frombytes('P',(640,480),pixels)
            image.putpalette(bytes(component for i in range(256) for component in palette[i*4:i*4+3]))
            rgba_sha=hashlib.sha256(image.convert('RGBA').tobytes()).hexdigest()
            for name,(directory,checkpoint) in targets.items():
                row=video[name][index]
                if name=='browser':match_browser(expected,row,rgba_sha)
                else:
                    require(all(row[k]==expected[k] for k in ('level','cf','poly_list','restart_cd_audio')),
                            name+' presentation state differs')
                    equal_bytes(pixels,(directory/f'video/f{index:05d}.bin').read_bytes(),name+' indexed frame '+str(index))
                    equal_bytes(palette,(directory/f'video/f{index:05d}.pal').read_bytes(),name+' palette '+str(index))
                results[name]['frame_hashes'].append(dict(index=index,framebuffer_sha256=expected['framebuffer_sha256'],
                                                        palette_sha256=expected['palette_sha256']))
            if index==0:
                for case,data in (('indexed-pixel',pixels),('palette-color',palette)):
                    bad=bytearray(data);bad[len(data)//2]^=1
                    try:equal_bytes(data,bad,case)
                    except RuntimeError:negative[case]=dict(rejected=True)
                    else:raise AssertionError('damaged '+case+' accepted')
                try:equal_bytes(pixels,pixels[:-1],'truncated image')
                except RuntimeError:negative['truncated-image']=dict(rejected=True)
                else:raise AssertionError('truncated image accepted')
                for case,field in [('presentation','flip'),('clock','clock_calls'),('canvas','canvas_rgba_sha256'),
                                   ('palette','palette_sha256'),('byte-comparison','original_bytes_compared')]:
                    bad=copy.deepcopy(video['browser'][0])
                    bad[field]=bad[field]+1 if field in ('flip','clock_calls') else False if field=='original_bytes_compared' else '0'*64
                    try:match_browser(expected,bad,rgba_sha)
                    except RuntimeError:negative[case]=dict(rejected=True)
                    else:raise AssertionError('damaged browser '+case+' accepted')
    return dict(scope=__doc__.strip(),pass_=True,original_video_sha256=source['video_sha256'],
        original_trace_sha256=source['trace_sha256'],bounded_frames=end,
        racing_frames=sum(f['level']!=0 for f in source['frames'][:end]),
        attached_device_palettes=sum(f['device_palette_observed'] for f in source['frames'][:end]),
        missing_device_palette_frames=source['missing_device_palette_frames'],
        original_frames_after_audio_endpoint=source['frame_count']-end,targets=results,negative_cases=negative)
