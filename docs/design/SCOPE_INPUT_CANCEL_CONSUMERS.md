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
| Resizable (`src/base/cui_resizable.c`) | Source inspection: handle reducer retains `active_handle`/`dragging`, reads targeted MOVE/UP without consumption checks, and emits completion. Runtime counterexample not yet established. | Test nested old MOVE+UP, idle state, fresh input and callback counts/order; preserve legacy behavior when reset is unused. |
| Rating (`src/base/cui_numeric.c`) | Source inspection: targeted KEY_DOWN mutates value without checking consumption. Activation cancellation alone does not cover this branch. Runtime counterexample not yet established. | Test consumed owned arrows during lifetime reset and ordinary legacy keys separately. |

## Remaining audit

These are inspection candidates, **not established defects or verified support**:

- Table resize/reorder/selection and its retained drag state.
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
