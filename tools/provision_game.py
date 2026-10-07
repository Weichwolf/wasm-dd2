#!/usr/bin/env python3
"""Provision the original DD2 Windows files and lossless CD audio from a BIN/CUE ZIP.

Uses Python's standard library plus git, CMake and a C/C++ compiler for unshieldv3.
All downloaded and copyrighted content stays in directories ignored by Git.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parent.parent
URL = "https://d1.xp.myabandonware.com/t/6eda21f2-526b-4be9-ab3b-659c2f817a75/Destruction-Derby-2_Win_EN_ISO-Version.zip"
ARCHIVE_SHA256 = "2b8a46a41194b53bc3f0bd5cf67a51b62c89c77319fd27938d90c7e7e2051f93"
EXTRACTOR_REV = "4e5700fb97ba9f0338b49c1031834ce42177f265"
GAME_HASHES = {
    "dd2h.exe": "0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2",
    "Dirinfo": "03c6ca7adc5e616a4784a82f3b1b7a1f85489e97318b7e01f867e65504d5f22b",
}
SECTOR_BYTES = 2352


def sha256(path):
    with path.open("rb") as src:
        return hashlib.file_digest(src, "sha256").hexdigest()


def download(url, dest):
    partial = dest.with_suffix(dest.suffix + ".partial")
    print(f"Downloading {url}", flush=True)
    with urllib.request.urlopen(url, timeout=60) as src, partial.open("wb") as out:
        shutil.copyfileobj(src, out, 1024 * 1024)
    partial.replace(dest)


def parse_cue(text, bin_size):
    tracks = []
    for line in text.splitlines():
        match = re.fullmatch(r"\s*TRACK\s+(\d+)\s+(\S+)\s*", line, re.I)
        if match:
            tracks.append({"number": int(match[1]), "type": match[2].upper()})
        match = re.fullmatch(r"\s*INDEX\s+01\s+(\d+):(\d+):(\d+)\s*", line, re.I)
        if match:
            if not tracks:
                raise ValueError("CUE index without track")
            minutes, seconds, frames = map(int, match.groups())
            if seconds >= 60 or frames >= 75:
                raise ValueError("Invalid CUE timestamp")
            tracks[-1]["start_sector"] = (minutes * 60 + seconds) * 75 + frames
    if not tracks or tracks[0]["type"] != "MODE2/2352" or bin_size % SECTOR_BYTES:
        raise ValueError("Expected DD2's MODE2/2352 disc image")
    if any("start_sector" not in track for track in tracks):
        raise ValueError("Every track needs INDEX 01")
    end = bin_size // SECTOR_BYTES
    for track in reversed(tracks):
        track["end_sector"] = end
        if not 0 <= track["start_sector"] < end:
            raise ValueError("Invalid track extent")
        end = track["start_sector"]
    return tracks


def iso_read(raw, offset, count):
    """Read ISO9660 bytes from the data payload of Mode 2 Form 1 raw sectors."""
    result = bytearray()
    while count:
        sector, within = divmod(offset, 2048)
        raw.seek(sector * SECTOR_BYTES)
        data = raw.read(SECTOR_BYTES)
        if len(data) != SECTOR_BYTES or data[15] != 2 or data[18] & 0x20:
            raise ValueError("Invalid/truncated Mode 2 Form 1 sector")
        n = min(count, 2048 - within)
        result.extend(data[24 + within:24 + within + n])
        offset += n
        count -= n
    return bytes(result)


def extract_installer(raw, dest):
    pvd = iso_read(raw, 16 * 2048, 2048)
    if pvd[:7] != b"\x01CD001\x01":
        raise ValueError("ISO9660 primary volume descriptor missing")
    root = pvd[156:190]
    extent = struct.unpack_from("<I", root, 2)[0]
    size = struct.unpack_from("<I", root, 10)[0]
    directory = iso_read(raw, extent * 2048, size)
    pos = 0
    while pos < len(directory):
        length = directory[pos]
        if not length:
            pos = (pos // 2048 + 1) * 2048
            continue
        record = directory[pos:pos + length]
        name = record[33:33 + record[32]].decode("ascii", errors="replace")
        if name.split(";")[0].upper() == "DATA.Z":
            extent = struct.unpack_from("<I", record, 2)[0]
            size = struct.unpack_from("<I", record, 10)[0]
            with dest.open("wb") as out:
                offset = extent * 2048
                remaining = size
                while remaining:
                    n = min(remaining, 1024 * 1024)
                    out.write(iso_read(raw, offset, n))
                    offset += n
                    remaining -= n
            return
        pos += length
    raise ValueError("DATA.Z missing from the disc")


def extractor():
    source = ROOT / "deps" / "unshieldv3"
    if source.exists():
        rev = subprocess.check_output(["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip()
        if rev != EXTRACTOR_REV:
            raise ValueError(f"{source} must be at unshieldv3 revision {EXTRACTOR_REV}")
    else:
        subprocess.run(["git", "clone", "https://github.com/wfr/unshieldv3.git", str(source)], check=True)
        subprocess.run(["git", "-C", str(source), "checkout", "--detach", EXTRACTOR_REV], check=True)
    build = source / "build"
    binary = build / "unshieldv3"
    if not binary.exists():
        subprocess.run(["cmake", "-S", str(source), "-B", str(build)], check=True)
        subprocess.run(["cmake", "--build", str(build), "-j2"], check=True)
    return binary


def install_files(unpacked, dest):
    dest.mkdir(parents=True, exist_ok=True)
    for name, expected in GAME_HASHES.items():
        if sha256(unpacked / name) != expected:
            raise ValueError(f"Unexpected {name}; this port needs the supported dd2h build")
    for file in unpacked.iterdir():
        target = dest / file.name
        if target.exists():
            if file.name == "SaveGames":
                print("Keeping existing SaveGames", flush=True)
                continue
            if sha256(target) != sha256(file):
                raise ValueError(f"Existing {target} differs; refusing to overwrite")
        else:
            shutil.copyfile(file, target)


def extract_redbook(raw, tracks, dest):
    """Copy raw CDDA sectors exactly; no lossy codec, sample conversion or resampling."""
    dest.mkdir(parents=True, exist_ok=True)
    manifest = {"sector_bytes": SECTOR_BYTES, "sectors_per_second": 75,
                "sample_rate": 44100, "channels": 2, "bits_per_sample": 16,
                "tracks": []}
    for track in tracks:
        entry = dict(track)
        if track["type"] == "AUDIO":
            name = f"track{track['number']:02d}.cdda"
            target = dest / name
            partial = target.with_suffix(".partial")
            raw.seek(track["start_sector"] * SECTOR_BYTES)
            remaining = (track["end_sector"] - track["start_sector"]) * SECTOR_BYTES
            digest = hashlib.sha256()
            with partial.open("wb") as out:
                while remaining:
                    data = raw.read(min(remaining, 1024 * 1024))
                    if not data:
                        raise ValueError(f"Truncated audio track {track['number']}")
                    digest.update(data)
                    out.write(data)
                    remaining -= len(data)
            expected = digest.hexdigest()
            if target.exists() and sha256(target) != expected:
                partial.unlink()
                raise ValueError(f"Existing {target} differs; refusing to overwrite")
            partial.replace(target)
            entry.update(file=name, sha256=expected,
                         sample_frames=(track["end_sector"] - track["start_sector"]) * 588)
            print(f"CDDA track {track['number']:02d}: {entry['sample_frames']} frames", flush=True)
        manifest["tracks"].append(entry)
    (dest / "disc.json").write_text(json.dumps(manifest, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", default=URL, help="download URL for the supported ZIP")
    parser.add_argument("--archive", type=Path, help="use an existing ZIP (offline after extractor setup)")
    parser.add_argument("--game-dir", type=Path, default=ROOT / "DestructionDerby2")
    args = parser.parse_args()
    cache = ROOT / "deps" / "downloads"
    cache.mkdir(parents=True, exist_ok=True)
    archive = args.archive or cache / "Destruction-Derby-2_Win_EN_ISO-Version.zip"
    if not archive.exists():
        if args.archive:
            raise ValueError(f"Archive missing: {archive}")
        download(args.url, archive)
    if sha256(archive) != ARCHIVE_SHA256:
        raise ValueError(f"ZIP SHA-256 mismatch: {archive}; expected {ARCHIVE_SHA256}")
    print("ZIP SHA-256 verified", flush=True)
    tool = extractor()
    with tempfile.TemporaryDirectory(prefix="dd2-provision-", dir=cache) as staging:
        staging = Path(staging)
        with zipfile.ZipFile(archive) as z:
            bins = [i for i in z.infolist() if i.filename.lower().endswith(".bin")]
            cues = [i for i in z.infolist() if i.filename.lower().endswith(".cue")]
            if len(bins) != 1 or len(cues) != 1:
                raise ValueError("Expected one BIN and one CUE in the ZIP")
            tracks = parse_cue(z.read(cues[0]).decode("ascii"), bins[0].file_size)
            raw_path = staging / "disc.bin"
            # ZipFile.open verifies the BIN CRC when read to the end. Ignore archive
            # paths and extract only selected files to our own fixed staging names.
            with z.open(bins[0]) as src, raw_path.open("wb") as out:
                shutil.copyfileobj(src, out, 1024 * 1024)
        with raw_path.open("rb") as raw:
            installer = staging / "DATA.Z"
            extract_installer(raw, installer)
            unpacked = staging / "game"
            unpacked.mkdir()
            subprocess.run([str(tool), "extract", str(installer), str(unpacked)], check=True)
            install_files(unpacked, args.game_dir)
            extract_redbook(raw, tracks, args.game_dir / "Redbook")
    from rewrite.reference import reference_file
    subprocess.run([sys.executable, str(reference_file("re_out/extract_image.py")),
                    str(args.game_dir / "dd2h.exe"), str(args.game_dir / "dd2_image.bin")], check=True)
    print(f"Provisioned {args.game_dir}: game, memory image and 18 lossless CDDA tracks", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, subprocess.CalledProcessError, zipfile.BadZipFile) as exc:
        print(f"Provisioning failed: {exc}", file=sys.stderr)
        sys.exit(1)
