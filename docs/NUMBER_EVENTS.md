# Aggregate NumberInput CHANGE observation

Include `cui/cui_number_events.h` and use `cui_base_number_input_events` or
`cui_number_input_events` with the existing arguments plus a
`cui_number_event_handler`. Both return the unchanged native `control` result and
an actual `change_count` of zero or one. The callback is
`void callback(void *user, const cui_number_change_event *event)`.

The pointer-free payload contains shared `event` metadata and finite double
`before`/`after` values. It observes the final committed numeric owner, not draft
revision or intermediate steps. Exact numeric inequality is used: signed zeros
compare equal, adjacent finite doubles and subnormals remain distinct, and changes
that cancel within a call do not notify. Invalid/nonfinite entry owners are still
rejected by the native reducer. No epsilon, float narrowing or extra formatting
is introduced.

```c
static void number_changed(void *user, const cui_number_change_event *event) {
    double *last_observed = user;
    *last_observed = event->after;
}
cui_number_event_result result = cui_number_input_events(
    ui, id, theme, cui_string("Quantity"), state, value, options,
    (cui_number_event_handler){number_changed, last_observed});
/* result.control retains changed/submitted/invalid/step_direction/error. */
```

An absent callback accepts and ignores user data and still performs ordinary
reduction. `step_by/step_user` remain the existing step-magnitude policy with the
same invocation order; they are not observer conflicts or requested-value
callbacks. Missing observers never make the control readonly.

Delivery occurs after Base reduction's final bookkeeping/semantics/invalidation,
before Styled painting. A numeric change already committed despite an edit error
still notifies; neither `error` nor callback count promises successful rendering
or rollback. In particular, failed clamp-draft normalization can preserve the
parsed draft value rather than the entry owner. The original result is returned
without suppressing its error. Enter without numeric change does not notify;
`submitted` remains its legacy aggregate flag. This API adds no REQUEST, COMMIT,
SUBMIT or per-event notification.

Only ordinary active, enabled, editable submissions notify. Inert/layout-only,
outside-frame, disabled and readonly calls do not notify. They still execute the
legacy reducer once: draft synchronization/normalization and any legacy owner
mutation remain compatible, so zero callbacks does not imply no model changes.
External owner synchronization completed before entry is not a numeric change of
this call. Draft-only edits, preedit and invalid drafts also do not establish one.

Metadata is CHANGE at the NumberInput root ID, source UNKNOWN and HAS_FRAME only.
The same nonzero `cui_frame_sequence` before/after reduction proves the current
ordinary completion frame, excluding passive/protected phases. No input index,
source or activation flag is inferred. This is not a physical-input audit log.

Callback/user are borrowed only during this call; no registration is retained.
The payload may be copied. Keep context, state, editor, composition resources,
owner value and user storage alive until the outer call returns and applicable
frame rendering consumes its borrowed data. No recursive submission, edit/state/
selection/composition mutation, rebind, destruction or exception unwinding through
C during notification. Updating unrelated application state is allowed.

The observer may assign a finite new owner value through its own captured pointer.
The event remains the frozen native before/after snapshot and does not recursively
notify. Current semantics and Styled paint retain the already-reduced draft;
the next submission notices the owner mismatch and synchronizes normally without
notification for that synchronization alone. No hidden post-callback draft rewrite
or `last_value` adjustment is performed. Nonfinite observer assignments are outside
the supported update contract and cause native validation failure next time.

Unit coverage compares Base/Styled facades with legacy passive/disabled/readonly
reduction, checks multi-step coalescing, Enter/blur normalization, finite precision,
real edit-capacity failure after owner mutation, policy call counts, owner updates
and ignored user data without a callback. Existing numeric tests remain unchanged.
Bridge NumberInput still uses its existing queued LEGACY payload; this C API does
not add typed bridge scalar events, bindings or full input-family parity.
