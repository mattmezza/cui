"""Host-only guard tests; never execute a Mach-O binary."""
import argparse
import importlib.util
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("macos_cross", Path(__file__).resolve().parents[1] / "scripts/macos_cross.py")
cross = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cross)


class MacOSCrossTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.cc = self.root / "arm64-apple-darwin-clang"
        self.cc.write_text("#!/bin/sh\nexit 0\n")
        self.cc.chmod(0o755)
        self.sdk = self.root / "MacOSX.sdk"
        (self.sdk / "usr/include").mkdir(parents=True)
        (self.sdk / "System/Library/Frameworks").mkdir(parents=True)
        self.prefix = self.root / "deps"
        (self.prefix / "lib/pkgconfig").mkdir(parents=True)
        for name in ("sdl3", "freetype2", "harfbuzz"):
            (self.prefix / "lib/pkgconfig" / (name + ".pc")).touch()
        self.args = argparse.Namespace(cc=str(self.cc), sdk=str(self.sdk), prefix=str(self.prefix),
                                       arch="arm64", deployment_target="11.0", build_dir=str(self.root / "build"))

    def test_target_isolation_and_starter_only(self):
        with patch.dict(os.environ, {"PKG_CONFIG_PATH": "/usr/lib/pkgconfig", "CFLAGS": "-I/usr/include", "CPATH": "/usr/include"}):
            configure, build, env = cross.commands(self.args)
        self.assertEqual(env["PKG_CONFIG_PATH"], "")
        self.assertNotIn("CFLAGS", env)
        self.assertNotIn("CPATH", env)
        self.assertTrue(env["PKG_CONFIG_LIBDIR"].startswith(str(self.prefix)))
        self.assertIn("-DCUI_REQUIRE_NATIVE=ON", configure)
        self.assertIn("-DCUI_BUILD_CPP_EXAMPLE=OFF", configure)
        self.assertEqual(build[-2:], ["--target", "cui_native_starter"])

    def test_no_sdk_is_an_error(self):
        self.args.sdk = str(self.root / "absent")
        with self.assertRaisesRegex(ValueError, "Apple SDK"):
            cross.commands(self.args)

    def test_missing_target_dependency_is_an_error(self):
        (self.prefix / "lib/pkgconfig/harfbuzz.pc").unlink()
        with self.assertRaisesRegex(ValueError, "harfbuzz"):
            cross.commands(self.args)

    def test_arm64_minimum(self):
        self.args.deployment_target = "10.15"
        with self.assertRaisesRegex(ValueError, "11.0"):
            cross.commands(self.args)

    def test_main_configures_and_builds_without_running_target(self):
        argv = ["--cc", str(self.cc), "--sdk", str(self.sdk), "--prefix", str(self.prefix),
                "--arch", "arm64", "--deployment-target", "11.0"]
        with patch.object(cross.subprocess, "run") as run, patch.object(cross.shutil, "which", return_value="/usr/bin/pkg-config"):
            self.assertEqual(cross.main(argv), 0)
        self.assertEqual(len(run.call_args_list), 3)
        self.assertEqual(run.call_args_list[0].args[0][0], "pkg-config")
        self.assertEqual(run.call_args_list[2].args[0][-1], "cui_native_starter")


if __name__ == "__main__":
    unittest.main()
