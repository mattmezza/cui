# Controlled sibling audit

This audit began at base `513883e` (integration/next17). It exercises real
frame/event routing, caller-owned values, completed-frame semantics, and actual
draw commands. The follow-up fixes ListMulti callback forwarding in the base
reducer. The default audit now passes; `--radio` separately preserves the known
Radio regression. CMake registration remains the integrator's responsibility.

## Confirmed defects

1. **Fixed: ListMulti confirmation/cancel callback user pointer.**
   Previously, `cui_base_list_multi` replaced `options.activation_user` with
   its private stack reducer to intercept `on_activate`, while forwarding the
   caller's `on_confirm` and `on_cancel` callbacks unchanged. Both received the
   private reducer pointer instead of the caller's user object.
   All three callbacks now use trampolines that preserve the original user pointer.
   The original defect was at `src/base/cui_choice.c:1006` in the audited base. A callback
   which writes through its expected owner pointer could corrupt the reducer
   or adjacent stack memory. The regression safely compares pointers without
   dereferencing the unexpected pointer. It covers accept and restore/reject
   policies, confirmation order/count, Escape, and idle frames. The registered
   Choice suite additionally writes final selection through the actual owner
   object from confirmation and cancellation callbacks and verifies final
   completed-frame semantics. Disabled paths emit no notifications.

2. **Remaining: Radio publishes stale earlier-sibling semantics.**
   Render radio option0 followed by option1 sharing one `int *`, initially 0;
   click option1. The owner value becomes 1, but the completed frame reports both
   radios as checked. Only the newly activated radio's semantic is refreshed by
   `cui_base_radio` (`src/base/cui_base.c:156`). Accessibility consumers can
   observe mutually exclusive controls simultaneously selected. The source
   explicitly schedules a repaint for earlier siblings; that next event-free
   frame correctly reports only option1. The draw list also contains both
   selected indicators in the activation frame and only one after that natural
   corrective redraw. The regression fails on stale completed-frame semantics;
   it records existing delayed-paint behavior without claiming paint can be
   retroactively rebuilt by the per-radio API.

The original audit reported five failing observations across these two defects.
The default executable covers the fixed ListMulti behavior and other passing
controls; `--radio` still reports one failure. No Radio production change is
included in this follow-up.

## Covered behavior and API limits

| API | Actual external-owner scenario | Observation |
| --- | --- | --- |
| List | `on_activate` replaces selection or restores the original selection | Final same-frame semantics and subsequent render callback see the final owner value; callback runs once. |
| ListMulti | `on_activate` replaces the full boolean array, including the earlier sibling, or restores it | Final same-frame semantics and actual checkmark location follow the owner array; callback runs once. Confirmation/cancel preserve the caller user pointer after the fix. |
| Radio | Later option changes the shared integer | Earlier semantic stays checked until scheduled redraw, as above; disabled option does not change the value. |
| Checkbox | Individually caller-owned booleans | Completed-frame semantics match both independent values; disabled item is inert. There is no dedicated CheckboxGroup or change-request callback API in this snapshot. |
| Tabs | Later tab changes the shared stable ID | Earlier and later semantics reflect final selection in the activation frame. |
| Tree | Later row changes the shared node ID | Final semantics are correct in the activation frame and repaint is scheduled. |
| Stepper | Later item changes the shared index | Final semantics are correct; one actual `Current` label is painted; repaint is scheduled. |
| MenuTree radio entries | Later entry changes the shared integer while popup stays open | Both sibling semantics reflect final selection in the activation frame; disabled entry is inert. |

List callbacks are synchronous notifications after the reducer writes selection,
not dedicated cancellable requests. An owner can restore selection inside the
notification, which this suite tests. Radio, Checkbox, Tabs, Menu, Tree and
Stepper do not expose selection-request callbacks; the suite does not invent a
rejection contract for them. Custom content/render callbacks are not treated as
controlled change callbacks. No cross-widget retroactive repaint or arbitrary
owner-state tracking is claimed.

List, ListMulti, Tabs and MenuTree complete their same-group state/paint in the
activation frame and do not request another corrective frame in these fixtures.
The audit records this rather than requiring unnecessary redraws. An event-free
host repaint is also exercised to verify stable state and no duplicate callbacks.
Radio's explicit corrective redraw is checked before that event-free frame.
All fixtures use public APIs; disabled controls and disabled list items are
exercised through actual routed pointer events.

## Reproduction

From the isolated checkout:

```sh
cmake -S . -B build -G Ninja
cmake --build build
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude \
  tests/unit/test_controlled_siblings.c build/libcui.a -lm \
  -o /tmp/cui-controlled-siblings
/tmp/cui-controlled-siblings
/tmp/cui-controlled-siblings --radio # intentionally fails for the known Radio gap
ctest --test-dir build --output-on-failure
```

The default audit returns 0; the separate Radio diagnostic returns 1. The
existing CTest suite does not yet register this new audit. The Choice suite's
notification regression is registered through its existing target. A sanitizer
build of `cui` and both audit selections exercises all cases.

Validation: warnings-as-errors compilation passes. The existing CTest suite
passes 90/91; its only failure is `distribution`, because `dist/cui_choice.h`
needs regeneration after the base reducer fix. Distribution output belongs to
the integrator. The audit passes by default, and `--radio` preserves its one
expected failure. ASan/UBSan pass the default audit and registered Choice suite
without runtime errors.
