"""Read-only real-keyboard road policy extracted from the verified original trial.

Only car zero is driven; game state, clocks, RNG and results are never written.
This host recording policy does not promise a competitive lap or racing parity.
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


class PredictiveKeyboardDriver:
    @staticmethod
    def steer(read, row):
        steps = int.from_bytes(read(0x7746b8, 4), 'little', signed=True)
        if not 1 <= steps <= 8:
            raise ValueError('Original adaptive physics batch outside its game bounds')
        steering = row['steering']
        span = 32 * steps
        coast = max(0, steering - span) if steering >= 0 else min(0, steering + span)
        candidates = [(None, coast), ('Left', max(-256, steering - span)),
                      ('Right', min(256, steering + span))]
        key, predicted = min(candidates, key=lambda item: abs(item[1] - row['target_steering']))
        row['wanted'] = [k for k in row['wanted'] if k not in ('Left', 'Right', 'space')]
        if key:
            row['wanted'].append(key)
        row.update(predicted_steering=predicted, predicted_physics_steps=steps)
        return row


class ArcRoadKeyboardDriver:
    """Follow the original linked road's arc instead of averaging far strips.

    This is a real-keyboard recording policy only. No engine fields, clocks,
    random values, race progress or results are changed. A completed positive
    multiplayer fixture still requires actual regular finishes by both humans.
    """
    def __init__(self):
        self.centers=None
        self.indices={}
        self.strips={}
        self.lane_fraction=None
        self.anchor=None
        self.moved_tick=0
        self.reverse_until=0

    def strip(self,read,offset):
        if offset in self.strips:return self.strips[offset]
        def i32(a):return int.from_bytes(read(a,4),'little',signed=True)
        base,vertices=i32(0x77cef8),i32(0x77cef4)
        address=base+4+offset
        kind,lanes=read(address,2)
        if not 0<=offset<0x100000 or kind>9 or not 1<=lanes<=32:
            raise ValueError('Original road strip outside recording bounds')
        vertex=int.from_bytes(read(address+16,2),'little')
        a=vertex+i32(0x463dcc+kind*8)
        b=vertex+i32(0x463dd0+kind*8)+lanes+1
        points=[]
        lane=min(lanes-1,math.floor(self.lane_fraction*lanes))
        for k in (a+lane,a+lane+1,b+lane,b+lane+1):
            raw=read(vertices+k*12,12)
            points.append([int.from_bytes(raw[j:j+4],'little',signed=True) for j in (0,8)])
        result=dict(center=[sum(p[j] for p in points)/4 for j in (0,1)],
                    next=i32(address+20),previous=i32(address+24),kind=kind,
                    number=int.from_bytes(read(address+14,2),'little'))
        self.strips[offset]=result
        return result

    def route(self,read):
        if self.centers is not None:return
        base=int.from_bytes(read(0x77cef8,4),'little',signed=True)
        current=int.from_bytes(read(0x7926a4,4),'little',signed=True)
        lanes=read(base+4+current+1,1)[0]
        # FUN_00428678 reads (int at terrain+0x27) >> 24: lane is
        # the high byte at +0x2a, not the low byte of that packed word.
        lane=int.from_bytes(read(0x7926ba,1),'little',signed=True)
        if not 1<=lanes<=32 or not 0<=lane<lanes:
            raise ValueError('Initial original road lane outside its strip')
        # Keep the starting lane instead of crossing the whole starting grid
        # to the road midpoint. FUN_00428678 uses these same four lane vertices.
        self.lane_fraction=(lane+.5)/lanes
        # The main loop starts at offset zero. The checkpoint count is not
        # the number of all FD strips: Generate_Strip_Normals and
        # Init_Track_Strip_Numbers also traverse separate kind-8 branches.
        offset=0
        centers=[]
        for index in range(4096):
            if offset in self.indices:raise ValueError('Premature original road loop')
            self.indices[offset]=index
            strip=self.strip(read,offset)
            centers.append(strip['center'])
            offset=strip['next']
            if offset==0:break
        else:raise ValueError('Original road loop did not close')
        self.centers=centers

    def target(self,read,row):
        self.route(read)
        offset=int.from_bytes(read(0x7926a4,4),'little',signed=True)
        self.strip(read,offset)
        current=self.indices.get(offset)
        pos=row['position']
        # Use the actual car's branch in both directions. At a split the
        # main loop's next link does not enter its side branch, so reconstruct
        # the local path around the car rather than indexing the main loop.
        before=[];cursor=offset
        for _ in range(8):
            cursor=self.strip(read,cursor)['previous'];before.append(cursor)
        path=before[::-1]+[offset];cursor=offset
        for _ in range(9):
            cursor=self.strip(read,cursor)['next'];path.append(cursor)
        nearest=None
        for i in range(len(path)-1):
            a=self.strip(read,path[i])['center'];b=self.strip(read,path[i+1])['center']
            delta=[b[j]-a[j] for j in (0,1)];length=sum(v*v for v in delta)
            fraction=max(0,min(1,sum((pos[j]-a[j])*delta[j] for j in (0,1))/length)) if length else 0
            point=[a[j]+fraction*delta[j] for j in (0,1)]
            distance=math.dist(pos,point)
            if nearest is None or distance<nearest[0]:nearest=(distance,i,point)
        distance,i,point=nearest
        remaining=max(900,min(2400,max(0,row['speed'])*7))
        cursor=path[i+1]
        for _ in range(4096):
            end=self.strip(read,cursor)['center'];length=math.dist(point,end)
            if length>=remaining:
                target=[point[j]+(end[j]-point[j])*remaining/length for j in (0,1)]
                return target,distance,current
            remaining-=length;point=end;i+=1
            cursor=path[i+1] if i+1<len(path) else self.strip(read,cursor)['next']
        raise ValueError('Degenerate original road loop')

    def controls(self,read,tick):
        row=observe(read,True)
        target,distance,index=self.target(read,row)
        offset=int.from_bytes(read(0x7926a4,4),'little',signed=True)
        strip=self.strip(read,offset)
        row.update(road_offset=offset,road_kind=strip['kind'],road_number=strip['number'],
                   road_lane_fraction=self.lane_fraction,
                   road_branch=index is None,
                   pit_in=int.from_bytes(read(0x46704c,4),'little'),
                   pit_stop=int.from_bytes(read(0x467054,4),'little'))
        angle=math.atan2(target[0]-row['position'][0],target[1]-row['position'][1])*4096/(2*math.pi)
        error=(angle-row['heading']+6144)%4096-2048
        if self.anchor is None or math.dist(self.anchor,row['position'])>=300:
            self.anchor=row['position'][:];self.moved_tick=tick
        if tick>=self.reverse_until and tick-self.moved_tick>=180 and abs(row['speed'])<10:
            self.reverse_until=tick+180;self.moved_tick=self.reverse_until
        reverse=tick<self.reverse_until
        desired=-error*.75+row['yaw_rate']*4
        backwards=row['speed']<-5 or reverse and row['speed']<=5
        if backwards:desired=-desired
        fast=abs(error)>500 and abs(row['speed'])<80
        limit=80 if abs(error)>300 else 140 if distance>800 else 190
        wanted=['z'] if reverse else ['a'] if row['speed']<limit else ['z'] if row['speed']>limit+25 else []
        if reverse:desired=0;fast=False
        row.update(target=target,heading_error=error,road_distance=distance,route_index=index,
            target_steering=max(-400 if fast else -240,min(400 if fast else 240,desired)),
            wanted=wanted,manoeuvre='reverse' if reverse else 'forward',
            recovery_phase='back-away' if reverse else 'arc-follow',
            steering_direction='backwards' if backwards else 'forwards')
        if row['pit_in']:
            # Play_Game uses FUN_00447300 rather than keyboard handling in
            # the pits. Release real keys while the original drives itself.
            self.anchor=row['position'][:];self.moved_tick=tick
            self.reverse_until=tick
            row.update(wanted=[],manoeuvre='pit',recovery_phase='original-pit-control')
            return row
        if not fast:return PredictiveKeyboardDriver.steer(read,row)
        steps=int.from_bytes(read(0x7746b8,4),'little',signed=True)
        if not 1<=steps<=8:raise ValueError('Original adaptive physics batch outside bounds')
        steering=row['steering'];span=64*steps
        coast=max(0,steering-32*steps) if steering>=0 else min(0,steering+32*steps)
        key,predicted=min([(None,coast),('Left',max(-511,steering-span)),
                          ('Right',min(511,steering+span))],key=lambda p:abs(p[1]-row['target_steering']))
        if key:row['wanted']+=['space',key]
        row.update(predicted_steering=predicted,predicted_physics_steps=steps)
        return row
