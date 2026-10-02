#!/usr/bin/env python3
"""Generate CD geometry for the platform backend from provisioned BIN/CUE data."""
import argparse
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("header", type=Path)
    args = parser.parse_args()
    disc = json.loads(args.manifest.read_text())
    if [disc[k] for k in ("sector_bytes", "sectors_per_second", "sample_rate", "channels", "bits_per_sample")] != [2352, 75, 44100, 2, 16]:
        raise ValueError("Unsupported CD audio format")
    tracks, end = disc["tracks"], 0
    if len(tracks) != 19 or tracks[0]["type"] != "MODE2/2352":
        raise ValueError("Expected DD2 disc with 19 tracks")
    for number, track in enumerate(tracks, 1):
        if track["number"] != number or track["start_sector"] != end or track["end_sector"] <= end:
            raise ValueError("Invalid TOC")
        end = track["end_sector"]
        if number > 1:
            if track["type"] != "AUDIO" or track["file"] != f"track{number:02d}.cdda":
                raise ValueError("Invalid audio track")
            if (args.manifest.parent / track["file"]).stat().st_size != (end - track["start_sector"]) * 2352:
                raise ValueError("Truncated audio track")
    args.header.write_text("/* Generated from provisioned disc.json; do not edit. */\n" +
        "#define DD2_CD_TRACKS 19\nstatic const unsigned dd2_cd_sectors[20] = {" +
        ",".join(str(t["start_sector"]) for t in tracks) + f",{end}" + "};\n")


if __name__ == "__main__":
    main()
