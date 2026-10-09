"""Build editable, entirely authored race-car/cockpit geometry in Blender 4.3.

No original game data, external meshes, fonts, textures or sound are read.
Run with: blender --background --factory-startup --python this_file -- --output assets
Preview renders are explicitly directed to /tmp/wasm-dd2/.
"""
import argparse
import json
import math
from pathlib import Path
import struct
import sys

import bpy
from mathutils import Vector

if bpy.app.version[:2] != (4, 3):
    raise RuntimeError('Reproducible asset recipes require Blender 4.3')

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--output', type=Path, default=ROOT / 'assets')
parser.add_argument('--preview', type=Path)
args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
output = args.output.resolve()
preview = args.preview.resolve() if args.preview else None
if preview and not str(preview).startswith('/tmp/wasm-dd2/'):
    raise ValueError('Preview/verification renders belong under /tmp/wasm-dd2/')
for directory in ('blender', 'runtime/models', 'runtime/textures'):
    (output / directory).mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0
scene.render.engine = 'CYCLES'
scene.cycles.samples = 128
# Debian's Blender build does not include OpenImageDenoise. Use actual samples
# so headless review also works without an optional denoiser implementation.
scene.cycles.use_denoising = False
bpy.context.preferences.filepaths.save_version = 0
scene.render.resolution_x = 1280
scene.render.resolution_y = 720
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.view_settings.view_transform = 'AgX'
scene.render.film_transparent = False
scene.world.color = (0.09, 0.11, 0.15)


def material(name, color, metal=0.0, rough=0.5):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    shader = mat.node_tree.nodes.get('Principled BSDF')
    shader.inputs['Base Color'].default_value = (*color, 1.0)
    shader.inputs['Metallic'].default_value = metal
    shader.inputs['Roughness'].default_value = rough
    mat.diffuse_color = (*color, 1.0)
    return mat


paint = material('SaffronPaint', (0.93, 0.26, 0.035), 0.62, 0.24)
paint.node_tree.nodes.get('Principled BSDF').inputs['Coat Weight'].default_value = 0.4
navy = material('NavyPaint', (0.015, 0.052, 0.09), 0.45, 0.3)
metal = material('BrushedAlloy', (0.31, 0.36, 0.42), 0.85, 0.32)
chrome = material('MachinedAlloy', (0.66, 0.72, 0.79), 0.92, 0.2)
rubber = material('Rubber', (0.015, 0.018, 0.023), 0.0, 0.83)
cloth = material('SeatFabric', (0.027, 0.039, 0.052), 0.0, 0.91)
carbon = material('DashboardComposite', (0.019, 0.025, 0.032), 0.08, 0.5)
red = material('SafetyRed', (0.66, 0.025, 0.012), 0.05, 0.35)
white = material('IvoryMarkings', (0.86, 0.9, 0.88), 0.02, 0.4)
black = material('GaugeBlack', (0.005, 0.009, 0.016), 0.05, 0.48)
glass = material('WindowGlass', (0.10, 0.19, 0.24), 0.0, 0.13)
glass.node_tree.nodes.get('Principled BSDF').inputs['Transmission Weight'].default_value = 0.82
glass.node_tree.nodes.get('Principled BSDF').inputs['Alpha'].default_value = 0.28
glass.diffuse_color = (0.10, 0.19, 0.24, 0.28)
# A thin clear laminate retains the forward view. The mix avoids treating a
# single polygon as the boundary of a solid glass volume in review renders.
glass.diffuse_color = (0.10, 0.19, 0.24, 0.18)
transparent = glass.node_tree.nodes.new('ShaderNodeBsdfTransparent')
laminate = glass.node_tree.nodes.new('ShaderNodeBsdfGlass')
laminate.inputs['Color'].default_value = (0.92, 0.96, 1.0, 1.0)
laminate.inputs['Roughness'].default_value = 0.07
mix = glass.node_tree.nodes.new('ShaderNodeMixShader')
mix.inputs[0].default_value = 0.18
glass.node_tree.links.new(transparent.outputs[0], mix.inputs[1])
glass.node_tree.links.new(laminate.outputs[0], mix.inputs[2])
glass.node_tree.links.new(mix.outputs[0], glass.node_tree.nodes.get('Material Output').inputs['Surface'])


