"""Check first-difference localization and completeness of measured car history."""
from pathlib import Path
import tempfile
import unittest

from trace_native_physics import compare_cars, compare_callers


class PhysicsHistoryTests(unittest.TestCase):
    def test_prior_race_paths_and_exact_car_field(self):
        with tempfile.TemporaryDirectory(prefix='dd2-physics-test-',dir='/tmp') as temporary:
            source=Path(temporary)/'original';source.mkdir()
            native=Path(temporary)/'native';native.mkdir()
            (source/'preceding-cars/demo000').mkdir(parents=True)
            layout=[dict(name='primitive',address=0x78a520,size=20*8),
                    dict(name='handling',address=0x792a00,size=20*4)]
            frames=[dict(index=0,prefix='preceding-cars/demo000/frame00000',level=9,cf=0,ticks=0,clock_calls=1),
                    dict(index=0,prefix='frame00000',level=8,cf=0,ticks=0,clock_calls=100)]
            measured=[dict(row,index=i,prefix=f'allframe{i:05d}') for i,row in enumerate(frames)]
            original=bytes(range(240))
            for row,actual in zip(frames,measured):
                (source/(row['prefix']+'.cars')).write_bytes(original)
                (native/(actual['prefix']+'.cars')).write_bytes(original)
            reference=dict(physics_layout=layout,frames=frames)
            self.assertIsNone(compare_cars(reference,measured,source,native))
            changed=bytearray(original);changed[160+7*4+2]^=1
            (native/'allframe00001.cars').write_bytes(changed)
            difference=compare_cars(reference,measured,source,native)
            self.assertEqual((difference['index'],difference['level'],difference['car'],difference['field_offset'],difference['address']),
                             (1,8,7,'0x2',hex(0x792a00+7*4+2)))
            self.assertEqual((source/'frame00000.cars').read_bytes(),original)
            (native/'allframe00001.cars').write_bytes(original)
            difference=compare_cars(reference,measured[:1],source,native)
            self.assertEqual(difference['error'],'car checkpoint count differs')
            measured[1]['ticks']=1
            self.assertEqual(compare_cars(reference,measured,source,native)['error'],'car checkpoint phase differs')

    def test_callers_reject_phase_function_and_truncated_trace(self):
        originals=[dict(caller=0x4245bd,level=10,cf=151,ticks=302)]
        observed=[dict(index=0,function='Setup_Debris',level=10,cf=151,ticks=302)]
        definitions=[(0x424200,'Setup_Debris')]
        self.assertIsNone(compare_callers(originals,observed,definitions))
        changed=[dict(observed[0],ticks=303)]
        self.assertEqual(compare_callers(originals,changed,definitions)['index'],0)
        changed=[dict(observed[0],function='Car_Drive_Motion')]
        self.assertEqual(compare_callers(originals,changed,definitions)['index'],0)
        self.assertEqual(compare_callers(originals,[],definitions)['error'],'random caller count differs')

    def test_movement_stage_and_car_identity_are_required(self):
        with tempfile.TemporaryDirectory(prefix='dd2-motion-test-',dir='/tmp') as temporary:
            root=Path(temporary)
            layout=[dict(name='handling',address=0x792a00,size=20*4)]
            expected=dict(index=0,prefix='step00000',level=8,cf=160,ticks=320,clock_calls=45000,
                          rng_calls=16600,phase='before Car_Movement',car=0)
            (root/'step00000.cars').write_bytes(bytes(80))
            reference=dict(physics_layout=layout,frames=[expected])
            self.assertIsNone(compare_cars(reference,[dict(expected)],root,root))
            for field,value in (('phase','after Car_Movement'),('car',1),('rng_calls',16601)):
                changed=dict(expected);changed[field]=value
                self.assertEqual(compare_cars(reference,[changed],root,root)['error'],'car checkpoint phase differs')


if __name__=='__main__':unittest.main()
