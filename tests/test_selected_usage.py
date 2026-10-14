#!/usr/bin/env python3
"""Compile selected gallery examples as complete portable C11 units."""
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SelectedUsageTests(unittest.TestCase):
    def test_all_supported_modes_sizes_and_disabled_states_compile(self):
        script = '''
import {selectedUsage} from './web/modules/cui_usage.js';
const cases=[];
for(const [name,modes] of [['Button',[1,2,3]],['Input',[4,5,6,7,8,9]]])
 for(const mode of modes)for(let size=0;size<15;size++)
  for(const disabled of [false,true])cases.push({name,mode,size,disabled,
   code:selectedUsage(name,mode,size,disabled)});
if(selectedUsage('Button',0)!==null||selectedUsage('Input',0)!==null||
 selectedUsage('Checkbox',1)!==null)throw Error('legacy excerpt fallback');
process.stdout.write(JSON.stringify(cases));
'''
        cases = json.loads(subprocess.check_output(
            ['node', '--input-type=module', '-e', script], cwd=ROOT, text=True))
        self.assertEqual(len(cases), 270)
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary) / 'example.c'
            for case in cases:
                with self.subTest(**{k: v for k, v in case.items() if k != 'code'}):
                    code = case['code']
                    self.assertIn(f".disabled = {str(case['disabled']).lower()}", code)
                    if case['name'] == 'Input' and case['mode'] == 9:
                        self.assertIn('cui_regex_matcher(&view->session)', code)
                        self.assertIn('cui_regex_session_destroy', code)
                        self.assertIn('1024 * 1024', code)
                    elif case['name'] == 'Input' and case['mode'] >= 7:
                        self.assertIn('CUI_EDIT_FORMAT_PATTERN' if case['mode'] == 7 else 'CUI_EDIT_FORMAT_NUMBER', code)
                        self.assertIn('.format_workspace = {workspace, capacity}', code)
                    elif case['name'] == 'Input':
                        self.assertIn(f".masked = {str(case['mode'] != 5).lower()}", code)
                        self.assertIn('NEW_PASSWORD' if case['mode'] == 6 else 'PURPOSE_PASSWORD', code)
                    else:
                        self.assertIn(f".multiple = {str(case['mode'] == 2).lower()}", code)
                        self.assertIn(f".vertical = {str(case['mode'] == 3).lower()}", code)
                        self.assertIn('.change_user = view', code)
                        self.assertIn('free(view->workspace)', code)
                    source.write_text(code)
                    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                                    '-pedantic', '-fsyntax-only', '-I', str(ROOT / 'include'),
                                    str(source)], check=True, capture_output=True)


if __name__ == '__main__':
    unittest.main()
