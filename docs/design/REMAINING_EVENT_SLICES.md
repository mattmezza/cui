# Remaining component event slices

Read-only audit at `c55a9601`. This is a prioritized work list, not a replacement
78-row matrix or a parity promotion. The [78-family inventory](COMPONENT_EVENTS_COVERAGE.md)
remains the baseline; its later additions are not fully reflected in its legacy
rows. No manifest entry should be removed. The [contract](COMPONENT_EVENTS_CONTRACT.md)
requires preserving ownership and timing, not making every family emit the same
callback. Subsequent implemented additions are listed below; the priority table
records the original audit recommendations and is not a current completion count.

## Already landed: do not repeat

[Button/Checkbox](../CONTROL_EVENTS.md), [Switch/Toggle](../BOOLEAN_EVENTS.md),
[Base controlled double Slider](../SLIDER_EVENTS.md), [RadioGroup](../SELECTION_EVENTS.md),
[Styled Select](../SELECT_EVENTS.md), [Base Input](../TEXT_INPUT_EVENTS.md),
[Styled Input](../TEXT_INPUT_STYLED_EVENTS.md), and
[Base/Styled Textarea and Editor](../MULTILINE_EVENTS.md) now have additive APIs.
Their headers are authoritative; these are partial entrypoint/family contracts.
Input delivers per-event CHANGE/SUBMIT, multiline delivers at most one aggregate
CHANGE, and controlled Slider reports lifecycle/requests without writing confirmed
values. Missing-handler and count semantics also differ. There is no uniformity
claim and no basis for renaming requests into committed changes.

Additional implemented slices: [Styled controlled Slider](../STYLED_SLIDER_EVENTS.md),
[Combobox](../COMBOBOX_EVENTS.md), [Surface](../SURFACE_EVENTS.md),
[MenuTree](../MENU_EVENTS.md), [ContextMenu](../CONTEXT_MENU_EVENTS.md),
[NumberInput](../NUMBER_EVENTS.md), [Tabs](../TABS_EVENTS.md),
[Pagination/Stepper/Rating](../NUMERIC_SELECTION_EVENTS.md), and
[Calendar/DatePicker/ColorPicker](../PICKER_EVENTS.md),
[Simple Menu](../SIMPLE_MENU_EVENTS.md), [DropdownButton](../DROPDOWN_EVENTS.md),
[OTP](../OTP_EVENTS.md), [Collapsible/Accordion](../DISCLOSURE_EVENTS.md),
[List/ToggleGroup](../LIST_EVENTS.md), and [Tree](../TREE_EVENTS.md).
Tree covers selection, expansion and row invocation. DataTable currently has
[sort, scalar/set-selection and column width/move observers](../TABLE_EVENTS.md); Settings navigation, built-in scalar fields and committed text edits are implemented; further Table
and Settings domains remain in progress. Proposals must not be read as implemented APIs.

## Highest-value independent slices

The original proposals below are an audit baseline; the additions above supersede
its Styled Slider, NumberInput, Rating, Stepper/Pagination and Tabs recommendations.
List/ToggleGroup is also implemented; its row below is historical. Prefer one
family per review and preserve all
old signatures/options/results. An aggregate observer needs UNKNOWN source and no
index; per-event evidence must be captured at the actual reducer seam. Neither
approach supplies rejection for an already-mutated pointer value.

