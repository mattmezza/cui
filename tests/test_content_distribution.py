#!/usr/bin/env python3
"""Compile generated Content directly, without an umbrella hiding dependencies."""
import importlib.util
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ContentDistributionTests(unittest.TestCase):
    def test_direct_content_implementation(self):
        compiler = shlex.split(os.environ.get("CC", "cc"))
        if not compiler or not shutil.which(compiler[0]):
            self.skipTest("C compiler unavailable")
        spec = importlib.util.spec_from_file_location("content_amalgamate", ROOT / "scripts/amalgamate.py")
        generator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(generator)
        with tempfile.TemporaryDirectory(prefix="cui-content-dist-") as directory:
            output = Path(directory)
            for name, sources in generator.MODULES.items():
                if (ROOT / "include/cui" / (name + ".h")).exists():
                    (output / (name + ".h")).write_text(generator.generate(name, sources), encoding="utf-8")
            for name in ("cui_platform.h", "cui_renderer.h"):
                (output / name).write_text((ROOT / "include/cui" / name).read_text(encoding="utf-8"), encoding="utf-8")
            source = output / "content.c"
            source.write_text('#define CUI_CONTENT_IMPLEMENTATION\n#include "cui_content.h"\n', encoding="utf-8")
            result = subprocess.run(compiler + ["-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                                               "-fsyntax-only", "-I" + directory, str(source)],
                                    text=True, capture_output=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            # C++ consumers get public declarations; C implementation remains C11.
            cxx = shlex.split(os.environ.get("CXX", "c++"))
            if cxx and shutil.which(cxx[0]):
                declarations = output / "declarations.cpp"
                declarations.write_text('#include "cui_content.h"\n#include "cui_hover_events.h"\n', encoding="utf-8")
                result = subprocess.run(cxx + ["-std=c++17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                                              "-fsyntax-only", "-I" + directory, str(declarations)],
                                        text=True, capture_output=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)



if __name__ == "__main__":
    unittest.main()
