"""GDB-only renderer-cycle capture for original and native executables.

64 presentations cover the highlight counter; 256 cover the car rotation.

Original breakpoints are hardware-only. Draw_All first copies/presents the
existing framebuffer (PutDrawEnv/PutDispEnv), then rasterizes the current OT
(DrawOTag). Capture entry, matching what the browser's Flip actually presents.
Original calls: 0x420cb8 -> 0x412bf4, 0x420cbd -> 0x412ca0,
0x420cdc -> 0x412883. A capture at Draw_All return instead sees the next image.
"""
import json
from pathlib import Path
import gdb
from artifacts import check_space


def record_cycle(output, frames, entry, hardware=False, already_at_entry=False):
    directory = Path(output) / "cycle"
    directory.mkdir()
    kind = gdb.BP_HARDWARE_BREAKPOINT if hardware else gdb.BP_BREAKPOINT
    enter = gdb.Breakpoint(f"*0x{entry:x}", type=kind)
    enter.silent = True
    result = []
    for index in range(frames):
        if index or not already_at_entry:
            gdb.execute("continue")
        if int(gdb.parse_and_eval("$pc")) != entry:
            raise RuntimeError("Cycle failed to reach Draw_All entry")
        inferior = gdb.selected_inferior()
        read = lambda address, size: bytes(inferior.read_memory(address, size))
        phase = int.from_bytes(read(0x4699cc, 4), "little", signed=True)
        cf = int.from_bytes(read(0x462ff0, 4), "little", signed=True)
        level = int.from_bytes(read(0x936ff4, 4), "little", signed=True)
        if phase not in range(64):
            raise RuntimeError(f"Highlight cycle is not settled: phase={phase}")
        prefix = f"frame{index:03d}"
        (directory / f"{prefix}-framebuf.bin").write_bytes(read(0x700450, 307200))
        (directory / f"{prefix}-palette.bin").write_bytes(read(0x700050, 1024))
        result.append({"index": index, "phase": phase, "cf": cf, "level": level,
                       "prefix": prefix,
                       "race_car": int.from_bytes(read(0x467400, 4), "little", signed=True),
                       "car_angles": [int.from_bytes(read(0x468eb4+2*i, 2), "little", signed=True)
                                      for i in range(3)],
                       "poly_list": int.from_bytes(read(0x940010, 4), "little")})
        check_space(output)
    enter.delete()
    (directory / "cycle.json").write_text(json.dumps({
        "stage": "Draw_All entry / pending presentation", "frames": result,
        "scope": "complete rendered cycle with observed car pose; audio and wall-clock timing not compared"
    }, indent=2)+"\n")
    print(f"Rendered cycle: {len(result)} frames -> {directory}", flush=True)