texture_metadata = json.loads((output / 'runtime/textures/materials.json').read_text())
texture_sets = texture_metadata['textures']
texture_bindings = {
    paint.name: 'paint-flake', navy.name: 'paint-flake', rubber.name: 'tire-rubber',
    cloth.name: 'seat-fabric', carbon.name: 'carbon-twill',
    metal.name: 'brushed-alloy', chrome.name: 'brushed-alloy',
}
def bind_maps(mat, name):
    recipe = next(item for item in texture_sets if item['name'] == name)
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    shader = nodes.get('Principled BSDF')
    maps = {}
    for kind in ('albedo', 'roughness', 'normal'):
        image = bpy.data.images.load(str(output / 'runtime' / recipe['maps'][kind]['path']), check_existing=True)
        image.colorspace_settings.name = 'sRGB' if kind == 'albedo' else 'Non-Color'
        image.pack()
        node = nodes.new('ShaderNodeTexImage')
        node.image = image
        node.extension = 'REPEAT'
        maps[kind] = node
    tint = nodes.new('ShaderNodeMixRGB')
    tint.blend_type = 'MULTIPLY'
    tint.inputs[0].default_value = 1.0
    tint.inputs[1].default_value = mat.diffuse_color
    links.new(maps['albedo'].outputs['Color'], tint.inputs[2])
    links.new(tint.outputs[0], shader.inputs['Base Color'])
    links.new(maps['roughness'].outputs['Color'], shader.inputs['Roughness'])
    bump = nodes.new('ShaderNodeNormalMap')
    links.new(maps['normal'].outputs['Color'], bump.inputs['Color'])
    links.new(bump.outputs[0], shader.inputs['Normal'])


for mat in (paint, navy, rubber, cloth, carbon, metal, chrome):
    bind_maps(mat, texture_bindings[mat.name])

uv_density = {'DashboardComposite': 4.0, 'SeatFabric': 4.0,
              'BrushedAlloy': 3.0, 'MachinedAlloy': 3.0, 'Rubber': 4.0}


def finish(obj, name, mat, bevel=0, smooth=False):
    obj.name = name
    obj.data.materials.append(mat)
    obj.data.update()
    uv = obj.data.uv_layers.active or obj.data.uv_layers.new(name='MaterialUV')
    for face in obj.data.polygons:
        axis = max(range(3), key=lambda component: abs(face.normal[component]))
        components = ((1, 2), (0, 2), (0, 1))[axis]
        for corner in face.loop_indices:
            point = obj.data.vertices[obj.data.loops[corner].vertex_index].co
            density = uv_density.get(mat.name, 1.0)
            uv.data[corner].uv = (point[components[0]] * density, point[components[1]] * density)
    if bevel:
        mod = obj.modifiers.new('Manufactured edge bevel', 'BEVEL')
        mod.width = bevel
        mod.segments = 2
    if smooth:
        for face in obj.data.polygons:
            face.use_smooth = True
    return obj


def box(name, location, size, mat, bevel=0.015):
    bpy.ops.mesh.primitive_cube_add(size=1, location=location)
    obj = bpy.context.object
    obj.scale = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return finish(obj, name, mat, bevel)


def mesh(name, vertices, faces, mat, bevel=0):
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], faces)
    data.update()
    obj = bpy.data.objects.new(name, data)
    scene.collection.objects.link(obj)
    return finish(obj, name, mat, bevel)


def tube(name, start, end, radius, mat, sides=12):
    direction = Vector(end) - Vector(start)
    bpy.ops.mesh.primitive_cylinder_add(vertices=sides, radius=radius,
                                       depth=direction.length, location=(Vector(start)+Vector(end))/2)
    obj = bpy.context.object
    obj.rotation_euler = direction.to_track_quat('Z', 'Y').to_euler()
    return finish(obj, name, mat, smooth=True)


def torus(name, center, major, minor, mat, rotation=(0, 0, 0), segments=40):
    bpy.ops.mesh.primitive_torus_add(major_segments=segments, minor_segments=8,
                                    major_radius=major, minor_radius=minor,
                                    location=center, rotation=rotation)
    return finish(bpy.context.object, name, mat, smooth=True)


