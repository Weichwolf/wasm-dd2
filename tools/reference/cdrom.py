"""Build the virtual CD device using the provisioned, hash-verified disc."""
import hashlib
import json
from pathlib import Path
import subprocess

SOURCE = Path(__file__).with_name("wine_cdrom.c")


def build_cdrom(game, output):
    root = game / "Redbook"
    disc = json.loads((root / "disc.json").read_text())
    if [disc[key] for key in ("sector_bytes", "sectors_per_second", "sample_rate", "channels", "bits_per_sample")] != [2352, 75, 44100, 2, 16]:
        raise ValueError("Unsupported CD format")
    tracks = disc["tracks"]
    if len(tracks) != 19 or tracks[0]["type"] != "MODE2/2352":
        raise ValueError("Expected the DD2 data track and 18 audio tracks")
    end = 0
    for number, track in enumerate(tracks, 1):
        if track["number"] != number or track["start_sector"] != end or track["end_sector"] <= end:
            raise ValueError("Invalid disc track order or extent")
        end = track["end_sector"]
        if number == 1:
            continue
        if track["type"] != "AUDIO" or track["file"] != f"track{number:02d}.cdda":
            raise ValueError("Invalid CDDA track")
        path = root / track["file"]
        if path.stat().st_size != (end - track["start_sector"]) * 2352:
            raise ValueError(f"Truncated {path}")
        with path.open("rb") as audio:
            if hashlib.file_digest(audio, "sha256").hexdigest() != track["sha256"]:
                raise ValueError(f"Corrupt {path}")
    output.mkdir(parents=True, exist_ok=True)
    starts = [t["start_sector"] for t in tracks] + [end]
    (output / "toc.h").write_text(
        "#define TRACK_COUNT 19\nstatic const int starts[] = {" +
        ",".join(map(str, starts)) + "};\n")
    libraries = []
    # Same soname in both directories: the loader selects the correct ELF class.
    # This avoids wrong-ELF warnings in Wine's mix of 32/64-bit Unix processes.
    for bits in (32, 64):
        directory = output / f"lib{bits}"
        directory.mkdir(exist_ok=True)
        library = directory / "dd2_cdrom.so"
        subprocess.run(["gcc", f"-m{bits}", "-shared", "-fPIC", "-Wall", "-Wextra",
                        "-Werror", f"-I{output}", str(SOURCE), "-ldl", "-o", str(library)], check=True)
        libraries.append(directory)
    return disc, libraries
