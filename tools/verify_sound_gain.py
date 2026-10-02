#!/usr/bin/env python3
"""Compare Float32 gain samples with real Wine DirectSound and both ports.

The reference uses a constant mono source at the device rate, so its amplitude
comparison needs no waveform phase guessing. Startup/trailing silence is checked
and reported separately; this does not establish stream-start/timing parity,
resampler fidelity, original game full-output parity or Windows hardware parity.
"""
import argparse
import hashlib
import json
import os
import shutil
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from generate_sound_gain import gains, render, OUTPUT
from verify_sound_cursor import wine_probe

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools/reference"))
from audio import build_audio, summarize_audio

CASES = [(0,0),(-1,0),(-599,0),(-600,0),(-601,0),(-1200,0),(-9599,0),(-9600,0),
         (-10000,0),(-1376,600),(-1376,-600),(-1376,10000),(-1376,-10000)]
SILENCE = bytes(8)


def f32(number):
    return struct.unpack("<f",struct.pack("<f",number))[0]


def frame(table, volume, pan):
    def gain(db):
        return 1.0 if db>=0 else 0.0 if db<=-9600 else f32(table[-db]/65535)
    return struct.pack("<ff",f32(0.5*gain(volume-max(pan,0))),
                       f32(0.5*gain(volume+min(pan,0))))


def sweep_pcm(table):
    data = bytearray(frame(table,0,0)*220)
    now = 10
    for volume,pan in [(-i,0) for i in range(10001)] + [(-1376,p) for p in (600,-600,3000,-3000,10000,-10000)]:
        count = (now+1)*22050//1000 - now*22050//1000
        data.extend(frame(table,volume,pan)*count)
        now += 1
    # Products and additions are individually rounded to Float32. The final sum
    # remains above 1.0; no per-source integer saturation is allowed.
    gain = f32(table[1376]/65535)
    mixed = f32(f32(0.5+32767/32768)+f32((-16385/32768)*gain))
    assert mixed > 1.0
    count = (now+1)*22050//1000 - now*22050//1000
    data.extend(struct.pack("<ff",mixed,mixed)*count)
    return bytes(data)


def reference_frames(directory):
    report = summarize_audio(directory)
    payload = bytearray()
    for stream in report["streams"]:
        if (stream["format"],stream["rate"],stream["channels"],stream["frame_bytes"]) != ("FLOAT_LE",22050,2,8):
            raise RuntimeError("Wine gain fixture committed an unexpected device format")
        payload.extend((directory/stream["file"]).read_bytes())
    return [bytes(payload[i:i+8]) for i in range(0,len(payload),8)],hashlib.sha256(payload).hexdigest()


def validate_reference(directory, expected):
    frames,sha = reference_frames(directory)
    if any(item not in (SILENCE,expected) for item in frames):
        raise RuntimeError("Wine gain sample differs; no amplitude tolerance is allowed")
    begin,end = 0,len(frames)
    if expected != SILENCE:
        while begin<end and frames[begin]==SILENCE:begin+=1
        while end>begin and frames[end-1]==SILENCE:end-=1
        if end-begin<220 or any(item!=expected for item in frames[begin:end]):
            raise RuntimeError("Missing/interrupted reference tone")
    return {"accepted_frames":len(frames),"leading_silence_frames":begin if expected!=SILENCE else None,
            "tone_frames":end-begin if expected!=SILENCE else None,
            "trailing_silence_frames":len(frames)-end if expected!=SILENCE else None,
            "expected_frame_hex":expected.hex(),"sample_bytes_sha256":sha}


def validate_mix(directory, table):
    frames,sha = reference_frames(directory)
    parts = [0.5,32767/32768,f32((-16385/32768)*f32(table[1376]/65535))]
    allowed = set()
    for mask in range(8):
        total = 0.0
        for index,part in enumerate(parts):
            if mask&(1<<index):total=f32(total+part)
        allowed.add(struct.pack("<ff",total,total))
    peak = f32(f32(parts[0]+parts[1])+parts[2])
    expected = struct.pack("<ff",peak,peak)
    if any(item not in allowed for item in frames):
        raise RuntimeError("Wine three-source mix differs from an exact source combination")
    # Source starts/stops can straddle a mixer period; report these phases
    # instead of claiming stream-start equality. Require the full mixed plateau.
    full = sum(item==expected for item in frames)
    if peak<=1.0 or full<220:raise RuntimeError("Wine failed to preserve the unclipped Float32 mixed plateau")
    return {"accepted_frames":len(frames),"fully_mixed_frames":full,
            "mixed_frame_hex":expected.hex(),"sample_bytes_sha256":sha}