| Priority / slice | Actual source seam and minimal recommendation | Required proof |
| --- | --- | --- |
| 1. Styled controlled Slider | [`cui_slider`](../../src/components/cui_components.c) calls legacy `cui_base_slider` with a float pointer; it does not expose the landed double controlled facade in [`cui_slider_events.h`](../../include/cui/cui_slider_events.h). Add a distinct Styled controlled entrypoint reusing the exact Base controlled reducer and typed phases, with caller-owned preview/capture state. Do not silently redirect the float API or convert doubles through floats. Audit track geometry/hit bounds before promising visual equivalence. | Extend [`test_slider_events.c`](../../tests/unit/test_slider_events.c), [`test_slider_controlled.c`](../../tests/unit/test_slider_controlled.c), and Styled coverage in [`test_components.c`](../../tests/unit/test_components.c): accepted/rejected owner requests, drag/key/cancel, >float precision, bounds/pixels, no callback writes, null-handler behavior. |
| 2. Number Input aggregate numeric CHANGE | [`cui_base_number_input`](../../src/base/cui_numeric.c) has a mutable draft edit plus `double *value`; valid draft parses at the end, Enter/blur clamp, step keys/buttons can update during reduction. `step_by` in [`cui_numeric.h`](../../include/cui/cui_numeric.h) is a transformation policy, not a notification. Start with one typed double before/after observer around the complete Base/Styled call, separately from draft revision. No per-event numeric CHANGE reconstructed from the final parse; no generic COMMIT/SUBMIT inferred from the aggregate boolean. | [`test_numeric.c`](../../tests/unit/test_numeric.c), [`test_button_numeric.c`](../../tests/unit/test_button_numeric.c): invalid editable draft, valid draft/value divergence, exponent/IME, Enter versus blur, custom stepping, clamp/format failure, multiple updates, owner-pointer compatibility and copy/lifetime limits. Exact per-event submit requires a later audited loop hook. |
| 3. Rating | [`cui_base_rating`](../../src/base/cui_numeric.c) normalizes/clamps the float even before interactive reduction, then processes star/clear/key inputs. Start with aggregate applied float CHANGE including explicit normalization reason/unknown cause; alternatively introduce genuine per-event hooks only after separating normalization from input transitions. Never claim owner rejection. | [`test_numeric.c`](../../tests/unit/test_numeric.c): half-step pointer position, keyboard, clear, readonly/disabled normalization, nonfinite input, repeated events with equal final value, Styled paint equivalence. |
| 4. Stepper + Pagination, separately reviewable | [`cui_base_stepper` and `cui_base_pagination`](../../src/base/cui_numeric.c) mutate size_t indices and also normalize out-of-range values. Cheap aggregate typed index notifications are viable; per-event notifications require narrow reducer hooks to preserve every intermediate assignment. Pagination is one-based (zero for no pages); Stepper is zero-based, with caller-owned statuses. Do not share an ambiguous generic index meaning. | [`test_numeric.c`](../../tests/unit/test_numeric.c): disabled-item suppression, empty models, normalization, several activations, keyboard/accessibility where supported, no fabricated activation for normalization, Base/Styled equivalence. |
| 5. Tabs | [`cui_base_tabs`](../../src/base/cui_choice.c) combines selection, scroll/overflow and drag requests; [`cui_tabs`](../../src/components/cui_choice.c) owns Styled composition. Begin with selection-only typed ID CHANGE, distinct from drag/reorder request and overflow lifecycle. Per-event selection is viable only at actual assignment sites and any overflow path; aggregate is a smaller explicit exception. | [`test_choice.c`](../../tests/unit/test_choice.c): keyboard/activation/disabled, overflow selection, drag request without selected change, scroll-only silence, callback mutation restrictions and unchanged animation/paint. Reuse metadata vocabulary from [`cui_selection_events.h`](../../include/cui/cui_selection_events.h), not its payload where cause is unavailable. |
| 6. List / ToggleGroup | [`cui_choice_private_list_events`](../../src/base/cui_choice.c) is a genuine ordered loop; existing on_activate/on_confirm/on_cancel in [`cui_choice.h`](../../include/cui/cui_choice.h) already provide actions in event order. Add optional typed evidence at that seam if needed; selection CHANGE must be distinct from confirming the already-selected item. Multi-list/ToggleGroup need item+bool transitions, not one selected ID. | [`test_choice.c`](../../tests/unit/test_choice.c): unchanged-item confirmation, multiple activations, Escape, secondary modifier, virtualization/window changes, callback order and no duplicate legacy callbacks. Do not duplicate existing List actions merely to rename them. |

Styled wrappers must not reduce twice. If they contain late mutation (as Editor's
completion popup does), observe the complete outer call or add the hook at each
real mutation seam; do not assume all Styled controls only paint after Base.

## Subsequent interactive work

The menu, picker, OTP, disclosure and Tree recommendations below describe the
original audit. The implemented guides above supersede those missing-API claims;
remaining work includes additional domains and explicit exceptions, rather than
reimplementing those facades.

- **Menu/DropdownButton:** follow the existing
  [menu/surface proposal](MENU_DIALOG_EVENTS_SLICE.md) after in-flight work settles.
  [`cui_choice.h`](../../include/cui/cui_choice.h) and
  [`src/base/cui_choice.c`](../../src/base/cui_choice.c) distinguish action IDs,
  check/radio state, popup open/close and nested tree navigation. A final selected
  ID cannot represent repeated action activations. Use `test_choice.c` for legacy
  event-order, disabled, submenu and keep-open behavior; keep close lifecycle
  separate from selection or activation.
