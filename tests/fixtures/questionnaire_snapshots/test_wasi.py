#!/usr/bin/env python3
"""Extend the pinned WASI runner in memory for this unregistered worker slice."""
import importlib.util
import hashlib
from pathlib import Path

root = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location("snapshots_worker_wasi", root / "scripts/test_wasi.py")
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
runner.SOURCES = (*runner.SOURCES, "src/base/cui_questionnaire_snapshots.c")
runner.SUITES = (*runner.SUITES, "questionnaire_snapshots")
original_fingerprint = runner.source_fingerprint

def source_fingerprint(source_root):
    digest = hashlib.sha256(original_fingerprint(source_root).encode())
    digest.update((source_root / "src/base/cui_questionnaire_snapshot_math.h").read_bytes())
    return digest.hexdigest()

runner.source_fingerprint = source_fingerprint
if __name__ == "__main__":
    raise SystemExit(runner.main())
