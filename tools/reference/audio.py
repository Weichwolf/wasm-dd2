"""Build the scoped ALSA observer and validate its exact accepted-write captures."""
import hashlib
import json
from pathlib import Path
import subprocess

SOURCE = Path(__file__).with_name("wine_audio.c")


def build_audio(output):
    libraries=[]
    for bits in (32,64):
        directory=output/f"lib{bits}"
        directory.mkdir(parents=True,exist_ok=True)
        subprocess.run(["gcc",f"-m{bits}","-shared","-fPIC","-O2","-Wall","-Wextra","-Werror",
                        "-pthread",str(SOURCE),"-ldl","-o",str(directory/"dd2_audio.so")],check=True)
        subprocess.run(["gcc",f"-m{bits}","-shared","-fPIC","-DPIC","-O2","-Wall","-Wextra","-Werror",
                        str(SOURCE.with_name("alsa_clock.c")),"-Wl,-l:libasound.so.2",
                        "-o",str(directory/"dd2_clock.so")],check=True)
        # glibc expands $LIB in absolute dlopen paths to Debian's multiarch
        # directory for the calling process. ALSA prefixes bare library names
        # with its system plugin directory instead of using LD_LIBRARY_PATH.
        arch=subprocess.check_output(["gcc",f"-m{bits}","-print-multiarch"],text=True).strip()
        plugin=output/"lib"/arch/"dd2_clock.so"
        plugin.parent.mkdir(parents=True,exist_ok=True)
        if plugin.is_symlink():plugin.unlink()
        plugin.symlink_to(directory.resolve()/"dd2_clock.so")
        libraries.append(directory)
    return libraries


def summarize_audio(directory):
    if (directory/"error.txt").exists():
        raise RuntimeError((directory/"error.txt").read_text())
    streams=[]
    logs = sorted(directory.glob("stream-*.jsonl"))
    if {log.with_suffix(".pcm") for log in logs} != set(directory.glob("stream-*.pcm")):
        raise ValueError("Missing PCM file or journal")
    for log in logs:
        events=[json.loads(line) for line in log.read_text().splitlines()]
        if not events or events[0]["event"]!="format":
            raise ValueError("Missing committed ALSA format")
        info=events[0]
        if any(type(info[key]) is not int or info[key] <= 0
               for key in ("rate", "channels", "frame_bytes")):
            raise ValueError("Invalid committed ALSA format")
        if info["frame_bytes"] % info["channels"]:
            raise ValueError("Invalid committed ALSA frame size")
        total=0;last_time=0
        for event in events:
            if event["time_ns"]<last_time:
                raise ValueError("Non-monotonic capture timestamps")
            last_time=event["time_ns"]
            if event["event"]=="write":
                accepted=event["accepted"]
                if type(accepted) is not int or type(event["requested"]) is not int or event["requested"] < 0:
                    raise ValueError("Invalid accepted-write count")
                if not event["call_begin_ns"] <= event["call_end_ns"] <= event["time_ns"]:
                    raise ValueError("Invalid ALSA call timestamp bounds")
                if event["offset_frames"]!=total or accepted>event["requested"]:
                    raise ValueError("Invalid accepted-write extent")
                total+=max(0,accepted)
            if event["event"]=="close" and (event is not events[-1] or event["frames"] != total):
                raise ValueError("Invalid closed stream extent")
        pcm=log.with_suffix(".pcm")
        if pcm.stat().st_size!=total*info["frame_bytes"]:
            raise ValueError("Captured PCM size differs from accepted writes")
        with pcm.open("rb") as file:
            sha=hashlib.file_digest(file,"sha256").hexdigest()
        streams.append({**info,"file":pcm.name,"accepted_frames":total,"sha256":sha,
                        "events":log.name,"closed":events[-1]["event"]=="close",
                        "accepted_seconds":total/info["rate"],
                        "observed_seconds":(last_time-info["time_ns"])/1e9})
    if not streams or not any(stream["accepted_frames"] for stream in streams):
        raise ValueError("Selected process produced no captured ALSA frames")
    report={"scope":"actual Wine ALSA accepted PCM; transport/timing alignment and port comparison pending",
            "streams":streams}
    (directory/"summary.json").write_text(json.dumps(report,indent=2)+"\n")
    return report
