"""Exercise package runner guards without launching native programs."""
import importlib.util
import os
from pathlib import Path
from unittest.mock import patch
import unittest
ROOT = Path(__file__).resolve().parents[2]
class Guards(unittest.TestCase):
    def test_hostile_ambient_display_always_uses_xvfb(self):
        for script in ('test_binding_packages', 'test_compiled_binding_packages'):
            spec = importlib.util.spec_from_file_location(script, ROOT / 'scripts' / (script + '.py'))
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            args = ['runner', '--native', '--font', str(ROOT / 'LICENSE')]
            args += ['--artifacts', '/tmp/no-artifacts'] if script == 'test_binding_packages' else ['--go-artifacts', '/tmp/no-go', '--rust-artifacts', '/tmp/no-rust']
            def run(command, **kwargs):
                self.assertEqual(command[0], 'xvfb-run')
                self.assertNotIn('DISPLAY', kwargs['env'])
                self.assertNotIn('WAYLAND_DISPLAY', kwargs['env'])
                self.assertEqual(kwargs['env']['CUI_PACKAGE_SMOKE_BACKEND'], 'none')
                self.assertIn('--virtual-token', command)
                return type('Result', (), {'returncode': 0})()
            with patch.dict(os.environ, {'DISPLAY': ':0', 'WAYLAND_DISPLAY': 'wayland-0', 'CUI_PACKAGE_VIRTUAL': '1', 'CUI_PACKAGE_SMOKE_BACKEND': 'sdl'}), patch('sys.argv', args), patch.object(module.subprocess, 'run', run):
                with self.assertRaises(SystemExit) as result: module.main()
                self.assertEqual(result.exception.code, 0)
if __name__ == '__main__': unittest.main()
