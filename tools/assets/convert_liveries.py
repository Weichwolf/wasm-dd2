#!/usr/bin/env python3
"""One-time offline conversion of driver/class paint and number variants.

The game only loads the committed meshes/PNGs and compiled scene references.
Original inputs, palette tables and subdivision are never runtime dependencies.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from assets.convert_reference import ROOT, Stream, Materials, Resources, convert_mesh, digest
from assets.prepare_scenes import prepare
from rewrite.verify_archive import ORIGINAL_SHA256
from rewrite.verify_levels import assets
from rewrite.verify_meshes import LEVELS, level_expected

EXE_SHA256 = '0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'
NUMBERS = [1, 0, 7, 13, 17, 35, 37, 40, 42, 47, 50, 52, 53, 64, 66, 69, 77, 82, 88, 99]
LODS = ('close', 'medium', 'distant')
SECTIONS = dict(zip((17, 16, 15), LODS))
TABLES = ((0x466834, 0x46688c), (0x4668ec, 0x466934), (0x4669b4, 0x4669bc))
DOORS = ((0x466796, 0x46679f), (0x466814, 0x466817))


def extract_recipe(executable):
    pe = executable.read_bytes()
    if digest(pe) != EXE_SHA256:
        raise ValueError('Unsupported original executable')
    header, = struct.unpack_from('<I', pe, 0x3c)
    count, = struct.unpack_from('<H', pe, header + 6)
    optional, = struct.unpack_from('<H', pe, header + 20)
    sections = [struct.unpack_from('<8s8I', pe, header + 24 + optional + index * 40) for index in range(count)]
    def read(address, size):
        relative = address - 0x400000
        for section in sections:
            if section[2] <= relative and relative + size <= section[2] + section[3]:
                begin = section[4] + relative - section[2]
                return pe[begin:begin + size]
        raise ValueError('Unbacked offline PE field')
    def ordinals(address):
        raw = read(address, 256)
        if 255 not in raw:
            raise ValueError('Unterminated offline ordinal list')
        return list(raw[:raw.index(255)])
    def paint(address):
        result = {}
        for index in range(16):
            part, pointer = struct.unpack('<iI', read(address + index * 8, 8))
            if part == -1:
                return result
            if not 0 <= part < 8:
                raise ValueError('Invalid offline paint region')
            for ordinal in ordinals(pointer):
                key = str(ordinal)
                if key in result and result[key] != part:
                    raise ValueError('Conflicting paint regions')
                result[key] = part
        raise ValueError('Unterminated offline paint table')
    rules = {}
    for index, (lod, pair) in enumerate(zip(LODS, TABLES)):
        doors = {str(25 if index == 0 else 29): sorted(set(v for address in DOORS[index]
                                                         for v in ordinals(address)))} if index < 2 else {}
        rules[lod] = dict(paint={str(opcode): paint(address) for opcode, address in zip((25,29), pair)}, numbers=doors)
    return dict(format='DD2LIVERYRECIPE1', source_executable_sha256=EXE_SHA256,
                driver_numbers=NUMBERS, template_driver=18, human_classes=['rookie','amateur','pro'],
                body_lods=rules)


def sprites(files, code):
    level = files[f'LEV{code}\\LEVEL.DAT']
    begin, end = struct.unpack_from('<II', level, 12)
    count, = struct.unpack_from('<I', level, begin)
    if end - begin != 4 + count * 24:
        raise ValueError('Invalid sprite extent')
    result = {}
    for cursor in range(begin + 4, end, 24):
        row = level[cursor:cursor + 24]
        name = row[14:].split(b'\0',1)[0].decode('ascii')
        u, v = struct.unpack_from('<HH', row)
        bank, = struct.unpack_from('<H', row, 10)
        result[name] = dict(u=u, v=v % 256, page=v // 256, bank=bank)
    return result


def selection(sprite, variant, recipe):
    driver = variant if variant < 20 else 0
    car_class = variant - 19 if variant >= 20 else 0
    number = recipe['driver_numbers'][driver]
    digits = sprite[f'DR{number:02}A']
    banks = []
    for part, letter in enumerate('BADCEBA'):
        name = (f'SMCL{number:02}{letter}' if part >= 5 else
                f'CLT{number:02}{letter}{4-car_class}' if car_class else f'CLUT{number:02}{letter}')
        banks.append(sprite[name]['bank'] if driver != 18 else None)
    banks.append(sprite[f'P1D1T{4-car_class}']['bank'] if car_class else digits['bank'])
    return dict(driver=driver, car_class=car_class, number=number, banks=banks,
                digits=digits, template_digits=sprite['DR88A'])


def paint_mesh(mesh, materials, rules, selected):
    # Ordinals are within the exact source opcode group, before tessellation.
    ordinals = Counter()
    painted, audit = [], []
    base_definitions = len(materials.definitions)
    for face in mesh['faces']:
        row = list(face)
        opcode = row[15]
        ordinal = ordinals[opcode]
        ordinals[opcode] += 1
        family = str(opcode & 253)
        part = rules['paint'].get(family, {}).get(str(ordinal))
        numbered = ordinal in rules['numbers'].get(family, [])
        if selected['driver'] != 18 and part is not None:
            row[13] = selected['banks'][part]
        if numbered:
            row[13] = selected['banks'][7]
            page_flags, flags, *uvs = materials.definitions[row[12]]
            source, target = selected['template_digits'], selected['digits']
            page_flags = (page_flags & ~31) | target['page']
            for corner in range(row[17]):
                uvs[2*corner] = (uvs[2*corner] - source['u'] + target['u']) & 255
                uvs[2*corner+1] = (uvs[2*corner+1] - source['v'] + target['v']) & 255
            row[12] = len(materials.definitions)
            materials.definitions.append((page_flags, flags, *uvs))
        if (opcode & 253) == 1:
            for corner in range(row[17]):
                row[corner * 3 + 2] = 0
        if row[18]:
            definition = materials.definitions[row[12]]
            audit.append(dict(opcode=opcode, ordinal=ordinal, bank=row[13], page=definition[0]&31,
                              uv=list(definition[2:]), part=part, number_face=numbered))
        painted.append(row)
    for opcode, count in ordinals.items():
        family = str(opcode & 253)
        if any(int(index) >= count for index in rules['paint'].get(family, {})):
            raise ValueError('Paint ordinal outside source opcode group')
        if any(index >= count for index in rules['numbers'].get(family, [])):
            raise ValueError('Number ordinal outside source opcode group')
    return dict(mesh, faces=painted), audit, base_definitions


def convert(archive, executable, runtime, recipe_path):
    original=archive.read_bytes()
    if digest(original)!=ORIGINAL_SHA256:
        raise ValueError('Expected supported unmodified original archive')
    recipe=extract_recipe(executable)
    recipe_path.parent.mkdir(parents=True,exist_ok=True)
    recipe_path.write_text(json.dumps(recipe,indent=2)+'\n')
    files=assets(original)
    manifest=json.loads((runtime/'reference/manifest.json').read_text())
    resources=Resources(runtime)
    converted=[]
    for code in LEVELS:
        binary,_=level_expected(files,code)
        stream=Stream(binary);_,count=stream.words(2)
        for _ in range(count):stream.words(6);stream.mesh()
        meshes={};section,=stream.words(1)
        while section!=22:
            mesh=stream.mesh()
            if section in SECTIONS:meshes[SECTIONS[section]]=mesh
            section,=stream.words(1)
        materials=Materials(files,code,resources);sprite=sprites(files,code)
        level=next(v for v in manifest['levels'] if v['level']==code)
        scene_path=runtime/level['scene'];scene=json.loads(scene_path.read_text())
        variants=[]
        for variant in range(22):
            selected=selection(sprite,variant,recipe);exports={}
            for lod in LODS:
                mesh,audit,base=paint_mesh(meshes[lod],materials,recipe['body_lods'][lod],selected)
                item,evidence=convert_mesh(mesh,materials,resources)
                del materials.definitions[base:]
                if item is None:
                    raise ValueError('Empty livery body')
                name=f'car-{lod}-{variant:02}'
                scene['templates'][name]=item
                level['templates'][name]=evidence
                exports[lod]=dict(**item,triangles=evidence['triangles'],surfaces=audit)
            variants.append(dict(variant=variant,driver=selected['driver'],car_class=selected['car_class'],
                                 number=selected['number'],models=exports))
        scene_path.write_text(json.dumps(scene,indent=2)+'\n')
        converted.append(dict(level=code,variants=variants))
        print(json.dumps(dict(level=code,variants=len(variants))),flush=True)
    manifest['files'].update(resources.files);manifest['models'].update(resources.models)
    manifest['liveries']=dict(recipe=str(recipe_path.relative_to(ROOT)),levels=converted)
    manifest['limitations']=[v for v in manifest['limitations'] if 'default game still' not in v
                             and 'Driving metadata, livery variants' not in v]
    pending='UI, new audio, cockpit/sky policy and visual damage migration remain pending.'
    if pending not in manifest['limitations']:manifest['limitations'].append(pending)
    prepare(runtime,manifest)
    (runtime/'reference/manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    return manifest


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive',type=Path,default=ROOT/'DestructionDerby2/Dirinfo')
    parser.add_argument('--executable',type=Path,default=ROOT/'DestructionDerby2/dd2h.exe')
    parser.add_argument('--runtime',type=Path,default=ROOT/'assets/runtime')
    parser.add_argument('--recipe',type=Path,default=ROOT/'assets/recipes/reference-liveries.json')
    args=parser.parse_args()
    manifest=convert(args.archive,args.executable,args.runtime,args.recipe)
    print(json.dumps(dict(models=len(manifest['models']),resources=len(manifest['files']))))


if __name__=='__main__':main()
