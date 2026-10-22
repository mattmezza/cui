"""Worker-only focused WASI registration; canonical integration remains separate."""
import hashlib
import importlib.util
from pathlib import Path

root = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location("canonical_wasi", root / "scripts/test_wasi.py")
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
runner.SUITES = ("dropdown_button_styles",)
original_fingerprint = runner.source_fingerprint


def fingerprint(source_root):
    digest = hashlib.sha256(original_fingerprint(source_root).encode())
    digest.update((source_root / "tests/fixtures/dropdown_button_styles/legacy_box.h").read_bytes())
    return digest.hexdigest()


runner.source_fingerprint = fingerprint
raise SystemExit(runner.main())
