"""Exercise newcomer-facing manifest queries without native dependencies."""
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class StatusTests(unittest.TestCase):
    def query(self, *args):
        return subprocess.run([sys.executable, str(ROOT / 'scripts/cui_status.py'), *args],
                              cwd=ROOT, text=True, capture_output=True)

    def test_selected_components_include_evidence_and_limits(self):
        result = self.query('input', 'Icon', 'Sidebar')
        self.assertEqual(result.returncode, 0, result.stderr)
        for name in ('Input', 'Icon', 'Sidebar'):
            self.assertIn(name + ': tested', result.stdout)
        self.assertIn('Evidence: tests/', result.stdout)
        self.assertIn('Limitation:', result.stdout)
        self.assertNotIn('Avatar:', result.stdout)

    def test_unknown_component_does_not_silently_show_everything(self):
        result = self.query('MissingComponent')
        self.assertEqual(result.returncode, 2)
        self.assertIn('Unknown component(s): MissingComponent', result.stderr)
        self.assertEqual(result.stdout, '')

    def test_summary_lists_every_family(self):
        result = self.query()
        self.assertEqual(result.returncode, 0, result.stderr)
        rows = [line for line in result.stdout.splitlines() if line.startswith('| ')]
        self.assertEqual(len(rows), 80)  # 78 families plus table header/separator.
        self.assertIn('| Input | tested |', result.stdout)


if __name__ == '__main__':
    unittest.main()
