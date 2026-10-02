#!/usr/bin/env python3
"""Compare complete original AVI ADPCM source audio with actual Wine ACM.

Win32 acmStreamConvert and the native/WASM decoders receive the same original
format and compressed blocks. Every PCM16LE byte at the source's 22050Hz is
compared directly. Actual movie presentation, output resampling/mixing and
original dd2h.exe's accepted audio/timing remain separate work.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
from verify_movie_codec import riff_chunks,compare
from verify_sound_cursor import wine_probe

ROOT=Path(__file__).resolve().parent.parent
SOURCE=ROOT/"tools/movie_audio_test.c"
DECODER=ROOT/"re_out/dd2_msadpcm.c"


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mingw",default="i686-w64-mingw32-gcc")
    parser.add_argument("--node",default="node")
    parser.add_argument("--emcc",default="emcc")
    parser.add_argument("--ffmpeg",help="also measure its default ADPCM source conversion")
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    env={k:v for k,v in os.environ.items() if not k.startswith("DD2_")}
    report={"scope":__doc__,"films":[],"fixture_sha256":hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
            "decoder_sha256":hashlib.sha256(DECODER.read_bytes()).hexdigest(),
            "wine_version":subprocess.check_output(["wine","--version"],text=True).strip()}
    with tempfile.TemporaryDirectory(prefix="dd2-movie-audio-") as tmp:
        directory=Path(tmp)
        output=args.output.resolve() if args.output else directory/"results"
        output.mkdir(parents=True,exist_ok=False)
        exe=output/"audio.exe";native=directory/"native";wasm=directory/"wasm.js"
        subprocess.run([args.mingw,"-O2","-Wall","-Wextra","-Werror",str(SOURCE),
                        "-lmsacm32","-o",str(exe)],check=True)
        common=["-std=gnu99","-O2","-Wall","-Wextra","-Werror",f"-I{ROOT/'re_out'}",str(DECODER),str(SOURCE)]
        subprocess.run(["gcc","-m32","-no-pie","-fsanitize=address,undefined","-fno-sanitize-recover=all",
                        *common,"-o",str(native)],check=True)
        subprocess.run([args.emcc,*common,"-sNODERAWFS=1","-sEXIT_RUNTIME=1","-o",str(wasm)],check=True)
        report["wine_executable_sha256"]=hashlib.sha256(exe.read_bytes()).hexdigest()
        for filename in ("Intro.avi","Outro.avi"):
            film=output/Path(filename).stem.lower();film.mkdir()
            path=ROOT/"DestructionDerby2"/filename;formats=[];packets=[]
            for kind,data in riff_chunks(path):
                if kind==b"strf" and len(data)>=22 and data[:2]==b"\x02\x00":
                    formats.append(data)
                elif kind==b"01wb":
                    packets.append(data)
            if len(formats)!=1 or not packets:
                raise RuntimeError("Expected one original MSADPCM format and audio stream")
            format=formats[0];data=b"".join(packets)
            channels,rate=struct.unpack_from("<HI",format,2)
            block=struct.unpack_from("<H",format,12)[0];samples=struct.unpack_from("<H",format,18)[0]
            if not block or any(len(packet)%block for packet in packets):
                raise RuntimeError("Original AVI audio packets contain partial blocks")
            frames=len(data)//block*samples
            expected=dict(rate=rate,channels=channels,frames=frames,bytes=frames*channels*2)
            bundle=film/"audio.packets"
            bundle.write_bytes(struct.pack("<II",len(format),len(data))+format+data)
            reference=film/"wine.pcm"
            actual=wine_probe(exe,film,env,arguments=tuple("Z:"+str(p).replace("/","\\") for p in (bundle,reference)))
            shutil.rmtree(film/"wine-prefix")
            if actual!=expected or reference.stat().st_size!=expected["bytes"]:
                raise RuntimeError("Actual ACM did not convert the complete original audio")
            result={"file":filename,"avi_sha256":hashlib.sha256(path.read_bytes()).hexdigest(),
                    "original_packets":len(packets),"compressed_bytes":len(data),"block_bytes":block,
                    "samples_per_block":samples,"format_hex":format.hex(),**expected,"targets":[]}
            for target,command in (("native-asan-ubsan",[str(native)]),("wasm",[args.node,str(wasm)])):
                pcm=film/f"{target}.pcm"
                run=subprocess.run([*command,str(bundle),str(pcm)],env=env,capture_output=True,text=True,timeout=30)
                (film/f"{target}.log").write_text(run.stdout+run.stderr)
                if run.returncode or json.loads(run.stdout)!=expected:
                    raise RuntimeError(f"{filename} {target}: ADPCM source conversion failed: {run.stderr}")
                if compare(reference,pcm)!=expected["bytes"]:
                    raise RuntimeError("Wrong complete decoded PCM extent")
                pcm.unlink();result["targets"].append(target)
                print(f"PASS {filename} {target}: {frames} stereo frames, all {expected['bytes']} PCM bytes exact",flush=True)
                # Incomplete blocks/formats and invalid predictor indices
                # must fail before creating an output stream, rather than
                # producing apparently successful shortened source audio.
                result.setdefault("invalid_input_rejected",{})[target]=[]
                cases=(("partial-block",format,data[:-1]),
                       ("incomplete-format",format[:-1],data),
                       ("invalid-predictor",format,bytes([7])+data[1:]))
                for label,bad_format,bad_data in cases:
                    invalid=directory/f"{target}-{label}.packets"
                    invalid.write_bytes(struct.pack("<II",len(bad_format),len(bad_data))+bad_format+bad_data)
                    bad_pcm=directory/f"{target}-{label}.pcm"
                    failure=subprocess.run([*command,str(invalid),str(bad_pcm)],env=env,capture_output=True,text=True,timeout=30)
                    if (not failure.returncode or bad_pcm.exists() or "movie audio:" not in failure.stderr
                            or "AddressSanitizer" in failure.stderr or "runtime error:" in failure.stderr):
                        raise RuntimeError(f"{target}: invalid ADPCM {label} did not fail safely: {failure.stderr}")
                    result["invalid_input_rejected"][target].append(label)
                print(f"PASS {filename} {target}: partial block/format and invalid predictor rejected safely",flush=True)
            # Reject a changed actual decoded sample using the full byte check.
            changed=directory/"altered.pcm";shutil.copyfile(reference,changed)
            with changed.open("r+b") as file:
                file.seek(1000);original=file.read(1);file.seek(1000);file.write(bytes([original[0]^1]))
            try:
                compare(reference,changed)
            except RuntimeError:
                result["altered_sample_rejected"]=True
            else:
                raise RuntimeError("Accepted altered source PCM bit")
            changed.unlink()
            if args.ffmpeg:
                pcm=film/"ffmpeg.pcm"
                subprocess.run([args.ffmpeg,"-v","error","-i",str(path),"-map","0:a:0","-acodec","pcm_s16le",
                                "-f","s16le",str(pcm)],check=True,timeout=30)
                try:
                    compare(reference,pcm)
                except RuntimeError as error:
                    result["ffmpeg_default_matches_wine"]=False
                    result["ffmpeg_difference"]=str(error)
                else:
                    result["ffmpeg_default_matches_wine"]=True
                pcm.unlink()
                print(f"Observed {filename}: default FFmpeg audio matches Wine={result['ffmpeg_default_matches_wine']}",flush=True)
            with reference.open("rb") as file:result["source_pcm_sha256"]=hashlib.file_digest(file,"sha256").hexdigest()
            bundle.unlink();report["films"].append(result)
            (output/"report.json").write_text(json.dumps(report,indent=2)+"\n")


if __name__=="__main__":
    main()
