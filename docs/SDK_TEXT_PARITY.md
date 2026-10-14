# Native SDK contextual text build

Native Go and Rust SDK builds now enable the same `CUI_TEXT_BIDI` implementation
as the default canonical native text target. The generated minimal SDK builds
`cui_bidi` from `src/text/cui_bidi.c` and pinned SheenBidi unity objects, then links
it into `cui_text`. The generator copies the pinned dependency manifest, complete
SheenBidi headers/sources/license and local patch pin. Portable SDK builds retain
their previous dependency scope; native mode enables this additional analyzer.

Go native mode defines `CUI_TEXT_BIDI`, supplies the private SheenBidi header paths,
and compiles text, analyzer and SheenBidi in separate generated translation units.
`SB_CONFIG_UNITY` belongs only to the vendor unit. That unit applies the exact four
existing canonical vendor warning exceptions with a diagnostic push/pop:
unused parameter/value/function and implicit fallthrough. CUI source warnings
remain errors. Rust native packaging enables bidi in both staged SDK and canonical
fallback builds; its static archive link order includes `cui_bidi` after `cui_text`.

Before this change the SDK native build compiled only the legacy FreeType/HarfBuzz
path. `cui_font_text_geometry_line` returned `UNAVAILABLE`, mixed-paragraph layouts
were unavailable, and contextual colored drawing returned false without emission.
This was a packaging build gap, not a separately documented native feature limit.
Ordinary button smoke tests did not exercise it. The public text API still permits
explicitly disabled bidi builds, with their documented legacy behavior.

Run the comparison with host development packages for SDL3 >=3.2, FreeType,
HarfBuzz, CMake/Ninja and a C11 compiler:

```sh
python tests/packaging/test_sdk_text.py \
  --build-dir /tmp/cui-sdk-text \
  --build-type Release \
  --font /usr/share/fonts/noto/NotoSans-Regular.ttf \
  --rtl-font /usr/share/fonts/noto/NotoSansHebrew-Regular.ttf \
  --bindings
```

CI can substitute equivalent explicit font paths. Choose a primary face covering
Latin but lacking Hebrew and a fallback covering Hebrew, so the fixture actually
requires a second face. All three implementations receive the same files; the
script records their SHA256 values and combined text source fingerprints. It
checks freshly generated SDKs before building and compares canonical, Go SDK and
Rust SDK results. Font names, exact metrics or raster values are not hardcoded;
results must agree within this host. The current fixture specifically exercises
Latin/Hebrew, not all Unicode scripts or every font.

The fixture asserts contextual paragraph-relative byte identity, invalid UTF-8
boundary rejection without output mutation, caret/hit/selection geometry, four
Hebrew selection clusters, actual odd-level RTL and fallback runs, nonzero shaped
glyphs and two colored glyph groups with real coverage pixels. Missing bidi cannot
silently pass: contextual status must be OK and colored emission must succeed.
`--bindings` additionally copies Go/Rust modules into private consumers, builds
native mode, and runs real SDL frames and click callbacks with mixed Latin/Hebrew
button text and an explicit fallback. Only the repository invisible Xvfb runner
launches those consumers; the C font fixture itself opens no display or window.
No package publication or desktop-session fallback occurs.

Validation covers native build fidelity for these text facilities. It does not
claim complete Editor behavior, full native accessibility, all renderer/platform
parity, or self-contained font/system-library distribution. Root integration must
regenerate SDK receipts after canonical Text/SheenBidi/Dock patches and rebuild
published artifacts; copying an old prebuilt bridge retains its old capabilities.
