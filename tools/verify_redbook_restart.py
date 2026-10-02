#!/usr/bin/env python3
"""Check original-style MCI Stop/TO-only restart at CD sector resolution.

--wine uses an explicitly supplied Q-channel sector and Wine's actual mixer.
Both backends report the same public MCI position before restart. Accepted Wine
waveform prefixes must match literal source starts; queued control edges are
reported separately. This does not prove a live game's Q-channel clock/timing.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
from verify_sound_cursor import wine_probe

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "tools/redbook_restart_test.c"
sys.path.insert(0, str(ROOT / "tools/reference"))
from audio import build_audio, summarize_audio
from cdrom import build_cdrom


def float_pcm(raw):
    return b"".join(struct.pack("<ff", left/32768, right/32768)
                    for left,right in struct.iter_unpack("<hh",raw))


def validate_wine(directory,waves):
    summary=summarize_audio(directory)
    payload=bytearray()
    if len(summary["streams"])!=1:
        raise RuntimeError("Expected one retained actual Wine shared device")
    for stream in summary["streams"]:
        if (stream["format"],stream["rate"],stream["channels"],stream["frame_bytes"]) != ("FLOAT_LE",44100,2,8):
            raise RuntimeError("Unexpected actual Wine restart PCM format")
        payload.extend((directory/stream["file"]).read_bytes())
    frames=[bytes(payload[i:i+8]) for i in range(0,len(payload),8)]
    # Controls are separated by 300ms of stopped transport. Boundaries are
    # declared signal-to-silence transitions, never a fitted waveform phase.
    records=[]; cursor=0
    for index,wave in enumerate(waves):
        if wave[:8]==bytes(8):
            raise RuntimeError("Fixture needs a nonzero first source frame")
        begin=next((i for i in range(cursor,len(frames)) if frames[i]!=bytes(8)),None)
        if begin is None:
            raise RuntimeError("Missing Wine start/restart waveform")
        silence=0; end=None
        for i in range(begin,len(frames)):
            silence=silence+1 if frames[i]==bytes(8) else 0
            if silence>=4410:
                end=i+1-silence; cursor=i+1; break
        if end is None:
            raise RuntimeError("Missing complete drained Wine control interval")
        active=bytes(payload[begin*8:end*8])
        if len(active)>len(wave) or active!=wave[:len(active)]:
            mismatch=next((i for i,(a,b) in enumerate(zip(active,wave)) if a!=b),None)
            raise RuntimeError(f"Wine phase {index}: literal source start differs at byte {mismatch}")
        if end-begin < (3000 if index==0 else 11025):
            raise RuntimeError("Wine waveform prefix too short for the declared control interval")
        records.append({"phase":index,"begin_accepted_frame":begin,"end_accepted_frame":end,
                        "exact_source_frames":end-begin,"sha256":hashlib.sha256(active).hexdigest()})
    if any(frame!=bytes(8) for frame in frames[cursor:]):
        raise RuntimeError("Unexpected waveform after final drained stop")
    return {"accepted_frames":len(frames),"phases":records,
            "scope":"literal source waveform prefixes; queued control/timer edges excluded"}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wine",action="store_true")
    parser.add_argument("--mingw",default="i686-w64-mingw32-gcc")
    parser.add_argument("--node",default="node")
    parser.add_argument("--emcc",default="emcc")
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    game=ROOT/"DestructionDerby2"
    env={k:v for k,v in os.environ.items() if not k.startswith("DD2_")}
    env.update(DD2_CD_ROOT=str(game/"Redbook"),DD2_SND_RATE="44100")
    fixture={"track":2,"start_sector_offset":450,"restart_sector_offset":457,"public_position":117833730}
    raw=(game/"Redbook/track02.cdda").read_bytes()
    first=raw[450*2352:]; resumed=raw[457*2352:]
    waves=[float_pcm(source[:44100*4]) for source in (first,resumed)]
    source=first[:4454*4]+resumed[:13230*4]
    mixed=float_pcm(first[:4454*4])+bytes(13230*8)+float_pcm(resumed[:13230*4])+bytes(8820*8)
    report={"scope":__doc__,"fixture":fixture,"targets":[],
            "fixture_source_sha256":hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
            "source_pcm_bytes":len(source),"mixed_pcm_bytes":len(mixed)}
    with tempfile.TemporaryDirectory(prefix="dd2-restart-") as tmp:
        directory=Path(tmp)
        output=args.output.resolve() if args.output else directory/"results"
        output.mkdir(parents=True,exist_ok=False)
        subprocess.run([sys.executable,str(ROOT/"tools/generate_cd_toc.py"),str(game/"Redbook/disc.json"),
                        str(directory/"dd2_disc.h")],check=True)
        qfile=output/"hardware-q-sector.txt"
        if args.wine:
            executable=output/"restart.exe"
            subprocess.run([args.mingw,"-Wall","-Wextra","-Werror",f"-I{directory}",str(SOURCE),
                            "-lwinmm","-ldsound","-o",str(executable)],check=True)
            libraries=build_audio(directory/"audio-libraries")
            _,cd_libraries=build_cdrom(game,directory/"cd-libraries")
            device=directory/"cd-device";device.touch()
            (output/"audio").mkdir()
            wine_env={**env,"DD2_CD_DEVICE":str(device),"DD2_CD_Q_POSITION":str(qfile),
                      "DD2_AUDIO_CAPTURE":str(output/"audio"),"DD2_AUDIO_RATE":"44100",
                      "DD2_AUDIO_PROCESS":"restart.exe","LD_PRELOAD":"dd2_audio.so dd2_cdrom.so",
                      "LD_LIBRARY_PATH":":".join(map(str,libraries+cd_libraries))}
            alsa=f'pcm_type.dd2clock {{ lib "{directory}/audio-libraries/$LIB/dd2_clock.so" }}\npcm.!default {{ type dd2clock }}\n'
            actual=wine_probe(executable,output,wine_env,alsa_config=alsa,cd_device=device,
                              arguments=("Z:"+str(qfile).replace("/","\\"),))
            shutil.rmtree(output/"wine-prefix")
            if actual!=fixture:
                raise RuntimeError("Wine fixture public position differs")
            report["wine"]=validate_wine(output/"audio",waves)
            # Reject the old within-sector restart and an altered accepted bit.
            # Neither check changes or fits the real Wine capture.
            wrong=float_pcm(raw[457*2352+338*4:457*2352+338*4+44100*4])
            try:
                validate_wine(output/"audio",[waves[0],wrong])
            except RuntimeError:
                pass
            else:
                raise RuntimeError("Accepted the old fractional-sample restart")
            broken=directory/"altered-audio"
            shutil.copytree(output/"audio",broken)
            filename=summarize_audio(broken)["streams"][0]["file"]
            payload=bytearray((broken/filename).read_bytes())
            frame=report["wine"]["phases"][1]["begin_accepted_frame"]+1000
            payload[frame*8]^=1
            (broken/filename).write_bytes(payload)
            try:
                validate_wine(broken,waves)
            except RuntimeError:
                pass
            else:
                raise RuntimeError("Accepted an altered Wine restart PCM bit")
            report["negative_controls"]={"fractional_sample_restart":True,"altered_accepted_bit":True}
            report["wine"].update(version=subprocess.check_output(["wine","--version"],text=True).strip(),
                                 executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest())
            (output/"wine-proof.json").write_text(json.dumps(report["wine"],indent=2)+"\n")
            print("PASS actual Wine: reported Q position and both literal source starts exact",flush=True)
            print("PASS old fractional-sample restart and altered accepted bit rejected",flush=True)
        common=["-std=gnu99","-w","-DDD2_NO_FOPEN_WRAP","-ffunction-sections","-fdata-sections",
                f"-I{directory}",f"-I{ROOT/'re_out'}",str(ROOT/"re_out/dd2_cd.c"),
                str(ROOT/"re_out/dd2h_stubs.c"),str(SOURCE),"-Wl,--gc-sections"]
        native,wasm=directory/"native",directory/"wasm.js"
        subprocess.run(["gcc","-m32","-no-pie",*common,"-o",str(native)],check=True)
        subprocess.run([args.emcc,*common,"-sNODERAWFS=1","-sEXIT_RUNTIME=1","-sGLOBAL_BASE=10485760",
                        "--pre-js",str(ROOT/"tools/node_env.js"),"-o",str(wasm)],check=True)
        for target,command in (("native",[str(native)]),("wasm",[args.node,str(wasm)])):
            pcm=output/f"{target}-source.pcm"; mix=output/f"{target}-mixed.pcm"
            run=subprocess.run([*command,str(qfile)],env={**env,"DD2_CDPCM":str(pcm),"DD2_MIXPCM":str(mix)},
                               capture_output=True,text=True,timeout=30)
            if run.returncode:
                raise RuntimeError(f"{target} failed: {run.stderr}")
            if json.loads(run.stdout)!=fixture:
                raise RuntimeError(f"{target}: public position differs")
            if pcm.read_bytes()!=source or mix.read_bytes()!=mixed:
                raise RuntimeError(f"{target}: full controlled Stop/TO-only source or mix differs")
            report["targets"].append(target)
            print(f"PASS {target}: {len(source)} source and {len(mixed)} complete mix bytes exact",flush=True)
        (output/"report.json").write_text(json.dumps(report,indent=2)+"\n")


if __name__=="__main__":
    main()
