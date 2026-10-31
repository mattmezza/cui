# Core numeric accessibility input (unreleased)

The additive declarations in `include/cui/cui.h` provide explicit numeric consumer
capabilities, typed SET_VALUE queueing and current-semantic input claims. They do
not implement a Slider model, bridge/binding authoring, native AccessKit numeric
mapping, browser semantic bridge or physical assistive-technology acceptance.
Existing Focus/Activate behavior and legacy Slider routes remain unchanged.

## Registration and completed authority

A painted custom Slider semantic explicitly calls
`cui_set_accessibility_numeric_actions(ui,id,mask)` after setting semantic details.
Only INCREMENT/DECREMENT/SET_VALUE capability bits are supported. Registration is
per current frame/semantic, not inferred from role. Disabled, readonly and modal
scope still gate eligibility. Missing ID/wrong role/unknown mask/lifecycle rejects;
inert/layout-only registration is ignored with false/no mutation. A zero mask
before opt-in allocates nothing; first valid nonzero mask allocates bounded owned
storage retained until context destruction. Capacity failure leaves registration
and queues unchanged and reports CUI_ERROR_CAPACITY.

`cui_accessibility_action_supported` remains an outside-frame completed-snapshot
query. A legacy Slider that never registers has no numeric actions. Re-register
every successful frame; omission/revocation does not inherit an old capability.
Capability does not imply a value will change at a bound or that owner accepts it.

`cui_accessibility_numeric_snapshot_ready` reports only valid completed Core
semantics: false initially/in-frame/after invalidating failed begin/end, true after
ordinary successful end even without numeric registration/storage. Clearing error
alone cannot restore an invalidated snapshot. Known in-frame errors prevent new
claims/publication even if cleared before end; only a fresh good frame restores
readiness. A private invalid-frame marker persists until a new admitted begin.
New scroll-layout opt-in failure always ends non-OK/empty and invalidates numeric
authority until a successful fresh frame. Already claimed input is not rolled back
by a later error. Renderer/native/platform success is not certified by readiness.
Old Focus/Activate paths do not use the new predicate.

`cui_frame_input_snapshot_ready` is the distinct in-frame observer: true only in
ordinary admitted composition with valid prior completed Core geometry/semantics
and no known invalidating current-frame error. It is false outside that phase;
it does not certify final current geometry or current target/modal eligibility.
An empty prior semantic list and zero numeric allocations/capabilities can still
be ready. Inert/layout-only composition requires the separate frame_sequence
eligibility check; readiness alone does not permit reduction. Known current errors
remain invalid after clear, and only successful end establishes next-frame
readiness. Legacy input routes do not consult either new gate. Every rejected
V2 begin conservatively expires new numeric/input authority, including options,
budget or allocation failures before common begin; existing error codes, queued
input and old Focus/Activate routing are preserved. Failed buffer growth still
expires the old V2 list under its existing lifetime contract. This observer does
not implement the planned typed Slider UI reducer or any host delivery.

## Typed queue, ownership and claims

Use existing `cui_queue_accessibility_action` for opted-in INCREMENT/DECREMENT.
For SET_VALUE use `cui_queue_accessibility_set_value_v1` with exact payload size,
version 1/reserved 0 and finite double in **published semantic numeric units**.
The consumer defines domain/step conversion; Core does not infer it. Generic
no-payload SET_VALUE is invalid; zero is a valid typed value, not missing data.
The queue copies the scalar, so stack payload lifetime ends at return. Invalid,
unsupported, lifecycle or queue-full failure preserves queued input. Existing
sticky errors must be handled; only actual capacity failures justify retry.

During painted submission, after current semantics/details/registration, inspect
actual frame inputs and call `cui_claim_accessibility_numeric_input_v1` with exact
output size. It rechecks target, previous/current Slider role/capability, enabled/
readonly/modal eligibility and unconsumed input. Success returns owned v1 action/
target/flags/value and consumes once. Failure preserves output and consumption.
Input consumption is independent of owner acceptance. No focus change, pointer
capture, activation identity, keyboard event or fabricated source is produced.
Freeze frame/index and ACCESSIBILITY metadata before invoking a mutable external
application callback; never use an activation-snapshot helper for numeric input.
Callbacks may update external owner/userdata safely under their family's borrowed
state/lifetime rules. Core retains no callback or owner pointer here.

Typed payloads are stored by original queued-event origin; actual touch-cancel
insertions/reordering do not attach a value to the wrong dispatch index. Synthetic
inputs have no typed value. Copies returned by claim survive frame/context lifetime.
New frames clear payload presence; stale/omitted/disabled inputs never replay.

## Memory, ABI and verification scope

No existing public struct grows; old enum values 0/1 remain. Private idle cost is
one pointer plus snapshot-valid/current-frame-invalid bools and alignment (measure actual context delta,
not a universal 8-byte claim). No numeric allocation is added to cui_create.
With N=max_elements, exact opt-in allocation is
`sizeof(private_numeric_state)+2*N*sizeof(uint32_t)`: current/previous masks plus
two EVENT_MAX=256 bounded scalar/presence arrays and origin count. No per-event
allocation or hidden registry. All storage frees with context. New clients require
matching new symbols; old source/binaries retain their existing routes.

Focused source/public/distribution tests cover owned payloads, original input
order, actual touch-origin shifts, repeat/invalid claim, masks/role/readonly/modal/
omission/compaction, queue capacity/recovery, legacy Focus/Activate and actual
scroll-layout failure/error-clear recovery. TU-local allocator instrumentation
checks zero legacy/zero-mask allocations and atomic first-allocation failure,
retention/free and exact sizeof evidence without production hooks. The observer source/distribution fixtures and numeric allocator fixture passed
all three focused private-Xvfb tests with strict C11 warnings on the combined
CoreV2 base. They include actual failed V2 buffer growth with queued Focus,
Activate and key replay, first/fresh-frame authority, known current errors,
clear-only rejection, modal/omission distinctions, V1 early and V2 late real
scroll-layout failure, and matching/wrong-end lifecycle recovery. The observer
adds no context fields or allocation. On the tested 64-bit Linux ABI the combined
context is 255520 bytes versus 255504 without its existing numeric block (delta 16);
private numeric fixed storage is 8200 bytes, each scalar slot 16, and opt-in N=32
allocation 8456 bytes. The allocator fixture measures that block relative to the
surrounding V2 layout, not the whole prefix. Measurements are ABI-specific.

See [numeric action design](design/SLIDER_ACCESSIBILITY_ACTIONS.md) for independent
native-mode/host SET_VALUE and future model/binding gates. Core helper tests do
not establish Linux native numeric delivery or full Slider family completion.
