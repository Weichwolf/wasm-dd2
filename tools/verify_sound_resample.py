#!/usr/bin/env python3
"""Compare complete one-shot waveforms against captured real Wine FIR output.

The synthetic source begins at cursor zero and plays to completion. Only device
preroll/trailing silence is separated and reported; every active PCM byte must
match. This is filter/format calibration, not original game stream-start parity.
--wine recaptures the same COM fixture through the clocked, observed ALSA device.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from verify_sound_cursor import wine_probe
from generate_sound_fir import OUTPUT, render

ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "tools/reference/sound_resample_wine10.json"
sys.path.insert(0, str(ROOT / "tools/reference"))
from audio import build_audio, summarize_audio

CASES = [(11025,16,1),(16000,16,1),(22050,16,1),(44100,16,1),
         (88200,16,1),(11025,8,1),(44100,8,2),(16000,16,2)]
SILENCE = bytes(8)


def active_wave(payload, frames):
    if len(payload)%8:raise RuntimeError("Partial stereo Float32 frame")
    begin = 0
    while begin<len(payload) and payload[begin:begin+8]==SILENCE:begin+=8
    end = begin+frames*8
    if end>len(payload) or payload[end-8:end]==SILENCE:
        raise RuntimeError("Truncated/missing active reference waveform")
    if any(payload[end:]):raise RuntimeError("Unexpected audio after complete one-shot waveform")
    wave = payload[begin:end]
    return wave,{"leading_silence_frames":begin//8,"active_frames":frames,
                 "trailing_silence_frames":(len(payload)-end)//8,
                 "active_sha256":hashlib.sha256(wave).hexdigest()}


def reference_wave(directory, frames, rate=22050):
    summary=summarize_audio(directory)
    if len(summary["streams"])!=1:raise RuntimeError("Expected one reference PCM device stream")
    stream=summary["streams"][0]
    if (stream["format"],stream["rate"],stream["channels"],stream["frame_bytes"]) != ("FLOAT_LE",rate,2,8):
        raise RuntimeError("Unexpected reference device format")
    wave,report=active_wave((directory/stream["file"]).read_bytes(),frames)
    report["accepted_frames"]=stream["accepted_frames"]
    return wave,report


def check_wave(wave, expected_hash):
    if hashlib.sha256(wave).hexdigest()!=expected_hash:
        raise RuntimeError("One-shot PCM differs from actual Wine capture; no tolerance allowed")


def check_loop(payload, cycle):
    if len(payload)%8:raise RuntimeError("Partial stereo Float32 frame")
    begin,end=0,len(payload)
    while begin<end and payload[begin:begin+8]==SILENCE:begin+=8
    while end>begin and payload[end-8:end]==SILENCE:end-=8
    if end-begin<220*8:raise RuntimeError("Missing/short loop tone")
    for offset in range(begin,end,8):
        phase=(offset-begin)%len(cycle)
        if payload[offset:offset+8]!=cycle[phase:phase+8]:
            raise RuntimeError("Loop FIR waveform differs from actual Wine cycle")
    return {"leading_silence_frames":begin//8,"active_frames":(end-begin)//8,
            "trailing_silence_frames":(len(payload)-end)//8,"cycle_hex":cycle.hex()}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--node",default="node")
    parser.add_argument("--emcc",default="emcc")
    parser.add_argument("--wine",action="store_true")
    parser.add_argument("--mingw",default="i686-w64-mingw32-gcc")
    parser.add_argument("--output",type=Path)
    parser.add_argument("--device-rates",type=int,nargs="+",choices=(22050,44100,48000),default=[22050,44100,48000])
    args=parser.parse_args()
    if OUTPUT.read_text()!=render():raise RuntimeError("FIR table is not reproducible")
    expected=json.loads(MANIFEST.read_text())
    source=ROOT/"tools/sound_resample_test.c"
    if hashlib.sha256(source.read_bytes()).hexdigest()!=expected["fixture_source_sha256"]:
        raise RuntimeError("Fixture source changed since real reference calibration")
    with tempfile.TemporaryDirectory(prefix="dd2-resample-build-") as temporary:
        directory=Path(temporary)
        output=args.output.resolve() if args.output else directory/"captures"
        output.mkdir(parents=True,exist_ok=False)
        env={k:v for k,v in os.environ.items() if not k.startswith("DD2_")}
        common=["-std=gnu89","-w","-DDD2_NO_FOPEN_WRAP","-ffunction-sections","-fdata-sections",
                f"-I{ROOT/'re_out'}",str(ROOT/"re_out/dd2h_stubs.c"),str(source),"-Wl,--gc-sections"]
        native,wasm=directory/"native",directory/"wasm.js"
        # Match the production native mixer: exact FIR arithmetic must run
        # faster than its elapsed-time audio clock, without fast-math.
        subprocess.run(["gcc","-m32","-no-pie","-O2","-fno-strict-aliasing",*common,"-o",str(native)],check=True)
        subprocess.run([args.emcc,*common,"-sNODERAWFS=1","-sEXIT_RUNTIME=1","-sGLOBAL_BASE=10485760",
                        "--pre-js",str(ROOT/"tools/node_env.js"),"-o",str(wasm)],check=True)
        # This oracle executes actual CPU x87 arithmetic and compares all four
        # operations before its output is used to verify WASM's software model.
        wide=ROOT/"tools/sound_wide_test.c"
        subprocess.run(["gcc","-m32","-O2","-Wall","-Wextra","-Werror",f"-I{ROOT/'re_out'}",
                        str(wide),"-o",str(directory/"wide-native")],check=True)
        subprocess.run([args.emcc,"-O2","-Wall","-Wextra","-Werror",f"-I{ROOT/'re_out'}",str(wide),
                        "-sNODERAWFS=1","-sEXIT_RUNTIME=1","-o",str(directory/"wide-wasm.js")],check=True)
        for target,command in (("native",[str(directory/"wide-native")]),("wasm",[args.node,str(directory/"wide-wasm.js")])):
            subprocess.run([*command,str(output/f"wide-{target}.bin")],env=env,check=True,timeout=30)
        if (output/"wide-native.bin").read_bytes()!=(output/"wide-wasm.bin").read_bytes():
            raise RuntimeError("WASM software precision differs from actual x87 oracle")
        report={"scope":"complete synthetic one-shot FIR/format waveforms; preroll reported separately; original full-output/timing parity pending",
                "wine":args.wine,"x87_cpu_results":1084900,"cases":[],"loops":[]}
        if args.wine:
            executable=directory/"resample.exe"
            subprocess.run([args.mingw,"-Wall","-Wextra","-Werror",str(source),"-ldsound","-o",str(executable)],check=True)
            libraries=build_audio(directory/"libraries")
            alsa=f'pcm_type.dd2clock {{ lib "{directory}/libraries/$LIB/dd2_clock.so" }}\npcm.!default {{ type dd2clock }}\n'
            report["fixture_exe_sha256"]=hashlib.sha256(executable.read_bytes()).hexdigest()
        for rate,frequency,bits,channels in [(r,*c) for r in args.device_rates for c in CASES]:
            env["DD2_SND_RATE"]=str(rate)
            count=(4096*rate+frequency-1)//frequency
            case=output/f"device{rate}-frequency{frequency}-bits{bits}-channels{channels}";case.mkdir()
            record={"device_rate":rate,"frequency":frequency,"bits":bits,"channels":channels,"targets":{}}
            settings={"source_frames":4096,"frequency":frequency,"bits":bits,"channels":channels,"status":0}
            matching=[c for c in expected["cases"] if (c.get("device_rate",22050),c["frequency"],c["bits"],c["channels"])==(rate,frequency,bits,channels)]
            if len(matching)!=1:raise RuntimeError("Missing/duplicate calibrated device/format case")
            reference=matching[0]
            wave=None
            if args.wine:
                (case/"audio").mkdir()
                capture_env={**env,"DD2_AUDIO_CAPTURE":str(case/"audio"),"DD2_AUDIO_RATE":str(rate),
                             "DD2_AUDIO_PROCESS":"resample.exe","LD_PRELOAD":"dd2_audio.so",
                             "LD_LIBRARY_PATH":":".join(map(str,libraries))}
                actual=wine_probe(executable,case,capture_env,alsa_config=alsa,arguments=("-",str(frequency),str(bits),str(channels)))
                shutil.rmtree(case/"wine-prefix")
                if actual!=settings:raise RuntimeError("Wine source/controls/status differ")
                wave,record["wine"]=reference_wave(case/"audio",count,rate)
                check_wave(wave,reference["active_sha256"])
            for target,command in (("native",[str(native)]),("wasm",[args.node,str(wasm)])):
                pcm=case/f"{target}.pcm"
                result=subprocess.run([*command,str(pcm),str(frequency),str(bits),str(channels)],
                                      env=env,text=True,capture_output=True,check=True,timeout=30)
                if json.loads(result.stdout)!=settings:raise RuntimeError(f"{target} controls/status differ")
                actual,record["targets"][target]=active_wave(pcm.read_bytes(),count)
                check_wave(actual,reference["active_sha256"])
                if wave is not None and actual!=wave:raise RuntimeError(f"{target} bytes differ from fresh Wine capture")
                if json.loads(pcm.with_suffix(".pcm.json").read_text())!={"format":"FLOAT_LE","rate":rate,"channels":2}:
                    raise RuntimeError("Port PCM metadata differs")
            # Prove hashes reject an altered bit, missing final sample and a
            # duplicated first sample, including float errors smaller than an ULP.
            for broken in (bytes([actual[0]^1])+actual[1:],actual[:-8],actual[:8]+actual):
                try:check_wave(broken,reference["active_sha256"])
                except RuntimeError:pass
                else:raise RuntimeError("Accepted corrupt/resized waveform")
            report["cases"].append(record)
            (output/"report.json").write_text(json.dumps(report,indent=2)+"\n")
            print(f"PASS device {rate}Hz, source {frequency}Hz/{bits}-bit/{channels}ch: {count} complete exact frames",flush=True)
        selected_loops=[c for c in expected["loops"] if c.get("device_rate",22050) in args.device_rates]
        if len(selected_loops)!=2*len(args.device_rates):raise RuntimeError("Missing/duplicate device loop cases")
        for reference in selected_loops:
            rate=reference.get("device_rate",22050);env["DD2_SND_RATE"]=str(rate)
            frequency=reference["frequency"];cycle=bytes.fromhex(reference["cycle_hex"])
            case=output/f"device{rate}-loop-frequency{frequency}";case.mkdir()
            settings={k:reference[k] for k in ("source_frames","frequency","bits","channels","status","loop")}
            record={"device_rate":rate,"frequency":frequency,"targets":{}}
            if args.wine:
                (case/"audio").mkdir()
                capture_env={**env,"DD2_AUDIO_CAPTURE":str(case/"audio"),"DD2_AUDIO_RATE":str(rate),
                             "DD2_AUDIO_PROCESS":"resample.exe","LD_PRELOAD":"dd2_audio.so",
                             "LD_LIBRARY_PATH":":".join(map(str,libraries))}
                actual=wine_probe(executable,case,capture_env,alsa_config=alsa,arguments=("-",str(frequency),"8","1","loop"))
                shutil.rmtree(case/"wine-prefix")
                if actual!=settings:raise RuntimeError("Wine loop controls/status differ")
                summary=summarize_audio(case/"audio")
                if len(summary["streams"])!=1:raise RuntimeError("Unexpected reference loop device streams")
                stream=summary["streams"][0]
                if (stream["format"],stream["rate"],stream["channels"],stream["frame_bytes"]) != ("FLOAT_LE",rate,2,8):
                    raise RuntimeError("Unexpected reference loop device format")
                record["wine"]=check_loop((case/"audio"/stream["file"]).read_bytes(),cycle)
            for target,command in (("native",[str(native)]),("wasm",[args.node,str(wasm)])):
                pcm=case/f"{target}.pcm"
                result=subprocess.run([*command,str(pcm),str(frequency),"8","1","loop"],
                                      env=env,text=True,capture_output=True,check=True,timeout=30)
                if json.loads(result.stdout)!=settings:raise RuntimeError(f"{target} loop controls/status differ")
                record["targets"][target]=check_loop(pcm.read_bytes(),cycle)
            broken=bytearray(cycle*220);broken[len(cycle)]^=1
            try:check_loop(broken,cycle)
            except RuntimeError:pass
            else:raise RuntimeError("Accepted corrupt loop waveform")
            report["loops"].append(record)
            (output/"report.json").write_text(json.dumps(report,indent=2)+"\n")
            print(f"PASS device {rate}Hz, source {frequency}Hz short loop: all active PCM frames match exact Wine cycle",flush=True)


if __name__ == "__main__":
    main()
