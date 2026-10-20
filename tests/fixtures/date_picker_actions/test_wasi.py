#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Private opt-in registration using the repository's pinned offline WASI driver."""
import importlib.util
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('cui_pinned_wasi', root / 'scripts/test_wasi.py')
driver = importlib.util.module_from_spec(spec)
spec.loader.exec_module(driver)
sys.argv += ['--root', str(root), '--suite', 'date_picker_actions']
raise SystemExit(driver.main())
