# Linux to macOS native starter

This is a caller-provisioned cross-build path, not an SDK downloader or a
self-contained macOS release pipeline. No Apple SDK or OSXCross installation is
bundled. Without those inputs only the host-side guard tests can be validated.
The Linux build does not establish macOS runtime, graphics or accessibility parity.

## Provision the inputs

1. Build [OSXCross](https://github.com/tpoechtrager/osxcross) on Linux using an
   explicitly selected full Git commit and record that commit, compiler/linker
   versions and build options in your build provenance. Obtain an Apple SDK under
   its applicable license, following the upstream
   [SDK instructions](https://github.com/tpoechtrager/osxcross/blob/master/README.SDK.md).
   Record the SDK version and archive SHA-256. This repository deliberately does
   not select or download someone else's SDK or a floating toolchain revision.
2. Choose one architecture (`arm64` or `x86_64`) and deployment version. ARM64
   requires macOS 11.0 or newer; the deployment version must not exceed the SDK
   version. Use a separate build directory and dependency prefix per architecture.
3. Cross-build and install **macOS** SDL3 (>= 3.2), FreeType and HarfBuzz into an
   absolute prefix such as `/opt/cui-deps/arm64`. Pin and record source revisions
   or archive hashes for every dependency and its transitive dependencies.
   Use the same compiler, SDK, architecture and minimum macOS version for all.
   Disable dependency examples/tests and optional platform backends you do not
   need; never run configure-time target executables on Linux. The repository's
   Linux packages and Linux AccessKit archive cannot be reused.

The pinned dependency builder in this repository installs shared SDL3, FreeType
and HarfBuzz and their license files into a dedicated target prefix:

```sh
cmake -S tools/native-deps -B build/macos-deps -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/macos-osxcross.cmake" \
  -DCUI_DEPS_PREFIX="$PWD/build/macos-prefix" \
  -DCUI_MACOS_CC=/opt/osxcross/target/bin/arm64-apple-darwin24-clang \
  -DCUI_MACOS_SDK=/opt/osxcross/target/SDK/MacOSX15.sdk \
  -DCUI_MACOS_PREFIX="$PWD/build/macos-prefix" \
  -DCUI_MACOS_ARCH=arm64 -DCUI_MACOS_DEPLOYMENT_TARGET=11.0
cmake --build build/macos-deps --parallel 4
```

Use `--prefix "$PWD/build/macos-prefix"` in the starter helper below when using
this builder. It forwards the toolchain and target settings into each dependency.

For CMake-based dependency builds, reuse `cmake/toolchains/macos-osxcross.cmake`
with the five `CUI_MACOS_*` values shown below and an absolute
`CMAKE_INSTALL_PREFIX`. HarfBuzz/FreeType bootstrap and feature choices depend
on your selected pinned releases; provision their complete transitive closure.
Supply `sdl3.pc`, `freetype2.pc`, `harfbuzz.pc` and transitively required `.pc`
files under `<prefix>/lib/pkgconfig` or `<prefix>/share/pkgconfig`. Their absolute
include/library paths must point to your macOS prefix, never Linux `/usr`.
Use shared dependencies for this helper; fully static builds may need additional
private link flags and are not automatically supported. Stage dylibs separately
and adjust install names/rpaths for redistribution on macOS.

## Configure and compile

```sh
python scripts/macos_cross.py \
  --cc /opt/osxcross/target/bin/arm64-apple-darwin24-clang \
  --sdk /opt/osxcross/target/SDK/MacOSX15.sdk \
  --prefix /opt/cui-deps/arm64 \
  --arch arm64 --deployment-target 11.0 \
  --build-dir build-macos-arm64
```

Replace the illustrative compiler/SDK paths with your actual installation. The
helper requires host CMake, Ninja, Python and pkg-config. It verifies SDK layout
and target dependency metadata, clears common host compiler/pkg-config overrides,
configures with native adapters required, disables SDL fetching and the C++
example, and builds only `cui_native_starter`. `--configure-only` stops after
configuration. Use a fresh directory when changing compiler, SDK or architecture.

The equivalent toolchain parameters for dependency builds are:

```text
-DCMAKE_TOOLCHAIN_FILE=/absolute/path/to/cui/cmake/toolchains/macos-osxcross.cmake
-DCUI_MACOS_CC=/opt/osxcross/target/bin/arm64-apple-darwin24-clang
-DCUI_MACOS_SDK=/opt/osxcross/target/SDK/MacOSX15.sdk
-DCUI_MACOS_PREFIX=/opt/cui-deps/arm64
-DCUI_MACOS_ARCH=arm64
-DCUI_MACOS_DEPLOYMENT_TARGET=11.0
```

CMake searches target roots for headers, libraries and packages; host programs
remain discoverable. pkg-config searches only the supplied target prefix, with
no Linux fallback. This cannot repair malformed `.pc` files pointing outside the
prefix: audit dependency metadata and actual link commands. The Apple linker is
the final check that supplied libraries are compatible Mach-O artifacts.

Do **not** run CTest in the macOS cross-build directory on Linux: it contains
macOS executables. Host-only tooling checks are `python tests/test_macos_cross.py`.
Transfer the resulting executable and required dylibs to an actual compatible
Mac for launch, font, input, rendering and deployment validation. Signing,
notarization, universal binaries and distributable `.app` bundles remain separate
release work; a successful cross compile alone validates none of these.
