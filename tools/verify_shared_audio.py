#!/usr/bin/env python3
"""Check the ordered CD/effects mix against captured Wine PCM and both ports.

Wine's timer-driven preroll/control edges are reported separately. The comparison
anchors at the first nonzero CD source frame and requires the entire subsequent
observed prefix byte for byte, without fitting a phase or allowing sample errors.
It does not establish queued-control timing or original game full-stream parity.
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
from generate_sound_gain import gains
from verify_sound_cursor import wine_probe

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "tools/shared_audio_test.c"
MANIFEST = ROOT / "tools/reference/shared_audio_wine10.json"
sys.path.insert(0, str(ROOT / "tools/reference"))
from audio import build_audio, summarize_audio
from cdrom import build_cdrom


def f32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def waveform(raw):
    last = f32((-16385 / 32768) * f32(gains()[1376] / 65535))
    baseline = struct.pack("<ff", f32(0.5 + last), f32(0.5 + last))
    wave = bytearray()
    alternative = bytearray()
    for left, right in struct.iter_unpack("<hh", raw):
        wave.extend(struct.pack("<ff", f32(f32(0.5 + left / 32768) + last),
                                f32(f32(0.5 + right / 32768) + last)))
        alternative.extend(struct.pack("<ff", f32(f32(0.5 + last) + left / 32768),
                                       f32(f32(0.5 + last) + right / 32768)))
    allowed = {bytes(8), struct.pack("<ff", 0.5, 0.5),
               struct.pack("<ff", last, last), baseline}
    return bytes(wave), bytes(alternative), baseline, allowed


def validate_reference(directory, wave, alternative, baseline, allowed):
    summary = summarize_audio(directory)
    payload = bytearray()
    for stream in summary["streams"]:
        if (stream["format"], stream["rate"], stream["channels"], stream["frame_bytes"]) != ("FLOAT_LE", 44100, 2, 8):
            raise RuntimeError("Wine committed an unexpected shared mixer format")
        payload.extend((directory / stream["file"]).read_bytes())
    frames = [bytes(payload[i:i+8]) for i in range(0, len(payload), 8)]
    first = next(i for i in range(len(wave)//8) if wave[i*8:i*8+8] != baseline)
    begin = next((i for i, frame in enumerate(frames) if frame not in allowed), None)
    if begin is None:
        raise RuntimeError("Missing actual CD waveform in Wine mix")
    end = begin
    while end < len(frames) and first + end - begin < len(wave)//8:
        source = first + end - begin
        if frames[end] != wave[source*8:source*8+8]:
            break
        end += 1
    if end-begin < 22050 or any(frame not in allowed for frame in frames[end:]):
        raise RuntimeError(f"Wine shared mix differs at source frame {first+end-begin}; no sample tolerance")
    active = bytes(payload[begin*8:end*8])
    wrong = sum(wave[i*8:i*8+8] != alternative[i*8:i*8+8] for i in range(first, first+end-begin))
    if not wrong:
        raise RuntimeError("Fixture failed to distinguish source creation order")
    return {"first_nonzero_source_frame": first, "exact_wave_frames": end-begin,
            "wave_sha256": hashlib.sha256(active).hexdigest(),
            "order_sensitive_frames": wrong, "accepted_frames": len(frames),
            "leading_control_frames": begin, "trailing_control_frames": len(frames)-end}


def check_negative_captures(directory, wave, alternative, baseline, allowed):
    first = next(i for i in range(len(wave)//8) if wave[i*8:i*8+8] != baseline)
    marker = wave[first*8:(first+32)*8]
    with tempfile.TemporaryDirectory(prefix="dd2-shared-negative-") as tmp:
        for defect in ("one-bit", "wrong-order", "lost-cd"):
            broken = Path(tmp)/defect
            shutil.copytree(directory, broken)
            changed = False
            for pcm in sorted(broken.glob("*.pcm")):
                data = bytearray(pcm.read_bytes())
                start = data.find(marker)
                if start < 0:
                    continue
                if defect == "one-bit":
                    data[start+8*1000] ^= 1
                elif defect == "wrong-order":
                    count = min(len(data)-start, len(wave)-first*8)//8
                    data[start:start+count*8] = alternative[first*8:(first+count)*8]
                else:
                    data[:] = baseline*(len(data)//8)
                pcm.write_bytes(data); changed = True
            if not changed:
                raise RuntimeError("Negative calibration has no actual Wine waveform")
            try:
                validate_reference(broken, wave, alternative, baseline, allowed)
            except RuntimeError:
                pass
            else:
                raise RuntimeError(f"Accepted damaged Wine shared mix: {defect}")
    print("PASS altered bit, wrong summation order and lost CD rejected", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--node", default="node")
    parser.add_argument("--emcc", default="emcc")
    parser.add_argument("--wine", action="store_true")
    parser.add_argument("--mingw", default="i686-w64-mingw32-gcc")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--record", action="store_true", help="replace calibration metadata after a real Wine capture")
    args = parser.parse_args()
    if args.record and not args.wine:
        parser.error("--record requires --wine")
    game = ROOT / "DestructionDerby2"
    raw = (game / "Redbook/track02.cdda").read_bytes()[:44100*4]
    wave, alternative, baseline, allowed = waveform(raw)
    fixture = {"rate": 44100, "track": 2, "order": ["effect-half", "cd", "effect-gain"]}
    env = {k: v for k, v in os.environ.items() if not k.startswith("DD2_")}
    env.update(DD2_SND_RATE="44100", DD2_CD_ROOT=str(game / "Redbook"))
    with tempfile.TemporaryDirectory(prefix="dd2-shared-audio-") as tmp:
        directory = Path(tmp)
        output = args.output.resolve() if args.output else directory / "captures"
        output.mkdir(parents=True, exist_ok=False)
        subprocess.run(["python3", str(ROOT / "tools/generate_cd_toc.py"),
                        str(game / "Redbook/disc.json"), str(directory / "dd2_disc.h")], check=True)
        common = ["-std=gnu99", "-w", "-DDD2_NO_FOPEN_WRAP", "-ffunction-sections", "-fdata-sections",
                  f"-I{directory}", f"-I{ROOT / 're_out'}", str(ROOT / "re_out/dd2h_stubs.c"),
                  str(ROOT / "re_out/dd2_cd.c"), str(SOURCE), "-Wl,--gc-sections"]
        native, wasm = directory / "native", directory / "wasm.js"
        subprocess.run(["gcc", "-m32", "-no-pie", *common, "-o", str(native)], check=True)
        subprocess.run([args.emcc, *common, "-sNODERAWFS=1", "-sEXIT_RUNTIME=1", "-sGLOBAL_BASE=10485760",
                        "--pre-js", str(ROOT / "tools/node_env.js"), "-o", str(wasm)], check=True)
        report = {"scope": __doc__, "fixture": fixture, "source_sha256": hashlib.sha256(raw).hexdigest(), "targets": []}
        if args.wine:
            executable = directory / "shared.exe"
            subprocess.run([args.mingw, "-Wall", "-Wextra", "-Werror", str(SOURCE),
                            "-ldsound", "-lwinmm", "-o", str(executable)], check=True)
            libraries = build_audio(directory / "audio-libraries")
            _, cd_libraries = build_cdrom(game, directory / "cd-libraries")
            device = directory / "cd-device"; device.touch()
            (output / "audio").mkdir()
            capture_env = {**env, "DD2_AUDIO_CAPTURE": str(output / "audio"), "DD2_AUDIO_RATE": "44100",
                           "DD2_AUDIO_PROCESS": "shared.exe", "DD2_CD_DEVICE": str(device),
                           "LD_PRELOAD": "dd2_audio.so dd2_cdrom.so",
                           "LD_LIBRARY_PATH": ":".join(map(str, libraries + cd_libraries))}
            alsa = f'pcm_type.dd2clock {{ lib "{directory}/audio-libraries/$LIB/dd2_clock.so" }}\npcm.!default {{ type dd2clock }}\n'
            actual = wine_probe(executable, output, capture_env, alsa_config=alsa, cd_device=device)
            shutil.rmtree(output / "wine-prefix")
            if actual != fixture:
                raise RuntimeError("Wine fixture setup differs")
            reference = validate_reference(output / "audio", wave, alternative, baseline, allowed)
            check_negative_captures(output / "audio", wave, alternative, baseline, allowed)
            report["wine"] = reference
            if args.record:
                calibration = {"scope": __doc__, "fixture": fixture, "source_sha256": report["source_sha256"],
                               "fixture_source_sha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(), **reference}
                MANIFEST.write_text(json.dumps(calibration, indent=2) + "\n")
        else:
            calibration = json.loads(MANIFEST.read_text())
            if calibration["source_sha256"] != report["source_sha256"] or calibration["fixture"] != fixture:
                raise RuntimeError("Recorded Wine fixture source/setup differs")
            if calibration["fixture_source_sha256"] != hashlib.sha256(SOURCE.read_bytes()).hexdigest():
                raise RuntimeError("Fixture code differs from actual Wine calibration")
            reference = calibration
            first, count = reference["first_nonzero_source_frame"], reference["exact_wave_frames"]
            if hashlib.sha256(wave[first*8:(first+count)*8]).hexdigest() != reference["wave_sha256"]:
                raise RuntimeError("Ordered waveform differs from actual Wine calibration")
            report["wine_calibration"] = reference
        expected = (struct.pack("<ff", 0.5, 0.5)*3528 + wave[:30870*8] + baseline*4410 + bytes(3528*8))
        music = bytes(3528*8) + b"".join(struct.pack("<ff", l/32768, r/32768)
                    for l, r in struct.iter_unpack("<hh", raw[:30870*4])) + bytes((4410+3528)*8)
        for target, command in (("native", [str(native)]), ("wasm", [args.node, str(wasm)])):
            mixed, cd, part = [output / f"{target}-{kind}.pcm" for kind in ("mixed", "source", "music")]
            run = subprocess.run(command, env={**env, "DD2_MIXPCM": str(mixed), "DD2_CDPCM": str(cd),
                                 "DD2_MUSICPCM": str(part)}, stdout=subprocess.PIPE, text=True, check=True, timeout=30)
            if json.loads(run.stdout) != fixture or mixed.read_bytes() != expected:
                raise RuntimeError(f"{target}: full controlled mixed output differs")
            if cd.read_bytes() != raw[:30870*4] or part.read_bytes() != music:
                raise RuntimeError(f"{target}: music/source clock differs")
            for pcm in (mixed, part):
                if json.loads(Path(str(pcm)+".json").read_text()) != {"format": "FLOAT_LE", "rate": 44100, "channels": 2}:
                    raise RuntimeError("Shared mixer format metadata differs")
            report["targets"].append({"target": target, "full_mixed_pcm_bytes": len(expected), "cd_frames": 30870})
            print(f"PASS {target}: full controlled shared mix, CD source and music part exact", flush=True)
        (output / "report.json").write_text(json.dumps(report, indent=2)+"\n")
        print(f"PASS actual Wine ordered waveform: {reference['exact_wave_frames']} exact frames; "
              f"{reference['order_sensitive_frames']} distinguish summation order", flush=True)


if __name__ == "__main__":
    main()
