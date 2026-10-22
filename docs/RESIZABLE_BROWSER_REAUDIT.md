# Resizable drag re-audit

Audited source: `5da9acf8bb33344c4f2ad28f5a3870beab7ddb97`.
No production regression was reproduced in a freshly built local WASM gallery.
This records tested behavior on that source, not a claim about every deployed
artifact, native window adapter or complete frozen-family parity.

The earlier wide-layout failure was fixed by `3a81b7ae`: the editor's fixed
600 px maximum plus the explorer's 300 px maximum made wide containers
infeasible. The current story uses a responsive editor maximum. No second fix
is needed for that already-corrected case. The original reproduction and fix
are recorded in `docs/RESIZABLE_DRAG_AUDIT.md`.

| Proof | Actual coverage |
| --- | --- |
| `tests/web/test_resizable_repeated_drag.py` | Desktop DPR 1/2 and CDP touch at 390 px/DPR 2; both axes; six alternating release/regrab drags per axis; both pane sizes and positions and painted rectangles; min/max; Home/End/arrows; disabled input; viewport changes to 500/390/800 px followed by more dragging |
| `tests/web/test_resizable_drag.py` | 24 mouse/touch cases across 390/1280/1920 px, both axes and comfortable/compact density; actual pane/content geometry, disabled state and keyboard resize |
| Release `test_resizable` through `scripts/test_local.py` | Sizing, invalid declarations, constraints, persistence, targeted input, completion callbacks, paint/motion and nested groups |
| WASI `resizable` suite | Same portable unit suite, including nested cross-axis growth and inner capture leaving the parent unchanged; this is not a browser nested-gallery test |

All checks passed. The nested fixture lives in `tests/unit/test_resizable.c`
(`nested_content` and `test_styled`), rather than pretending the two-pane gallery
contains a nested example. Native window/GPU interaction remains unaudited here;
native validation used the private-display wrapper only.

Reproduce with a pinned tool environment, headless browsers and a home-cache
temporary/build directory:

```sh
python scripts/build_web.py --output /path/outside/source/web
uv run --offline tests/web/test_resizable_repeated_drag.py --directory /path/outside/source/web
uv run --offline tests/web/test_resizable_drag.py --directory /path/outside/source/web --output /path/outside/source/report
python scripts/test_local.py --build-dir /path/outside/source/native -- -R '^resizable$' --output-on-failure
uv run --offline scripts/test_wasi.py --build-dir /path/outside/source/wasi --suite resizable
```

The native build must first configure the source and build `test_resizable`.
No dependencies were installed from the network and no desktop windows opened.
