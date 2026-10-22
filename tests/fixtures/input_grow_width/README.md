# Input grow-width containment fixture

Build only the focused fixture and existing Input/InputGroup regressions. All
native tests run through the private-display wrapper; no dependency installation.

```sh
cmake -S tests/fixtures/input_grow_width -B build-input-grow -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-input-grow --target test_input_grow_width test_input test_input_group_events
python scripts/test_local.py --build-dir build-input-grow -R '^(input_grow_width|input|input_group_events)$' --output-on-failure
```

Repeat in a separate Debug build with `-DCUI_SANITIZE=ON` for ASan/UBSan.

The distinct same-frame caret/scroll resize defect is **not fixed** by this slice.
To register the opt-in requirement probe, configure a separate build with
`-DCUI_INPUT_GROW_CARET_PROBE=ON`, build `test_input_grow_width`, and run the wrapper
with `-R '^input_grow_caret_probe$' --output-on-failure`. That probe currently
returns failure on 400→220 and 220→500 transitions. It is not marked expected-pass
or WILL_FAIL and is omitted from the ordinary passing suite. The 500→500 frame
shows the existing next-frame convergence. No browser proof is claimed here.
