#!/usr/bin/env python3
"""Check 32/64-bit real-ALSA observation, exact data and out-of-scope passthrough."""
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import shutil
import sys
import argparse
from contextlib import ExitStack
from audio import build_audio, summarize_audio, summarize_played_audio
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, prepare_output, check_space


def playback_checks(root, bits, library, config):
    binary=root/f"playback{bits}"
    subprocess.run(["gcc",f"-m{bits}","-O2","-Wall","-Wextra","-Werror",
                    str(Path(__file__).with_name("audio_playback_test.c")),
                    "-Wl,-l:libasound.so.2","-o",str(binary)],check=True)
    results=[]
    for rate in (22050,44100,48000):
        for format in ("float","s16"):
            capture=root/f"played-{bits}-{rate}-{format}";capture.mkdir()
            expected=root/f"expected-{bits}-{rate}-{format}.pcm"
            env={k:v for k,v in os.environ.items() if not k.startswith("DD2_")}
            env.update(ALSA_CONFIG_PATH=str(config),DD2_AUDIO_CAPTURE=str(capture),
                       DD2_AUDIO_RATE=str(rate),LD_PRELOAD=str(library/"dd2_audio.so"))
            run=subprocess.run([str(binary),format,str(expected)],env=env,check=True,
                               timeout=10,capture_output=True,text=True)
            settings=json.loads(run.stdout)
            report=summarize_audio(capture,require_played=True)
            if len(report["played_streams"])!=1:raise RuntimeError("Expected one consumed stream")
            stream=report["played_streams"][0]
            actual=(capture/stream["file"]).read_bytes()
            if actual!=expected.read_bytes():raise RuntimeError("Device-consumed PCM differs from independent ALSA counter/pattern oracle")
            if stream["played_frames"]!=settings["expected_frames"] or not stream["closed"]:
                raise RuntimeError("Incorrect closed played extent")
            if report["streams"][0]["accepted_frames"]<=stream["played_frames"] or len(stream["segments"])<5:
                raise RuntimeError("Lost discarded samples or pause/transport gaps")
            events=[json.loads(line) for line in (capture/stream["events"]).read_text().splitlines()]
            if not any(e["event"]=="xrun" for e in events):raise RuntimeError("Missing device underrun boundary")
            # Deliberately damage real recordings. Frame order and timing must
            # not become a successful report from partial or corrupt evidence.
            if rate==44100 and format=="float":
                for defect in ("truncated","missing-journal","offset","source-end","interval",
                               "overlap","future","close-count","unknown-event"):
                    broken=root/f"played-{bits}-broken-{defect}";shutil.copytree(capture,broken)
                    journal=broken/stream["events"];pcm=broken/stream["file"]
                    data=[json.loads(line) for line in journal.read_text().splitlines()]
                    played=[e for e in data if e["event"]=="played"]
                    if defect=="truncated":pcm.write_bytes(pcm.read_bytes()[:-1])
                    elif defect=="missing-journal":journal.unlink()
                    else:
                        if defect=="offset":played[0]["offset_frames"]+=1
                        elif defect=="source-end":played[0]["source_end"]+=1
                        elif defect=="interval":played[0]["sample_end_ns"]-=100000
                        elif defect=="overlap":played[1]["sample_begin_ns"]=played[0]["sample_end_ns"]-1
                        elif defect=="future":played[0]["sample_end_ns"]=played[0]["time_ns"]+1
                        elif defect=="close-count":data[-1]["played_frames"]+=1
                        else:data[1]["event"]="unexpected"
                        journal.write_text("".join(json.dumps(e)+"\n" for e in data))
                    try:summarize_played_audio(broken,required=True)
                    except (ValueError,RuntimeError):pass
                    else:raise RuntimeError(f"Accepted broken played capture: {defect}")
                changed=bytearray(actual);changed[len(changed)//2]^=1
                if bytes(changed)==expected.read_bytes():raise RuntimeError("Played payload mutation escaped exact comparison")
            results.append({"bits":bits,"format":format,"rate":rate,"settings":settings,
                            "accepted_frames":report["streams"][0]["accepted_frames"],"played":stream})
            check_space(root)
            print(f"PASS {bits}-bit {format}/{rate}Hz consumed PCM: {len(actual)} exact bytes, rewind/drop/pause/drain/wrap/XRUN",flush=True)
    capture=root/f"played-{bits}-outside";capture.mkdir()
    env.update(DD2_AUDIO_CAPTURE=str(capture))
    subprocess.run([str(binary),"s16",str(root/f"expected-{bits}-outside.pcm"),"outside"],env=env,check=True,timeout=10,capture_output=True)
    if any(capture.iterdir()):raise RuntimeError("Captured unrelated virtual-device process")
    capture=root/f"played-{bits}-active-prepare";capture.mkdir()
    env.update(DD2_AUDIO_CAPTURE=str(capture))
    subprocess.run([str(binary),"s16",str(root/f"expected-{bits}-active-prepare.pcm"),"active-prepare"],env=env,check=True,timeout=10,capture_output=True)
    try:summarize_audio(capture,require_played=True)
    except RuntimeError as error:
        if "Active prepare reset" not in str(error):raise
    else:raise RuntimeError("Accepted an unobservable active prepare reset")
    return results


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    WORK.mkdir(parents=True,exist_ok=True)
    with ExitStack() as stack:
        if args.output:
            root=prepare_output(args.output)
            if WORK.resolve() not in root.parents:parser.error("output must remain inside /tmp/wasm-dd2")
            root.mkdir(parents=True,exist_ok=False)
        else:
            root=Path(stack.enter_context(tempfile.TemporaryDirectory(prefix="audio-observer-test-",dir=WORK)))
        playback_results=[]
        libraries=build_audio(root/"libraries")
        config=root/"asound.conf"
        config.write_text("pcm.!default { type null }\n")
        expected=struct.pack("<64f",*[(i-32)/64 for i in range(64)])
        for bits,library in zip((32,64),libraries):
            clock_binary=root/f"clock{bits}"
            subprocess.run(["gcc",f"-m{bits}","-Wall","-Wextra","-Werror",
                            str(Path(__file__).with_name("audio_clock_test.c")),
                            "-Wl,-l:libasound.so.2","-o",str(clock_binary)],check=True)
            clock_config=root/f"clock{bits}.conf"
            clock_config.write_text(f'pcm_type.dd2clock {{ lib "{root}/libraries/$LIB/dd2_clock.so" }}\npcm.!default {{ type dd2clock }}\n')
            clock_env={k:v for k,v in os.environ.items() if not k.startswith("DD2_")}
            clock_env.update(ALSA_CONFIG_PATH=str(clock_config),DD2_AUDIO_RATE="44100")
            subprocess.run([str(clock_binary)],env=clock_env,check=True,timeout=10)
            print(f"PASS {bits}-bit clocked device: sample-rate clock and transport")
            playback_results.extend(playback_checks(root,bits,library,clock_config))
            binary=root/f"test{bits}"
            subprocess.run(["gcc",f"-m{bits}","-Wall","-Wextra","-Werror",
                            str(Path(__file__).with_name("audio_observer_test.c")),
                            "-Wl,-l:libasound.so.2","-o",str(binary)],check=True)
            for role in ("inside","outside"):
                capture=root/f"{bits}-{role}";capture.mkdir()
                env={k:v for k,v in os.environ.items() if not k.startswith("DD2_")}
                env.update(ALSA_CONFIG_PATH=str(config),DD2_AUDIO_CAPTURE=str(capture),
                           DD2_AUDIO_RATE="44100",LD_PRELOAD=str(library/"dd2_audio.so"))
                subprocess.run([str(binary),role],env=env,check=True,timeout=10)
                if role=="outside":
                    if any(capture.iterdir()):raise RuntimeError("Captured an unrelated process")
                    print(f"PASS {bits}-bit unrelated process: no capture and no rate restriction")
                    continue
                report=summarize_audio(capture)
                if len(report["streams"])!=2:raise RuntimeError("Missing/reused PCM stream")
                for stream in report["streams"]:
                    if [stream[k] for k in ("rate","channels","format","frame_bytes","accepted_frames","closed")] != [44100,2,"FLOAT_LE",8,32,True]:
                        raise RuntimeError("Incorrect committed format/frame extent")
                    if (capture/stream["file"]).read_bytes()!=expected:
                        raise RuntimeError("Accepted PCM payload changed or included failed write")
                    events=[json.loads(line) for line in (capture/stream["events"]).read_text().splitlines()]
                    if not any(event["event"]=="write" and event["accepted"]<0 for event in events):
                        raise RuntimeError("Failed write not recorded")
                    for transport in ("snd_pcm_drop","snd_pcm_prepare","snd_pcm_rewind","close"):
                        if not any(event["event"]==transport for event in events):raise RuntimeError(f"Missing {transport}")
                print(f"PASS {bits}-bit observer: 2 fresh streams, 512 exact bytes, failed writes excluded and transport recorded")
                # Corrupt real captures rather than constructing a second mixer
                # implementation. Acceptance reports must fail closed.
                for defect in ("truncated", "missing-journal", "timestamp", "offset", "close-count", "io-error"):
                    broken=root/f"{bits}-{defect}"
                    shutil.copytree(capture,broken)
                    stream=report["streams"][0]
                    pcm=broken/stream["file"]
                    journal=broken/stream["events"]
                    if defect=="truncated":
                        pcm.write_bytes(pcm.read_bytes()[:-1])
                    elif defect=="missing-journal":
                        journal.unlink()
                    elif defect=="io-error":
                        (broken/"error.txt").write_text("capture I/O failure\n")
                    else:
                        events=[json.loads(line) for line in journal.read_text().splitlines()]
                        if defect=="timestamp":
                            events[1]["time_ns"]=events[0]["time_ns"]-1
                        elif defect=="offset":
                            next(event for event in events if event["event"]=="write")["offset_frames"]+=1
                        else:
                            events[-1]["frames"]+=1
                        journal.write_text("".join(json.dumps(event)+"\n" for event in events))
                    try:
                        summarize_audio(broken)
                    except (ValueError, RuntimeError):
                        pass
                    else:
                        raise RuntimeError(f"Accepted broken capture: {defect}")
                print(f"PASS {bits}-bit capture validation: six corrupted capture cases rejected")
        stale=root/"stale-output";stale.mkdir();(stale/"audio").mkdir()
        rejected=subprocess.run([sys.executable,str(Path(__file__).with_name("capture.py")),
                                 "--mode","audio","--audio","--audio-tail","1","--output",str(stale)],
                                capture_output=True,text=True,timeout=10)
        if rejected.returncode!=2 or "use a fresh directory" not in rejected.stderr:
            raise RuntimeError("Capture launcher failed to reject reused audio output")
        print("PASS reused audio output rejected before launching Wine")
        (root/"report.json").write_text(json.dumps({"scope":"Real 32/64-bit ALSA capture/device acceptance with independent known sample patterns and frozen delay counters; complete original A/V comparison pending",
            "pass":True,"played_cases":playback_results,"played_corruption_cases":18,
            "active_prepare_rejected":True},indent=2)+"\n")
        for path in root.rglob("*"):
            if path.is_file() and not path.is_symlink() and path.suffix in (".pcm",".bin"):
                path.unlink()


if __name__=="__main__":
    main()
