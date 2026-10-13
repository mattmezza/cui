# Gallery touch coverage

Audit baseline: release `65da7b2`. The final run passed 41 of 42 assertions across 36 families; Carousel failed its swipe assertion. All 78 stories rendered without page errors. This inventory covers all 78 manifest families. Rendering a story is an inventory check, not touch verification. “Verified” means only the named representative assertions passed using actual Chromium CDP `Input.dispatchTouchEvent` contacts. It does not mean every variant, platform or interaction in that family passed.

Run from the repository root after building the WASI gallery:

```sh
python scripts/build_web.py --zig /path/to/pinned/zig
uv run tests/web/test_touch_components.py --chromium /path/to/chromium
```

The default command fails on a failed interaction. `--allow-defects` continues and returns success for evidence collection; inspect the JSON report and printed failures before assessing readiness. `--report PATH` saves semantic inventory and assertion results. `--inventory-only` only renders the 78 stories. Dependencies are pinned in the test script.

Configuration inputs create fixtures but do not perform the asserted component gestures. Textarea uses one row to produce overflow. Editor receives keyboard Enter events to create overflow before its touch pan assertion; this does not verify touch typing or IME. Component taps, drags, pans and pinch use CDP contacts, never mouse emulation or direct Core event injection. Disabled Button, List option and Resizable assertions verify unchanged state. Button backtracking drag and nested Dialog checkbox drag verify ghost-tap suppression. Dock moves a real tab between workspace regions. VirtualList asserts a changed bounded row window; DataTable tests both pixel scrolling and visible grid-cell selection.

## Matrix

Applicability describes the default gallery fixture. “Conditional” means a display or container family can have interactive options or children; those paths still need coverage. “Not applicable” means no intrinsic touch action is exposed in this fixture, not that the family was verified to pan its ancestors. Status `Unverified` always remains explicit for those rows.

| Family | Applicability | Status | Assertions or remaining scope |
| --- | --- | --- | --- |
| Accordion | Applicable | Verified | tap expands Configuration. |
| AlertDialog | Applicable | Verified | touch opens and dismisses overlay. |
| Alert | Not applicable | Unverified | Default fixture has no intrinsic touch action. |
| Attachment | Conditional | Unverified | Default fixture displays content; optional actions/interactive children unverified. |
| Avatar | Conditional | Unverified | Default fixture displays content; optional actions/interactive children unverified. |
| Badge | Conditional | Unverified | Default fixture displays content; optional actions/interactive children unverified. |
| Bubble | Conditional | Unverified | Default fixture displays content; optional actions/interactive children unverified. |
| Button | Applicable | Verified | tap; backtracking drag suppresses ghost tap; disabled tap. |
| Checkbox | Applicable | Verified | two taps toggle checked state. |
| Clipboard | Applicable | Unverified | Copy action and platform clipboard unverified. |
| Collapsible | Applicable | Verified | tap expands Show details. |
| DropdownButton | Applicable | Unverified | Primary/split popup actions unverified. |
| Empty | Not applicable | Unverified | Default fixture has no intrinsic touch action. |
| Focus Trap | Applicable | Unverified | Touch focus containment unverified. |
| GroupBox | Conditional | Unverified | Default fixture displays content; optional actions/interactive children unverified. |
| HoverCard | Applicable | Verified | touch focus reveals profile card. |
| Icon | Conditional | Unverified | Default fixture displays content; optional actions/interactive children unverified. |
| Image | Conditional | Unverified | Default fixture displays content; optional actions/interactive children unverified. |
| Kbd | Not applicable | Unverified | Default fixture has no intrinsic touch action. |
| Label | Not applicable | Unverified | Default fixture has no intrinsic touch action. |
| Marker | Not applicable | Unverified | Default fixture has no intrinsic touch action. |
| Message | Conditional | Unverified | Default fixture displays content; optional actions/interactive children unverified. |
| MessageScroller | Applicable | Verified | vertical pan consumes logical pixels. |
| Notification | Conditional | Unverified | Default fixture displays content; optional actions/interactive children unverified. |
| Pagination | Applicable | Verified | touch advances current page. |
| Progress | Not applicable | Unverified | Default fixture has no intrinsic touch action. |
| Radio | Applicable | Verified | tap selects Priority. |
| Rating | Applicable | Verified | touch selects five-star rating. |
| Shimmer | Not applicable | Unverified | Default fixture has no intrinsic touch action. |
| Skeleton | Not applicable | Unverified | Default fixture has no intrinsic touch action. |
| Spinner | Not applicable | Unverified | Default fixture has no intrinsic touch action. |
| Stepper | Applicable | Verified | touch selects Review step. |
| Switch | Applicable | Verified | two taps toggle checked state. |
| Tag | Conditional | Unverified | Default fixture displays content; optional actions/interactive children unverified. |
| Toggle | Applicable | Verified | two taps toggle pressed state. |
| Tooltip | Applicable | Verified | touch focus reveals tooltip content. |
| Calendar | Applicable | Verified | touch selects calendar day. |
| ColorPicker | Applicable | Unverified | Swatch and continuous color drag unverified. |
| Combobox | Applicable | Unverified | Touch suggestion selection unverified. |
| DatePicker | Applicable | Unverified | Calendar popup selection unverified. |
| Editor | Applicable | Verified | vertical pan consumes pixels after keyboard-created overflow. |
| Form | Applicable | Unverified | Submit and touch validation flow unverified. |
| Input | Applicable | Verified | touch focuses editable control (text/IME not asserted). |
| Input Group | Applicable | Unverified | Grouped action/focus flow unverified. |
| NumberInput | Applicable | Verified | touch focuses editable control (text/IME not asserted). |
| OtpInput | Applicable | Verified | touch focuses editable control (text/IME not asserted). |
| Questionnaire | Applicable | Unverified | Question transitions and answers unverified. |
| Select | Applicable | Verified | touch opens popup, selects Priority, closes popup. |
| Slider | Applicable | Verified | horizontal touch drag changes value. |
| Speech | Applicable | Unverified | Speech input/device permission flow unverified. |
| Textarea | Applicable | Verified | vertical pan consumes pixels in overflowing one-row fixture. |
| TimeField | Applicable | Verified | touch focuses editable control (text/IME not asserted). |
| Carousel | Applicable | Unverified | FAILED: slow horizontal swipe leaves selected page unchanged; see defect below. |
| DescriptionList | Not applicable | Unverified | Default fixture has no intrinsic touch action. |
| Dialog | Applicable | Verified | touch opens and dismisses overlay; nested modal blocks parent scrolling and preserves drag-vs-tap. |
| Dock | Applicable | Verified | touch drags tab from center into right edge. |
| List | Applicable | Verified | touch selects Priority option; disabled option preserves selection. |
| Menu | Applicable | Verified | touch opens menu and invokes option. |
| Popover | Applicable | Verified | touch opens and dismisses overlay. |
| Resizable | Applicable | Verified | touch resizes panel; disabled resize remains locked. |
| Root View | Conditional | Unverified | Default fixture displays content; optional actions/interactive children unverified. |
| Scrollable | Applicable | Verified | vertical pan consumes logical pixels. |
| Settings | Applicable | Unverified | Navigation and compound control/scroll behavior unverified. |
| Sheet | Applicable | Verified | touch opens and dismisses overlay. |
| Sidebar | Applicable | Verified | vertical pan consumes logical pixels; touch selects Files. |
| StatusBar | Conditional | Unverified | Default fixture displays content; optional actions/interactive children unverified. |
| Tabs | Applicable | Verified | tap selects Activity. |
| Theme | Not applicable | Unverified | Default fixture has no intrinsic touch action. |
| TitleBar | Conditional | Unverified | Default fixture displays content; optional actions/interactive children unverified. |
| Toolbar | Applicable | Unverified | Grouped action/overflow behavior unverified. |
| Tree | Applicable | Unverified | Touch selection/expand and scroll unverified. |
| VirtualList | Applicable | Verified | vertical pan consumes logical pixels; touch changes bounded visible row window. |
| Chart | Not applicable | Unverified | Default fixture has no intrinsic touch action. |
| Command | Applicable | Unverified | Command selection and confirmation unverified. |
| DataTable | Applicable | Verified | vertical pan consumes logical pixels; touch selects actual visible grid cell. |
| Plot | Applicable | Verified | two actual touch contacts zoom opted-in viewport. |
| Table | Not applicable | Unverified | Default fixture has no intrinsic touch action. |
| TextView | Applicable | Unverified | Text selection, links and enabled scrolling unverified. |

