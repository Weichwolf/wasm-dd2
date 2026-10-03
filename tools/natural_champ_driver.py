"""Read-only road follower for recording actual original keyboard input.

This driver does not promise a win or a finish by completed laps. A destroyed
car is a legitimate natural completion, distinguished in the capture report.
Ports replay the recorded key transitions, rather than running another driver.
"""
import math


def metrics(read):
    def i32(address):
        return int.from_bytes(read(address, 4), 'little', signed=True)
    player = i32(0x93ded0)
    if player != 0:
        raise ValueError('This diagnostic drives player car zero only')
    return dict(speed=i32(0x792a7a),heading=int.from_bytes(read(0x78a792,2),'little')&4095,
                position=[i32(0x78a744),i32(0x78a74c)],
                lap=int.from_bytes(read(0x795c48,2),'little'),
                lap_progress=int.from_bytes(read(0x795c4a,2),'little'),dead=i32(0x792ac6),
                planar_speed=i32(0x792a76),finished_laps=int.from_bytes(read(0x795c52,2),'little'))


def observe(read):
    def i32(address):
        return int.from_bytes(read(address, 4), 'little', signed=True)
    row=metrics(read)
    strip_base, vertices = i32(0x77cef8), i32(0x77cef4)
    offset = i32(0x7926a4)
    speed = row['speed']
    ahead = min(6, max(2, math.floor(speed / 100)))
    for _ in range(ahead):
        offset = i32(strip_base + 4 + offset + 20)
    strip = strip_base + 4 + offset
    kind, lanes = read(strip, 2)
    first = int.from_bytes(read(strip + 16, 2), 'little')
    a = first + i32(0x463dcc + kind * 8)
    b = first + i32(0x463dd0 + kind * 8) + lanes + 1
    points = [(i32(vertices + i * 12), i32(vertices + i * 12 + 8))
              for i in (a, a + lanes, b, b + lanes)]
    center = [sum(point[j] for point in points) / 4 for j in (0, 1)]
    position,heading = row['position'],row['heading']
    desired = math.atan2(center[0] - position[0], center[1] - position[1]) * 4096 / (2 * math.pi)
    difference = (desired - heading + 6144) % 4096 - 2048
    wanted = []
    if speed < 400:
        wanted.append('a')
    if difference > 70:
        wanted.append('Left')
    elif difference < -70:
        wanted.append('Right')
    return dict(row,wanted=wanted,heading_error=difference,target=center)


class KeyboardDriver:
    def __init__(self):
        self.position = None
        self.last_movement = 0
        self.reverse_until = 0

    def controls(self, read, tick):
        row = observe(read)
        position = row['position']
        if self.position is None or sum(abs(a-b) for a,b in zip(position,self.position)) > 500:
            self.position = position
            self.last_movement = tick
        if tick < self.reverse_until:
            row['wanted'] = ['z', 'Right' if row['heading_error'] > 0 else 'Left']
            row['manoeuvre'] = 'reverse'
        elif tick - self.last_movement >= 75:
            self.reverse_until = tick + 100
            self.last_movement = self.reverse_until
            row['wanted'] = ['z', 'Right' if row['heading_error'] > 0 else 'Left']
            row['manoeuvre'] = 'reverse'
        else:
            row['manoeuvre'] = 'forward'
        return row
