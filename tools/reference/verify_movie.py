#!/usr/bin/env python3
"""Verify complete accepted movie source PCM in an unmodified original capture.

Use capture.py --mode audio --audio --audio-rate 22050 --keep-movie
--wine-debug=-all,+iccvid,+mciavi --audio-tail 3 --timeout 120. Compare with
verify_movie_audio.py's complete actual ACM source, already matched on both
ports. Every source byte and every extra accepted silent byte is checked;
no alignment, silence trimming or waveform fitting is allowed. This does
not establish original movie pixels, queue timing or port presentation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import tempfile
from audio import summarize_audio

ROOT=Path(__file__).resolve().parents[2]
EXE_SHA256="0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2"


def verify(capture,source,movie='Intro.avi'):
    outro=movie=='Outro.avi'
    summary=json.loads((capture/"audio/summary.json").read_text())
    checked=summarize_audio(capture/"audio",write=False)
    if checked["streams"]!=summary["streams"]:
        raise ValueError("Captured stream metadata differs from accepted-write journals")
    provenance=json.loads((capture/'report.json').read_text()) if outro else summary
    if provenance.get("exe_sha256")!=EXE_SHA256 or provenance.get("exe_modified") is not False:
        raise ValueError("Expected the supported unmodified original-game capture")
    if outro:
        if (not provenance.get('pass_') or provenance.get('engine_state_writes') is not False or
                not provenance.get('original_process_exited') or not provenance.get('outro_started') or
                provenance.get('outro_frames')!=1812 or provenance.get('before_accept',{}).get('name')!='CREDITZ!'):
            raise ValueError('Expected complete real-input original Outro capture')
    elif summary.get("movie_autoskip") is not False:
        raise ValueError("Expected a full intro without automatic Escape")
    if not outro and (summary.get("virtual_device_rate")!=22050 or summary.get("virtual_device")!="clock"):
        raise ValueError("Original ADPCM movie reference needs the declared 22050Hz clocked device")
    if not outro and summary.get("wine_debug")!="-all,+iccvid,+mciavi":
        raise ValueError("Missing explicit original movie format/command trace")
    streams=[s for s in summary["streams"] if s["format"]=="S16_LE"]
    if len(streams)!=(2 if outro else 1):
        raise ValueError("Unexpected actual S16 movie stream count; inspect MCIAVI_OpenAudio")
    stream=streams[-1]
    if (stream["rate"],stream["channels"],stream["frame_bytes"],stream["closed"])!=(22050,2,4,True):
        raise ValueError("Unexpected original movie stream format/completion")
    reference=json.loads((source/"report.json").read_text())
    info=next((film for film in reference["films"] if film["file"]==movie),None)
    if info is None or info["targets"]!=["native-asan-ubsan","wasm"]:
        raise ValueError("Missing complete native/WASM-matched actual ACM movie source")
    if reference["decoder_sha256"]!=hashlib.sha256((ROOT/"re_out/dd2_msadpcm.c").read_bytes()).hexdigest():
        raise ValueError("ACM proof describes a different portable decoder")
    if info["avi_sha256"]!=hashlib.sha256((ROOT/"DestructionDerby2"/movie).read_bytes()).hexdigest():
        raise ValueError("ACM proof describes a different original AVI")
    pcm=source/Path(movie).stem.lower()/"wine.pcm"
    with pcm.open("rb") as file:source_hash=hashlib.file_digest(file,"sha256").hexdigest()
    if source_hash!=info["source_pcm_sha256"] or pcm.stat().st_size!=info["bytes"]:
        raise ValueError("Actual ACM source PCM is incomplete or altered")
    actual=capture/"audio"/stream["file"]
    source_bytes=0;silent_bytes=0
    with pcm.open("rb") as expected,actual.open("rb") as accepted:
        while True:
            block=expected.read(65536)
            if not block:break
            other=accepted.read(len(block))
            if other!=block:
                mismatch=next((i for i,(a,b) in enumerate(zip(block,other)) if a!=b),min(len(block),len(other)))
                raise ValueError(f"Original movie source differs or ends at byte {source_bytes+mismatch}")
            source_bytes+=len(block)
        while True:
            block=accepted.read(65536)
            if not block:break
            if any(block):raise ValueError("Original writes nonzero samples after the complete declared source")
            silent_bytes+=len(block)
    log=(capture/"wine.log").read_text(errors="replace")
    if outro:
        if re.findall(r'MCI_OPEN_ELEMENT L"([^"]+)"',log)!=['INTRO.AVI','OUTRO.AVI']:
            raise ValueError('Unexpected original movie open sequence')
        log=log.split('MCI_OPEN_ELEMENT L"OUTRO.AVI"!',1)[1]
    if ((not outro and "MCI_OPEN_ELEMENT L\"INTRO.AVI\"" not in log)
            or "PUT_DESTINATION (0,48)-(640,432)" not in log
            or "MCIAVI_OpenVideo bih.biBitCount=32" not in log
            or "MCIAVI_OpenVideo bih.biWidth=320" not in log
            or "MCIAVI_OpenVideo bih.biHeight=192" not in log
            or "MCIAVI_OpenAudio Can't open low level audio device" in log):
        raise ValueError("Original movie command/format trace differs or failed audio")
    painted=[int(n) for n in re.findall(r"MCIAVI_PaintFrame Painting frame (\d+)",log)]
    if not painted or min(painted)!=0:
        raise ValueError("No original movie presentation starting at frame0")
    if outro and painted!=list(range(1812)):
        raise ValueError('Original Outro did not present its complete exclusive endpoint')
    return {"scope":__doc__,"movie":movie,"exe_modified":False,"exe_sha256":EXE_SHA256,
            "source_pcm_sha256":source_hash,"accepted_pcm_sha256":stream["sha256"],
            "format":"S16_LE","rate":22050,"channels":2,
            "complete_source_bytes":source_bytes,"complete_source_frames":source_bytes//4,
            "additional_silent_frames":silent_bytes//4,"total_accepted_frames":stream["accepted_frames"],
            "source_width":320,"source_height":192,"codec_output_bpp":32,
            "destination_rectangle":[0,48,640,432],"paint_trace_calls":len(painted),
            "first_painted_frame":min(painted),"last_painted_frame":max(painted),
            "presentation_pixel_comparison":"pending","queue_control_timing_comparison":"pending"}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture",required=True,type=Path)
    parser.add_argument("--source",required=True,type=Path,help="verify_movie_audio.py output directory")
    parser.add_argument("--report",type=Path)
    parser.add_argument("--negative-controls",action="store_true",
                        help="reject changed source/tail bits in temporary copies, preserving accepted-write extents")
    parser.add_argument('--movie',choices=('Intro.avi','Outro.avi'),default='Intro.avi')
    args=parser.parse_args()
    if args.report and args.report.exists():parser.error("report already exists; use a fresh path")
    report=verify(args.capture.resolve(),args.source.resolve(),args.movie)
    if args.negative_controls:
        report["negative_controls"]=[]
        cases=[("changed-source-bit",1000,"source differs")]
        if report["additional_silent_frames"]:
            cases.append(("nonzero-tail",report["complete_source_bytes"],"nonzero samples"))
        work=Path('/tmp/wasm-dd2');work.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="movie-source-negative-",dir=work) as tmp:
            for label,offset,reason in cases:
                changed=Path(tmp)/label;changed.mkdir()
                shutil.copytree(args.capture/'audio',changed/'audio')
                shutil.copyfile(args.capture/'wine.log',changed/'wine.log')
                if args.movie=='Outro.avi':shutil.copyfile(args.capture/'report.json',changed/'report.json')
                summary=json.loads((changed/"audio/summary.json").read_text())
                stream=[s for s in summary["streams"] if s["format"]=="S16_LE"][-1]
                pcm=changed/"audio"/stream["file"]
                with pcm.open("r+b") as file:
                    file.seek(offset);byte=file.read(1);file.seek(offset);file.write(bytes([byte[0]^1]))
                with pcm.open("rb") as file:stream["sha256"]=hashlib.file_digest(file,"sha256").hexdigest()
                # Preserve valid journal extents and update the file hash so
                # rejection exercises source bytes, not stale hash metadata.
                (changed/"audio/summary.json").write_text(json.dumps(summary,indent=2)+"\n")
                try:
                    verify(changed,args.source.resolve(),args.movie)
                except ValueError as error:
                    if reason not in str(error):raise
                    report["negative_controls"].append({"case":label,"rejected":True})
                else:
                    raise ValueError(f"Accepted negative control {label}")
    if args.report:args.report.write_text(json.dumps(report,indent=2)+"\n")
    print(f"PASS unmodified original {args.movie}: all {report['complete_source_bytes']} accepted source PCM bytes exact, "
          f"{report['additional_silent_frames']} extra accepted silent frames; "
          "32-bit codec/640x384 destination confirmed")


if __name__=="__main__":
    main()