## Confirmed defect

**Carousel slow swipe fails to advance.** On the default horizontal `Workflow carousel`, the viewport is 716 logical pixels wide. Start in its content center, send four touch moves totaling 65% of its width to the left, with 50ms between moves, then release. `gallery_carousel_stats(0)` remains at page 0 instead of advancing. This happens with actual PointerEvents generated from CDP contacts. The test preserves this failed assertion rather than changing the source under audit.

The root is a horizontal scroll owner. Core translates touch pan into precise scroll events. Carousel currently treats a gap of 28ms as a wheel-burst end and snaps a partial move before the next touch move. This is a source-based explanation to investigate, not a verified fix. Relevant owners should preserve the continuous contact lifecycle or grant explicit drag ownership; normal slow finger movement must not require sub-28ms events.

## Remaining coverage

Nested **modal** ownership is verified: dragging in the child modal neither scrolls the obscured parent nor toggles its checkbox; a later tap toggles the child checkbox. Nested **scroll boundary chaining** is unverified in this gallery audit: no default fixture has two simultaneously overflowing scroll owners. Core headless nested-chain tests are separate evidence and do not establish gallery/component coverage. Add an actual nested overflow fixture before marking this behavior verified here.

These assertions run in headless Chromium with emulated touchscreen contacts. Physical Android/iOS hardware, Safari, Firefox, native SDL, multiwindow routing, pen contacts, custom cursors, every disabled variant, cancellation during unmount and all 78 family variants remain unverified by this suite. Public shared cursor/state hooks and prior Core/SDL tests are documented in [INTERACTION.md](INTERACTION.md); they are not a substitute for this component inventory. Input focus assertions do not establish soft-keyboard, composition, text-selection or IME behavior. A display family marked not applicable still depends on its containing scroll/viewport owner for gestures.

Validation for this audit branch: warning-enabled CMake/Ninja build passed; local CTest passed 88/88. No source behavior or existing test files were changed.
