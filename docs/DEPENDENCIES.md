# Dependencies

Core: C11 compiler + pinned vendored Clay (zlib). Build: CMake >=3.20, Ninja or Make; Python 3 for generation/checks. Optional native gallery: SDL3 >=3.2, FreeType and HarfBuzz via pkg-config; system font or configured font path. Optional UTF-8 processing dependency utf8proc is available but is not automatically a grapheme/bidi implementation. No GPUI/Rust/JavaScript runtime required.
Development host observations: GCC 16.2.1; SDL3 3.4.16; FreeType pkg-config 26.6.20; HarfBuzz 14.5.1; utf8proc 2.11.3. System dependencies are discovered rather than downloaded; source snapshot versions and hashes are tracked in third_party/dependencies.json. Offline core builds need no network. Native CI dependencies must be pinned where downloaded.

The optional website documentation build uses `uv run scripts/build_docs.py`,
with PEP 723 pins for markdown-it-py 4.0.0 (MIT) and mdurl 0.1.2 (MIT).
They run only during generation; published pages contain rendered HTML and
require no Markdown runtime or Python environment. Raw Markdown HTML is escaped.
Run `uv run tests/test_docs.py` to check rendering, links, assets and document coverage.

Optional bidi analysis uses vendored SheenBidi 3.0.0, commit `cfe430e7375a7845b679adae9d51dac6deaa8858`, Apache-2.0, Unicode 17. All vendored files and normative conformance fixtures have checked SHA256 pins. `CUI_BUILD_BIDI=OFF` omits the compiled `cui_bidi` library. Core never links it implicitly. The optional static library includes its backend objects, so `pkg-config --libs cui-bidi` needs no additional runtime installation. CUI wrapper/tests use the normal warning policy; four specific unused/fallthrough warnings are suppressed only for unmodified upstream. See [BIDI.md](BIDI.md) for current analysis and integration limits.
