#!/usr/bin/env python3
"""Compare all DD2 Cinepak source frames with actual Wine Win32 ICCVID.

The same original sequential compressed packets feed real ICDecompress and
both portable decoders. Every top-down RGB24 byte is compared directly,
including empty AVI hold packets. This is decoder evidence; the original
game's MCI presentation/scaling/timing and movie audio are separate work.
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
from verify_sound_cursor import wine_probe

ROOT=Path(__file__).resolve().parent.parent
FIXTURE=ROOT/"tools/movie_codec_test.c"
DECODER=ROOT/"re_out/dd2_cinepak.c"


def packets(path):
    data=path.read_bytes();frames=[];formats=[]
    def walk(at,end):
        while at<end:
            if at+8>end:
                raise ValueError("Incomplete RIFF chunk header")
            kind=data[at:at+4];size=struct.unpack_from("<I",data,at+4)[0]
            limit=at+8+size
            if limit>end:
                raise ValueError("RIFF chunk exceeds container")
            if kind in (b"RIFF",b"LIST"):
                if size<4:
                    raise ValueError("Incomplete RIFF/LIST type")
                walk(at+12,limit)
            elif kind==b"strf" and size>=40 and data[at+8+16:at+8+20]==b"cvid":
                header=struct.unpack_from("<IiiHH4s",data,at+8)
                formats.append((header[1],header[2]))
            elif kind==b"00dc":
                frames.append(data[at+8:limit])
            at=limit+(size&1)
        if at!=end:
            raise ValueError("Invalid RIFF padding extent")
    if data[:4]!=b"RIFF" or data[8:12]!=b"AVI " or struct.unpack_from("<I",data,4)[0]+8!=len(data):
        raise ValueError("Expected complete original AVI RIFF")
    walk(12,len(data))
    if len(formats)!=1 or not frames:
        raise ValueError("Expected one Cinepak format and video stream")
    return formats[0],frames


def compare(first,second):
    offset=0
    with first.open("rb") as reference,second.open("rb") as target:
        while True:
            a=reference.read(65536);b=target.read(65536)
            if a!=b:
                changed=next((i for i,(x,y) in enumerate(zip(a,b)) if x!=y),min(len(a),len(b)))
                raise RuntimeError(f"RGB bytes differ at {offset+changed}: {first} vs {second}")
            if not a:
                return offset
            offset+=len(a)


def summarize(path,width,height,count,directory):
    frame_bytes=width*height*3;hashes=[]
    checkpoints={0,1,2,25,150,count//2,count-2,count-1}
    with path.open("rb") as file:
        for frame in range(count):
            pixels=file.read(frame_bytes)
            if len(pixels)!=frame_bytes:
                raise RuntimeError("Incomplete actual Wine source frame")
            hashes.append(hashlib.sha256(pixels).hexdigest())
            if frame in checkpoints:
                (directory/f"frame{frame:04d}.rgb").write_bytes(pixels)
        if file.read(1):
            raise RuntimeError("Unexpected actual Wine frames after declared end")
    with path.open("rb") as file:
        digest=hashlib.file_digest(file,"sha256").hexdigest()
    return {"width":width,"height":height,"frames":count,"rgb_bytes":frame_bytes*count,
            "rgb_sha256":digest,"frame_sha256":hashes,"saved_checkpoints":sorted(checkpoints)}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mingw",default="i686-w64-mingw32-gcc")
    parser.add_argument("--node",default="node")
    parser.add_argument("--emcc",default="emcc")
    parser.add_argument("--ffmpeg",help="also demonstrate whether its default RGB output matches the Wine decoder")
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    env={k:v for k,v in os.environ.items() if not k.startswith("DD2_")}
    report={"scope":__doc__,"films":[],"fixture_sha256":hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
            "decoder_sha256":hashlib.sha256(DECODER.read_bytes()).hexdigest(),
            "wine_version":subprocess.check_output(["wine","--version"],text=True).strip()}
    with tempfile.TemporaryDirectory(prefix="dd2-movie-codec-") as tmp:
        directory=Path(tmp)
        output=args.output.resolve() if args.output else directory/"results"
        output.mkdir(parents=True,exist_ok=False)
        exe=output/"codec.exe";native=directory/"native";wasm=directory/"wasm.js"
        subprocess.run([args.mingw,"-O2","-Wall","-Wextra","-Werror",str(FIXTURE),
                        "-lmsvfw32","-o",str(exe)],check=True)
        common=["-std=gnu99","-O2","-Wall","-Wextra","-Werror",f"-I{ROOT/'re_out'}",str(DECODER),str(FIXTURE)]
        subprocess.run(["gcc","-m32","-no-pie","-fsanitize=address,undefined",*common,"-o",str(native)],check=True)
        subprocess.run([args.emcc,*common,"-sNODERAWFS=1","-sEXIT_RUNTIME=1","-o",str(wasm)],check=True)
        report["wine_executable_sha256"]=hashlib.sha256(exe.read_bytes()).hexdigest()
        for movie in ("Intro.avi","Outro.avi"):
            film=output/Path(movie).stem.lower();film.mkdir()
            path=ROOT/"DestructionDerby2"/movie
            (width,height),frames=packets(path)
            expected=dict(width=width,height=height,frames=len(frames),empty_packets=sum(not p for p in frames))
            bundle=film/"original.packets"
            with bundle.open("wb") as file:
                file.write(struct.pack("<III",width,height,len(frames)))
                for frame in frames:
                    file.write(struct.pack("<I",len(frame)));file.write(frame)
            reference=film/"wine.rgb"
            actual=wine_probe(exe,film,env,arguments=tuple("Z:"+str(p).replace("/","\\") for p in (bundle,reference)))
            shutil.rmtree(film/"wine-prefix")
            if actual!=expected:
                raise RuntimeError("Actual Wine source decoder records differ")
            result={"file":movie,"avi_sha256":hashlib.sha256(path.read_bytes()).hexdigest(),
                    "empty_packets":expected["empty_packets"],"targets":[],
                    **summarize(reference,width,height,len(frames),film)}
            # Compare all bytes before retaining hashes/checkpoints; never
            # replace an exact whole-stream comparison with selected samples.
            for target,command in (("native-asan-ubsan",[str(native)]),("wasm",[args.node,str(wasm)])):
                decoded=film/f"{target}.rgb"
                run=subprocess.run([*command,str(bundle),str(decoded)],env=env,capture_output=True,text=True,timeout=30)
                (film/f"{target}.log").write_text(run.stdout+run.stderr)
                if run.returncode or json.loads(run.stdout)!=expected:
                    raise RuntimeError(f"{movie} {target}: source decode failed: {run.stderr}")
                if compare(reference,decoded)!=result["rgb_bytes"]:
                    raise RuntimeError("Wrong full source-frame extent")
                decoded.unlink();result["targets"].append(target)
                print(f"PASS {movie} {target}: {len(frames)} frames, all {result['rgb_bytes']} RGB bytes exact",flush=True)
            # A changed decoded pixel must fail the same direct byte compare.
            altered=directory/"altered.rgb";shutil.copyfile(reference,altered)
            with altered.open("r+b") as file:
                file.seek(1000);original=file.read(1);file.seek(1000);file.write(bytes([original[0]^1]))
            try:
                compare(reference,altered)
            except RuntimeError:
                result["altered_pixel_rejected"]=True
            else:
                raise RuntimeError("Accepted altered decoded RGB bit")
            altered.unlink()
            if args.ffmpeg:
                other=film/"ffmpeg.rgb"
                subprocess.run([args.ffmpeg,"-v","error","-i",str(path),"-map","0:v:0",
                                "-pix_fmt","rgb24","-f","rawvideo",str(other)],check=True,timeout=30)
                try:
                    compare(reference,other)
                except RuntimeError as error:
                    result["ffmpeg_default_matches_wine"]=False
                    result["ffmpeg_difference"]=str(error)
                else:
                    result["ffmpeg_default_matches_wine"]=True
                other.unlink()
                print(f"Observed {movie}: default FFmpeg matches Wine={result['ffmpeg_default_matches_wine']}",flush=True)
            reference.unlink();bundle.unlink()
            report["films"].append(result)
            (output/"report.json").write_text(json.dumps(report,indent=2)+"\n")


if __name__=="__main__":
    main()
