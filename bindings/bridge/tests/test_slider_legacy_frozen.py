"""Fieldwise text trace captured from ecb97d5e bridge against the same engine.

Also independently matched the preserved pre-typed runtime. No candidate-generated
expectation, padding bytes or dynamic loading of two modules in one process.
"""
from pathlib import Path
import re
import subprocess
import sys

actual = subprocess.check_output([sys.argv[1]], text=True)
expected = Path(__file__).with_name("slider_legacy_ecb97d5e.txt").read_text()
# C %a's default trailing-zero/exponent spelling varies across CRTs. Compare the
# exact represented double, preserving signed zero, with no numeric tolerance.
def canonical(text):
    return re.sub(r"[-+]?0[xX][0-9a-fA-F]+(?:\.[0-9a-fA-F]*)?[pP][-+]?\d+",
                  lambda m: float.fromhex(m[0]).hex(), text)


assert canonical(actual) == canonical(expected), f"legacy scalar trace changed\nEXPECTED:\n{expected}\nACTUAL:\n{actual}"
