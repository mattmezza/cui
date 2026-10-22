#!/usr/bin/env python3
"""Private registration through the existing pinned offline WASI driver."""
import importlib.util
from pathlib import Path
import sys
root = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('cui_wasi', root / 'scripts/test_wasi.py')
driver = importlib.util.module_from_spec(spec)
spec.loader.exec_module(driver)
sys.argv += ['--root', str(root), '--suite', 'date_time_picker', '--suite', 'date_time_picker_styled']
raise SystemExit(driver.main())
