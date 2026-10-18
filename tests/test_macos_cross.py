"""Host-only guard tests; never execute a Mach-O binary."""
import argparse
import importlib.util
import os
import subprocess
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
        self.assertEqual(env["PKG_CONFIG_LIBDIR"].split(os.pathsep),
                         [str(self.prefix.resolve() / "lib/pkgconfig"),
                          str(self.prefix.resolve() / "share/pkgconfig")])
        self.assertIn("-DCUI_REQUIRE_NATIVE=ON", configure)
        self.assertIn("-DCUI_BUILD_CPP_EXAMPLE=OFF", configure)
        self.assertEqual(build[-2:], ["--target", "cui_native_starter"])

    @unittest.skipIf(os.name == "nt", "directory symlink privilege is not guaranteed")
    def test_dependency_prefix_alias_is_canonicalized(self):
        alias = self.root / "dependency alias"
        alias.symlink_to(self.prefix, target_is_directory=True)
        self.args.prefix = str(alias)
        configure, _, env = cross.commands(self.args)
        self.assertEqual(env["PKG_CONFIG_LIBDIR"].split(os.pathsep),
                         [str(self.prefix.resolve() / "lib/pkgconfig"),
                          str(self.prefix.resolve() / "share/pkgconfig")])
        self.assertIn("-DCUI_MACOS_PREFIX=" + str(self.prefix.resolve()), configure)

    def test_compiler_symlink_name_is_preserved(self):
        wrapper = self.root / "wrapper"
        wrapper.write_text("#!/bin/sh\nexit 0\n")
        wrapper.chmod(0o755)
        self.cc.unlink()
        self.cc.symlink_to(wrapper)
        configure, _, _ = cross.commands(self.args)
        self.assertIn("-DCUI_MACOS_CC=" + str(self.cc), configure)

    def test_toolchain_selects_target_cxx(self):
        cxx = self.root / "arm64-apple-darwin-clang++"
        cxx.write_text("#!/bin/sh\nexit 0\n")
        cxx.chmod(0o755)
        toolchain = Path(__file__).resolve().parents[1] / "cmake/toolchains/macos-osxcross.cmake"
        script = self.root / "check.cmake"
        script.write_text(f'''set(CUI_MACOS_CC "{self.cc.as_posix()}")
set(CUI_MACOS_SDK "{self.sdk.as_posix()}")
set(CUI_MACOS_PREFIX "{self.prefix.as_posix()}")
set(CUI_MACOS_ARCH arm64)
set(CUI_MACOS_DEPLOYMENT_TARGET 11.0)
include("{toolchain.as_posix()}")
if(NOT CMAKE_CXX_COMPILER STREQUAL "{cxx.as_posix()}")
 message(FATAL_ERROR "Host C++ compiler selected")
endif()
''')
        result = subprocess.run(["cmake", "-P", str(script)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

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
