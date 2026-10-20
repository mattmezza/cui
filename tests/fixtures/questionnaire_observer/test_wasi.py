#!/usr/bin/env python3
"""Extend the pinned runner in memory for the unregistered observer worker."""
import hashlib
import importlib.util
from pathlib import Path

root = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location("observer_worker_wasi", root / "scripts/test_wasi.py")
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
runner.SOURCES = (*runner.SOURCES, "src/base/cui_questionnaire_observer.c")
runner.SUITES = (*runner.SUITES, "questionnaire_observer")
original_fingerprint = runner.source_fingerprint

def source_fingerprint(source_root):
    digest = hashlib.sha256(original_fingerprint(source_root).encode())
    digest.update((source_root / "src/base/cui_questionnaire_observer_internal.h").read_bytes())
    return digest.hexdigest()

runner.source_fingerprint = source_fingerprint
if __name__ == "__main__":
    raise SystemExit(runner.main())
