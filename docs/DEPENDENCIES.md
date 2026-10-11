# Dependencies

Core: C11 compiler + pinned vendored Clay (zlib). Build: CMake >=3.20, Ninja or Make; Python 3 for generation/checks. Optional native gallery: SDL3 >=3.2, FreeType and HarfBuzz via pkg-config; system font or configured font path. Optional UTF-8 processing dependency utf8proc is available but is not automatically a grapheme/bidi implementation. No GPUI/Rust/JavaScript runtime required.
Development host observations: GCC 16.2.1; SDL3 3.4.16; FreeType pkg-config 26.6.20; HarfBuzz 14.5.1; utf8proc 2.11.3. System dependencies are discovered rather than downloaded; source snapshot versions and hashes are tracked in third_party/dependencies.json. Offline core builds need no network. Native CI dependencies must be pinned where downloaded.
