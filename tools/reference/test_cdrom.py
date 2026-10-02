#!/usr/bin/env python3
"""Check the virtual device against manifest extents and original CDDA bytes."""
import ctypes as C
import errno
import os
from pathlib import Path
import tempfile
from cdrom import build_cdrom

ROOT = Path(__file__).resolve().parents[2]


class Address(C.Union):
    _fields_ = [("lba", C.c_int), ("msf", C.c_ubyte * 3)]


class Entry(C.Structure):
    _fields_ = [("track", C.c_ubyte), ("control", C.c_ubyte),
                ("format", C.c_ubyte), ("address", Address), ("mode", C.c_ubyte)]


class AudioRead(C.Structure):
    _fields_ = [("address", Address), ("format", C.c_ubyte),
                ("frames", C.c_int), ("buffer", C.c_void_p)]


def main():
    game = ROOT / "DestructionDerby2"
    with tempfile.TemporaryDirectory(prefix="dd2-cdrom-test-") as tmp:
        output = Path(tmp)
        disc, libraries = build_cdrom(game, output)
        marker = output / "device"
        marker.touch()
        os.environ.update(DD2_CD_DEVICE=str(marker), DD2_CD_ROOT=str(game / "Redbook"))
        library = C.CDLL(str(libraries[1] / "dd2_cdrom.so"), use_errno=True)
        ioctl = library.ioctl
        ioctl.restype = C.c_int
        ioctl.argtypes = [C.c_int, C.c_ulong, C.c_void_p]
        with marker.open("rb") as device:
            fd = device.fileno()
            header = (C.c_ubyte * 2)()
            assert ioctl(fd, 0x5305, header) == 0 and list(header) == [1, 19]
            tracks = disc["tracks"]
            for i in range(20):
                sector = tracks[i]["start_sector"] if i < 19 else tracks[-1]["end_sector"]
                for fmt in (1, 2):
                    entry = Entry(track=i + 1 if i < 19 else 0xAA, format=fmt)
                    assert ioctl(fd, 0x5306, C.byref(entry)) == 0
                    if fmt == 1:
                        assert entry.address.lba == sector
                    else:
                        m, s, f = entry.address.msf
                        assert (m * 60 + s) * 75 + f == sector + 150
                    assert entry.control == (0x41 if i == 0 else 1)
                    assert entry.mode == (2 if i == 0 else 0)
            checks = 0
            for i, track in enumerate(tracks[1:], 1):
                # A beginning, an interior read, and a read spanning the next track.
                for at, count in [(track["start_sector"], 2),
                                  (track["start_sector"] + 37, 3),
                                  (track["end_sector"] - 1, 2 if i < 18 else 1)]:
                    expected = bytearray()
                    for sector in range(at, at + count):
                        source = next(t for t in tracks[1:] if t["start_sector"] <= sector < t["end_sector"])
                        with (game / "Redbook" / source["file"]).open("rb") as audio:
                            audio.seek((sector - source["start_sector"]) * 2352)
                            expected += audio.read(2352)
                    for fmt in (1, 2):
                        buffer = C.create_string_buffer(count * 2352)
                        request = AudioRead(format=fmt, frames=count, buffer=C.addressof(buffer))
                        if fmt == 1:
                            request.address.lba = at
                        else:
                            n = at + 150
                            request.address.msf[:] = [n // 4500, n // 75 % 60, n % 75]
                        assert ioctl(fd, 0x530E, C.byref(request)) == 0
                        assert buffer.raw == expected
                        checks += 1
            for track in (0, 20, 255):
                invalid = Entry(track=track, format=1)
                assert ioctl(fd, 0x5306, C.byref(invalid)) == -1 and C.get_errno() == errno.EINVAL
            invalid = Entry(track=2, format=255)
            assert ioctl(fd, 0x5306, C.byref(invalid)) == -1 and C.get_errno() == errno.EINVAL
            assert ioctl(fd, 0x5305, None) == -1 and C.get_errno() == errno.EFAULT
            assert ioctl(fd, 0xDEADBEEF, None) == -1 and C.get_errno() == errno.ENOTTY
            request.format = 1
            for at, count in ((0, 1), (tracks[-1]["end_sector"], 1),
                              (tracks[-1]["end_sector"] - 1, 2),
                              (tracks[1]["start_sector"], -1)):
                request.address.lba, request.frames = at, count
                assert ioctl(fd, 0x530E, C.byref(request)) == -1 and C.get_errno() == errno.EINVAL
        with (output / "unrelated").open("w+b") as other:
            assert ioctl(other.fileno(), 0x5305, header) == -1 and C.get_errno() == errno.ENOTTY
        print(f"CD device: 20 TOC entries in LBA/MSF, {checks} exact CDDA reads; invalid requests and descriptor isolation passed")


if __name__ == "__main__":
    main()
