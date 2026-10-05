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


def observe(read, steady=False):
    def i32(address):
        return int.from_bytes(read(address, 4), 'little', signed=True)
    row=metrics(read)
    strip_base, vertices = i32(0x77cef8), i32(0x77cef4)
    offset = i32(0x7926a4)
    speed = row['speed']
    ahead = min(6, max(2, math.floor(speed / 100)))
    centers = []
    first_ahead = min(6, max(2, math.floor(max(speed, 0) / 40)))
    selected = range(first_ahead, first_ahead + 5) if steady else (ahead,)
    for index in range(max(selected) + 1):
        if index in selected:
            strip = strip_base + 4 + offset
            kind, lanes = read(strip, 2)
            first = int.from_bytes(read(strip + 16, 2), 'little')
            a = first + i32(0x463dcc + kind * 8)
            b = first + i32(0x463dd0 + kind * 8) + lanes + 1
            points = [(i32(vertices + i * 12), i32(vertices + i * 12 + 8))
                      for i in (a, a + lanes, b, b + lanes)]
            centers.append([sum(point[j] for point in points) / 4 for j in (0, 1)])
        if index < max(selected):
            offset = i32(strip_base + 4 + offset + 20)
    center = [sum(point[j] for point in centers) / len(centers) for j in (0, 1)]
    position,heading = row['position'],row['heading']
    desired = math.atan2(center[0] - position[0], center[1] - position[1]) * 4096 / (2 * math.pi)
    difference = (desired - heading + 6144) % 4096 - 2048
    wanted = []
    if speed < (250 if steady else 400):
        wanted.append('a')
    if steady:
        row.update(steering=i32(0x792a82),yaw_rate=i32(0x792a04)/8192,
                   front_damage=i32(0x792aee),rear_damage=i32(0x792af6),
                   strip=i32(0x7926ac),
                   track_strips=int.from_bytes(read(0x466df2+i32(0x936ff4)*6,2),'little'),
                   target_strips=[first_ahead, first_ahead + 4])
        steering = max(-192, min(192, -difference * 0.6 + row['yaw_rate'] * 6))
        row['target_steering'] = steering
        if row['steering'] > steering + 40:
            wanted.append('Left')
        elif row['steering'] < steering - 40:
            wanted.append('Right')
    elif difference > 70:
        wanted.append('Left')
    elif difference < -70:
        wanted.append('Right')
    return dict(row,wanted=wanted,heading_error=difference,target=center)


class KeyboardDriver:
    def __init__(self, steady=False, movement_distance=500, stall_ticks=75, progress_ticks=200,
                 slow_ticks=0, minimum_speed=40):
        self.steady = steady
        self.movement_distance = movement_distance
        self.stall_ticks = stall_ticks
        self.progress_ticks = progress_ticks
        self.position = None
        self.last_movement = 0
        self.reverse_until = 0
        self.progress = None
        self.last_progress = 0
        self.slow_ticks = slow_ticks
        self.minimum_speed = minimum_speed
        self.slow_since = None

    def controls(self, read, tick):
        row = observe(read, self.steady)
        position = row['position']
        if self.position is None or sum(abs(a-b) for a,b in zip(position,self.position)) > self.movement_distance:
            self.position = position
            self.last_movement = tick
        if self.steady:
            # Lap progress is a confirmed checkpoint sequence, not current
            # travel: it stays at its previous maximum after reversing. Watch
            # the physical FD strip, including its normal end-to-start wrap.
            progress = row['strip']
            if self.progress is None or 0 < (progress-self.progress) % row['track_strips'] < row['track_strips']/2:
                self.last_progress = tick
            self.progress = progress
        slow_stall = False
        if self.slow_ticks:
            # Sliding along a wall can advance position/strips while the car
            # barely accelerates. Allow normal acceleration after reversing,
            # then recover through real brake/reverse keys if it stays slow.
            if tick < self.reverse_until or row['speed'] >= self.minimum_speed:
                self.slow_since = None
            elif self.slow_since is None:
                self.slow_since = tick
            slow_stall = self.slow_since is not None and tick-self.slow_since >= self.slow_ticks
            row['slow_forward_ticks'] = 0 if self.slow_since is None else tick-self.slow_since
        if tick >= self.reverse_until and (tick - self.last_movement >= self.stall_ticks or
                self.steady and tick - self.last_progress >= self.progress_ticks or slow_stall):
            self.reverse_until = tick + 100
            self.last_movement = self.reverse_until
            self.last_progress = self.reverse_until
            self.slow_since = None
        if tick < self.reverse_until:
            row['wanted'] = ['z', 'Right' if row['heading_error'] > 0 else 'Left']
            row['manoeuvre'] = 'reverse'
        else:
            row['manoeuvre'] = 'forward'
        if self.steady:
            # A reversing car rotates in the opposite direction for the same
            # wheel angle. Brake with forward steering until it backs up, and
            # keep reverse steering while the accelerator stops that motion.
            backwards = row['speed'] < -5 or (abs(row['speed']) <= 5 and tick < self.reverse_until)
            limit = 96 if backwards else 192
            steering = -row['heading_error'] * 0.6 + row['yaw_rate'] * 6
            if backwards:
                steering = -steering
            row['target_steering'] = max(-limit, min(limit, steering))
            row['steering_direction'] = 'backwards' if backwards else 'forwards'
            row['wanted'] = [key for key in row['wanted'] if key not in ('Left', 'Right')]
            tolerance = 24 if backwards or row['speed'] < 40 else 40
            if row['steering'] > row['target_steering'] + tolerance:
                row['wanted'].append('Left')
            elif row['steering'] < row['target_steering'] - tolerance:
                row['wanted'].append('Right')
        return row
