#!/usr/bin/env python3
"""Local safety/invocation contracts for the dedicated Linux runner workflow.

Uses the standard library; hosted Actions execution remains a separate check.
"""
from pathlib import Path
import re
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / '.github/workflows/binding-native-run.yml'


def validate(text):
    errors = []
    required = (
        'runs-on: ubuntu-24.04', 'contents: read', 'timeout-minutes: 45',
        "python-version: '3.12'", "node-version: '25.8.0'", "go-version: '1.23.6'",
        'rustup toolchain install 1.85.1 --profile minimal',
        'python scripts/generate_go_sdk.py --check',
        'cmake -S tools/native-deps', 'python tests/test_native_deps.py --prefix',
        '-DCUI_REQUIRE_NATIVE=ON', '-DCUI_BUILD_NATIVE=ON',
        '-DCUI_BUILD_BRIDGE=ON', '-DCUI_SANITIZE=OFF',
        '--target cui_bridge', 'xvfb xauth xdotool', 'LD_RUN_PATH=$RUNNER_TEMP/cui-run-deps/lib',
        'rustup run 1.85.1 python scripts/test_binding_run.py',
        '--source-root "$PWD"', '--build-dir "$RUNNER_TEMP/cui-binding-run"',
        '--library "$RUNNER_TEMP/cui-run-native/libcui_bridge.so"',
        '--font "$PWD/third_party/inter/Inter-Regular.otf"',
        '--node-include "$node_include" --mode all', 'test -f "$node_include/node_api.h"',
    )
    for item in required:
        if item not in text:
            errors.append('missing contract: ' + item)
    actions = re.findall(r'^\s*- uses: (\S+)', text, re.M)
    if len(actions) != 4 or any(not re.fullmatch(r'actions/[\w-]+@[0-9a-f]{40}', action) for action in actions):
        errors.append('all four actions must be immutable SHA pinned')
    for forbidden in ('CUI_SANITIZE=ON', 'id-token: write', '${{ secrets.',
                      'DISPLAY:', 'WAYLAND_DISPLAY:', 'LD_PRELOAD:', 'npm publish',
                      'cargo publish', 'twine upload'):
        if forbidden in text:
            errors.append('forbidden workflow setting: ' + forbidden)
    return errors


class WorkflowTests(unittest.TestCase):
    def test_current_workflow_contract(self):
        self.assertEqual(validate(WORKFLOW.read_text()), [])

    def test_rejects_partial_mode_asan_and_mutable_actions(self):
        source = WORKFLOW.read_text()
        for before, after in (('--mode all', '--mode dynamic'),
                              ('CUI_SANITIZE=OFF', 'CUI_SANITIZE=ON'),
                              ('actions/checkout@11d5960a326750d5838078e36cf38b85af677262', 'actions/checkout@main')):
            with self.subTest(after=after):
                self.assertTrue(validate(source.replace(before, after)))

    def test_runner_paths_are_initialized_inside_a_step(self):
        source = WORKFLOW.read_text()
        job_header = source.split('    steps:', 1)[0]
        self.assertNotIn('${{ runner.', job_header)
        for name in ('PKG_CONFIG_PATH', 'LD_RUN_PATH'):
            self.assertRegex(source, name + r'=\$RUNNER_TEMP/[^\n]+ >> "\$GITHUB_ENV"')

    def test_filtered_bridge_jobs_build_portable_runner(self):
        source = (ROOT / '.github/workflows/ci.yml').read_text()
        for job in ('node-builders', 'go-rust-builders'):
            with self.subTest(job=job):
                section = source.split('  ' + job + ':', 1)[1]
                section = re.split(r'\n  [a-zA-Z][\w-]*:', section, maxsplit=1)[0]
                build = re.search(r'cmake --build [^\n]+--target ([^\n]+)', section)
                self.assertIsNotNone(build)
                self.assertIn('test_bridge_portable_run', build.group(1).split())

    def test_every_literal_run_block_is_valid_bash(self):
        # This workflow deliberately uses only multiline literal shell blocks.
        lines = WORKFLOW.read_text().splitlines()
        scripts = []
        for index, line in enumerate(lines):
            if line.strip() != 'run: |':
                continue
            indent = len(line) - len(line.lstrip())
            body = []
            for candidate in lines[index + 1:]:
                if candidate.strip() and len(candidate) - len(candidate.lstrip()) <= indent:
                    break
                body.append(candidate[indent + 2:])
            scripts.append('\n'.join(body))
        self.assertEqual(len(scripts), 6)
        for script in scripts:
            result = subprocess.run(['bash', '-n'], input=script, text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == '__main__':
    unittest.main()
