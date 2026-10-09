#!/usr/bin/env python3
"""Check subdivision surface/attribute contracts without original game files."""
import unittest

import numpy as np

from convert_reference import subdivide


class SubdivisionContract(unittest.TestCase):
    def test_surface_area_winding_and_uv_interpolation(self):
        # A sloping triangle with a nonuniform UV mapping makes accidental
        # coordinate-only subdivision or a winding reversal observable.
        triangle = np.array([[0., 0., 0., 0., -2**-.5, 2**-.5, .1, .2],
                             [2., 0., 0., 0., -2**-.5, 2**-.5, .9, .3],
                             [0., 2., 2., 0., -2**-.5, 2**-.5, .2, .8]])
        original = triangle.copy()
        result = subdivide([triangle])
        self.assertEqual(result.shape, (16, 3, 8))
        np.testing.assert_array_equal(triangle, original)
        positions = result[:, :, :3]
        areas = np.cross(positions[:, 1] - positions[:, 0], positions[:, 2] - positions[:, 0])
        source_area = np.cross(triangle[1, :3] - triangle[0, :3], triangle[2, :3] - triangle[0, :3])
        np.testing.assert_allclose(areas, np.tile(source_area / 16, (16, 1)))
        np.testing.assert_allclose(areas.sum(axis=0), source_area)
        self.assertTrue(np.all(positions[:, :, 1] == positions[:, :, 2]))
        weights = np.stack((1 - positions[:, :, 0] / 2 - positions[:, :, 1] / 2,
                            positions[:, :, 0] / 2, positions[:, :, 1] / 2), axis=-1)
        self.assertTrue(np.all(weights >= 0))
        np.testing.assert_allclose(result[:, :, 6:], weights @ triangle[:, 6:])
        np.testing.assert_allclose(result[:, :, 3:6], np.broadcast_to(triangle[0, 3:6], (16, 3, 3)))

    def test_neighboring_uv_seams_remain_separate(self):
        first = np.array([[0., 0., 0., 0., 0., 1., 0., 0.],
                          [1., 0., 0., 0., 0., 1., 1., 0.],
                          [0., 1., 0., 0., 0., 1., 0., 1.]])
        second = first.copy()
        second[:, 6:] += 2
        result = subdivide([first, second])
        self.assertEqual(result.shape[0], 32)
        np.testing.assert_array_equal(result[:16, :, :6], result[16:, :, :6])
        np.testing.assert_allclose(result[16:, :, 6:] - result[:16, :, 6:], 2)


if __name__ == '__main__':
    unittest.main()
