"""Offline mini-source producer closure; no canonical SDK generation or compiler.

Not executed at this source checkpoint. Exercises actual snapshot/copy producers
with a bounded tracked-file fixture containing the real new private header bytes.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
PRIVATE = 'src/bridge/cui_bridge_time_internal.h'


def load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'scripts' / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class Closure(unittest.TestCase):
    def test_sdk_and_package_track_private_header_and_hash_it(self):
        sdk, package = load('generate_go_sdk'), load('package_bindings')
        payload = (ROOT / PRIVATE).read_bytes()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / 'source'
            files = {PRIVATE: payload,
                     'src/bridge/cui_bridge.c': b'#include "cui_bridge_time_internal.h"\n',
                     'include/cui/cui_bridge.h': b'/* public fixture */\n',
                     'LICENSE': b'fixture\n', 'third_party/dependencies.json': b'[]\n',
                     'cmake/sources.json': b'{"modules":{"fixture":["src/bridge/cui_bridge.c"]}}\n'}
            for name, data in files.items():
                p = root / name
                p.parent.mkdir(parents=True, exist_ok=True)
                p.write_bytes(data)
            tracked = ('\0'.join(files) + '\0').encode()

            def git(argv, **kwargs):
                if argv[1] == 'ls-files': return tracked
                if argv[1] == 'rev-parse': return 'fixture-revision\n'
                if argv[1] == 'diff': return b''
                raise AssertionError(argv)

            with patch.object(sdk, 'ROOT', root), patch.object(sdk.subprocess, 'check_output', side_effect=git):
                snapshot = sdk.snapshot()
            self.assertEqual(snapshot[PRIVATE], payload)
            row = hashlib.sha256(payload).hexdigest() + '  ' + PRIVATE + '\n'
            self.assertIn(row.encode(), snapshot['SOURCE_FILES.sha256'])
            self.assertNotIn(PRIVATE, snapshot['CUI_SOURCES.cmake'].decode())
            # The private header belongs inside its bridge TU, not a new module.
            for language in ('python', 'node'):
                destination = Path(directory) / language
                with patch.object(package, 'ROOT', root), patch.object(package.subprocess, 'check_output', side_effect=git):
                    package.copy_source(destination, omit_npm_ignored=language == 'node')
                self.assertEqual((destination / PRIVATE).read_bytes(), payload)
                self.assertIn(row, (destination / 'SOURCE_FILES.sha256').read_text())
                receipt = json.loads((destination / 'SOURCE_PROVENANCE.json').read_text())
                self.assertFalse(receipt['untracked_files_included'])


if __name__ == '__main__':
    unittest.main()
