#!/usr/bin/env python3
"""Register the worker suite in memory; reuse the pinned canonical WASI runner."""
import importlib.util
from pathlib import Path

root = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location("questionnaire_worker_wasi", root / "scripts/test_wasi.py")
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
runner.SUITES = (*runner.SUITES, "questionnaire_keyboard")
if __name__ == "__main__":
    raise SystemExit(runner.main())
