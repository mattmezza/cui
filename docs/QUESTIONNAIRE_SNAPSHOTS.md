# Questionnaire snapshots

Include `cui_questionnaire_snapshots.h` and rebuild with the matching library.
These pure Base helpers copy effective answers into caller-owned item records,
choice descriptors and text storage. They do not validate, submit, call handlers,
synchronize an editor draft, service pending confirmation or change the model.
They do not provide event observation or freeze a legacy callback sequence.

Measure reports item/choice counts and text bytes including one NUL per copied
string. Copy always measures again, then checks every capacity before writing to
any workspace region. CAPACITY returns exact current requirements and an empty
new output while preserving all prior workspace bytes. INVALID/UNKNOWN_ITEM/
OVERFLOW clear requirements and output. Null mandatory output pointers are INVALID.
No truncation occurs, including when a positive text capacity is insufficient.

All supplied workspace regions, required/output records and model/schema/editor
storage must be mutually disjoint. This is a caller precondition; arbitrary pointer
overlap/provenance is not detected. No concurrent model mutation is allowed during
a call. Successful workspace reuse invalidates older snapshots; failed copies do
not. A structure copy alone does not clone its pointed-to strings/arrays, and a
memcpy-relocated arena does not automatically rebase them.

## Caller-owned retained copy

This example retains a submission independently of subsequent model edits and
source definitions. The bounded application workspace may be too small; the result
and measured requirements tell the caller that explicitly. The saved object must
stay at the same address while its snapshot is in use.

```c
#include "cui/cui_questionnaire_snapshots.h"
typedef struct {
    cui_questionnaire_item_snapshot items[16];
    cui_str choices[64];
    char text[4096];
    cui_questionnaire_submission_snapshot snapshot;
} saved_questionnaire;

cui_questionnaire_snapshot_result save_questionnaire(
    const cui_questionnaire_state *state, saved_questionnaire *saved,
    cui_questionnaire_snapshot_requirements *required) {
    if (!saved)
        return CUI_QUESTIONNAIRE_SNAPSHOT_INVALID;
    const cui_questionnaire_snapshot_workspace workspace = {
        .items = saved->items, .item_capacity = 16,
        .choices = saved->choices, .choice_capacity = 64,
        .text = saved->text, .text_capacity = sizeof saved->text
    };
    return cui_questionnaire_submission_copy(state, workspace, required, &saved->snapshot);
}
```

For one item use `answer_measure`/`answer_copy`, with at least one workspace item
record and a separate `cui_questionnaire_item_snapshot` output. That record includes
a copied identity name, original schema index, status, disabled flag and answer.
Individual disabled items retain name/status but expose an empty answer. Submission
copies omit disabled items, include unanswered and skipped enabled items, and use
schema order. They are available before validation/completion; copying a submission
does not certify that it is valid.

Selected enabled choice values use definition order, not selection order. Labels,
descriptions and validation messages are not included. Freeform comes from the
retained effective answer; an unsynchronized editor draft is excluded. Whitespace
normalization remains the model's responsibility. `has_freeform=false` has a NULL
zero-length string; zero choices/submissions have NULL array pointers.

Text is packed as each name, its choices, then present freeform, with an appended
NUL for each string. cui_str lengths exclude appended NUL; empty names/values
still consume one byte. The current model rejects embedded NUL in its UTF-8
validation, so initialized valid models cannot supply embedded-NUL values.
Use length-aware consumers. There is no deduplication or borrowed schema string in
a successful snapshot. Buffers survive model reset, edits and source teardown for
as long as the caller keeps the snapshot workspace alive and unchanged.

`query_copy` needs no workspace. It reports enabled progress, current schema index,
action visibility, confirmability and completion. No current enabled item gives
SIZE_MAX item/position, progress zero and false action/confirmability flags. A
filled invalid answer remains confirmable; this flag means answered/skipped, not
validated. Visibility reflects model policy, not disabled ancestors or host-omitted
buttons. Failed query resets the same sentinels.

Existing model bounds remain: 4096 items and default4096/max1MiB input answer bytes.
Snapshot counts/string totals and record extents use checked size_t arithmetic,
without a new fixed choice/string cap. The arithmetic test probes private helpers
at SIZE_MAX boundaries without forging a publicly valid model or reading fictitious
string storage. Portable C cannot validate arbitrary source pointer provenance.

Worker validation includes strict Release headless model/Styled regressions,
focused snapshot tests, compiled C example and C++ header, plus focused WASI. Pure
copy-out has no pixels; actual WASM execution verifies portable buffer behavior.
No generic bridge or binding API is added. Snapshot query/copy support does not
complete Questionnaire's event payload, compound presentation, localization, Form
semantic or broader style gaps.
