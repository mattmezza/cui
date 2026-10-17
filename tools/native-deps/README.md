# Pinned native dependencies

This standalone CMake superbuild installs SDL 3.2.28, FreeType 2.13.3, and
HarfBuzz 11.0.1 into one private prefix. Source archives use immutable commit
IDs and SHA-256 checksums. Shared libraries are the default so the native bundle
can deploy them. `-DBUILD_SHARED_LIBS=OFF` selects static libraries instead.
No system installation or administrator privileges are required.

From the repository root on macOS or Linux:

```sh
cmake -S tools/native-deps -B build/native-deps -G Ninja \
  -DCUI_DEPS_PREFIX="$PWD/build/native-prefix"
cmake --build build/native-deps --parallel 4
python tests/test_native_deps.py --prefix build/native-prefix
PKG_CONFIG_PATH="$PWD/build/native-prefix/lib/pkgconfig" \
  cmake -S . -B build -G Ninja -DCUI_BUILD_NATIVE=ON
cmake --build build
```

The dependency smoke test compiles and executes a consumer of SDL, FreeType,
HarfBuzz, and the HarfBuzz FreeType integration. It never initializes video or
opens a window. On Linux, run CUI's full native tests with
`python scripts/test_local.py --build-dir build` (the Xvfb wrapper). On macOS,
follow [the native Mac workflow](../../docs/MACOS.md), which enables the safe
headless test configuration before running that wrapper. GPU checks unsupported by the virtual display remain deferred.

For cross compilation, pass an absolute `CMAKE_TOOLCHAIN_FILE` path to this
superbuild, just as for CUI. The nested configurations receive that toolchain,
`CMAKE_OSX_SYSROOT`, `CMAKE_OSX_ARCHITECTURES`,
`CMAKE_OSX_DEPLOYMENT_TARGET`, compiler overrides, and compiler/linker flags.
For the repository's osxcross toolchain, `CUI_MACOS_CC`, optional `CUI_MACOS_CXX`, `CUI_MACOS_SDK`,
`CUI_MACOS_PREFIX`, `CUI_MACOS_ARCH`, and `CUI_MACOS_DEPLOYMENT_TARGET` are
also forwarded. Use a separate build directory and prefix for each architecture
and toolchain. The smoke test supports `--cc /path/to/target-clang --compile-only`
to validate cross linking without trying to execute a target binary.

Each dependency installs into `lib`, with pkg-config metadata in `lib/pkgconfig`.
Point the CUI configure environment's `PKG_CONFIG_PATH` there; for cross builds,
set `PKG_CONFIG_LIBDIR` there and clear `PKG_CONFIG_PATH` to prevent host package
fallback. FreeType disables optional compression, PNG, and HarfBuzz integrations;
HarfBuzz then builds its FreeType integration against the installed FreeType.
This avoids a dependency cycle and accidental host compression dependencies.
HarfBuzz utilities and subset libraries are omitted.

Upstream license files are copied under `share/licenses/{sdl3,freetype,harfbuzz}`
in the prefix for inclusion with deployed libraries. FreeType's dual licensing
terms are retained, including both its FreeType license and GPL text. Source
archives and build trees are retained in the dependency build directory.
