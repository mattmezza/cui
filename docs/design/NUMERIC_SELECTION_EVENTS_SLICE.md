# Rating, Stepper and Pagination CHANGE observers

Design only; audited at `b4146c89`. Tabs is excluded and belongs to its separate
workstream. These controls already mutate caller-owned scalar values and return
aggregate bools. The smallest compatible facade observes their final committed
values, rather than introducing controlled requests or reconstructing activations.
No production code, Core routing, bridge schema or generated SDK changes are
proposed in this checkpoint.

## Current behavior

| Family | Owner and normalization | Input/multiplicity and presentation |
| --- | --- | --- |
| [Pagination](../../src/base/cui_numeric.c#L469) | `size_t *page`, one-based when total is nonzero; zero when empty. Clamps to `[1,total]` before processing input, even when disabled. Bool compares entry and final page. | All queued activations can update the owner. First/Previous/Next/Last use stable part IDs. Numbered buttons map through the **entry normalized page's** item window, while final semantics/Styled paint use the final page's window. |
| [Rating](../../src/base/cui_numeric.c#L524) | `float *value`; maximum defaults to 5 and caps at 100. Nonfinite owners normalize to zero; finite values round to whole/half steps and clamp to the effective range before input, including disabled/readonly calls. Bool is `!isfinite(before) || after != before`. | Multiple star/clear activations and root-targeted key-down events can coalesce. Pointer left-half selection gives half a star; keyboard/other star activation chooses the full star. Home clears even when `clearable` is false; Delete/Backspace clearing requires it. |
| [Stepper](../../src/base/cui_numeric.c#L587) | `size_t *current`, zero-based index; clamps to last item, or zero with no items, even when disabled. Rejects missing descriptors with nonzero count and counts over 100. | Multiple activations select the last eligible step. Per-step disabled state gates activation. Status descriptors are caller-owned and **not** rewritten to match selection. Root is a non-tab-stop group; each step is a button. |

Public types/signatures and stable part-ID rules are in
[`cui_numeric.h`](../../include/cui/cui_numeric.h#L65). No family here has a legacy
value-change callback. Rating has readonly in addition to disabled; Pagination
and Stepper have only disabled. None has a draft revision or NumberInput's
`step_by`, parsing, Enter-submit or edit error result. Do not reuse NumberInput's
double payload or add its callbacks to these APIs.

All three reducers process existing input then publish semantics/invalidate and
return final inequality. They expose neither a per-event result list nor a
counter. Returning to the entry value yields false even after several accepted
operations. Selecting the already-current step/page or a no-op at a boundary is
not a CHANGE, and must not become an ACTIVATE notification through this facade.
Their loops use existing activation/target queries; do not add consumption,
nonrepeat filtering or new accessibility behavior while adding observation.

[`cui_pagination`](../../src/components/cui_numeric.c#L154),
[`cui_rating`](../../src/components/cui_numeric.c#L229) and
[`cui_stepper`](../../src/components/cui_numeric.c#L296) each call Base once before
painting. They subsequently read the owner again. Rating also prepares retained
icon resources; failure to prepare artwork is not evidence that Base did not
mutate. Stepper's selected index determines the visible "Current" label;
nonselected ACTIVE status is presented as Pending, while Error/Complete retain
their existing presentation. These painting and ownership rules must remain
unchanged in the legacy entrypoints.

## Proposed portable types and entrypoints

A separate `cui_numeric_selection_events.h` can include existing numeric types
and shared metadata. Keep the three event types distinct so one-based page,
zero-based step index and float rating cannot be accidentally interchanged.

```c
typedef struct {
    cui_component_event_metadata event;
    size_t before, after, total;
} cui_pagination_change_event;
typedef struct {
    cui_component_event_metadata event;
    size_t before, after, count;
} cui_stepper_change_event;
typedef struct {
    cui_component_event_metadata event;
    float before, after;
    unsigned maximum; /* Effective defaulted/capped maximum. */
    bool before_finite;
} cui_rating_change_event;
typedef void (*cui_pagination_change_fn)(void *user, const cui_pagination_change_event *event);
typedef void (*cui_stepper_change_fn)(void *user, const cui_stepper_change_event *event);
typedef void (*cui_rating_change_fn)(void *user, const cui_rating_change_event *event);
typedef struct { cui_pagination_change_fn change; void *user; } cui_pagination_event_handler;
typedef struct { cui_stepper_change_fn change; void *user; } cui_stepper_event_handler;
typedef struct { cui_rating_change_fn change; void *user; } cui_rating_event_handler;
typedef struct {
    bool control; /* Original aggregate bool, not validation/render success. */
    size_t change_count; /* Actual observer invocations: zero or one. */
} cui_numeric_selection_event_result;

cui_numeric_selection_event_result cui_base_pagination_events(
    cui_context *, cui_id, size_t *page, size_t total, bool compact, bool disabled,
    cui_pagination_event_handler);
cui_numeric_selection_event_result cui_pagination_events(
    cui_context *, cui_id, const cui_theme *, size_t *page, size_t total,
    bool compact, bool disabled, cui_pagination_event_handler);
cui_numeric_selection_event_result cui_base_rating_events(
    cui_context *, cui_id, cui_str name, float *value, cui_rating_options,
    cui_rating_event_handler);
cui_numeric_selection_event_result cui_rating_events(
    cui_context *, cui_id, const cui_theme *, cui_str name, float *value,
    cui_rating_options, cui_rating_event_handler);
cui_numeric_selection_event_result cui_base_stepper_events(
    cui_context *, cui_id, const cui_step *, size_t count, size_t *current,
    bool disabled, cui_stepper_event_handler);
cui_numeric_selection_event_result cui_stepper_events(
    cui_context *, cui_id, const cui_theme *, const cui_step *, size_t count,
    size_t *current, bool vertical, bool disabled, cui_stepper_event_handler);
```

`size_t` payloads preserve the native unsigned width, including Pagination totals
near SIZE_MAX; do not pass through semantic float or double. Total/count are
by-value context snapshots, not proof of item storage lifetime or immutable items.
They distinguish empty zero from a nonempty Stepper's valid index zero. Effective
Rating maximum is also a scalar snapshot, not a new range policy.

Rating deliberately differs from NumberInput: NaN/Inf input owners are normalized
by the legacy reducer rather than rejected. Preserve `before` as its float value
and expose `before_finite=false`; consumers must not perform ordinary equality on
NaN or assume its payload bits are canonical. `after` is the finite committed
rating. Notify for nonfinite-to-finite normalization in an eligible active call,
matching the legacy changed condition. For finite values use exact float
inequality, no epsilon or double reinterpretation; signed zeros compare equal.
Rounded/clamped finite normalization can itself produce CHANGE. Do not claim a
pointer/keyboard cause for any of these corrections.

## Delivery, passive behavior and implementation seams

Capture the entry owner before native normalization. Run Base exactly once,
capture final value and notify once after final semantics/invalidation, before
Styled painting. Pagination/Stepper use exact entry/final inequality; Rating also
uses the nonfinite-entry rule. Preserve the original bool independently of handler
presence and later drawing/resource errors. An absent callback accepts/ignores
user data and still reduces. No listener registration, heap allocation or queued
payload is needed. Handler/user are borrowed only for the call.

Only ordinary active, enabled calls notify; Rating additionally requires not
readonly. Require the same nonzero `cui_frame_sequence` before and after native
reduction. Emit shared CHANGE metadata at the family root ID, source UNKNOWN,
HAS_FRAME only. No event index, source flag, activation snapshot, request or
commit/submission reason can be inferred from the aggregate. Suppress notification
for inert/layout-only/outside-frame and restricted calls while preserving their
legacy reduction, including normalization. **Zero callbacks is not a promise of
no owner mutation.** Test this especially for disabled out-of-range pages/indices
and readonly nonfinite ratings. Do not patch passive Base behavior as a side effect
of an observer implementation.

Whole-frame Base wrappers appear sufficient because no Base value callback seam
must be preserved. Styled variants need a private common implementation with the
old entrypoint using an unobserved Base path and the new entrypoint observing that
same single reduction. Do not wrap Styled after it has already painted or submit
Base a second time. Native validation/Styled preflight rejection must preserve
its existing bool and produce no invented event. A committed normalization/change
is still observable if a later artwork, layout or resource error occurs; callbacks
do not certify rendering success. Expose no synthetic error field in this result:
the legacy Core diagnostics remain the error channel.

These are committed observers, not controlled selection: returning void cannot
reject, roll back or defer the native write. Pagination's entry-window mapping,
Rating's half-step rules and Stepper's descriptor statuses remain family-specific.
Do not unify them into a generic list selection reducer or add bubbling/dispatch.

## Callback lifetime and owner mutation policy

Payloads are pointer-free and may be copied. Owner pointers, user data, descriptors,
titles/descriptions and context resources must remain valid through the outer call
and applicable frame rendering. Do not destroy/rebind the owner/context, mutate
Step descriptors, recursively submit, or unwind foreign exceptions through C.
Update unrelated application state and defer teardown safely.

For the smallest first implementation, **owner mutation during the observer is
unsupported for all three families**, including Base, to keep one uniform family
contract. Styled implementations reread the owner to paint. Unlike NumberInput,
whose paint reads its reduced draft and therefore safely defers a finite observer
owner assignment to next-frame synchronization, an immediate assignment here can
change displayed page/star/current-step state after old semantics were published.
Do not silently promise coherent paint/semantics under that mutation. Applications
can copy the event and assign the owner after the outer call. A later explicit
snapshot-paint design could permit mutation, but requires separate geometry,
semantics and lifetime review; it is not needed to ship these observers.

## Evidence and production gates

[`models`](../../tests/unit/test_numeric.c#L35) exercises Pagination item windows,
bounded output, ellipses and SIZE_MAX totals. The
[`other_events`](../../tests/unit/test_numeric.c#L263) scenario covers keyboard
Next, same-frame Last/Previous ending at page 99, pointer Last, half-star pointer
selection, Rating root keyboard adjustment/clear, readonly Rating, eligible versus
disabled Stepper and whole-family disabled input. It runs Base/Styled variations
via the existing scene helpers. [`stepper_visible_selection`](../../tests/unit/test_numeric.c#L485)
checks actual "Current" painting across selection, and
[`rating_star_graphics`](../../tests/unit/test_numeric.c#L516) checks artwork.
[`pagination_counter_center`](../../tests/unit/test_numeric.c#L710) checks compact/
noncompact label geometry. These are existing source evidence, not new executed
validation for this documentation-only checkpoint.

Before production, compare old/new owner, bool, semantics, focus, consumption and
geometry in a meaningful headless harness. Cover each Base/Styled entrypoint;
normalization with no input; empty/count/total changes; invalid counts/descriptors;
Pagination entry-window remapping and SIZE_MAX; multiple changes cancelling to
entry; repeated same choice; whole/per-item disabled and Rating readonly;
inert/layout-only; real supported pointer, key and accessibility activation;
Rating NaN/Inf/out-of-range/half rounding and signed zero; zero/one callbacks,
borrowed user data without callback and retained payload copies. Distinguish
unsupported or absent accessibility actions rather than simulating parity. Keep
legacy tests unchanged and compile C11/C++ headers. Run any native CTest through
the private headless wrapper; no build is needed for this proposal.

Bridge Pagination/Rating currently convert native results into queued legacy
CHANGE, and Stepper uses its existing grouped-selection bridge path. All remain
V1 LEGACY payloads; reserved selection/scalar tags do not become supported because
these C facades exist. Exact native `size_t` values are not proof of exact existing
bridge numeric transport. Binding subscriptions, controlled requests, per-input
multiplicity and Tabs remain excluded.

## Implementation checkpoints

Pagination now has an additive Base wrapper and a narrow Styled common seam.
Legacy paint and reduction remain on their original unobserved path. Stepper
has a separate aggregate wrapper/common seam with zero-based exact index payloads;
its legacy descriptor statuses and rejection rules remain unchanged. Rating
now has a distinct float/nonfinite-aware aggregate wrapper and a narrow Styled
common seam. All three families preserve their direct legacy Base path and paint;
none adds per-event requests or bridge tags. The implemented ownership restriction
remains: callbacks may copy payloads/request deferred work, but owner mutation is
unsupported until the outer call returns.