# A welded lower shell, cut open over the cockpit; real arch cutouts let tires
# sit inside the body rather than intersecting a solid rectangular chassis.
stations = [(-2.40, 0.94, 0.78), (-1.85, 1.02, 0.86), (-1.1, 1.05, 0.9),
            (0.6, 1.05, 0.91), (1.7, 1.03, 0.87), (2.40, 0.96, 0.7)]
vertices = []
for y, width, upper in stations:
    vertices.extend([(-width*0.93, y, 0.28), (width*0.93, y, 0.28),
                     (width, y, upper), (-width, y, upper)])
faces = [(0, 3, 2, 1), (20, 21, 22, 23)]
for ring in range(len(stations)-1):
    for side in (0, 1, 3):  # upper opening retains the interior
        a = ring*4+side
        b = ring*4+(side+1)%4
        faces.append((a, b, b+4, a+4))
body = mesh('Body.Shell', vertices, [tuple(reversed(face)) for face in faces], paint)
# Give the open welded shell physical thickness before volume-based arch cuts.
shell = body.modifiers.new('Welded sheet thickness', 'SOLIDIFY')
shell.thickness = 0.018
shell.offset = -1.0
bpy.context.view_layer.objects.active = body
bpy.ops.object.modifier_apply(modifier=shell.name)
for y in (-1.45, 1.45):
    for x in (-1.03, 1.03):
        bpy.ops.mesh.primitive_cylinder_add(vertices=40, radius=0.395, depth=0.8,
                                           location=(x, y, 0.345), rotation=(0, math.pi/2, 0))
        cutter = bpy.context.object
        bpy.context.view_layer.objects.active = body
        mod = body.modifiers.new('Wheel clearance', 'BOOLEAN')
        mod.operation = 'DIFFERENCE'
        mod.object = cutter
        bpy.ops.object.modifier_apply(modifier=mod.name)
        bpy.data.objects.remove(cutter, do_unlink=True)
mod = body.modifiers.new('Folded body edges', 'BEVEL')
mod.width = 0.015
mod.segments = 2

hood = mesh('Body.Hood', [(-1.04,.95,.904),(0,.95,.938),(1.04,.95,.904),
                         (-.964,2.37,.715),(0,2.37,.732),(.964,2.37,.715)],
            [(0,1,4,3),(1,2,5,4)], paint, .008)
trunk = mesh('Body.Deck', [(-1,-1.12,.91),(1,-1.12,.91),(.93,-2.36,.81),(-.93,-2.36,.81)],
             [(3,2,1,0)], navy, .01)
roof = box('Body.Roof', (0,-.13,1.38), (1.52,1.48,.045), navy, .04)
for sign in (-1,1):
    tube('Body.FrontPillar', (sign*.76,.61,1.37), (sign*.99,1.04,.9), .043, navy)
    tube('Body.RearPillar', (sign*.76,-.86,1.37), (sign*1.0,-1.27,.89), .049, navy)
    tube('Body.WindowSill', (sign*1.02,-1.22,.92), (sign*1.02,1.05,.94), .025, navy)
    box('Body.DoorHandle', (sign*1.06,-.64,.86), (.021,.15,.035), black, .008)
    box('Body.SideSkirt', (sign*1.01,0,.29), (.09,2.2,.11), navy, .024)
    # The splitter and stripes use authored geometry, not image decals.
    mesh('Body.SideStripe', [(sign*1.055,-1.1,.67),(sign*1.055,.9,.67),
                            (sign*1.055,.9,.79),(sign*1.055,-1.1,.79)],
         [(0,1,2,3) if sign > 0 else (3,2,1,0)],white)
    box('Body.MirrorHousing', (sign*1.12,.56,1.03), (.18,.24,.105), navy, .035)
    box('Body.MirrorLens', (sign*1.12,.44,1.04), (.135,.015,.065), chrome, .012)
