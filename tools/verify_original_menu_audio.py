#!/usr/bin/env python3
"""Compare complete bounded original menu-device PCM with production port sources.

Offsets come from original mixer blocks and the consumed ALSA probe. This
diagnoses original mixing and FIR bytes, including startup effect/CD overlap,
without searching PCM for an alignment. It does not prove live
original/port start times, identical-input behavior, intro output or physical DAC
equivalence. Every captured menu-device sample is compared, including silence.
"""
import argparse
from array import array
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import wave

from artifacts import WORK, prepare_output, check_space, open_files

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference'))
from audio import summarize_audio
from capture import EXE_SHA256
from mixer_timeline import original_menu_timeline
SCOPE = ('Complete bounded original menu-device PCM with independently rendered '
         'production port sources at independently traced sample positions; live input/clock and '
         'intro/hardware equivalence pending')


def digest(data):
    return hashlib.sha256(data).hexdigest()


def source_wave(game):
    archive = (game/'Dirinfo').read_bytes()
    if digest(archive) != '03c6ca7adc5e616a4784a82f3b1b7a1f85489e97318b7e01f867e65504d5f22b':
        raise ValueError('Original Dirinfo changed')
    rows = [archive[i:i+24] for i in range(0,0x2808,24)]
    row = next(row for row in rows if row[:18].split(b'\0')[0] == b'VAGS\\BANK1.SBK')
    sector, size = struct.unpack_from('<HI',row,18)
    bank = archive[sector*2048:sector*2048+size]
    if struct.unpack_from('<I',bank,12)[0] != 45:
        raise ValueError('Unexpected original sound bank')
    # Rotate_Slab_On @0x450ecc calls Play_Sound(-1,0x939410,42,4095,2048,0).
    # Load_Game_Vags @0x448310 copies these original 28-byte BANK1 entries.
    offset = struct.unpack_from('<I',bank,16+42*28)[0]
    riff = bank[offset:offset+struct.unpack_from('<I',bank,offset+4)[0]+8]
    with wave.open(io.BytesIO(riff)) as wav:
        if (wav.getnchannels(),wav.getsampwidth(),wav.getframerate(),wav.getnframes()) != (1,1,11025,6314):
            raise ValueError('Unexpected slab source format')
        raw = wav.readframes(wav.getnframes())
    return raw, dict(bank_index=42,frames=6314,rate=11025,bits=8,channels=1,
                    riff_sha256=digest(riff),source_sha256=digest(raw))


def trace_sources(path):
    text = path.read_text(errors='replace')
    buffers, controls, plays = [], {}, []
    current = None
    for line in text.splitlines():
        match = re.search(r'DirectSoundDevice_CreateSoundBuffer \(formattag=0x([0-9a-f]+),chans=(\d+),samplerate=(\d+),.*bitspersamp=(\d+)',line)
        if match:
            tag, channels, rate, bits = (int(v,16) if i==0 else int(v) for i,v in enumerate(match.groups()))
            current = dict(tag=tag,channels=channels,rate=rate,bits=bits)
        match = re.search(r'secondarybuffer_create Created buffer at ([0-9A-F]+)',line)
        if match:
            if current is None:raise ValueError('Missing traced source format')
            obj = match[1]
            buffers.append(dict(object=obj,**current))
            controls[obj] = dict(position=0,pan=0,volume=0,frequency=current['rate'])
        match = re.search(r'IDirectSoundBufferImpl_(SetCurrentPosition|SetPan|SetVolume|SetFrequency) \(([0-9A-F]+),(-?\d+)\)',line)
        if match:
            key = dict(SetCurrentPosition='position',SetPan='pan',SetVolume='volume',SetFrequency='frequency')[match[1]]
            controls[match[2]][key] = int(match[3])
        match = re.search(r'IDirectSoundBufferImpl_Play \(([0-9A-F]+),00000000,00000000,([0-9A-F]+)\)',line)
        if match:
            plays.append(dict(object=match[1],flags=int(match[2],16),**controls[match[1]],trace=line))
    if len(buffers)<46 or (len(buffers)-1)%45 or len(plays)!=2:
        raise ValueError('Expected complete 45-source bank generations, one CD buffer and exactly two plays')
    effect, cd = buffers[-46+42], buffers[-1]
    wanted = [dict(object=effect['object'],flags=0,position=0,pan=0,volume=-1,frequency=5512),
              dict(object=cd['object'],flags=1,position=0,pan=0,volume=0,frequency=44100)]
    if [{k:v for k,v in play.items() if k!='trace'} for play in plays] != wanted:
        raise ValueError('Original source controls differ; no fitting of controls allowed')
    if effect != dict(object=effect['object'],tag=1,channels=1,rate=11025,bits=8) or cd != dict(object=cd['object'],tag=1,channels=2,rate=44100,bits=16):
        raise ValueError('Unexpected actual source formats')
    return dict(trace_sha256=digest(text.encode()),bank_generations=(len(buffers)-1)//45,
                effect=effect,cd=cd,plays=plays)


