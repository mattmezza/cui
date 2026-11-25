# Input grow-width containment fixture

Build only the focused fixture and existing Input/InputGroup regressions. All
native tests run through the private-display wrapper; no dependency installation.

```sh
cmake -S tests/fixtures/input_grow_width -B build-input-grow -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-input-grow --target test_input_grow_width test_input test_input_group_events
python scripts/test_local.py --build-dir build-input-grow -R '^(input_grow_width|input|input_group_events)$' --output-on-failure
```

Repeat in a separate Debug build with `-DCUI_SANITIZE=ON` for ASan/UBSan.

The ordinary `input_grow_width` test requires the caret, selection and IME
anchor to follow each of the 400→220→500→180 frames. It also covers centered
and trailing placeholders, scroll recovery at capacity, and pointer hits in a
resized Input and InputGroup. The previous opt-in probe is part of this test.