mesh('Glass.Windscreen',[(-.96,1.025,.96),(.96,1.025,.96),(.73,.60,1.35),(-.73,.60,1.35)],[(3,2,1,0)],glass)
mesh('Glass.Rear',[(-.96,-1.25,.94),(.96,-1.25,.94),(.73,-.86,1.35),(-.73,-.86,1.35)],[(0,1,2,3)],glass)
for sign in (-1,1):
    mesh('Glass.Side',[ (sign*.98,-1.20,.95),(sign*.98,1.0,.96),
                       (sign*.74,.58,1.34),(sign*.74,-.85,1.34)],
         [(0,1,2,3) if sign > 0 else (3,2,1,0)],glass)
box('Body.FrontSplitter',(0,2.39,.26),(2.18,.25,.045),carbon,.015)
box('Body.RearBumper',(0,-2.41,.43),(1.95,.13,.18),navy,.045)
box('Body.NoseBumper',(0,2.40,.47),(1.95,.10,.23),navy,.04)
box('Body.Grille',(0,2.459,.59),(1.07,.012,.14),black,.012)
for x in (-.34,-.17,0,.17,.34):
    box('Body.GrilleSlat',(x,2.47,.59),(.018,.01,.12),metal,.003)
for sign in (-1,1):
    box('Body.Headlight',(sign*.70,2.455,.66),(.34,.018,.13),white,.02)
    box('Body.TailLight',(sign*.66,-2.483,.69),(.39,.018,.105),red,.018)
    tube('Body.WingStay',(sign*.64,-1.94,.88),(sign*.64,-2.01,1.12),.022,metal)
box('Body.RearWing',(0,-2.01,1.14),(2.02,.26,.042),carbon,.018)
for sign in (-1,1):
    box('Body.WingEndplate',(sign*1.01,-2.01,1.15),(.028,.29,.15),navy,.01)

# Four independently named/owned wheel assemblies, with real rim, brake,
# caliper and tread detail. Low-detail export will retain their named pivots.
for y, axle in ((1.45,'F'),(-1.45,'R')):
    for sign, side in ((-1,'L'),(1,'R')):
        prefix = 'Wheel'+axle+side
        x = sign*1.005
        center = (x,y,.345)
        torus(prefix+'.Tire',center,.258,.087,rubber,(0,math.pi/2,0))
        tube(prefix+'.Rim',(x-sign*.12,y,.345),(x+sign*.12,y,.345),.235,metal,32)
        tube(prefix+'.Brake',(x-sign*.025,y,.345),(x-sign*.01,y,.345),.184,chrome,32)
        tube(prefix+'.Hub',(x+sign*.118,y,.345),(x+sign*.15,y,.345),.065,navy,16)
        for spoke in range(6):
            angle=spoke*math.tau/6
            end=(x+sign*.14,y+math.sin(angle)*.21,.345+math.cos(angle)*.21)
            tube(prefix+'.Spoke',(x+sign*.14,y,.345),end,.02,chrome,8)
            bolt=(x+sign*.155,y+math.sin(angle)*.045,.345+math.cos(angle)*.045)
            tube(prefix+'.Bolt',bolt,(bolt[0]+sign*.012,bolt[1],bolt[2]),.009,chrome,6)
        for tread in range(32):
            angle=tread*math.tau/32
            p=(x,y+math.sin(angle)*.337,.345+math.cos(angle)*.337)
            block=box(prefix+'.Tread',p,(.17,.011,.012),rubber,.002)
            block.rotation_euler.x=-angle
        box(prefix+'.Caliper',(x-sign*.032,y+.15,.345),(.07,.08,.20),red,.018)

# Cockpit: structural cage, welded floor, seat/harness, steering linkage,
# dashboard with independent instruments, switch bank and pedals.
box('Cockpit.Floor',(0,-.2,.32),(1.79,2.42,.045),carbon,.01)
box('Cockpit.Firewall',(0,1.075,.61),(1.91,.035,.60),carbon,.01)
box('Cockpit.Headliner',(0,-.13,1.349),(1.44,1.40,.012),carbon,.008)
for sign in (-1,1):
    tube('Cockpit.CageFront',(sign*.79,.86,.37),(sign*.70,.57,1.29),.035,metal)
    tube('Cockpit.CageRoof',(sign*.70,.57,1.29),(sign*.70,-.84,1.29),.035,metal)
    tube('Cockpit.CageRear',(sign*.70,-.84,1.29),(sign*.85,-1.14,.36),.035,metal)
    tube('Cockpit.DoorBar',(sign*.85,-1.0,.60),(sign*.81,.82,.74),.032,metal)
    tube('Cockpit.DoorDiagonal',(sign*.85,-1.0,.92),(sign*.81,.82,.44),.03,metal)
