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
| Rating (`src/base/cui_numeric.c`) | Source inspection: targeted KEY_DOWN mutates before interaction and ignores consumption. Selective cancellation currently covers Space/Enter, so nested modal arrows need independent proof. Runtime counterexample not yet established. | Test direct and nested-modal arrows/Home/End/Delete during lifetime reset; preserve ordinary legacy keys. |
| Basic legacy Slider (`src/base/cui_base.c`) | Source inspection: raw targeted pointer/key paths ignore consumption. Activation cancellation alone cannot protect these value mutations. Runtime counterexample not yet established. | Test direct/nested stale samples, fresh input and frozen no-helper value traces. |
| Controlled legacy Slider (`src/base/cui_base.c`) | Source inspection: consumed samples are skipped, but caller-owned drag/preview can remain active on idle capture loss. The declared cancellation contract restores the gesture-start value. Runtime counterexample not yet established. | Prove idle reset disposition and BEGIN/UPDATE/COMMIT/CANCEL callbacks, values and ordering; preserve no-helper behavior. |

The typed Slider model already contains a snapshot-ready/pressed capture-loss
cancellation path. This source observation does not prove nested lifetime reset
coverage; retain it as a separate runtime acceptance case. Suppressing activation,
consuming input and cancelling a proposed value remain separate operations.

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

## Current unpublished proof

The copied marker/query and Scroll/Resizable candidates passed 22 headless cases
in `dialog-marker-first-runtime.log`: scrollbar mouse/idle/touch cancellation,
Resizable stale size/completion suppression and fresh input, scope query and prior
Dialog lifecycle/receipt/geometry checks. Production files remain unchanged.
Independent frozen no-helper reducer traces, additional Resizable idle/touch
cases and remaining consumer coverage are still required. This is not approval
to publish the Core API or a claim of all-component cancellation support.
