#!/usr/bin/env python3
"""Extend the pinned runner only for this unregistered worker suite."""
import hashlib
import importlib.util
from pathlib import Path
root = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('progress_worker', root / 'scripts/test_wasi.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
runner.SUITES = (*runner.SUITES, 'questionnaire_progress')
original = runner.source_fingerprint
def fingerprint(source_root):
    digest = hashlib.sha256(original(source_root).encode())
    digest.update((source_root / 'tests/fixtures/questionnaire_progress/renderer.h').read_bytes())
    return digest.hexdigest()
runner.source_fingerprint = fingerprint
if __name__ == '__main__':
    raise SystemExit(runner.main())
