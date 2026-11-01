# Dialog lifetime cancellation: builtin reducer checkpoint

Status: design proposal only. No marker/query or Base change is implemented.
The unpublished cancellation prototype remains gated on this remaining case.

## Actual counterexample

Session 36366 compiled the input fixture with strict C11/O3/warnings-as-errors,
then ran three private-Xvfb cases. Nested Body and independent gesture owner
passed. The actual public Base overlay scrollbar failed: after parent close/open,
its retained mouse drag applied the old MOVE+UP and changed child scroll from
0 to 285.714. The strict expected value remains zero. Log:
`/home/matteo/.cache/cui-hover-raw-build/dialog-gesture-actual-basebar-runtime.log`.

`src/base/cui_scroll.c:cui_base_scrollbar` keeps dragging/grab in caller-owned
`cui_scroll_state`. It uses `cui_event_target` without checking consumption.
Core capture cancellation and selective event consumption therefore cannot reset
that separate reducer state. The earlier custom-drag proof does not cover this
builtin. Changing targets would corrupt diagnostic identity; globally honoring
consumption would change existing Base behavior. Neither is proposed.

## Recommended narrow opt-in seam

Add a private `bool input_cancelled` to each current focus-scope record, initialized
false on `cui_begin_focus_scope`. The explicit cancellation helper, after its
existing full validation and before any mutation, resolves the current target
scope. Its atomic commit marks that scope. No previous-scope marker is read.
A descendant submitted later inherits cancellation by walking current parent
links; already submitted descendants see the same ancestor marker. Closing the
scope does not undo the frame-local marker. Next frame reconstructs all markers
false. This stores lifetime-reset intent, not a new input-dispatch decision.

Proposed portable read-only API:

```c
bool cui_scope_input_cancelled(const cui_context *ui);
```

The query returns true only inside an ordinary active current scope whose own or
ancestor marker is set. Outside submission, inert/layout-only/measurement/prepaint,
errored frames and missing scopes return false without mutation. It performs a
bounded current-parent walk; no ID folding, geometry, resource lookup or allocation.
It does not report cancellation of arbitrary other scopes and does not authorize
raw input acceptance. Calling cancellation late cannot roll back earlier callback
or reducer effects, consistent with the existing accepted-receipt contract.

The Base scrollbar reads this once during submission. When true, it clears its
local dragging flag before processing any samples, including idle frames. It
skips pointer/key samples marked consumed by explicit cancellation; non-touch
CANCEL retains the existing unconditional drag cleanup. Unconsumed input retains
the old math/page/key behavior. The marker does not itself consume unrelated
wheel/text/accessibility input, nor reset values/grab or alter semantics/focus.
When the explicit helper was never called, the new branch is false and the old
reducer statements/order/consumed-event behavior remain unchanged.

This is deliberately an opt-in reducer policy. Custom raw reducers must observe
`cui_event_consumed` and clear their persistent press state when this query is
true; Core cannot mutate arbitrary caller storage. Public interact/pressed and
activation receipts continue using the already proved cancellation operation.
No claim that arbitrary custom callbacks can be retroactively undone is made.

## Memory and alternatives

Current `cui_core_focus_scope` is `{cui_id id; size_t parent; bool modal, active;}`.
There are two arrays of 256 records. The added bool is expected to fit existing
padding on LP64 and WASM32 (expected record sizes 24 and 16 respectively), so the
expected context-size delta is zero and allocation/resource delta zero. This is
NOT measured evidence: an actual original/candidate compiler sizeof/offsetof
probe on both targets is required before approval/implementation. The original
prototype measured context size 255520 on this native configuration. Initialization
adds one scalar store per submitted scope; there is no zero-runtime-cost claim.
Previous array copies carry an unused marker byte; the query never reads it.

A per-event bitset costs at least 64 bytes for 512 samples and cannot represent
idle-frame local dragging cancellation without another scope marker. Reusing an
activation-slot sentinel risks reserved-state ambiguity and still lacks idle scope
intent. A lazy resource sidecar avoids unconditional record changes but introduces
allocation/capacity failure into a previously allocation-free helper and a lookup
on each reducer query. A public explicit reset hook on each custom/bar state would
require the parent to discover independently owned nested state, the failure that
this scoped operation is intended to handle. These alternatives are not preferred.

## Proof and transfer gate

First measure record/context sizes and query behavior in the isolated test adapter;
no production header/Core/Base change before review. Then retain the actual old
Base-bar failure and make its captured MOVE+UP, idle-held and touch counterparts
pass, followed by genuine fresh DOWN success. Verify parent reset reaches nested
modal/portal descendants but leaves siblings/background/trigger unchanged. Test
late-created descendants, next-frame marker reset, repeated cancellation, invalid
phase/graph/count preflight and no allocation. Test custom reducer cleanup using
the explicit query contract. Preserve delivered receipts and targets byte-for-byte.

Finally compare the uncalled-helper Base route against a frozen original reducer
trace across pointer/page/key/CANCEL/ordinary-consumed samples and disabled/hidden
bars, with fieldwise state comparison. Rerun the existing lifecycle, action,
gesture, drag, failure/recovery and legacy surface proofs. Only then propose public
Core/query and Base transfer plus metadata/generation. Full Dialog composition,
Root/native-host and all five foreign facades remain open.
