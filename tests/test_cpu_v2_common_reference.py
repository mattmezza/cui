#!/usr/bin/env python3
"""Offline relocation authority and negative semantic-mutation checks."""
import unittest
from cpu_v2_common_reference import RECEIPT, ROOT, renamed, tokens, verify


class CommonSourceTests(unittest.TestCase):
    def test_frozen_source_and_mechanical_namespace_only(self):
        verify()

    def test_literals_operators_and_directive_boundaries_are_not_erased(self):
        for a, b in [('x + +y', 'x++ + y'), ('"a b"', '"ab"'),
                     ('#define F(x) x', '#define F (x) x'),
                     ('#define X 1\nX', '#define X 1 X'), ('x < y', 'x <= y')]:
            self.assertNotEqual(tokens(a), tokens(b))
        self.assertEqual(tokens('x + y'), tokens('x\n +\t y /* inert */'))

    def test_payload_and_order_mutations_are_detected(self):
        source = 'if (ready) release(p); alpha = color.a * coverage;'
        for altered in [source.replace('ready', '!ready'),
                        source.replace('color.a * coverage', 'color.a + coverage'),
                        source.replace('release(p); alpha', 'alpha')]:
            self.assertNotEqual(tokens(source), tokens(altered))

    def test_sdk_discovery_includes_common_and_legacy_forwarders(self):
        import importlib.util
        spec = importlib.util.spec_from_file_location('cpu_common_sdk_generator',
                                                     ROOT / 'scripts/generate_go_sdk.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        files = module.snapshot()
        for old in RECEIPT['headers']:
            common = 'backends/common/' + old.replace('software', 'cpu', 1)
            self.assertIn(common, files)
            self.assertIn('backends/software/' + old, files)
            self.assertEqual(files[common], (ROOT / common).read_bytes())
        self.assertIn('backends/software/cui_software_v2_compat_internal.h', files)

    def test_native_bundle_carries_common_and_forwarding_headers(self):
        import importlib.util
        spec = importlib.util.spec_from_file_location('cpu_common_native_bundle',
                                                     ROOT / 'scripts/native_bundle.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        files = module.bundle_files()
        for old in RECEIPT['headers']:
            self.assertIn('vendor/backends/common/' + old.replace('software', 'cpu', 1), files)
            self.assertIn('vendor/backends/software/' + old, files)

    def test_common_header_change_invalidates_wasi_fingerprint_without_compiling(self):
        import importlib.util
        from pathlib import Path
        from unittest.mock import patch
        spec = importlib.util.spec_from_file_location('cpu_common_wasi_cache',
                                                     ROOT / 'scripts/test_wasi.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        changed = ROOT / 'backends/common/cui_cpu_v2_paint_internal.h'
        def contents(path, revision):
            return (str(path) + ('changed' if revision and path == changed else '')).encode()
        with patch.object(Path, 'read_bytes', lambda path: contents(path, False)):
            before = module.source_fingerprint(ROOT)
        with patch.object(Path, 'read_bytes', lambda path: contents(path, True)):
            after = module.source_fingerprint(ROOT)
        self.assertNotEqual(before, after)

    def test_legacy_paths_are_forwarders_and_all_private_aliases_retained(self):
        compat = (ROOT / 'backends/software/cui_software_v2_compat_internal.h').read_text()
        for old in RECEIPT['aliases']:
            self.assertIn('#define ' + old + ' ' + old.replace('software', 'cpu', 1), compat)
        for old in RECEIPT['headers']:
            source = (ROOT / 'backends/software' / old).read_text()
            self.assertIn('#include "cui_software_v2_compat_internal.h"', source)
            self.assertIn('../common/' + old.replace('software', 'cpu', 1), source)
            self.assertNotIn('static inline', source)
            self.assertLess(source.index('#include "cui_software_v2_compat_internal.h"'),
                            source.index('#include "../common/'))
        self.assertIn('CUI_CPU_V2_SHADOW_SAMPLE_HOOK() CUI_SOFTWARE_V2_SHADOW_SAMPLE_HOOK()', compat)
        fixture = (ROOT / 'tests/unit/test_cpu_v2_common.c').read_text()
        legacy = fixture.split('#else', 1)[1].split('#endif', 1)[0]
        self.assertLess(legacy.index('#include "../../backends/software/'),
                        legacy.index('#include "../../backends/common/'))



if __name__ == '__main__':
    unittest.main()