def compare_pcm(actual, expected):
    if len(actual)!=len(expected):raise ValueError('Partial/truncated original menu PCM')
    if actual!=expected:
        first = next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b)
        raise ValueError(f'Original menu mix differs at byte {first}, frame {first//8}')


def recompose(original, effect, music, starts):
    if not original or len(original)%8 or len(effect)%8 or len(music)%8:
        raise ValueError('Missing or partial stereo Float32 source')
    start,cd_start = starts['effect'],starts['cd']
    if not start<cd_start<len(original)//8 or cd_start+len(music)//8<len(original)//8:
        raise ValueError('Invalid traced source order/extent')
    rendered = bytearray(len(original))
    for frame in range(len(original)//8):
        fx = struct.unpack_from('<ff',effect,(frame-start)*8) if start<=frame<start+len(effect)//8 else (0.0,0.0)
        cd = struct.unpack_from('<ff',music,(frame-cd_start)*8) if frame>=cd_start else (0.0,0.0)
        # Exactly one Float32 addition in original creation order. A sum of two
        # Float32 values is exact in Python's double before rounding back to f32.
        struct.pack_into('<ff',rendered,frame*8,fx[0]+cd[0],fx[1]+cd[1])
    compare_pcm(original,rendered)
    return dict(compared_frames=len(original)//8,compared_bytes=len(original),
                effect_start_frames=start,cd_start_frames=cd_start,
                original_sha256=digest(original),rendered_sha256=digest(rendered))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,help='fresh original --mode audio --wine-debug=-all,+timestamp,+dsound capture; otherwise record now')
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--game-dir',type=Path,default=ROOT/'DestructionDerby2')
    parser.add_argument('--emcc',default='emcc')
    parser.add_argument('--node',default='node')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:parser.error('output must be under /tmp/wasm-dd2')
    output.mkdir(parents=True,exist_ok=False)
    capture = args.capture.resolve() if args.capture else output/'original'
    check_space(output)
    if not args.capture:
        subprocess.run([sys.executable,str(ROOT/'tools/reference/capture.py'),'--game-dir',str(args.game_dir),
            '--mode','audio','--audio','--audio-tail','3','--wine-debug=-all,+timestamp,+dsound','--output',str(capture)],check=True,timeout=120)
    checkpoint = json.loads((capture/'checkpoint.json').read_text())
    if checkpoint['exe_sha256']!=EXE_SHA256 or checkpoint['exe_modified'] or checkpoint['mode']!='audio':
        raise ValueError('Unmodified live original audio capture required')
    if checkpoint['end_state']['level']!=0 or checkpoint['end_state']['movie'] or checkpoint['end_state']['cd']!=dict(enabled=1,playing=1,**{'from':13,'to':14}):
        raise ValueError('Expected original menu/CD13 state')
    trace = trace_sources(capture/'wine.log')
    summary = summarize_audio(capture/'audio',write=False,require_played=True)
    if len(summary['played_streams'])!=2:raise ValueError('Expected intro and menu devices')
    stream = summary['played_streams'][-1]
    if (stream['format'],stream['rate'],stream['channels'],stream['frame_bytes'])!=('FLOAT_LE',44100,2,8):
        raise ValueError('Unexpected actual menu output format')
    original = (capture/'audio'/stream['file']).read_bytes()
    accepted_stream = summary['streams'][-1]
    accepted = (capture/'audio'/accepted_stream['file']).read_bytes()
    timeline = original_menu_timeline(capture/'wine.log',trace,capture/'audio'/stream['events'],
                                     capture/'audio'/accepted_stream['events'],stream,accepted_stream)
    raw,source = source_wave(args.game_dir)
    (output/'slab.raw').write_bytes(raw)
    cdda = array('h');cdda.frombytes((args.game_dir/'Redbook/track13.cdda').read_bytes())
    if sys.byteorder!='little':cdda.byteswap()
    music = array('f',(sample/32768.0 for sample in cdda))
    if sys.byteorder!='little':music.byteswap()
    music = music.tobytes()
    common = ['-std=gnu89','-w','-DDD2_NO_FOPEN_WRAP','-ffunction-sections','-fdata-sections',
              '-fno-strict-aliasing','-I'+str(ROOT/'re_out'),str(ROOT/'re_out/dd2h_stubs.c'),
              str(ROOT/'tools/menu_audio_source_test.c'),'-Wl,--gc-sections']
    subprocess.run(['gcc','-m32','-no-pie','-O2',*common,'-o',str(output/'source-native')],check=True)
    commands = [('native',[str(output/'source-native')])]
    for optimization in ('O0','O2'):
        js = output/f'source-{optimization}.js'
        subprocess.run([args.emcc,'-'+optimization,*common,'-sNODERAWFS=1','-sEXIT_RUNTIME=1',
            '-sGLOBAL_BASE=10485760','--pre-js',str(ROOT/'tools/node_env.js'),'-o',str(js)],check=True)
        commands.append(('wasm-'+optimization,[args.node,str(js)]))
    report = dict(scope=SCOPE,pass_=False,exe_sha256=EXE_SHA256,source=source,
                  original_capture=str(capture),original_device=stream,original_api=trace,
                  original_timeline=timeline,targets={})
    for target,command in commands:
        pcm = output/f'{target}.pcm'
        run = subprocess.run([*command,str(output/'slab.raw'),str(pcm)],check=True,text=True,capture_output=True,timeout=30)
        effect = pcm.read_bytes()
        if len(effect)!=66150*8 or not effect[:512].strip(b'\0') or any(effect[52000*8:]):
            raise ValueError('Incomplete source render/completion')
        result = recompose(original,effect,music,timeline['starts'])
        result['accepted'] = recompose(accepted,effect,music,timeline['starts'])
        if accepted[:len(original)]!=original:
            raise ValueError('Consumed samples differ from the independently validated accepted FIFO prefix')
        result['controls'] = json.loads(run.stdout)
        result['source_render_sha256'] = digest(effect)
        report['targets'][target] = result
        print(f'PASS {target}: all {len(original)} original menu-device bytes, including startup effect/CD overlap',flush=True)
        check_space(output)
    # Prove the comparison keeps initial silence, both channels, the overlap
    # and final CD samples. Source controls and offsets are never refitted here.
    offsets = report['targets']['native']
    for name,frame in [('prefix',0),('effect',offsets['effect_start_frames']+32),
                       ('overlap',offsets['cd_start_frames']+4000),('tail',len(original)//8-1)]:
        damaged = bytearray(original);damaged[frame*8+4]^=1
        try:recompose(damaged,effect,music,timeline['starts'])
        except ValueError:pass
        else:raise RuntimeError('Accepted changed '+name)
    try:recompose(original[:-1],effect,music,timeline['starts'])
    except ValueError:pass
    else:raise RuntimeError('Accepted truncated output')
    for name in ('effect','cd','both'):
        shifted = dict(timeline['starts'])
        for source in ('effect','cd'):
            if name in (source,'both'):shifted[source]+=1
        try:recompose(original,effect,music,shifted)
        except ValueError:pass
        else:raise RuntimeError('Accepted a one-sample source shift: '+name)
    # Damage the actual recorded clock metadata. The complete comparison must
    # reject even a legal-sized block count whose changed prefix shifts both
    # starts; looking up a new match in PCM would hide this error.
    lines = (capture/'wine.log').read_text().splitlines()
    first_block = timeline['blocks'][0]['line']-1
    cursor = next(i for i,line in enumerate(lines) if 'DSOUND_MixInBuffer sec_mixpos=0/6314' in line)
    bad_log = output/'damaged-mixer.log'
    for name in ('missing-primary-block','short-primary-block','source-cursor','source-size','missing-source-cursor'):
        changed = list(lines)
        if name=='missing-primary-block':del changed[first_block]
        elif name=='short-primary-block':
            changed[first_block] = changed[first_block].replace(
                f"(frames {timeline['blocks'][0]['frames']})",f"(frames {timeline['blocks'][0]['frames']-1})")
        elif name=='source-cursor':changed[cursor] = changed[cursor].replace('sec_mixpos=0/6314','sec_mixpos=1/6314')
        elif name=='source-size':changed[cursor] = changed[cursor].replace('/6314','/6315')
        else:del changed[cursor]
        bad_log.write_text('\n'.join(changed)+'\n')
        try:
            damaged = original_menu_timeline(bad_log,trace,capture/'audio'/stream['events'],
                                            capture/'audio'/accepted_stream['events'],stream,accepted_stream)
            recompose(original,effect,music,damaged['starts'])
        except ValueError:pass
        else:raise RuntimeError('Accepted damaged actual mixer timeline: '+name)
    bad_log.unlink()
    report.update(pass_=True,corruption_cases=5,source_shift_cases=3,mixer_metadata_corruption_cases=5)
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    opened = open_files()
    completed = [output/'slab.raw',*output.glob('*.pcm')]
    if not args.capture:
        completed.extend((capture/'audio').glob('*.pcm'))
    for path in completed:
        stat = path.stat()
        if (stat.st_dev,stat.st_ino) not in opened:path.unlink()
    print('Source positions come from mixer/device events; original/port live input scheduling remains unproven.',flush=True)


if __name__=='__main__':
    main()
