"""Bounded read-only hardware observations of original DirectDraw call returns.

The default PCs come from unchanged dd2h.exe PutDrawEnv/PutDispEnv call sites.
Optional Wine inspection uses two original Restore returns and two guarded Wine
format/error sites, still within four hardware slots. The profile is specific
to the recorded Debian Wine 10.0 DLL hash and rejects other implementations.
No software breakpoint, engine write, API interception or HRESULT override is used.
"""
import hashlib
import json
import time
from pathlib import Path

import gdb


def run(directory, inspect_wine=False):
    output = Path(directory)
    inferior = gdb.selected_inferior()
    rows, counts = [], {}

    def word(address):
        return int.from_bytes(inferior.read_memory(address, 4).tobytes(), 'little')

    class Return(gdb.Breakpoint):
        def __init__(self, address, kind):
            super().__init__(f'*0x{address:x}', type=gdb.BP_HARDWARE_BREAKPOINT)
            self.silent = True
            self.kind = kind

        def stop(self):
            result = int(gdb.parse_and_eval('$eax')) & 0xffffffff
            category = self.kind + ('-success' if result == 0 else '-failure')
            counts[category] = counts.get(category, 0) + 1
            # Retain two initial successful calls and at most eight failed calls
            # per site. These are observations, not a complete frame sequence.
            if counts[category] <= (2 if result == 0 else 8):
                primary, back = word(0x46043c), word(0x460440)
                rows.append(dict(
                    kind=self.kind, host_ns=time.monotonic_ns(),
                    pc=int(gdb.parse_and_eval('$pc')), hr=f'0x{result:08x}',
                    active=word(0x46042c), primary=primary, back=back,
                    primary_table=word(primary), back_table=word(back),
                    primary_restore=word(word(primary) + 0x6c),
                    back_restore=word(word(back) + 0x6c),
                    back_lock=word(word(back) + 0x64)))
            return (counts.get('draw-restore-return-failure', 0) >= 8 or
                    counts.get('flip-restore-return-failure', 0) >= 8 or
                    (output / 'observer-stop').exists())

    profile = None
    if inspect_wine:
        backend = Path('/usr/lib/i386-linux-gnu/wine/i386-windows/ddraw.dll')
        sha = hashlib.sha256(backend.read_bytes()).hexdigest()
        expected = '87c2d69d6879e43b34eefe4b9781a5995acb6dae1b510bffe4b1d33b4374c0be'
        if sha != expected:
            raise ValueError('Wine Restore profile does not support this DLL hash')
        primary = word(0x46043c)
        base = word(word(primary) + 0x6c) - 0x29cc0
        guards = {0x29cc0: '5589e55383ec148b5d08', 0x3220b: '39d0',
                  0x322c0: '83c45cbe4b027688'}
        for rva, expected_bytes in guards.items():
            actual = inferior.read_memory(base + rva, len(bytes.fromhex(expected_bytes))).tobytes()
            if actual.hex() != expected_bytes:
                raise ValueError('Mapped Wine Restore instructions differ from the guarded profile')
        profile = dict(dll_sha256=sha, image_base=base, instruction_guards=guards,
                       surface7_restore_rva=0x320e0,
                       format_ids={'P8_UINT': 10, 'B5G6R5_UNORM': 115, 'B8G8R8X8_UNORM': 118})

        class WinePoint(gdb.Breakpoint):
            def __init__(self, rva, kind):
                super().__init__(f'*0x{base+rva:x}', type=gdb.BP_HARDWARE_BREAKPOINT)
                self.silent = True
                self.kind = kind

            def stop(self):
                counts[self.kind] = counts.get(self.kind, 0) + 1
                if counts[self.kind] <= 8:
                    ebp = int(gdb.parse_and_eval('$ebp'))
                    surface = int(gdb.parse_and_eval('$ebx'))
                    device = word(surface + 0x48)
                    rows.append(dict(kind=self.kind, host_ns=time.monotonic_ns(),
                        pc=int(gdb.parse_and_eval('$pc')), caller=word(ebp + 4),
                        surface=surface, device=device, device_state=word(device + 0x54),
                        display=dict(width=word(ebp - 0x44), height=word(ebp - 0x40),
                                     format=word(ebp - 0x38)),
                        texture=dict(width=word(ebp - 0x18), height=word(ebp - 0x14),
                                     format=word(ebp - 0x30))))
                return (output / 'observer-stop').exists()

        points = [WinePoint(0x3220b, 'wine-format-compare'),
                  WinePoint(0x322c0, 'wine-wrong-mode'),
                  Return(0x412c53, 'draw-restore-return'),
                  Return(0x412cd5, 'flip-restore-return')]
    else:
        points = [Return(0x412c39, 'lock-return'),
                  Return(0x412c53, 'draw-restore-return'),
                  Return(0x412cc1, 'flip-return'),
                  Return(0x412cd5, 'flip-restore-return')]
    (output / 'observer-ready').touch()
    try:
        gdb.execute('continue')
    finally:
        for point in points:
            point.delete()
        (output / 'ddraw-returns.json').write_text(json.dumps(dict(
            scope=__doc__.strip(), hardware_breakpoints=4,
            engine_state_writes=False, wine_profile=profile,
            rows=rows, counts=counts), indent=2) + '\n')
