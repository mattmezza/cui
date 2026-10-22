#!/usr/bin/env python3
"""Worker-only additions to the canonical pinned WASI runner; no installation."""
import importlib.util
import hashlib
from pathlib import Path
root = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location("canonical_wasi", root / "scripts/test_wasi.py")
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
runner.SOURCES += ("src/components/cui_questionnaire_presentation.c",)
runner.SUITES += ("questionnaire_presentation",)
original_fingerprint = runner.source_fingerprint
def fingerprint(source_root):
    digest = hashlib.sha256(original_fingerprint(source_root).encode())
    digest.update((source_root / "src/components/cui_questionnaire_presentation_private.h").read_bytes())
    return digest.hexdigest()
runner.source_fingerprint = fingerprint
if __name__ == "__main__":
    raise SystemExit(runner.main())
