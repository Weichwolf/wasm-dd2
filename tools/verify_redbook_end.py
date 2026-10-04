#!/usr/bin/env python3
"""Characterize Wine's finite-CD ring end and check complete port ranges.

Actual Wine accepted PCM must be a literal source prefix for each declared
range; any missing tail and delayed short-range stop are reported explicitly.
Those are reference-driver observations, not native Windows behavior, and
are never substituted for either port's full source/mix expectation. This
fixture is not complete original-game audio/timing acceptance.
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
from artifacts import WORK, check_space
from redbook_verification import add_backend_arguments, output_directory, compile_backends, finish_report

ROOT=Path(__file__).resolve().parent.parent
SOURCE=ROOT/"tools/redbook_end_test.c"
LENGTHS=(8,39,40,50,52,53,65,66)
sys.path.insert(0,str(ROOT/"tools/reference"))
from audio import build_audio,summarize_audio
from cdrom import build_cdrom


def float_pcm(raw):
    return b"".join(struct.pack("<ff",left/32768,right/32768)
                    for left,right in struct.iter_unpack("<hh",raw))


def validate_wine(directory,waves,records):
    summary=summarize_audio(directory)
    if len(summary["streams"])!=1:
        raise RuntimeError("Expected one retained actual Wine device")
    stream=summary["streams"][0]
    if (stream["format"],stream["rate"],stream["channels"],stream["frame_bytes"]) != ("FLOAT_LE",44100,2,8):
        raise RuntimeError("Unexpected actual Wine CD-end format")
    payload=(directory/stream["file"]).read_bytes()
    frames=[payload[i:i+8] for i in range(0,len(payload),8)]
    phases=[];cursor=0
    # Observe declared nonzero starts and >=100ms of stopped silence. These
    # are signal boundaries, not fitted correlations, offsets or tolerances.
    for sectors,wave,record in zip(LENGTHS,waves,records):
        begin=next((i for i in range(cursor,len(frames)) if frames[i]!=bytes(8)),None)
        if begin is None:
            raise RuntimeError(f"Missing Wine {sectors}-sector waveform")
        silence=0;end=None
        for i in range(begin,len(frames)):
            silence=silence+1 if frames[i]==bytes(8) else 0
            if silence>=4410:
                end=i+1-silence;cursor=i+1;break
        if end is None:
            raise RuntimeError(f"Missing drained Wine {sectors}-sector boundary")
        active=payload[begin*8:end*8]
        if len(active)>len(wave) or active!=wave[:len(active)]:
            raise RuntimeError(f"Wine {sectors}-sector waveform differs from literal CD prefix")
        # Wine preloads the whole range through 39 sectors. Longer ranges
        # must at least preserve that complete prefill before reporting tails.
        if end-begin < min(sectors,39)*588:
            raise RuntimeError(f"Wine {sectors}-sector prefix ends before its prefill")
        phases.append({**record,"first_accepted_frame":begin,"last_accepted_frame":end,
                       "requested_source_frames":sectors*588,"exact_source_frames":end-begin,
                       "missing_source_frames":sectors*588-(end-begin),
                       "source_sha256":hashlib.sha256(active).hexdigest()})
    if any(frame!=bytes(8) for frame in frames[cursor:]):
        raise RuntimeError("Unexpected Wine audio after final drained range")
    return {"accepted_frames":len(frames),"phases":phases,
            "missing_tail_observed":any(p["missing_source_frames"] for p in phases),
            "short_range_stop_delayed":records[0]["mode_200ms"]!=525,
            "scope":"literal accepted source prefixes; queue/control timing excluded; Wine driver only"}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wine",action="store_true")
    parser.add_argument("--mingw",default="i686-w64-mingw32-gcc")
    parser.add_argument("--emcc",default="emcc")
    parser.add_argument("--node",default="node")
    parser.add_argument("--output",type=Path)
    add_backend_arguments(parser)
    args=parser.parse_args()
    game=ROOT/"DestructionDerby2"
    env={k:v for k,v in os.environ.items() if not k.startswith("DD2_")}
    env.update(DD2_CD_ROOT=str(game/"Redbook"),DD2_SND_RATE="44100")
    raw=(game/"Redbook/track02.cdda").read_bytes()[450*2352:(450+max(LENGTHS))*2352]
    waves=[float_pcm(raw[:sectors*2352]) for sectors in LENGTHS]
    # Each phase has a nonzero first/last frame; natural source silence must
    # not be confused with the declared stopped gap used by the observer.
    for wave in waves:
        if wave[:8]==bytes(8) or wave[-8:]==bytes(8) or bytes(4410*8) in wave:
            raise RuntimeError("CD fixture does not have unambiguous signal boundaries")
    expected=[dict(sectors=s,mode_200ms=525 if s==8 else 526,mode_1500ms=525) for s in LENGTHS]
    source=b"".join(raw[:sectors*2352] for sectors in LENGTHS)
    mixed=bytes(8820*8)+b"".join(wave+bytes((66150-len(wave)//8)*8) for wave in waves)
    report={"scope":__doc__,"fixture":{"track":2,"start_sector_offset":450,"lengths":LENGTHS,
            "initial_wait_ms":200,"range_wait_ms":1500},"targets":[],
            "fixture_source_sha256":hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
            "controlled_source_pcm_bytes":len(source),"controlled_mix_pcm_bytes":len(mixed)}
    WORK.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="redbook-cd-end-", dir=WORK) as tmp:
        directory=Path(tmp)
        output = output_directory(args, directory)
        if args.wine:
            executable=output/"end.exe"
            subprocess.run([args.mingw,"-Wall","-Wextra","-Werror",str(SOURCE),
                            "-lwinmm","-ldsound","-o",str(executable)],check=True)
            libs=build_audio(directory/"audio-libraries")
            _,cdlibs=build_cdrom(game,directory/"cd-libraries")
            device=directory/"cd-device";device.touch();(output/"audio").mkdir()
            wine_env={**env,"DD2_CD_DEVICE":str(device),"DD2_AUDIO_CAPTURE":str(output/"audio"),
                      "DD2_AUDIO_RATE":"44100","DD2_AUDIO_PROCESS":"end.exe",
                      "LD_PRELOAD":"dd2_audio.so dd2_cdrom.so",
                      "LD_LIBRARY_PATH":":".join(map(str,libs+cdlibs))}
            alsa=f'pcm_type.dd2clock {{ lib "{directory}/audio-libraries/$LIB/dd2_clock.so" }}\npcm.!default {{ type dd2clock }}\n'
            records=wine_probe(executable,output,wine_env,alsa_config=alsa,cd_device=device)
            shutil.rmtree(output/"wine-prefix")
            if ([r["sectors"] for r in records]!=list(LENGTHS)
                    or any(r["mode_1500ms"]!=525 for r in records)
                    or any(r["mode_200ms"] not in (525,526) for r in records)):
                raise RuntimeError("Unexpected actual Wine CD-end records")
            report["wine"]=validate_wine(output/"audio",waves,records)
            report["wine"].update(version=subprocess.check_output(["wine","--version"],text=True).strip(),
                                  executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest())
            # The verifier must reject a changed accepted bit and expose the
            # observed missing tail; it cannot claim complete Wine/port parity.
            altered=directory/"altered-audio";shutil.copytree(output/"audio",altered)
            filename=summarize_audio(altered)["streams"][0]["file"]
            payload=bytearray((altered/filename).read_bytes())
            first=report["wine"]["phases"][0]["first_accepted_frame"]
            payload[(first+1000)*8]^=1;(altered/filename).write_bytes(payload)
            try:
                validate_wine(altered,waves,records)
            except RuntimeError:
                report["negative_controls"]={"altered_accepted_bit_rejected":True}
            else:
                raise RuntimeError("Accepted an altered Wine source bit")
            (output/"wine-proof.json").write_text(json.dumps(report["wine"],indent=2)+"\n")
            print("PASS actual Wine: eight literal accepted source prefixes; changed bit rejected",flush=True)
            print(f"Wine reference observations: missing_tail={report['wine']['missing_tail_observed']}, "
                  f"short_stop_delayed={report['wine']['short_range_stop_delayed']}",flush=True)
        subprocess.run([sys.executable,str(ROOT/"tools/generate_cd_toc.py"),str(game/"Redbook/disc.json"),
                        str(directory/"dd2_disc.h")],check=True)
        backends, report["backend"] = compile_backends(args, directory, SOURCE, game)
        for target,command in backends:
            pcm=output/f"{target}-source.pcm";mix=output/f"{target}-mixed.pcm"
            run=subprocess.run(command,env={**env,"DD2_CDPCM":str(pcm),"DD2_MIXPCM":str(mix)},
                               capture_output=True,text=True,timeout=30)
            check_space(output)
            if run.returncode or "AddressSanitizer" in run.stderr or "runtime error:" in run.stderr or json.loads(run.stdout)!=expected:
                raise RuntimeError(f"{target}: controlled finite-range records differ: {run.stdout} {run.stderr}")
            if pcm.read_bytes()!=source or mix.read_bytes()!=mixed:
                raise RuntimeError(f"{target}: complete CD source or mixed stream differs")
            metadata=json.loads(Path(str(mix)+".json").read_text())
            if metadata!={"format":"FLOAT_LE","rate":44100,"channels":2}:
                raise RuntimeError(f"{target}: output format differs")
            (output/f"{target}-records.json").write_text(run.stdout)
            report["targets"].append(target)
            print(f"PASS {target}: eight complete ranges, {len(source)} source and {len(mixed)} mixed bytes exact",flush=True)
        finish_report(output, report, args.clean)


if __name__=="__main__":
    main()