- **Calendar/DatePicker, OTP, disclosure and Tree:** inspect typed result seams in
  [`cui_picker.h`](../../include/cui/cui_picker.h),
  [`cui_otp.h`](../../include/cui/cui_otp.h),
  [`cui_disclosure.h`](../../include/cui/cui_disclosure.h), and
  [`cui_tree.h`](../../include/cui/cui_tree.h). At the original audit these lacked a common typed
  notification facade; the guides above now define the implemented additions. Split date selection from displayed-month navigation;
  OTP draft CHANGE from completion; Accordion per-panel changes from final array;
  Tree selection from expansion/activation. An aggregate observer is compatible
  only if documented as losing multiplicity/cause. Dedicated existing evidence:
  `tests/unit/test_picker.c`, `test_otp.c`, `test_disclosure.c`, `test_tree.c`.
- **DataTable/Settings/Sidebar:** preserve their existing typed results and owner
  decisions. Table selection/sort/column notifications require stable IDs and
  column/value tags, not lossy float values; use
  [`test_table_selection_contracts.c`](../../tests/unit/test_table_selection_contracts.c).
  Settings on_change lacks a typed changed-value payload, and Sidebar's controlled
  expansion callback is a request, not proof of accepted expansion. See
  [`cui_settings.h`](../../include/cui/cui_settings.h),
  [`cui_sidebar.h`](../../include/cui/cui_sidebar.h), `test_settings.c`, `test_sidebar.c`.

## Existing callbacks and justified exceptions

Button's old typed click callback already carries copied evidence. List actions,
Command select/query/confirm/cancel, Attachment click/remove/retry, Sidebar actions,
Toolbar command activation and TitleBar close already provide useful synchronous
application hooks. Their result/callback coexistence should be documented and
tested, not dispatched again by reading results. These callbacks satisfy their
family action/request role; they do not all have shared metadata, typed values or
identical ordering. Source contracts:
[`cui_command.h`](../../include/cui/cui_command.h),
[`cui_media.h`](../../include/cui/cui_media.h),
[`cui_toolbar.h`](../../include/cui/cui_toolbar.h),
[`cui_chrome.h`](../../include/cui/cui_chrome.h).

Retain the contract's lifecycle exceptions: Surface close decision versus eventual
close; Notification retained descriptors; Speech generation-tagged owner-thread
pump; Carousel/Resizable/Root/Questionnaire/Dock retained registration; Plot/Chart
layout/prepaint/paint borrowing. Omitting draw does not cancel these lifetimes.
Do not replace explicit close-result latches, pump timing or retained teardown with
submission-borrowed generic handlers. Scrollable/VirtualList/MessageScroller also
need scroll/window timing contracts, not an invented item activation event.

Alert, Avatar, Badge, Bubble, Empty, GroupBox, Kbd, Label, Marker, Message,
Progress, Separator, Skeleton, Spinner, StatusBar and other composition-only
entries have no intrinsic editable value/action to observe. Icon/Image loading,
Theme loaders, Clipboard services and render/measurement/format/easing callbacks
are separate resource/policy roles. Child controls retain their own events; a
successful paint/submission boolean is not activation. See the baseline inventory
for each noninteractive exception rather than adding no-op family handlers.

## Foreign-language boundary and verification limits

The landed [V1 notifications](../BRIDGE_NOTIFICATIONS.md) and
[Go/Rust views](../BRIDGE_NOTIFICATIONS_GO_RUST.md) cover their explicit allowlist
(activation, applied boolean/table changes and surface close outcomes), preserving
legacy queues/timing. They do not expose native synchronous controlled requests,
Slider phases, Input per-event observers or multiline observers. The separate [opt-in C value poll](../BRIDGE_VALUE_NOTIFICATIONS.md) additionally
classifies its explicit scalar/text/selection allowlists; Python/JS/TypeScript/Go/Rust owning value wrappers are implemented. Other payloads remain legacy; typed C facades alone do not complete Python/JS/TS/Go/Rust coverage.
Do not retime queued handlers into rejectable requests. Future slices need tagged
owned payloads, bounded queue/copy behavior, full-width IDs/frame values, handler
retention/removal, exception/panic containment and stale-object tests. Follow
[the binding audit](COMPONENT_EVENTS_BINDINGS.md), not launcher-only examples.

This audit inspected current facade headers and the reducers above against the
78-family inventory and contract. It did not rerun runtime suites or certify all
78 implementations. Production slices require strict native private-display tests,
headless browser/foreign tests where applicable, exact legacy compatibility and
explicit evidence/count/lifetime limits. No feature-ledger status changes follow
from this document.
