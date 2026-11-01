"""Offline assembly-reader tests; never compile, initialize SDL or open windows."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sdl_guard_measurement', ROOT / 'tests/distribution/measure_sdl_v2_guard.py')
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class GuardMeasurementTests(unittest.TestCase):
    def test_native_zero_offset_and_unrelated_data(self):
        assembly = '''sdl_v1_size:
.long 128
unrelated:
.long 987
sdl_v1_align:
.long 8
sdl_v1_constructor_bytes:
.long 128
sdl_v1_offset_first:
.zero 4
'''
        self.assertEqual(MODULE.constants(assembly), {
            'size': 128, 'align': 8, 'constructor_bytes': 128, 'offset_first': 0})

    def test_wasm_constants(self):
        assembly = '''sdl_v1_size:
.int32 92
sdl_v1_align:
.int32 4
sdl_v1_constructor_bytes:
.int32 92
sdl_v1_offset_first:
.int32 0
'''
        self.assertEqual(MODULE.constants(assembly)['offset_first'], 0)
        self.assertEqual(MODULE.constants(assembly)['size'], 92)

    def test_incomplete_measurement_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'missing compact layout constants'):
            MODULE.constants('sdl_v1_size:\n.long 128\n')


if __name__ == '__main__':
    unittest.main()