def validate_negative_captures(output, table):
    # Mutate copies of real captures: prove the PCM gates reject an ULP error,
    # a lost audible tone and integer saturation of the mixed waveform.
    expected=frame(table,-600,0)
    with tempfile.TemporaryDirectory(prefix="dd2-gain-negative-") as temporary:
        root=Path(temporary)
        for defect in ("one-bit","all-silent","clipped-mix"):
            source=output/("three-source-mix" if defect=="clipped-mix" else "volume-600-pan0")/"audio"
            broken=root/defect;shutil.copytree(source,broken)
            files=sorted(broken.glob("*.pcm"))
            mutated=False
            for pcm in files:
                raw=bytearray(pcm.read_bytes())
                if defect=="one-bit":
                    offset=raw.find(expected)
                    if offset<0:continue
                    raw[offset]^=1;mutated=True
                elif defect=="all-silent":
                    raw[:]=bytes(len(raw));mutated=True
                else:
                    gain=f32(table[1376]/65535)
                    peak=f32(f32(0.5+32767/32768)+f32((-16385/32768)*gain))
                    original=struct.pack("<ff",peak,peak)
                    if original not in raw:continue
                    raw=raw.replace(original,struct.pack("<ff",1,1));mutated=True
                pcm.write_bytes(raw)
                if defect=="one-bit":break
            if not mutated:raise RuntimeError(f"Missing real capture for negative check: {defect}")
            try:
                if defect=="clipped-mix":validate_mix(broken,table)
                else:validate_reference(broken,expected)
            except RuntimeError:
                pass
            else:
                raise RuntimeError(f"Accepted damaged gain capture: {defect}")
    print("PASS one-bit corruption, lost tone and clipped reference mix rejected",flush=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--node",default="node")
    parser.add_argument("--emcc",default="emcc")
    parser.add_argument("--wine",action="store_true")
    parser.add_argument("--mingw",default="i686-w64-mingw32-gcc")
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    if OUTPUT.read_text()!=render():raise RuntimeError("Gain table is not reproducible")
    table=gains()
    with tempfile.TemporaryDirectory(prefix="dd2-gain-build-") as temporary:
        directory=Path(temporary)
        output=args.output.resolve() if args.output else directory/"captures"
        output.mkdir(parents=True,exist_ok=False)
        env={k:v for k,v in os.environ.items() if not k.startswith("DD2_")}
        env["DD2_SND_RATE"]="22050" # Gain calibration deliberately avoids a resampler.
        source=ROOT/"tools/sound_gain_test.c"
        common=["-std=gnu89","-w","-DDD2_NO_FOPEN_WRAP","-ffunction-sections","-fdata-sections",
                f"-I{ROOT/'re_out'}",str(ROOT/"re_out/dd2h_stubs.c"),str(source),"-Wl,--gc-sections"]
        native,wasm=directory/"native",directory/"wasm.js"
        subprocess.run(["gcc","-m32","-no-pie",*common,"-o",str(native)],check=True)
        subprocess.run([args.emcc,*common,"-sNODERAWFS=1","-sEXIT_RUNTIME=1","-sGLOBAL_BASE=10485760",
                        "--pre-js",str(ROOT/"tools/node_env.js"),"-o",str(wasm)],check=True)
        report={"scope":"gain/control calibration at matching source/device rate; full stream timing and original game parity pending",
                "cases":[],"volume_sweep":10001,"three_buffer_sum_above_one":True,"wine":args.wine}
        if args.wine:
            executable=directory/"gain.exe"
            subprocess.run([args.mingw,"-Wall","-Wextra","-Werror",str(source),"-ldsound","-o",str(executable)],check=True)
            report["fixture_exe_sha256"]=hashlib.sha256(executable.read_bytes()).hexdigest()
            report["fixture_source_sha256"]=hashlib.sha256(source.read_bytes()).hexdigest()
            libraries=build_audio(directory/"libraries")
            alsa=f'pcm_type.dd2clock {{ lib "{directory}/libraries/$LIB/dd2_clock.so" }}\npcm.!default {{ type dd2clock }}\n'
        for volume,pan in CASES:
            record={"volume":volume,"pan":pan,"expected_frame_hex":frame(table,volume,pan).hex(),"targets":[]}
            case=output/f"volume{volume}-pan{pan}";case.mkdir()
            if args.wine:
                (case/"audio").mkdir()
                capture_env={**env,"DD2_AUDIO_CAPTURE":str(case/"audio"),"DD2_AUDIO_RATE":"22050",
                             "DD2_AUDIO_PROCESS":"gain.exe","LD_PRELOAD":"dd2_audio.so",
                             "LD_LIBRARY_PATH":":".join(map(str,libraries))}
                actual=wine_probe(executable,case,capture_env,alsa_config=alsa,arguments=("-",str(volume),str(pan)))
                shutil.rmtree(case/"wine-prefix")
                if actual!={"volume":volume,"pan":pan,"source_rate":22050,"source_sample":16384}:
                    raise RuntimeError("Wine fixture source/settings differ")
                record["wine"]=validate_reference(case/"audio",frame(table,volume,pan))
            for target,command in (("native",[str(native)]),("wasm",[args.node,str(wasm)])):
                pcm=case/f"{target}.pcm"
                subprocess.run([*command,str(pcm),str(volume),str(pan)],env=env,check=True,capture_output=True,timeout=30)
                expected=sweep_pcm(table) if (volume,pan)==(0,0) else frame(table,volume,pan)*220
                if pcm.read_bytes()!=expected:raise RuntimeError(f"{target} gain/mix bytes differ at {volume}/{pan}")
                if json.loads(pcm.with_suffix(".pcm.json").read_text())!={"format":"FLOAT_LE","rate":22050,"channels":2}:
                    raise RuntimeError("PCM format metadata differs")
                record["targets"].append({"target":target,"pcm_bytes":len(expected)})
            report["cases"].append(record)
            (output/"report.json").write_text(json.dumps(report,indent=2)+"\n")
            print(f"PASS gain {volume}/pan {pan}: {'Wine reference and ' if args.wine else ''}native/WASM exact samples",flush=True)
        if args.wine:
            case=output/"mono8-source";case.mkdir();(case/"audio").mkdir()
            capture_env={**env,"DD2_AUDIO_CAPTURE":str(case/"audio"),"DD2_AUDIO_RATE":"22050",
                         "DD2_AUDIO_PROCESS":"gain.exe","LD_PRELOAD":"dd2_audio.so",
                         "LD_LIBRARY_PATH":":".join(map(str,libraries))}
            actual=wine_probe(executable,case,capture_env,alsa_config=alsa,arguments=("-","-600","0","mono8"))
            shutil.rmtree(case/"wine-prefix")
            if actual!={"volume":-600,"pan":0,"source_rate":22050,"source_sample":192,"mode":"mono8"}:
                raise RuntimeError("Wine mono8 fixture settings differ")
            report["wine_mono8"]=validate_reference(case/"audio",frame(table,-600,0))
            for target,command in (("native",[str(native)]),("wasm",[args.node,str(wasm)])):
                pcm=case/f"{target}.pcm"
                subprocess.run([*command,str(pcm),"-600","0","mono8"],env=env,check=True,capture_output=True,timeout=30)
                if pcm.read_bytes()!=frame(table,-600,0)*220:
                    raise RuntimeError(f"{target} mono8 gain samples differ")
            print("PASS real Wine/native/WASM mono8 normalization and gain",flush=True)
            case=output/"three-source-mix";case.mkdir();(case/"audio").mkdir()
            capture_env={**env,"DD2_AUDIO_CAPTURE":str(case/"audio"),"DD2_AUDIO_RATE":"22050",
                         "DD2_AUDIO_PROCESS":"gain.exe","LD_PRELOAD":"dd2_audio.so",
                         "LD_LIBRARY_PATH":":".join(map(str,libraries))}
            actual=wine_probe(executable,case,capture_env,alsa_config=alsa,arguments=("-","0","0","mix"))
            shutil.rmtree(case/"wine-prefix")
            if actual!={"volume":0,"pan":0,"source_rate":22050,"source_sample":16384,"mode":"mix"}:
                raise RuntimeError("Wine three-source fixture settings differ")
            report["wine_three_source_mix"]=validate_mix(case/"audio",table)
            validate_negative_captures(output,table)
            report["negative_capture_checks"]=["one-bit","all-silent","clipped-mix"]
            (output/"report.json").write_text(json.dumps(report,indent=2)+"\n")
            print("PASS real Wine three-source mix: exact Float32 sums above 1.0",flush=True)
        print("PASS all 10001 volume values, control/error probes, six pan steps and unclipped Float32 mix",flush=True)


if __name__=="__main__":
    main()
