#!/usr/bin/env python3
"""Offline acceptance-runner tests: no Go/Rust/native tools or dependencies."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import types
import unittest
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location('time_runner', Path(__file__).with_name('test_time_localized.py'))
RUNNER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RUNNER)


class RunnerTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.cache = self.root / 'target'
        self.cache.mkdir()
        self.providers = []
        for name in ('current', 'old', 'partial'):
            p = self.root / (name + '.so')
            p.write_text(name)
            self.providers.append(p)

    def args(self, language):
        return RUNNER.parser().parse_args([
            '--language', language, '--build-dir', str(self.root),
            '--library', str(self.providers[0]), '--old-library', str(self.providers[1]),
            '--partial-library', str(self.providers[2]), '--target-dir', str(self.cache),
            '--go', 'retained-go', '--cargo', 'retained-cargo'])

    def test_validation_refuses_desktop_missing_and_aliased_inputs(self):
        with self.assertRaisesRegex(ValueError, 'private display'):
            RUNNER.validate(self.args('go'), {}, 'linux')
        env = {'CUI_TEST_VIRTUAL_DISPLAY': '1'}
        with self.assertRaisesRegex(ValueError, 'Linux'):
            RUNNER.validate(self.args('go'), env, 'win32')
        a = self.args('rust')
        a.old_library = a.library
        with self.assertRaisesRegex(ValueError, 'distinct'):
            RUNNER.validate(a, env, 'linux')
        a = self.args('rust')
        a.target_dir = None
        with self.assertRaisesRegex(ValueError, 'existing reusable'):
            RUNNER.validate(a, env, 'linux')

    def test_go_plan_selects_actual_providers_offline_and_never_native(self):
        calls = []
        with patch.dict(os.environ, {'LD_PRELOAD': 'hostile', 'CUI_TEST_TIME_PROVIDER': 'leaked'}):
            RUNNER.run_suite(self.args('go'), lambda cmd,cwd,env,**kw: calls.append((cmd,cwd,env)))
        self.assertEqual(len(calls), 4)
        self.assertEqual([c[2]['CUI_BRIDGE_LIBRARY'] for c in calls],
                         [str(p) for p in (self.providers[0], self.providers[1], self.providers[2], self.providers[0])])
        for cmd,_,env in calls:
            self.assertNotIn('--native', cmd)
            self.assertNotIn('LD_PRELOAD', env)
            self.assertEqual(env['GOFLAGS'], '-p=1')
            self.assertEqual(env['GOPROXY'], 'off')
        self.assertNotIn('CUI_TEST_TIME_PROVIDER', calls[0][2])
        self.assertIn('^TestLocalizedTimeOptionalProvider$', calls[1][0])

    def test_rust_reuses_reported_executable_for_all_providers(self):
        exe = self.cache / 'actual-test-binary'
        exe.write_text('not executable; mock only')
        calls = []
        output = json.dumps({'reason':'compiler-artifact', 'target':{'name':'time_localized','kind':['test']}, 'executable':str(exe)})
        def run(cmd,cwd,env,**kwargs):
            calls.append((cmd,cwd,env,kwargs))
            return types.SimpleNamespace(stdout=output)
        RUNNER.run_suite(self.args('rust'), run)
        self.assertEqual(len(calls), 6)
        self.assertEqual(calls[0][0][-2:], ['--no-run','--message-format=json'])
        for index in (1,2,3):
            self.assertEqual(calls[index][0][0], exe)
        for name,provider in zip(('old','partial'), self.providers[1:]):
            self.assertEqual((self.root/'time-localized-providers'/name/'libcui_bridge.so').resolve(), provider)
        for _,_,env,_ in calls:
            self.assertEqual(env['CARGO_TARGET_DIR'], str(self.cache))
            self.assertEqual(env['CARGO_BUILD_JOBS'], '1')
            self.assertEqual(env['CUI_BRIDGE_LIB_DIR'], str(self.providers[0].parent))
        self.assertIn('--offline',calls[4][0])
        self.assertIn('--no-default-features',calls[5][0])

    def test_artifact_rejects_missing_ambiguous_and_escaped_paths(self):
        with self.assertRaisesRegex(ValueError, 'exactly one'):
            RUNNER.artifact('', self.cache)
        def record(path):
            return json.dumps({'reason':'compiler-artifact','target':{'name':'time_localized','kind':['test']},'executable':str(path)})
        with self.assertRaisesRegex(ValueError, 'escaped'):
            RUNNER.artifact(record(self.providers[0]),self.cache)
        for name in ('a','b'):
            (self.cache/name).write_text(name)
        with self.assertRaisesRegex(ValueError, 'exactly one'):
            RUNNER.artifact(record(self.cache/'a')+'\n'+record(self.cache/'b'),self.cache)

    def test_command_failure_stops_before_any_later_step(self):
        calls = []
        def fail(cmd,*args,**kwargs):
            calls.append(cmd)
            raise subprocess.CalledProcessError(1,cmd)
        with self.assertRaises(subprocess.CalledProcessError):
            RUNNER.run_suite(self.args('go'),fail)
        self.assertEqual(len(calls),1)

    def test_provider_alias_never_overwrites_owned_file(self):
        a = self.args('rust')
        out=self.root/'time-localized-providers'/'partial'
        out.mkdir(parents=True)
        (out/'libcui_bridge.so').write_text('keep')
        with self.assertRaisesRegex(ValueError,'refusing'):
            RUNNER.provider_dir(a,'partial',self.providers[2])
        self.assertEqual((out/'libcui_bridge.so').read_text(),'keep')


if __name__ == '__main__':
    unittest.main()
