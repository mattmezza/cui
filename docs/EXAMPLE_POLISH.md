# HoverCard, TitleBar and Toolbar examples

HoverCard's default height now fits its children instead of allocating a fixed
120-pixel panel. Positive explicit heights retain their existing fixed behavior
(including viewport bounding). Intrinsic cards keep a fixed-width, non-scrolling
inner content box so text wraps at the card width even though the outer panel
clips overflow. `max_height` optionally caps an intrinsic card; otherwise the
viewport is the cap. This does not add scrolling inside a tall card.

Collision placement uses context-owned retained measurements of the unclipped
inner content, so a temporary clip cannot lock the card to the wrong side of
its trigger. First opening,
width/theme/max-height changes and `content_revision` changes schedule a redraw
to settle placement using the actual height. Each measurement frame is bounded
by the remaining viewport extent; near an edge, placement may take subsequent
measurement redraws to settle. Stable content stops requesting those redraws.
Increment `content_revision` when changing arbitrary children at the same width
so a changed height also receives collision-placement redraws. No caller-owned
cleanup is required: the context retains only measurements, not the hover state,
child callback or child data. Closing/omitting the card allows its measurement
resource to be reclaimed by the ordinary context resource lifecycle.

The gallery's two-line profile card now uses intrinsic height. Headless tests
verify its actual height, longer-content reflow, narrower-width reflow, bounded
top/bottom placement, explicit height, optional height cap and closed-card
lifetime. The new default-height assertion fails against the original component.

The TitleBar gallery now overlays three colored-circle controls in the space
reserved by native traffic-light mode. They are explicitly a **browser
simulation**, with real semantic buttons that invoke the gallery's simulated
window host. Minimize, maximize/restore, close and the optional close callback
update a visible preview status. They respect the gallery's disabled switch and
hide in fullscreen with an explanatory message. The public TitleBar's native
mode still reserves OS controls and does not draw or claim to implement them.
No native host/backend or public Chrome policy changed.

Toolbar Find now searches a visible three-line document using the actual editor
query. Matching lines are highlighted and prefixed with `Match:`, with a matching
line count, an empty-query prompt and explicit no-results state. The bounded
document uses ASCII case-insensitive substring matching; this is a focused
example, not a general Unicode search engine. Ctrl/Cmd+F focuses Find while the
preview has focus, yielding to composition/preferred character input and to
disabled/hidden Find. The ordinary Input result reports query changes; existing
`gallery_toolbar_activations(4)` now exposes that count for the example regression
without changing fields 0–3. Disabled input cannot change the query.

## Validation and integration

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
python3 scripts/build_web.py --output build/web
uv run tests/web/test_example_polish.py --directory build/web --chromium /path/to/chrome
```

Strict-warning native/WASM builds pass. Full native CTest passes 93/94; the only
failure is the expected stale generated Content distribution header, which the
integrator owns. ASAN/UBSAN Content and Chrome tests pass. The dedicated headless
browser regression covers measured HoverCard height, actual colored-circle
canvas pixels, simulated host
actions and close override, fullscreen hiding, real keyboard Find text input,
query-change notifications, highlighted results/no-results and disabled input.
It uses pinned Playwright 1.58.0 and an installed Chromium fixture.

The integrator must regenerate distribution headers and update the existing
broad `test_browser.py` assertion that native TitleBar has zero gallery buttons:
the simulation has three outside fullscreen and zero inside fullscreen. No
CMake, global documentation, publishing, Alert, Bubble or asset changes are part
of this patch.