tube('Cockpit.CageCross',(-.70,-.84,1.29),(.85,-1.14,.36),.03,metal)
tube('Cockpit.HarnessBar',(-.81,-.68,.91),(.81,-.68,.91),.03,metal)
seat_x=-.40
box('Cockpit.SeatBase',(seat_x,-.37,.47),(.46,.56,.11),cloth,.055)
seat=box('Cockpit.SeatBack',(seat_x,-.62,.77),(.45,.13,.54),cloth,.055)
seat.rotation_euler.x=-.14
box('Cockpit.Headrest',(seat_x,-.64,1.07),(.26,.13,.18),cloth,.04)
for sign in (-1,1):
    box('Cockpit.SeatBolster',(seat_x+sign*.22,-.37,.54),(.08,.49,.16),cloth,.035)
    wing=box('Cockpit.SeatShoulder',(seat_x+sign*.22,-.58,.93),(.08,.22,.18),cloth,.035)
    wing.rotation_euler.z=sign*.25
    box('Cockpit.ShoulderHarness',(seat_x+sign*.11,-.539,.79),(.046,.018,.42),red,.007)
box('Cockpit.HarnessBuckle',(seat_x,-.27,.53),(.07,.035,.06),metal,.008)
box('Cockpit.Dash',(0,.76,.98),(1.70,.25,.19),carbon,.035)
box('Cockpit.Console',(0,.20,.57),(.18,.73,.28),carbon,.025)
tube('Cockpit.Shifter',(0,.26,.65),(0,.18,.87),.018,metal)
torus('Cockpit.ShiftKnob',(0,.18,.88),.025,.017,black,segments=16)
for index in range(3):
    pedal=box('Cockpit.Pedal',(seat_x-.12+index*.10,.71,.43),(.065,.026,.13),metal,.012)
    pedal.rotation_euler.x=.18
    tube('Cockpit.PedalLink',(pedal.location.x,.73,.40),(pedal.location.x,.81,.35),.012,metal,8)
tube('Steering.Column',(seat_x,.74,.71),(seat_x,.47,.99),.026,metal)
torus('Steering.Wheel',(seat_x,.47,.99),.172,.023,rubber,(math.pi/2,0,0),segments=40)
tube('Steering.Hub',(seat_x,.445,.99),(seat_x,.49,.99),.047,metal,16)
for angle in (math.pi/6,5*math.pi/6,3*math.pi/2):
    tube('Steering.Spoke',(seat_x,.47,.99),
         (seat_x+math.cos(angle)*.16,.47,.99+math.sin(angle)*.16),.013,metal,8)
for x, radius, gauge in ((-.40,.084,'Tachometer'),(-.20,.055,'Oil'),(.02,.055,'Temperature')):
    z=1.025
    tube('Cockpit.'+gauge,(x,.612,z),(x,.631,z),radius,black,40)
    torus('Cockpit.GaugeRim',(x,.61,z),radius,.005,chrome,(math.pi/2,0,0),segments=40)
    for tick in range(12):
        angle=-math.pi*.75+tick*math.pi*1.5/11
        tube('Cockpit.GaugeTick',(x+math.sin(angle)*radius*.74,.596,z+math.cos(angle)*radius*.74),
             (x+math.sin(angle)*radius*.92,.596,z+math.cos(angle)*radius*.92),.0018,white,6)
    tube('Cockpit.GaugeNeedle',(x,.59,z),(x-radius*.60,.59,z+radius*.32),.0026,red,6)
for index in range(5):
    x=.29+index*.064
    tube('Cockpit.Toggle',(x,.59,.975),(x,.57,1.008),.008,chrome,8)
    box('Cockpit.ToggleBase',(x,.613,.985),(.04,.018,.053),black,.006)
