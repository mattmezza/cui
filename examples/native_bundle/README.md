# CUI native source bundle

From the extracted bundle directory:

```sh
make
./build/native_starter
```

The consumer build needs a C11 compiler (`cc`), GNU make, `ar`, and `pkg-config`
with SDL3 >= 3.2, FreeType, and HarfBuzz development packages. It needs no Python,
CMake, Rust, network access, or installed CUI library. Python and CMake belong to
the repository's producer and validation workflow.

Check system prerequisites separately from bundle completeness:

```sh
pkg-config --atleast-version=3.2 sdl3
pkg-config --modversion sdl3 freetype2 harfbuzz
```

SDL2 cannot replace SDL3. Install development packages through your system's
package manager or explicitly build upstream releases before running `make`.
The build performs no downloads. These are dynamically linked system libraries;
shipping this source bundle is separate from deploying a binary and its libraries.

Each CUI source compiles separately into `build/vendor/`, then into static CUI
archives. Editing `main.c` rebuilds `build/main.o` and relinks the executable;
ordinary app edits leave CUI objects alone. `.d` files track header dependencies.
Use `make CC=clang` to select another compiler in a fresh extraction. If changing
compiler or flags after building, use a fresh extraction or remove your build
directory explicitly. `sources.mk` is generated from the repository's authoritative
source metadata. No implementation macros are required in consumer code.

The starter loads bundled Inter 4.1 Regular and Bold before creating its layout.
Unmodified official static OTF files and their SIL Open Font License are in
`assets/fonts/Inter-Regular.otf`, `assets/fonts/Inter-Bold.otf`, and
`assets/fonts/LICENSE.txt`. Keep the license when redistributing the fonts.
Defaults resolve relative to the executable (`build/../assets/fonts`), so launching
from another working directory works. Keep that directory relationship when moving
the app. Override each face explicitly if needed:

```sh
./build/native_starter --font /path/regular.otf --bold-font /path/bold.otf
./build/native_starter --frames 3 --capture frame.bmp
```

Only regular (400) and bold (700) are bundled; CUI does not synthesize other
weights or italics. Inter's script coverage is finite: this pair supplies neither
universal Unicode coverage nor automatic font fallback. The archive retains
font provenance and SHA-256 pins in `assets/fonts/manifest.json`, upstream
dependency records in `upstream-dependencies.json`, and checksums for every payload
file in `bundle-manifest.json` (excluding that checksum manifest itself).

`--frames` accepts positive decimal integers through `UINT_MAX`; only successful
renders count. A bounded run fails if the window closes early, layout/rendering
fails, a font is missing, or capture fails. `--capture` requires `--frames` and
captures the final frame before presentation. Capture paths are relative to the
working directory. Without `--frames`, close the window to exit. The button toggles
a greeting and demonstrates ordinary pointer/keyboard component interaction.

Linux X11 is the supported validated native target. Other platforms and display
servers are unverified. Native automated tests must use the repository's invisible
Xvfb wrapper (`python scripts/test_local.py --build-dir build`), never the desktop
display. Browser tests run headless; virtual-display results do not establish GPU
hardware coverage.

The dependency map is:

| Layer | Dependencies and purpose |
| --- | --- |
| Portable CUI | Clay layout, NanoSVG SVG rasterization, generated icon/theme tables, generated Unicode properties |
| SDL3 adapter | SDL3 host window, input, clipboard, cursors, and renderer |
| Native text adapter | FreeType font loading/rasterization and HarfBuzz shaping |
| Bidirectional text | Vendored SheenBidi, compiled as C with the bundle |
| Starter assets | Inter 4.1 static Regular/Bold OTF files, SIL Open Font License |

Sokol, SDL_GPU, the Rust regex provider, and native accessibility integrations
are outside the default build. Public headers may declare optional interfaces;
their presence does not mean those optional implementations are linked. All 78
component manifest entries are retained in `component-manifest.json`, including
their incomplete behavior and validation status.

Application layout and component calls use public CUI APIs. The starter uses
SDL's executable-path helper only to locate its bundled assets. Public CUI
interfaces contain no SDL or Clay types. This bundle demonstrates a minimal
native integration; it does not claim complete platform or component parity.
