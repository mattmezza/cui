# Scoped lifetime cancellation: consumer audit

Status: **partial source audit; no production marker API is implemented**.
See [the proposed marker contract](DIALOG_BUILTIN_CANCEL_MARKER.md).
The operation must preserve existing dispatch and no-helper behavior. A consumed
sample, a cancelled press, value acceptance and platform default prevention are
separate concepts.

## Confirmed gaps

| Consumer | Current evidence | Required acceptance |
| --- | --- | --- |
| Base scrollbar (`src/base/cui_scroll.c`) | Actual nested close/reopen test applies old captured MOVE+UP: scroll 0 → 285.714. Reducer ignores consumed samples and retains caller-owned dragging state. | Explicit reset must clear idle/held drag and suppress cancelled samples. Prove mouse/touch, fresh DOWN recovery and unchanged no-helper trace. |
| Resizable (`src/base/cui_resizable.c`) | Actual original nested reducer applies stale MOVE+UP: size 100 → 40 and spurious completion. It retains `active_handle`/`dragging` and ignores consumption. | Test nested old MOVE+UP, idle state, fresh input and callback counts/order; preserve legacy behavior when reset is unused. |
| Rating (`src/base/cui_numeric.c`) | Actual queued stale arrow changes 2 → 3: direct case is already consumed; nested-modal case remains unconsumed. Target identity stays intact. Activation cancellation does not cover this raw value path. | Test direct and nested-modal arrows/Home/End/Delete during lifetime reset; preserve ordinary legacy keys. |
| Basic legacy Slider (`src/base/cui_base.c`) | Actual queued stale arrow changes 2 → 3 in direct (consumed) and nested-modal (unconsumed) cases. Raw targeted processing bypasses activation cancellation. | Test direct/nested stale samples, fresh input and frozen no-helper value traces. |
| Controlled legacy Slider (`src/base/cui_base.c`) | Actual idle capture-loss fixture leaves caller-owned dragging active. The declared cancellation contract restores the gesture-start value; disposition and callback ordering need an explicit reviewed fix. | Prove idle reset disposition and BEGIN/UPDATE/COMMIT/CANCEL callbacks, values and ordering; preserve no-helper behavior. |

The typed Slider model already contains a snapshot-ready/pressed capture-loss
cancellation path. This source observation does not prove nested lifetime reset
coverage; retain it as a separate runtime acceptance case. Suppressing activation,
consuming input and cancelling a proposed value remain separate operations.

## Additional retained-state counterexamples

Six actual nested-owner diagnostics now produce **five failures and one pass**.
Table applies stale resize (100 → 138, one width callback) and reorder
([1,2] → [2,1], one move callback); an idle reset leaves resizing active.
ColorPicker applies a stale hue change and retains its drag flag on idle reset.
Each test first establishes capture through a real DOWN event. The typed Slider
capture-loss/fresh-gesture case passes using its existing reducer.

See [the source audit](DIALOG_RETAINED_CONSUMER_AUDIT.md) and
[the counterexample receipt](../validation/dialog-retained-consumer-counterexamples.json).
The [reviewed test-only reset plan](DIALOG_TABLE_COLOR_CANCEL_PROOF.md)
clears only scoped pending press state, preserves changes
already delivered, and emits no invented model/completion callback. Production
transfer remains gated by corrected runtime and frozen legacy traces.

## Remaining audit

These are inspection candidates, **not established defects or verified support**:

- Table selection and other retained states beyond the confirmed resize/reorder cases.
- Choice/menu/select popup pointer and keyboard reducers.
- Picker, legacy numeric controls, text selection, chrome/window drag and surfaces.
- Typed Slider models and other consumers that already check consumption: verify
  retained state also terminates correctly without replaying notifications.
- Styled wrappers and independently owned nested scopes/portals.

A file containing a consumption check does not establish that every mutation path
honors it. Audit each reducer and meaningful callback/result path, then attach
headless runtime evidence. Existing legacy entry points stay in regression tests.

## Custom reducers and completion gate

Core cannot reset arbitrary application-owned state or undo earlier callbacks.
Custom raw reducers need an explicit documented reset/query contract. Do not
advertise automatic cancellation of all custom children from builtin tests.

Before production transfer: measure the proposed storage on native and WASM,
prove scope/ancestor and invalid-phase behavior, close applicable builtin gaps,
retain exact target/receipt evidence, and verify uncalled-helper compatibility.
Update this inventory as consumers are tested or additional paths are found.

## Current unpublished proof

The copied marker/query and Scroll/Resizable candidates passed an initial 22 headless cases
in `dialog-marker-first-runtime.log`: scrollbar mouse/idle/touch cancellation,
Resizable stale size/completion suppression and fresh input, scope query and prior
Dialog lifecycle/receipt/geometry checks. Production files remain unchanged.
The final driver passes 26 checks, including Resizable idle/touch and a frozen
27-frame no-helper Scroll/Resizable trace (`dialog-marker-final-build-runtime.log`).
The separate numeric trace passes, but five stale-key/idle counterexamples fail
(`dialog-numeric-original-runtime.log`). Those policies and remaining consumer
coverage still gate production transfer. This is not approval
to publish the Core API or a claim of all-component cancellation support.

## Numeric and bare-scope candidate result

The copied candidate passes 38 checks in
`dialog-numeric-fallback-corrected-runtime.log` (0.24 seconds). These include the
five formerly failing numeric cases, one synchronous controlled Slider CANCEL
with the gesture-start value, fresh-input recovery and bare-scope Escape routing.
The unrelated-input fixture uses supported Textbox accessibility FOCUS; the first
run's unsupported ACTIVATE setup failure remains recorded. Existing Button
ACTIVATE preservation is independently covered.

Routed keys without current/prior semantics can use validated full-ID focus-scope
ancestry. Where a scope ID also identifies a semantic owner, semantic ownership
stays authoritative; contradictory aliases remain an explicit limitation. Capture,
contact and activation ownership are unchanged. Production files remain unchanged,
and remaining reducer coverage still gates transfer.
