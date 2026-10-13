# Remaining scroll and scrollbar audit, next27

This is a read-only source audit of CUI `3af85cb` against pinned gpui-component v0.7.1 commit `87d10ae5e1299d1be18670a37570c5402f952e34` at `/tmp/cui-gpui-reference`. It changes no source, manifest status or family certification. Reference behavior is primarily `crates/base/src/scrollbar.rs` and `crates/component/src/scroll/scrollable.rs`; similarly named shell wrappers are not treated as proof of the primary API.

The user's broad request is usable touch interaction throughout components, appropriate pan/scroll behavior and configurable pointers. Specific upstream animation/mode/style APIs are parity requirements, not evidence that the user separately requested every Rust API. Browser Shift-wheel usability is an adapter consistency concern; upstream axis locking alone does not establish a portable Shift-wheel contract.

## Evidence inventory

### shared routing

Status: partial tested. Evidence: `src/base/cui_scroll.c:201–223; tests/unit/test_scroll.c`.

Precise versus line steps, actual-change consumption, boundary deferral, axis lock and horizontal trapping exist. Existing tests assert consumed/refused events and nested touch routing. These do not certify every owner.

### VirtualList

Status: partial tested. Evidence: `src/base/cui_virtual_scroll.c:40–75; tests/unit/test_virtual_styled.c`.

Uses common scroll reduction and adds keyboard navigation. Windowing/model tests are separate from physical touch evidence.

### Sidebar

Status: partial tested. Evidence: `src/components/cui_sidebar.c:366–425; tests/unit/test_sidebar.c:252`.

Uses shared geometry, skin and scroll state with clipped translated content and cached rows. Existing precise-scroll fixture is not a physical native-device test.

### Tabs consumption

Status: source gap; runtime reproduction pending. Evidence: `src/base/cui_choice.c:340–366; tests/unit/test_choice.c:2322`.

Wheel/converted touch SCROLL changes offset without cui_consume_event or boundary deferral. Thumb reducer separately consumes keyboard events. Existing scrollbar tests cover drag, keyboard, hidden/style geometry; they do not establish nested SCROLL ownership.

### DataTable consumption

Status: source gap; runtime reproduction pending. Evidence: `src/base/cui_table.c:892–903; tests/unit/test_table.c`.

Body/root scroll offsets are changed in bespoke loops without consumption/deferral. Common bars exist, but their presence does not prove these loops chain correctly. Disabled/loading gates and final clamp exist.

### shift wheel

Status: browser supported; native gap. Evidence: `web/gallery.js:778–790; backends/sdl3/cui_sdl3.c:245–251`.

Browser converts shift+vertical delta to horizontal only when deltaX is zero. SDL wheel branch forwards x/y without shift mapping or wheel modifiers. Hosts therefore differ. Upstream lock_scroll_axis is not proof that shift conversion is required by that API.

### visibility modes

Status: upstream gap. Evidence: `include/cui/cui_theme.h:11–24; upstream crates/base/src/scrollbar.rs:48–66`.

Visible/Hidden/AutoHide/Inherited exist. Upstream Always/Scrolling/Hover includes distinct hover-only mode absent here. AUTO_HIDE also wakes on owner hover and holds focused descendants; do not equate it exactly with upstream Scrolling.

### idle scheduling

Status: upstream efficiency gap. Evidence: `src/base/cui_scroll.c:34–75; tests/unit/test_scroll.c:642–671; upstream scrollbar.rs:1344–1365,2165`.

CUI invalidates continuously through idle and fade; existing tests deliberately assert redraw during idle. Upstream requests animation only while moving and schedules a delayed idle notification. Changing this needs a reusable host-visible wake deadline, not simply removing invalidation.

### skin and motion

Status: partial implemented; upstream gap. Evidence: `include/cui/cui_theme.h:20–24; upstream scrollbar.rs:290–372,2330–2340`.

Thickness/minimum thumb/radii/track hiding/custom colors/fade/reduced motion exist. No independent active/hover width, thumb-hover entrance or configurable enter/expand choreography. Upstream drag notification throttling is also not represented by the CUI API.

### touch drag lifetime

Status: partial tested. Evidence: `tests/unit/test_scroll.c; tests/web/test_touch_components.py; upstream scrollbar.rs:1659–1708`.

Core contact routing, explicit drag priority and cancellation are present. Upstream thumb touch drag prevents default/stops propagation. Existing CUI CDP and queued SDL tests are synthetic input evidence; no physical OS/device certification.

## Smallest next implementation

Start with Tabs scroll ownership, before extending scrollbar styling. The current reducer accepts a routed SCROLL and changes offset, but leaves the event unconsumed. This is a concrete source omission relative to the shared reducer's documented contract; the resulting page fallback/double-owner behavior still requires a failing scene. Do not call this a reproduced browser defect yet.

Proposed ownership: only the Tabs scroll loop in `src/base/cui_choice.c`, relevant regressions in `tests/unit/test_choice.c`, and a new `tests/web/test_tabs_scroll_chaining.py`. No public API or Core/host changes are needed for this first task. For each individual event, compare bounded before/after offsets; consume only actual movement. At a boundary, defer through the existing Core scroll-owner path if that owner contract permits it. Preserve explicit horizontal trap/axis policies rather than treating every refusal as page fallback. Registering/painting semantics remains unchanged.

Required failing-before tests: horizontal and vertical Tabs nested in another owner; precise touch-converted and line-wheel deltas; consumed after movement; correct boundary parent movement exactly once; disabled/loading policy; already consumed event ignored; no capture cancellation or ghost activation. A real CDP touch contact must move visible tabs, then hand off at an appropriate boundary; mouse wheel/Shift-wheel should assert actual geometry. If Core routing cannot defer from the current receiving ID, report that precise limitation before adding an API. Audit DataTable's independent scroll loops next, without bundling them into this first patch.

The next styling task can append a `CUI_SCROLLBAR_HOVER` enum value and implement a documented hover-only policy in the shared opacity helper, with current-presence guards and all consumers tested. This is smaller than idle scheduling but requires an explicit decision about focus/drag visibility. Idle quiescence needs an append-only Core deadline query/request contract plus native/browser scheduling and tests; blindly removing idle invalidation would leave a visible bar indefinitely when the host sleeps. Neither is authorized by this audit.

## Boundaries and validation

Retain `POINTER_ENTER`/`POINTER_LEAVE` knownness and presence semantics: unknown first pointer cannot hover; leave preserves coordinates/capture and does not cancel; captured outside motion precedes leave; focus loss explicitly cancels then leaves. Scrollbar focus/drag may keep visibility independently of pointer presence. No sentinel coordinates, automatic cancellation on leave, native window-ID routing, or generic host wheel ownership rewrite is proposed.

Existing tests cited above are inspected evidence, not newly executed feature proofs. The source-identical presence warning build completed cleanly (`ninja: no work to do`); full CTest passed 115/117 in 16.34 seconds. The only failures were the known stale `distribution` check and `checkbox_motion` phantom-initial-hover fixture. Logs: `/home/matteo/.cache/cui-validation/scroll-gap-build.log` and `scroll-gap-ctest.log`. These remain separate from this docs-only audit. Earlier Chromium/CDP and queued SDL/Xvfb runs establish only those controlled scenarios, not physical iOS/Android, trackpad inertia, OS window leave, or native Shift-wheel parity. No full family is certified.
