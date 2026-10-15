# Native macOS applications

CUI's portable library has macOS CI evidence. The native workflow below adds the
SDL3/text adapters and a `.app` starter. A configured CI job or a successful Linux
build is not evidence that the app runs correctly on macOS. Record the revision,
Mac architecture, macOS version, dependency receipt and results when validating.

## Build on your Apple Silicon Mac

Install Xcode Command Line Tools, CMake 3.20+, Python 3, Git, pkg-config and a build
tool. Use an arm64 terminal (`uname -m` should print `arm64`), rather than a
Rosetta x86_64 shell. The commands below use Makefiles, so Ninja is optional. Run from the repo.
Dependency sources are pinned in `tools/native-deps/CMakeLists.txt`; this avoids
relying on changing Homebrew library versions. The initial build needs network
access. SDK and compiler remain your local Xcode tools.

```sh
cmake -S tools/native-deps -B build/macos-deps \
  -DCMAKE_BUILD_TYPE=Release \
  -DCUI_DEPS_PREFIX="$PWD/build/macos-prefix"
cmake --build build/macos-deps --parallel 4
python3 tests/test_native_deps.py --prefix "$PWD/build/macos-prefix"

PKG_CONFIG_PATH="$PWD/build/macos-prefix/lib/pkgconfig" \
cmake -S . -B build/macos \
  -DCMAKE_BUILD_TYPE=Release \
  -DCUI_REQUIRE_NATIVE=ON -DCUI_BUILD_NATIVE_STARTER=ON \
  -DCUI_HEADLESS_NATIVE_TESTS=ON -DCUI_BUILD_CPP_EXAMPLE=OFF \
  -DCUI_TEST_FONT="$PWD/third_party/inter/Inter-Regular.otf" \
  -DCUI_MACOS_DEPENDENCY_LICENSE_DIR="$PWD/build/macos-prefix/share/licenses"
cmake --build build/macos --parallel 4
python3 scripts/test_local.py --build-dir build/macos
cmake --install build/macos --prefix "$PWD/build/macos-installed"
```

The installed starter is `build/macos-installed/cui_native_starter.app`.
Installation deploys non-system dynamic dependencies into the bundle on a native
Mac, then applies and verifies ad-hoc development signatures without a signing
identity or keychain. Inspect installation output and test the installed app away from the build
prefix before distributing it. The build-tree bundle still references build
libraries. `CUI_REQUIRE_NATIVE=ON` makes missing native dependencies a configure
error instead of silently building only portable targets.

The bundle includes pinned Inter regular/bold fonts and their license. The
starter locates its default regular font relative to its executable, independent
of the launch directory; `--font PATH` overrides it. The larger starter currently
uses the regular face; bundling the bold face does not automatically register a
bold style. An application icon and your own bundle identifier/branding are
application work. See `examples/native_starter/README.md` for copied projects.

## Automated checks without desktop windows

`CUI_HEADLESS_NATIVE_TESTS=ON` forces the dummy SDL video driver and software
renderer for registered CTest tests. GPU checks cannot be enabled in this mode.
Use `scripts/test_local.py`, which recognizes this configuration on macOS and
preserves CTest's exit code. This exercises native adapter services without
validating Cocoa windows or Metal. Some shaping tests need additional distinct
family/Hebrew/Arabic font fixtures and can report skips; inspect the test output.
Do not treat skipped coverage as a pass.

A bounded headless starter capture can additionally verify packaged resources:

```sh
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  build/macos-installed/cui_native_starter.app/Contents/MacOS/cui_native_starter \
  --frames 3 --capture "$PWD/build/macos-starter.bmp"
```

## Manual Mac acceptance

These are intentional interactive launches, outside automated tests:

```sh
open build/macos-installed/cui_native_starter.app
```

Check the installed app from Finder and a different working directory. Verify:

- Navigation, editable settings, keyboard focus, shortcuts and clipboard.
- Window close/minimize/maximize/fullscreen behavior and native traffic lights.
- Rendering and text at Retina scale, resizing, and moving between displays.
- Input-method marked text, candidate placement and committed text.
- System appearance changes, sleep/resume and relaunch.
- Launch with the dependency prefix unavailable, proving deployed libraries and
  resources are sufficient. Use a copied bundle on another Mac for best evidence.

Record failures rather than assuming SDL's platform support proves CUI behavior.
The dedicated SDL_GPU adapter still lacks Metal shaders. Native macOS AX/VoiceOver
integration remains unfinished and requires separate implementation and acceptance;
this starter workflow does not establish accessible production-app support.

## Distribution and cross compilation

The installed app is a development artifact. Choose a deployment target and
architecture policy, add branding, and establish Developer ID signing,
entitlements as needed, notarization and release testing before public release.
Dependency fixup changes Mach-O files; sign after deployment, including nested
libraries. No signing identity or notarization credentials are requested by these
build scripts. Ad-hoc signatures are for local development; [Apple’s distribution
signing guidance](https://developer.apple.com/documentation/xcode/creating-distribution-signed-code-for-the-mac/)
describes the separate release identities. Universal apps require every dependency and executable to include
the same architecture slices; this workflow starts with one architecture.

See [Linux to macOS](MACOS_CROSS.md) for the cross-toolchain workflow. Cross-built
artifacts still require validation on a Mac; local Linux tests cannot execute
Mach-O binaries or establish Cocoa/Metal behavior.
