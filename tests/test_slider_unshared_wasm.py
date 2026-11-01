#!/usr/bin/env python3
"""Offline final-module memory policy tests; no compiler/dependencies/native UI."""
import importlib.util
from pathlib import Path
import unittest
spec = importlib.util.spec_from_file_location('verifier', Path(__file__).resolve().parents[1] / 'scripts/verify_slider_unshared_wasm.py')
verifier = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verifier)
MAGIC = b'\0asm\x01\0\0\0'


def section(tag, payload):
    assert len(payload) < 128
    return bytes((tag, len(payload))) + payload


class MemoryPolicy(unittest.TestCase):
    def test_defined_and_unshared_import(self):
        for blob, imported in ((section(5,b'\x01\x00\x01'),False),
                               (section(2,b'\x01\x03env\x06memory\x02\x01\x01\x02'),True)):
            result = verifier.verify(MAGIC + blob)
            self.assertFalse(result['shared'])
            self.assertEqual(result['memory']['imported'], imported)

    def test_shared_memory_defined_or_imported(self):
        for blob in (section(5,b'\x01\x03\x01\x02'),
                     section(2,b'\x01\x03env\x06memory\x02\x03\x01\x02')):
            with self.assertRaisesRegex(ValueError,'shared'):
                verifier.verify(MAGIC + blob)

    def test_no_multiple_duplicate_or_memory64(self):
        for blob in (b'', section(5,b'\x02\x00\x01\x00\x01'),
                     section(5,b'\x01\x00\x01') * 2, section(5,b'\x01\x04\x01')):
            with self.assertRaises(ValueError): verifier.verify(MAGIC + blob)

    def test_malformed_and_truncated_fail_closed(self):
        good = MAGIC + section(2,b'\x01\x03env\x06memory\x02\x01\x01\x02')
        for n in range(len(good)):
            with self.assertRaises(ValueError): verifier.verify(good[:n])
        for blob in (MAGIC+b'\x05\x80\x80\x80\x80\x80\x00',
                     MAGIC+section(5,b'\x01\x01\x03\x02'),
                     MAGIC+section(5,b'\x01\x00\x01\x00'),
                     MAGIC+section(2,b'\x01\x00\x00\x09')):
            with self.assertRaises(ValueError): verifier.verify(blob)


if __name__ == '__main__': unittest.main()
