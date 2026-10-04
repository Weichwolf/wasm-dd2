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


def record_cycle(output, frames, entry, hardware=False, already_at_entry=False, wanted_pairs=None):
    directory = Path(output) / "cycle"
    directory.mkdir()
    kind = gdb.BP_HARDWARE_BREAKPOINT if hardware else gdb.BP_BREAKPOINT
    enter = gdb.Breakpoint(f"*0x{entry:x}", type=kind)
    enter.silent = True
    result = []
    wanted = set(map(tuple, wanted_pairs)) if wanted_pairs is not None else None
    if wanted is not None and len(wanted) != frames:
        raise RuntimeError("Expected distinct highlight/card phase pairs")
    # The independent periods are 64 (slab highlight) and 51 (card +20 mod255).
    # Observe small counters at every entry; dump only the requested 64 images.
    max_entries = frames * 51 + frames if wanted is not None else frames
    observed = 0
    for step in range(max_entries):
        if step or not already_at_entry:
            gdb.execute("continue")
        if int(gdb.parse_and_eval("$pc")) != entry:
            raise RuntimeError("Cycle failed to reach Draw_All entry")
        inferior = gdb.selected_inferior()
        read = lambda address, size: bytes(inferior.read_memory(address, size))
        phase = int.from_bytes(read(0x4699cc, 4), "little", signed=True)
        card_phase = int.from_bytes(read(0x467390, 4), "little", signed=True) % 255
        cf = int.from_bytes(read(0x462ff0, 4), "little", signed=True)
        level = int.from_bytes(read(0x936ff4, 4), "little", signed=True)
        if phase not in range(64):
            raise RuntimeError(f"Highlight cycle is not settled: phase={phase}")
        observed += 1
        if wanted is not None:
            pair = (phase, card_phase)
            if pair not in wanted:
                continue
            wanted.remove(pair)
        index = len(result)
        prefix = f"frame{index:03d}"
        (directory / f"{prefix}-framebuf.bin").write_bytes(read(0x700450, 307200))
        (directory / f"{prefix}-palette.bin").write_bytes(read(0x700050, 1024))
        result.append({"index": index, "phase": phase, "card_phase": card_phase, "cf": cf, "level": level,
                       "prefix": prefix,
                       "race_car": int.from_bytes(read(0x467400, 4), "little", signed=True),
                       "car_angles": [int.from_bytes(read(0x468eb4+2*i, 2), "little", signed=True)
                                      for i in range(3)],
                       "poly_list": int.from_bytes(read(0x940010, 4), "little")})
        result[-1].update({name: int.from_bytes(read(address, size), "little", signed=size == 4)
                           for name, address, size in (
                               ("race_mode", 0x4673f8, 4), ("race_type", 0x4673f4, 4),
                               ("race_track", 0x4673fc, 4), ("playable_tracks", 0x467404, 4),
                               ("playable_bowls", 0x467408, 4), ("track_locked", 0x46a920, 1),
                               ("saved_track", 0x940224, 4), ("sound_volume", 0x467410, 4),
                               ("working_sound_volume", 0x93fd20, 4), ("master_sfx_volume", 0x462d84, 4))})
        check_space(output)
        if len(result) == frames:
            break
    enter.delete()
    if len(result) != frames:
        raise RuntimeError("Independent animation phases did not reach requested pairs")
    (directory / "cycle.json").write_text(json.dumps({
        "stage": "Draw_All entry / pending presentation", "frames": result,
        "observed_entries": observed, "wanted_pairs": wanted_pairs,
        "scope": "complete rendered cycle with observed car pose; audio and wall-clock timing not compared"
    }, indent=2)+"\n")
    print(f"Rendered cycle: {len(result)} frames -> {directory}", flush=True)
