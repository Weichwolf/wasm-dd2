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
from audio import build_audio, summarize_audio


def main():
    with tempfile.TemporaryDirectory(prefix="dd2-audio-observer-test-") as tmp:
        root=Path(tmp)
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


if __name__=="__main__":
    main()
