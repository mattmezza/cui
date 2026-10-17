# Installed native text validation

Run on Linux with native FreeType, HarfBuzz and SDL3 development/runtime dependencies available:

```sh
python tests/packaging/test_installed_text.py \
  --artifacts /path/to/native-artifacts \
  --font /usr/share/fonts/noto/NotoSans-Regular.ttf \
  --rtl-font /usr/share/fonts/noto/NotoSansHebrew-Regular.ttf \
  --build-dir /tmp/cui-installed-text
```

The artifact directory must contain exactly one Python wheel and one prebuilt npm
`.tgz`. This test never installs packages, downloads dependencies or starts a
window. It removes display variables, selects SDL's dummy driver, extracts only
ordinary archive files/directories into a temporary directory and compiles the
existing `sdk_text_fixture.c` against each package's headers and bundled
`libcui_bridge.so`. No package source is rebuilt or substituted for those libraries.
The canonical reference is built from this checkout with native text and bidi
support. `--reference-build DIR` can reuse a matching canonical build containing
`libcui_text.a`, `libcui.a` and `libcui_bidi.a`; the caller must ensure that build
matches the source being validated.

The shared fixture asserts contextual bidi geometry, invalid byte-range rejection,
RTL runs, fallback faces, nonzero glyph IDs, selection/caret behavior and colored
raster coverage. UNAVAILABLE results and missing RTL/fallback/color coverage fail
its assertions. Choose a primary font without Hebrew coverage so the fallback
assertion is meaningful. All printed fixture values must match canonical output
using exactly the same fonts on the same host. Font, artifact and library SHA256
values are printed. Library hashes are checked against wheel RECORD and any
provided build-info library SHA256 metadata; explicitly non-native metadata fails.

This proves packaged C text behavior. It does not prove Python/Node wrapper text
API coverage, package installation behavior, SDL rendering, GPU behavior or
cross-host raster reproducibility. Run `scripts/test_binding_packages.py --native
--require-prebuilt` separately for actual installed binding UI consumers under
its dedicated invisible Xvfb display.
