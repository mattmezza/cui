#!/usr/bin/env python3
"""Private registration through the existing pinned offline WASI driver."""
import importlib.util
from pathlib import Path
import sys
root = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('cui_wasi', root / 'scripts/test_wasi.py')
driver = importlib.util.module_from_spec(spec)
spec.loader.exec_module(driver)
driver.SOURCES += ('src/base/cui_date_time_picker.c',)
driver.SUITES = ('date_time_picker',)
original_fingerprint = driver.source_fingerprint
def fingerprint(source):
    import hashlib
    digest = hashlib.sha256(original_fingerprint(source).encode())
    digest.update((source / 'src/base/cui_date_time_internal.h').read_bytes())
    digest.update((source / 'src/base/cui_picker_composition_internal.h').read_bytes())
    return digest.hexdigest()
driver.source_fingerprint = fingerprint
sys.argv += ['--root', str(root), '--suite', 'date_time_picker']
raise SystemExit(driver.main())