box('Cockpit.FireBottle',(.49,-.43,.47),(.21,.44,.20),red,.04)

# Dedicated editable cameras/lights live in the .blend but never in runtime mesh.
def camera(name, position, target, lens):
    bpy.ops.object.camera_add(location=position)
    obj=bpy.context.object
    obj.name=name
    obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
    obj.data.lens=lens
    return obj

hero=camera('Review.Exterior',(6.8,7.3,4.0),(0,0,.65),55)
inside=camera('Review.Cockpit',(seat_x,-.30,1.19),(seat_x,5.0,1.13),23)
for name,loc,power,size in [('Key',(3,4,7),1800,5),('Fill',(-4,1,4),1200,4),('Rim',(0,-5,5),1700,3)]:
    bpy.ops.object.light_add(type='AREA', location=loc)
    obj=bpy.context.object;obj.name='Review.'+name;obj.data.energy=power;obj.data.shape='DISK';obj.data.size=size
    obj.rotation_euler=(Vector((0,0,.6))-obj.location).to_track_quat('-Z','Y').to_euler()
scene.camera=hero
# Floor is preview-only, excluded from runtime geometry.
floor=box('Review.Floor',(0,0,-.035),(200,200,.045),material('ReviewFloor',(.05,.065,.08),0,.68),0)
asphalt = material('ReviewAsphalt', (1.0, 1.0, 1.0), 0.0, .88)
bind_maps(asphalt, 'asphalt')
road = box('Review.Road', (0, 28, -.009), (6.4, 60, .004), asphalt, 0)
for side in (-1, 1):
    box('Review.RoadEdge', (side*2.95, 28, -.005), (.11, 60, .002), white, 0)
for distance in range(6, 55, 6):
    box('Review.RoadDash', (0, distance, -.005), (.10, 2.5, .002), white, 0)

bpy.ops.wm.save_as_mainfile(filepath=str(output/'blender/racer-r1.blend'),check_existing=False)

# Own float mesh container, separate from the original game's formats. Keep
# named components and motion pivots; draw preparation can batch shared states.
def convert(vector):
    return (float(vector[0]), float(vector[2]), float(vector[1]))


def part_role(obj):
    role = next((index for index, prefix in enumerate(
        ('WheelFL', 'WheelFR', 'WheelRL', 'WheelRR'), 1) if obj.name.startswith(prefix)), 0)
    if obj.name.startswith('Cockpit.'):
        role = 5
    if obj.name.startswith('Steering.'):
        role = 6
    pivot = Vector((0, 0, 0))
    if 1 <= role <= 4:
        pivot = Vector((-1.005 if role % 2 else 1.005, 1.45 if role <= 2 else -1.45, .345))
    if role == 6:
        pivot = Vector((seat_x, .47, .99))
    return role, convert(pivot)


def visible_in_lod(obj, detail):
    if detail == 'full':
        return True
    if detail == 'exterior':
        return not any(word in obj.name for word in (
            'Gauge', 'Tachometer', 'Temperature', 'Cockpit.Oil', 'Toggle', 'Pedal', '.Bolt', '.Tread'))
    return not obj.name.startswith(('Cockpit.', 'Steering.')) and not any(
        word in obj.name for word in ('.Bolt', '.Tread', '.Spoke', '.Brake', '.Caliper', 'GrilleSlat'))


