# Touch follow-up audit

Baseline: `fda9f70` on `integration/next16`, following the original 78-family
[coverage inventory](TOUCH_COMPONENT_COVERAGE.md). This follow-up adds actual
Chromium CDP touch contacts for ten interactive families. It changes no component,
gallery, Core or adapter source. It does not establish complete family parity.

Build the gallery with the pinned Zig toolchain, then run:

```sh
uv run tests/web/test_touch_more.py --chromium /path/to/chromium
```

The test imports the existing `Audit` helper from `test_touch_components.py`.
Both scripts must remain adjacent. Its Playwright dependency is pinned. The
default command returns failure when an assertion fails. `--allow-defects` is an
evidence-collection mode that returns success despite failed assertions; inspect
the printed results and JSON report. `--report PATH` overrides the default
`/tmp/cui-touch-followup-report.json`. `--inventory-only` records semantics without
running gestures and must not be interpreted as interaction verification.

All asserted component actions use `Input.dispatchTouchEvent`, producing browser
PointerEvents with actual contact identities. No component action substitutes a
mouse click, accessibility DOM click or direct `gallery_event` call. DOM options
only configure fixtures. Combobox receives ordinary keyboard text to prepare its
query; focusing, suggestion selection and outside dismissal use touch. Clipboard
permissions are granted to the browser context, and the test reads the real
`navigator.clipboard` contents after a touch copy action. It never stubs the host
clipboard provider.

## Results

The follow-up passed 15 of 16 cases. One controlled ToggleGroup semantic assertion
failed. These results cover representative gallery behaviors only.

| Family | Status | Assertions and limits |
| --- | --- | --- |
| DropdownButton | Verified | Touch primary action increments invocation count; split menu selects a real item and closes; outside touch dismisses; disabled primary and menu triggers do neither. Submenus remain unverified. |
| Combobox | Verified | Touch focuses the query, selects the matching Banana option, closes the popup, and exposes Banana as selected when the query reopens it. Outside dismissal and disabled touch are asserted. Query setup uses keyboard text. Multi-selection/chip removal and touch text entry remain unverified. |
| Command | Verified | Enabled row confirms and invokes its callback; disabled row does not confirm. A backtracking row drag causes no ghost confirmation. Touch opens the modal palette, confirms a row, and dismisses outside. Filtering and touch text entry remain unverified. |
| Calendar | Verified | Touch selects a date; disabled touch on another date preserves the first selection. Month/year navigation, ranges and disabled-date constraints remain unverified. |
| DatePicker | Verified | Touch opens the calendar, selects a date and closes with updated trigger text; outside touch dismisses; disabled trigger stays closed. Range mode remains unverified. |
| ColorPicker | Verified | Touch Red-channel drag changes RGBA; disabled drag preserves RGBA. Palette touch selects exact `0x5865F2FF`; a two-dimensional saturation/value drag changes color. Screen EyeDropper and other color models remain unverified. |
| Clipboard | Verified | Trusted touch copy writes `Hello from CUI` into the actual clipboard; touch read displays that content. Disabled copy preserves a distinct sentinel. Platform permission-denial paths remain unverified. |
| Toggle / ToggleGroup | Partial; defect | Standalone toggles twice and ignores disabled tap. Multiple selection retains Bold and Italic; disabled group preserves selection. Single selection updates the model correctly but leaves incorrect checked semantics, as detailed below. |
| Tree | Verified | Touch expands docs and exposes children, selects core.c, ignores a disabled base.c selection, and collapses src to remove its children. Overflow scrolling and custom row slots remain unverified. |
| Focus Trap | Verified | Touch opens the scope, focuses its first child, closes it, and restores opener focus. Keyboard containment is outside this touch assertion; nested traps remain unverified. |

Combobox query text and selected option are independent in the current API. The
test checks the selected option rather than requiring the query to be replaced
with its label. Likewise, the Command gallery's confirmation callback deliberately
keeps its surface open; the test checks confirmation and a separate outside
dismissal rather than inventing automatic close behavior.

## Confirmed defect

With the Toggle gallery in single-selection mode, Bold initially selected, touch
Italic and then idle for 150ms after the normal post-release wait. The controlled
model reports `[Bold=0, Italic=1, Underline=0]`, while checked semantics report
`[1, 1, 0]`. The selected values are therefore correct but their exposed state is
wrong. The failing case is retained in the default test command.

The gallery callback replaces sibling values while the group is being rendered.
An earlier sibling may already have been painted or registered before that
callback changes it. This is a source-review hypothesis for the component/gallery
owner; this audit does not claim a fix or claim that all controlled groups behave
this way. A fix must make rendered and semantic states agree after the callback
without requiring a later unrelated input event.

## Integrated correction

The integrator fixes controlled sibling semantics and requests repaint without replaying callbacks (`2ad721c`). The integrated Chromium follow-up passes all 16 assertions at source `57f8066`; the original baseline defect above remains historical evidence.

## IME scope and remaining work

The separate [IME documentation](IME.md) and `test_ime.py` explicitly cover four
consumers: Input, Textarea, Editor and Questionnaire input controls. This follow-up
does not run composition assertions. Ordinary keyboard preparation of a Combobox
query is not IME verification. NumberInput, OtpInput and Combobox composition,
preedit rendering, commit/cancel and undo remain unverified here; this audit makes
no claim that every textbox has those capabilities.

All gestures in this follow-up run in headless Chromium. Physical touchscreen
hardware, Safari/Firefox, native SDL, all variants, focus transfer during active
composition, permission-denied clipboard access and every remaining interactive
family are still outside its verified scope. Nested overflowing scroll boundary
chaining remains the gallery coverage gap described in the original inventory.

Validation: warning-enabled CMake/Ninja build (`-Wall -Wextra -Wpedantic -Werror`)
passed; local CTest passed 90/90. The new CDP suite reports its known failing
ToggleGroup assertion independently of those existing native tests.
