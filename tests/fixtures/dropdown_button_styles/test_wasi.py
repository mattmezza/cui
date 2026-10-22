"""Worker convenience entry delegates all registration to the canonical runner."""
import runpy
from pathlib import Path

runpy.run_path(str(Path(__file__).resolve().parents[3] / "scripts/test_wasi.py"), run_name="__main__")
