#!/usr/bin/env python3
"""Configure/build the native macOS starter using a caller-provisioned OSXCross SDK."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


def commands(args):
    # OSXCross wrappers can dispatch by symlink basename; preserve that name.
    cc = Path(os.path.abspath(args.cc))
    sdk = Path(args.sdk).resolve()
    prefix = Path(args.prefix).resolve()
    if not cc.is_file() or not os.access(cc, os.X_OK):
        raise ValueError("--cc must name an executable OSXCross C compiler")
    if not (sdk / "usr/include").is_dir() or not (sdk / "System/Library/Frameworks").is_dir():
        raise ValueError("--sdk must contain Apple SDK headers and frameworks")
    if not re.fullmatch(r"\d+\.\d+(?:\.\d+)?", args.deployment_target):
        raise ValueError("--deployment-target must be a numeric macOS version")
    if args.arch == "arm64" and int(args.deployment_target.split('.')[0]) < 11:
        raise ValueError("arm64 requires macOS 11.0 or newer")
    pcdirs = [prefix / "lib/pkgconfig", prefix / "share/pkgconfig"]
    for name in ("sdl3", "freetype2", "harfbuzz"):
        if not any((directory / (name + ".pc")).is_file() for directory in pcdirs):
            raise ValueError(f"target prefix is missing {name}.pc")
    env = os.environ.copy()
    for name in ("CPATH", "C_INCLUDE_PATH", "CPLUS_INCLUDE_PATH", "LIBRARY_PATH", "SDKROOT", "CMAKE_PREFIX_PATH", "CMAKE_TOOLCHAIN_FILE", "CC", "CXX", "CFLAGS", "CXXFLAGS", "OBJCFLAGS", "OBJCXXFLAGS", "CPPFLAGS", "LDFLAGS", "PKG_CONFIG", "CMAKE_INCLUDE_PATH", "CMAKE_LIBRARY_PATH", "CMAKE_FRAMEWORK_PATH", "CMAKE_APPBUNDLE_PATH"):
        env.pop(name, None)
    env.update(PKG_CONFIG_PATH="", PKG_CONFIG_LIBDIR=os.pathsep.join(map(str, pcdirs)),
               PKG_CONFIG_SYSROOT_DIR="", MACOSX_DEPLOYMENT_TARGET=args.deployment_target)
    root = Path(__file__).resolve().parents[1]
    configure = ["cmake", "-S", str(root), "-B", str(Path(args.build_dir).resolve()), "-G", "Ninja",
                 "-DCMAKE_TOOLCHAIN_FILE=" + str(root / "cmake/toolchains/macos-osxcross.cmake"),
                 "-DCUI_MACOS_CC=" + str(cc), "-DCUI_MACOS_SDK=" + str(sdk),
                 "-DCUI_MACOS_PREFIX=" + str(prefix), "-DCUI_MACOS_ARCH=" + args.arch,
                 "-DCUI_MACOS_DEPLOYMENT_TARGET=" + args.deployment_target,
                 "-DCUI_MACOS_DEPENDENCY_LICENSE_DIR=" + str(prefix / "share/licenses"),
                 "-DCMAKE_BUILD_TYPE=Release", "-DCUI_BUILD_NATIVE=ON", "-DCUI_REQUIRE_NATIVE=ON",
                 "-DCUI_BUILD_NATIVE_STARTER=ON", "-DCUI_BUILD_CPP_EXAMPLE=OFF", "-DCUI_FETCH_SDL=OFF",
                 "-DCUI_BUILD_ACCESSIBILITY=OFF"]
    return configure, ["cmake", "--build", str(Path(args.build_dir).resolve()), "--target", "cui_native_starter"], env


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("cc", "sdk", "prefix", "deployment-target"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--arch", choices=("arm64", "x86_64"), required=True)
    parser.add_argument("--build-dir", default="build-macos")
    parser.add_argument("--configure-only", action="store_true")
    args = parser.parse_args(argv)
    try:
        configure, build, env = commands(args)
        if not shutil.which("pkg-config"):
            raise ValueError("host pkg-config is required")
        subprocess.run(["pkg-config", "--exists", "sdl3 >= 3.2", "freetype2", "harfbuzz"], env=env, check=True)
        subprocess.run(configure, env=env, check=True)
        if not args.configure_only:
            subprocess.run(build, env=env, check=True)
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        print(f"macOS cross build: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
