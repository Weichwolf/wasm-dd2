#!/usr/bin/env python3
"""Blender worker: editable inward-facing panorama domes and owned mesh exports."""
import argparse
import json
import math
from pathlib import Path
import struct
import sys

import bpy
from mathutils import Vector


def text(value, length):
    encoded = value.encode('ascii')
    if len(encoded) >= length:
        raise ValueError('Sky resource name exceeds container limit')
    return encoded.ljust(length, b'\0')


def patch(recipe, band, quadrant):
    longitude, latitude = recipe['longitude_cells'], recipe['latitude_cells']
    radius, guard, height = recipe['radius_meters'], recipe['vertical_guard'], recipe['height']
    vertices, indices, lookup = [], [], {}
    def vertex(column, row):
        yaw = column * 2 * math.pi / longitude
        pitch = -math.pi / 2 + row * math.pi / latitude
        direction = (math.cos(pitch) * math.cos(yaw), math.sin(pitch), math.cos(pitch) * math.sin(yaw))
        uv = (column / longitude, (guard + .5 + row / latitude * (height - 2 * guard - 1)) / height)
        row_data = (*[value * radius for value in direction], *[-value for value in direction], *uv)
        if row_data not in lookup:
            lookup[row_data] = len(vertices)
            vertices.append(row_data)
        return lookup[row_data]
    first_row = 0 if band == 'lower' else latitude // 2
    for row in range(first_row, first_row + latitude // 2):
        for column in range(quadrant * longitude // 4, (quadrant + 1) * longitude // 4):
            a, b, c, d = (vertex(column, row), vertex(column + 1, row),
                          vertex(column, row + 1), vertex(column + 1, row + 1))
            if row != 0:
                indices.extend((a, b, c))
            if row != latitude - 1:
                indices.extend((c, b, d))
    return vertices, indices


def export(path, texture, vertices, indices):
    data = [struct.pack('<8s5I', b'DD2MESH2', 1, 1, 1, len(vertices), len(indices)),
            struct.pack('<64s64s64s', text(texture, 64), text('skies/roughness.png', 64), text('skies/normal.png', 64)),
            struct.pack('<32s4f2fI', text('sky-emission', 32), 1, 1, 1, 1, 0, 1, 0),
            struct.pack('<64s4I3f', text('panorama', 64), 0, 0, len(indices), 0, 0, 0, 0)]
    data.extend(struct.pack('<8f', *vertex) for vertex in vertices)
    data.append(struct.pack(f'<{len(indices)}I', *indices))
    path.write_bytes(b''.join(data))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--recipe', type=Path, required=True)
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
    recipe = json.loads(args.recipe.read_text())
    bpy.context.preferences.filepaths.save_version = 0
    bpy.data.objects.remove(bpy.data.objects.get('Cube'), do_unlink=True)
    for scene in list(bpy.data.scenes):
        for obj in list(scene.objects):
            bpy.data.objects.remove(obj, do_unlink=True)
    first = bpy.context.scene
    for profile_index, profile in enumerate(recipe['profiles']):
        code = profile['level'].lower()
        scene = first if profile_index == 0 else bpy.data.scenes.new('Sky.' + code)
        scene.name = 'Sky.' + code
        scene.unit_settings.system = 'METRIC'
        scene['theme'] = profile['theme']
        scene['recipe'] = 'assets/recipes/skies.json'
        scene['original_inputs_required'] = False
        image = bpy.data.images.load(str(args.output/f'runtime/skies/level-{code}.png'), check_existing=True)
        image.pack()
        image.filepath = f'//../runtime/skies/level-{code}.png'
        material = bpy.data.materials.new('Sky.' + code)
        material.use_nodes = True
        nodes = material.node_tree.nodes
        nodes.clear()
        output = nodes.new('ShaderNodeOutputMaterial')
        emission = nodes.new('ShaderNodeEmission')
        texture = nodes.new('ShaderNodeTexImage')
        texture.image = image
        material.node_tree.links.new(texture.outputs['Color'], emission.inputs['Color'])
        material.node_tree.links.new(emission.outputs[0], output.inputs['Surface'])
        for band in ('lower', 'upper'):
            for quadrant in range(4):
                vertices, indices = patch(recipe, band, quadrant)
                name = f'level-{code}-{band}-{quadrant}'
                mesh = bpy.data.meshes.new(name)
                mesh.from_pydata([(v[0], -v[2], v[1]) for v in vertices], [],
                                 [indices[index:index + 3] for index in range(0, len(indices), 3)])
                mesh.update()
                uv = mesh.uv_layers.new(name='Panorama')
                for face in mesh.polygons:
                    face.use_smooth = True
                    for loop in face.loop_indices:
                        uv.data[loop].uv = vertices[mesh.loops[loop].vertex_index][6:]
                mesh.materials.append(material)
                obj = bpy.data.objects.new(name, mesh)
                scene.collection.objects.link(obj)
                obj['runtime_template'] = f'sky-{band}-{quadrant}'
                # Export positions and UVs from the actual editable Blender mesh.
                actual_uv = {}
                for loop in mesh.loops:
                    actual_uv[loop.vertex_index] = tuple(uv.data[loop.index].uv)
                authored = []
                for index, vertex in enumerate(mesh.vertices):
                    position = (vertex.co.x, vertex.co.z, -vertex.co.y)
                    normal = -Vector(position).normalized()
                    authored.append((*position, *normal, *actual_uv.get(index, vertices[index][6:])))
                export(args.output/f'runtime/skies/{name}.dd2mesh', f'skies/level-{code}.png', authored, indices)
        camera = bpy.data.cameras.new('Review.' + code)
        obj = bpy.data.objects.new('Review.' + code, camera)
        scene.collection.objects.link(obj)
        obj.rotation_euler = Vector((0, -1, 0)).to_track_quat('-Z', 'Y').to_euler()
        camera.lens_unit = 'FOV'
        camera.angle = math.radians(70)
        camera.clip_start = .01
        scene.camera = obj
        scene.render.resolution_x, scene.render.resolution_y = 640, 360
    (args.output/'blender').mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(args.output/'blender/skies.blend'), compress=True)
    print(json.dumps(dict(scenes=11, sky_patches=88, original_inputs=False)))


if __name__ == '__main__':
    main()
