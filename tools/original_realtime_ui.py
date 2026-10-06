"""Read-only original UI acknowledgement with real X11 keyboard input.

Main-menu Button_Pressed (0x45186c) waits for Return release before opening
its destination. Driver-name directions instead repeat while held, after 20
and then five menu presentations. Acknowledge directions through selection
changes, and release other keys after their actual ReadPad acknowledgement.
No debugger stops, engine writes or original/port parity are implied.
"""
import ctypes
import ctypes.util
import struct
import time

from artifacts import check_space
from verify_configuration_persistence import OriginalUI


NAME_MENU = 0x469f70


class OriginalRealtimeUI(OriginalUI):
    def __init__(self, *args):
        super().__init__(*args)
        self.display = None
        self.input_history = []
        self.last_budget_check = 0
        try:
            self.x11 = ctypes.CDLL(ctypes.util.find_library('X11'))
            self.xtst = ctypes.CDLL(ctypes.util.find_library('Xtst'))
            self.x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
            self.x11.XOpenDisplay.restype = ctypes.c_void_p
            self.x11.XStringToKeysym.argtypes = [ctypes.c_char_p]
            self.x11.XStringToKeysym.restype = ctypes.c_ulong
            self.x11.XKeysymToKeycode.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
            self.x11.XKeysymToKeycode.restype = ctypes.c_ubyte
            self.x11.XSync.argtypes = [ctypes.c_void_p, ctypes.c_int]
            self.x11.XCloseDisplay.argtypes = [ctypes.c_void_p]
            self.xtst.XTestFakeKeyEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint,
                                                  ctypes.c_int, ctypes.c_ulong]
            self.xtst.XTestFakeKeyEvent.restype = ctypes.c_int
            self.display = self.x11.XOpenDisplay(self.env['DISPLAY'].encode())
            if not self.display:
                raise RuntimeError('Private original X11 display unavailable')
        except BaseException:
            self.stop()
            raise

    def edge(self, code, down):
        symbol = self.x11.XStringToKeysym(code.encode())
        key = self.x11.XKeysymToKeycode(self.display, symbol)
        if not symbol or not key:
            raise ValueError('Unknown X11 key: ' + code)
        if not self.xtst.XTestFakeKeyEvent(self.display, key, int(down), 0):
            raise RuntimeError('XTEST original keyboard edge rejected')
        self.x11.XSync(self.display, False)

    def stop(self):
        if self.display:
            self.x11.XCloseDisplay(self.display)
            self.display = None
        super().stop()

    def budget_check(self):
        check_space(self.output)
        self.last_budget_check = time.monotonic()

    def wait(self, predicate, timeout=20):
        end = min(self.deadline, time.monotonic() + timeout)
        while time.monotonic() < end:
            if time.monotonic() - self.last_budget_check >= 1:
                self.budget_check()
            result = predicate()
            if result:
                return result
            time.sleep(.001)
        raise TimeoutError('Original condition timed out: ' + str(self.state()))

    def observed_menu(self):
        menu = self.integer(0x940010)
        result = dict(level=self.integer(0x936ff4), menu=menu,
                      main_ring=(self.read(0x46965c, 4) + self.read(0x469674, 4)).hex(),
                      mode_ring=self.read(0x46a258, 4).hex(),
                      type_ring=self.read(0x46a5dc, 4).hex(),
                      result_ring=(self.read(0x46a3e4, 4) + self.read(0x46a3fc, 4)).hex(),
                      race_over_ring=(self.read(0x46b660, 4) + self.read(0x46b678, 4)).hex(),
                      type=self.integer(0x4673f4), mode=self.integer(0x4673f8),
                      car=self.integer(0x467400), track=self.integer(0x4673fc),
                      player=self.integer(0x93decc), count=self.integer(0x467658),
                      names=self.read(0x93e318, 120).hex())
        if menu == NAME_MENU:
            pointer = int.from_bytes(self.read(0x469fd4, 4), 'little')
            result.update(cursor=list(struct.unpack('<hh', self.read(0x469f34, 4))),
                          entered=self.read(pointer + 8, 10).split(b'\0')[0].decode('ascii'),
                          title=self.text(0x46a024))
        return result

    def key(self, code):
        self.settled()
        before = self.observed_menu()
        vk = {'Right': 0x27, 'Left': 0x25, 'Down': 0x28, 'Up': 0x26,
              'Return': 0x0d, 'Escape': 0x1b}[code]
        mapping = self.read(0x46302c, 14)
        bits = (1, 8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200,
                0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000)
        index = next(i for i in (0, 1, 2, 3, 4, 5, 8, 6, 9, 7, 10, 11, 12, 13)
                     if mapping[i] == vk)
        mask = bits[index]
        self.wait(lambda: int.from_bytes(self.read(0x754448, 2), 'little') & mask == 0)
        expected = None
        no_change = False
        if before['menu'] == NAME_MENU:
            x, y = before['cursor']
            column, row = (x - 0x30) // 16, (y - 0x7b) // 17
            if (x != column * 16 + 0x30 or y != row * 17 + 0x7b or
                    not (0 <= column < 14 and 0 <= row < 5)):
                raise RuntimeError('Original driver-name grid outside bounds')
            if code in ('Left', 'Right', 'Up', 'Down'):
                if code == 'Right': column = (column + 1) % 14
                elif code == 'Left': column = (column - 1) % 14
                elif code == 'Down': row = (row + 1) % 5
                else: row = (row - 1) % 5
                expected = [column * 16 + 0x30, row * 17 + 0x7b]
            elif code == 'Return':
                cell = row * 14 + column
                no_change = ((cell < 68 and len(before['entered']) == 8) or
                             (cell == 68 and not before['entered']))

        def effect():
            current = self.observed_menu()
            if before['menu'] == NAME_MENU:
                if current['menu'] != NAME_MENU:
                    return current
                if expected is not None:
                    return current if current['cursor'] != before['cursor'] else None
                return current if any(current[k] != before[k]
                                      for k in ('entered', 'names', 'title')) else None
            return current if current != before else None

        # Do the directory walk before holding a direction. Repeating it at
        # every poll, plus spawning keyup processes, can miss the repeat edge.
        self.budget_check()
        record = dict(key=code, before=before, expected_cursor=expected,
                      acknowledgement='selection-change' if expected else 'pad-mask',
                      begin_ns=time.monotonic_ns())
        self.input_history.append(record)
        released = False
        self.edge(code, True)
        try:
            if expected is None:
                self.wait(lambda: self.controls() & mask or
                          self.integer(0x936ff4) != before['level'], 8)
                record['acknowledged_ns'] = time.monotonic_ns()
                self.edge(code, False)
                record['released_ns'] = time.monotonic_ns()
                released = True
            observed = self.observed_menu() if no_change else self.wait(effect, 8)
            record.update(effect=observed, effect_ns=time.monotonic_ns())
        finally:
            if not released:
                self.edge(code, False)
                record['released_ns'] = time.monotonic_ns()
        self.wait(lambda: int.from_bytes(self.read(0x754448, 2), 'little') & mask == 0)
        after = self.observed_menu()
        record.update(after=after, end_ns=time.monotonic_ns())
        if expected is not None and (after['menu'] != NAME_MENU or after['cursor'] != expected):
            raise RuntimeError('Original direction overshot one name-grid step: ' + str(after))
        if no_change and any(after[k] != before[k] for k in ('menu', 'cursor', 'entered', 'names')):
            raise RuntimeError('Ignored original name edit changed the dialog')
        print('Observed original key:', code, hex(after['menu']), after.get('cursor'),
              after.get('entered'), flush=True)
        return record
