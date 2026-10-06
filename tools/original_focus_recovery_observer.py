"""Bounded read-only hardware observations of original DirectDraw call returns.

The four PCs come from the unchanged dd2h.exe PutDrawEnv/PutDispEnv call sites.
No software breakpoint, engine write, API interception or HRESULT override is used.
"""
import json
import time
from pathlib import Path

import gdb


def run(directory):
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
            engine_state_writes=False, rows=rows, counts=counts), indent=2) + '\n')
