"""Real keyboard navigation for Enter_Driver_Names' original 14 by 5 grid.

The File Manager uses a different name grid. These keys apply only to the
driver/fastest-lap dialog, whose name limit is eight characters. The optional
ninth character is a real input probe for that limit, not a supported name.
"""

GRID = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!?-., ' + '\xff\xfe'
COLUMNS, ROWS = 14, 5
BACKSPACE, ACCEPT = '\xff', '\xfe'


def move_keys(start, destination):
    """Use the original wrap directions between two visible grid cells."""
    row, column = start
    target_row, target_column = destination
    keys = []
    for actual, wanted, size, forward, reverse in (
            (column, target_column, COLUMNS, 'Right', 'Left'),
            (row, target_row, ROWS, 'Down', 'Up')):
        distance = (wanted - actual) % size
        keys += [forward] * distance if distance <= size // 2 else [reverse] * (size - distance)
    return keys


def name_actions(name, *, delete_last=False, confirm=True):
    """Plan real inputs with expected cursor and visible-buffer checkpoints."""
    if len(name) > 9 or any(char not in GRID[:-2] for char in name):
        raise ValueError('Use zero to eight supported characters, or nine for a limit probe')
    row, column, entered = 0, 0, ''
    actions = []
    selected = name + (BACKSPACE if delete_last else '') + (ACCEPT if confirm else '')
    for char in selected:
        target_row, target_column = divmod(GRID.index(char), COLUMNS)
        for key in move_keys((row, column), (target_row, target_column)):
            if key == 'Right': column = (column + 1) % COLUMNS
            elif key == 'Left': column = (column - 1) % COLUMNS
            elif key == 'Down': row = (row + 1) % ROWS
            else: row = (row - 1) % ROWS
            actions.append(dict(key=key, cursor=[column * 16 + 0x30, row * 17 + 0x7b], entered=entered))
        if char == BACKSPACE: entered = entered[:-1]
        elif char != ACCEPT and len(entered) < 8: entered += char
        actions.append(dict(key='Return', cursor=[column * 16 + 0x30, row * 17 + 0x7b],
                            entered=entered, accepts=char == ACCEPT))
    if not confirm:
        actions.append(dict(key='Escape', entered=entered, cancels=True))
    return actions


def enter_driver_name(ui, name, *, delete_last=False, confirm=True):
    """Dispatch and acknowledge genuine keys, checking read-only UI observations."""
    import struct
    if ui.integer(0x940010) != 0x469f70:
        raise RuntimeError('Driver/fastest-lap name dialog required')
    actual = list(struct.unpack('<hh', ui.read(0x469f34, 4)))
    if actual != [0x30, 0x7b]:
        raise RuntimeError('Name grid must start at its real initial selection')
    actions = name_actions(name, delete_last=delete_last, confirm=confirm)
    for action in actions:
        ui.key(action['key'])
        if action.get('accepts') or action.get('cancels'):
            continue
        if ui.integer(0x940010) != 0x469f70:
            raise RuntimeError('Name input unexpectedly left its dialog')
        actual = list(struct.unpack('<hh', ui.read(0x469f34, 4)))
        pointer = int.from_bytes(ui.read(0x469fd4, 4), 'little')
        entered = ui.read(pointer + 8, 10).split(b'\0', 1)[0].decode('ascii')
        if actual != action['cursor'] or entered != action['entered']:
            raise RuntimeError(f'Actual name grid differs: {actual!r}, {entered!r}; expected {action!r}')
    return actions
