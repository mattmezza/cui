# Optional Unicode regex Input gallery

The Input story's mode 9, **Unicode letters · regex**, uses the compiled optional
provider and anchored pattern `^[\p{L} ]*$`. It accepts Unicode letters and ASCII
spaces, including an empty value, and rejects digits and punctuation through
CUI's existing candidate acceptance. JavaScript does not validate text.

Default browser builds remain C11/Zig builds without Rust or a regex archive:

```sh
python scripts/build_web.py --output build/web
```

`gallery_regex_available()` returns zero and the mode is visibly disabled as
unavailable. The C mode setter also refuses mode 9 when no prepared provider
exists. The other Input modes keep their existing behavior.

To include the real provider, first build the pinned WASI archive using the
optional backend instructions in [INPUT_REGEX.md](INPUT_REGEX.md), then pass it
explicitly:

```sh
python scripts/build_web.py --output build/web-regex \
  --regex-library build/regex-wasi/target/wasm32-wasip1/release/libcui_regex.a
```

The archive must target wasm32-wasip1 and use the pinned backend/toolchain.
`build_web.py` does not invoke Rust or silently download/build a provider. A
missing archive or incompatible link fails the build. This path defines only
the gallery's optional provider seam and exports availability in both builds.

The story prepares its compiled expression and session outside UI frames after
successful gallery setup. Pattern admission is bounded to 64 bytes, candidate
text to 1024 bytes and session cache heap bytes to 1MiB. Failure destroys any
partial session and compiled handle, leaving the option unavailable. Successful
handles occupy stable global storage for the WASM instance lifetime; the matcher
borrows that persistent session, not a stack handle. Candidate acceptance, IME,
Undo and input error behavior are the existing Input/provider contracts.

Selecting this mode produces a complete copyable C11 translation unit, including
zero-initialized caller-owned handles, preparation, persistent-session drawing,
failure cleanup and shutdown cleanup outside a matcher lease. The application
must initialize successfully before drawing and retain its edit/state and
session across frames. The copied unit is a portable API example; link the
optional archive to execute it.

Proofs:

- `tests/test_selected_usage.py`: 270 complete strict C11 units across joined
  Button and Input modes, all 15 design choices and both disabled states.
- `tests/web/test_selected_usage_gallery.py`: actual desktop/mobile clipboard
  equality and strict compilation, including mode 9 when the provider is linked;
  a default build asserts unavailable mode and exercises the other examples.
- `tests/web/test_input_regex_gallery.py`: Wasmtime49 actual gallery initialization,
  typing and rejection with/without the provider (deterministic native-fallback
  metrics), followed by actual linked and default Chromium galleries,
  Unicode typing, rejected punctuation/digits preserving the committed value,
  accepted/rejected provisional IME text, one-step commit Undo/Redo and disabled
  no-archive mode at desktop/mobile widths. It uses finalized CUI text semantics
  and Chromium CDP events rather than a JavaScript acceptance substitute.

Run the browser fixtures with their pinned Playwright environment, passing
`--directory build/web-regex`, `--default-directory build/web`, and an optional
`--chromium` path to the regex interaction fixture. The clipboard fixture takes
`--directory` and `--chromium`.

This demonstrates one real optional provider in the gallery. It does not complete
Input or native provider/platform parity; backend packaging and general matcher
contracts remain documented in the separate regex guide.