def export_mesh(detail, ratio):
    objects = sorted((obj for obj in scene.objects if obj.type == 'MESH'
                      and not obj.name.startswith('Review.') and visible_in_lod(obj, detail)),
                     key=lambda obj: obj.name)
    materials = sorted({obj.active_material for obj in objects}, key=lambda mat: mat.name)
    texture_names = sorted({texture_bindings[mat.name] for mat in materials if mat.name in texture_bindings})
    textures = [next(item for item in texture_sets if item['name'] == name) for name in texture_names]
    vertices, indices, parts = [], [], []
    vertex_lookup = {}
    for obj in objects:
        modifier = None
        if ratio < 1.0 and len(obj.data.polygons) > 32:
            modifier = obj.modifiers.new('Export-only silhouette LOD', 'DECIMATE')
            modifier.ratio = ratio
        bpy.context.view_layer.update()
        evaluated = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
        data = evaluated.to_mesh()
        data.calc_loop_triangles()
        normal_matrix = obj.matrix_world.to_3x3().inverted().transposed()
        start = len(indices)
        uv = data.uv_layers.active
        for triangle in data.loop_triangles:
            positions = [obj.matrix_world @ data.vertices[index].co for index in triangle.vertices]
            if (positions[1] - positions[0]).cross(positions[2] - positions[0]).length_squared < 1e-18:
                continue
            # Reflecting Blender Z-up coordinates reverses orientation.
            for corner in (triangle.loops[0], triangle.loops[2], triangle.loops[1]):
                loop = data.loops[corner]
                position = obj.matrix_world @ data.vertices[loop.vertex_index].co
                normal = (normal_matrix @ data.corner_normals[corner].vector).normalized()
                coordinates = tuple(uv.data[corner].uv) if uv else (0.0, 0.0)
                vertex = struct.pack('<8f', *convert(position), *convert(normal), *coordinates)
                if vertex not in vertex_lookup:
                    vertex_lookup[vertex] = len(vertices)
                    vertices.append(vertex)
                indices.append(vertex_lookup[vertex])
        role, pivot = part_role(obj)
        parts.append((obj.name, materials.index(obj.active_material), start, len(indices) - start, role, *pivot))
        evaluated.to_mesh_clear()
        if modifier is not None:
            obj.modifiers.remove(modifier)
    label = 'racer-r1' if detail == 'full' else 'racer-r1-' + detail
    path = output / 'runtime/models' / (label + '.dd2mesh')
    with path.open('wb') as stream:
        stream.write(struct.pack('<8s5I', b'DD2MESH2', len(textures), len(materials), len(parts), len(vertices), len(indices)))
        for texture in textures:
            stream.write(struct.pack('<64s64s64s', *(texture['maps'][kind]['path'].encode('ascii')
                                                     for kind in ('albedo', 'roughness', 'normal'))))
        for mat in materials:
            shader = mat.node_tree.nodes.get('Principled BSDF')
            name = texture_bindings.get(mat.name)
            texture_index = texture_names.index(name) if name else 0xffffffff
            stream.write(struct.pack('<32s4f2fI', mat.name.encode('ascii'), *mat.diffuse_color,
                                     shader.inputs['Metallic'].default_value,
                                     shader.inputs['Roughness'].default_value, texture_index))
        for part in parts:
            stream.write(struct.pack('<64s4I3f', part[0].encode('ascii'), *part[1:]))
        for vertex in vertices:
            stream.write(vertex)
        for index in indices:
            stream.write(struct.pack('<I', index))
    statistics = {
        'schema': 2, 'generator': 'tools/assets/build_vehicle.py', 'blender': bpy.app.version_string,
        'authorship': 'Handwritten geometry/material recipes; no original DD2 or external asset input',
        'units': 'meters; +Y up, +Z forward; reflected Blender winding corrected',
        'lod': detail, 'materials': len(materials), 'part_count': len(parts),
        'vertices': len(vertices), 'triangles': len(indices) // 3, 'bytes': path.stat().st_size,
        'runtime_integrated': False, 'material_maps_exported': True,
        'review_only': ['lighting', 'floor', 'cameras', 'road'], 'cockpit_eye': convert(inside.location),
        'parts': [{'name': part[0], 'role': part[4], 'triangles': part[3] // 3} for part in parts],
    }
    (output / 'runtime/models' / (label + '.json')).write_text(json.dumps(statistics, indent=2) + '\n')
    return statistics


statistics = [export_mesh(detail, ratio) for detail, ratio in (
    ('full', 1.0), ('exterior', .65), ('npc', .35))]
if preview:
    preview.mkdir(parents=True,exist_ok=True)
    for cam,label in ((hero,'exterior'),(inside,'cockpit')):
        scene.camera=cam
        scene.render.filepath=str(preview/(label+'.png'))
        bpy.ops.render.render(write_still=True)
print('AUTHORED_MODEL ' + json.dumps([{key:row[key] for key in ('lod','part_count','triangles','bytes','runtime_integrated')} for row in statistics]))
