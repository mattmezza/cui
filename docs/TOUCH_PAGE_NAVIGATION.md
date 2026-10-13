# Mobile page touch navigation

The browser host now lets a single vertical contact navigate the document when
Core and its component reducers leave the movement unconsumed. This follows the
`3c8879ef` read-only audit (based on `3c992ec`), which passed only 3 of 10 navigation assertions. The implementation
branch starts at `d56820e`; its strict mobile suite passes 19/19 assertions.

This is headless Chromium with a 390 × 844 mobile viewport and actual
`Input.dispatchTouchEvent` contacts. It does not establish physical Android/iOS,
Safari or Firefox coverage. All 78 manifest families remain present.

After building the gallery with the pinned Zig toolchain, run:

```sh
uv run tests/web/test_touch_page_navigation.py --chromium /path/to/chromium
```

The script imports the adjacent `test_touch_components.py` helper and pins
Playwright. The default command fails on any assertion failure. `--allow-defects`
is evidence-collection mode and must not be treated as a passing result. The JSON
report defaults to `/tmp/cui-touch-page-navigation-report.json`; override it with
`--report PATH`.

## Ownership and lifecycle

Canvas retains `touch-action:none` so the browser cannot steal intrinsic control
drags, internal scrolling or opted-in viewport gestures. The private bounded
[host policy helper](../web/touch_page_host.h) examines actual Core dispatch after
component reducers. It uses Core targets, consumption, clipped hover/scope queries,
and completed semantic parents; JavaScript does not repeat hit testing.

The host gives Core first refusal. Consumed moves stay entirely with Core. A
single contact can claim page navigation only after eight pixels of dominant
vertical movement and an unconsumed move. Modal scopes deny fallback, including
hits on disabled modal content. Slider, Scrollbar, Separator, explicit drag
priority and opted-in gesture owners reserve their streams. Protected ownership
is retained from the initial down event. Completed semantic parents protect
custom drag ancestors as well as the direct target.

An unconsumed scroll can retain a nonzero Core target at a boundary: persistent
ownership and failed ancestor deferral do not necessarily clear it. The helper
therefore uses the reducers' final consumption rather than requiring target zero.
If a movement is partially consumed at a boundary, its entire delta stays with
Core; page handoff waits for a later unconsumed move. No residual delta is guessed.

On page claim the browser queues exactly one Core touch cancellation, clearing
pressed/captured component state. It retains browser pointer capture until the
contact ends and applies later page movement from client coordinates. It does
not feed the ended Core contact further moves or releases. A second contact
blocks page movement until the stream ends; a new finger arriving after page
claim is also kept out of Core to avoid becoming a fresh primary tap. A later
single-contact gesture starts normally. Cancellation, window blur and navigation
clear browser host contacts. Both C and JavaScript retain at most 16 contacts.

The helper retires raw pointer releases and Core-converted `SCROLL/ENDED` and
`GESTURE/ENDED` releases. Missing those converted endings would falsely mark
later gestures as multiple contacts. Synthetic capture-handoff cancellation is
kept separate from physical contact termination.

## Wrapper padding

The audit separately reproduced a trap in canvas wrapper padding. Changing only
`.canvas-wrap` from `overflow:auto` to `overflow:clip` restored native vertical
page scrolling there. Canvas remains responsive at 100% width; horizontal
Scrollable/VirtualList behavior remains inside Core. Code blocks retain their
separate `overflow:auto` behavior. The full browser suite, including 390px layout,
passed with this narrow change. Preview-card styling is unchanged.

## Verified assertions

| Beginning or owner of gesture | Verified result |
| --- | --- |
| Body margin and canvas wrapper padding | Native vertical document pan works. |
| Passive Avatar, Image and Label canvas | Vertical document pan works. Label is the typography-content representative; no manifest family was added. |
| Button drag without internal owner | Navigates page without activating the button. |
| Button backtracking drag, then actual tap | Drag produces no ghost activation; the next tap activates exactly once. |
| Scrollable with remaining range | Consumes exactly 110 logical pixels while the page stays fixed. |
| Scrollable bottom and top boundaries | Internal offset stays at 458 or zero; unconsumed vertical pan navigates the page. |
| Slider and Resizable | Relevant control values change while page position stays fixed. |
| ColorPicker saturation/value plane | Two-dimensional drag changes RGBA without moving the page. |
| Plot | Opted-in pan and actual two-contact pinch remain with the viewport; pinch changes its domain. |
| Dialog with disabled opener | Touch on modal content cannot navigate the document; close still works. |
| Multiple contacts | Do not navigate the page; a later single contact can navigate. |
| Second finger after page claim | Cannot ghost-activate a button; a later ordinary tap works. |
| Physical cancellation after page claim | Stops movement; Core cancellation count increases exactly once and later taps work. |

The new [headless C test](../tests/unit/test_touch_page_host.c) uses real Core
scenes to exercise passive eligibility, direct/ancestor drag protection, viewport
protection, modal denial including disabled content, consumed-move refusal,
boundary routing, converted terminal-event retirement, multitouch recovery,
cancellation and bounded contact capacity. The integrator registers it in the
native and WASI suites.

## Validation and limits

Warning-enabled native CMake/Ninja build passed. The new C test separately
compiled with `-std=c11 -Wall -Wextra -Wpedantic -Werror` and passed. Existing full
CTest ran 90/91: only the generated-distribution check failed because root-owned
distributions await regeneration; the earlier RTL render regression is absent
at this implementation baseline.

Real CDP suites passed: page navigation 19/19, original touch smoke, component
coverage 42/42, follow-up coverage 16/16, and full browser coverage of all 78
stories, mobile 390px layout and `/cui/` hosting. These are representative tested
behaviors, not complete family or device parity.

Canvas page fallback follows finger movement and does not synthesize fling
inertia. Physical mobile browsers, soft-keyboard/IME navigation, browser zoom,
all custom gesture combinations and every component variant remain unverified.
The partial-boundary-consumption limitation above is deliberate.

Unexpected browser pointer capture loss clears the host contact and cancels Core
once. A real CDP contact with `releasePointerCapture` verifies ghost suppression
and a usable subsequent tap. Normal release after pointer-up finds no contact.
