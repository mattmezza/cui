#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Private opt-in registration using the repository's pinned offline WASI driver."""
import importlib.util
from pathlib import Path
import sys
import re

root = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('cui_pinned_wasi', root / 'scripts/test_wasi.py')
driver = importlib.util.module_from_spec(spec)
spec.loader.exec_module(driver)
build = Path(sys.argv[sys.argv.index('--build-dir') + 1]).resolve()
build.mkdir(parents=True, exist_ok=True)
guide = build / 'date_picker_actions_guide.c'
guide.write_text(re.search(r'```c\n([^`]+)```', (root / 'docs/DATE_PICKER_ACTIONS.md').read_text()).group(1))
driver.SOURCES += (str(guide), 'src/base/cui_date_picker_actions.c', 'src/components/cui_date_picker_actions.c')
driver.SUITES = ('date_picker_actions',)
sys.argv += ['--root', str(root), '--suite', 'date_picker_actions']
raise SystemExit(driver.main())
