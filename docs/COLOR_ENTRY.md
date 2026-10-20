# Validated inline color entry

Include `cui/cui_color_entry.h` for a bounded hex field with separate draft,
preview and committed selection. Existing `cui_color_picker` remains an inline
HSV/RGBA picker; its strict parser and aggregate events are unchanged.

Initialize a zeroed, caller-owned `cui_color_entry_state` once, retain its address,
and destroy it after the final frame's draw data has been consumed. The state owns
a ten-byte draft and an allocated editor. It must not be copied or moved after
initialization, and its editor/buffer fields must not be modified directly.

The complete compiling owner example is
[`tests/unit/test_color_entry_owner.c`](../tests/unit/test_color_entry_owner.c).
It submits the actual styled field, types a shorthand color, commits with Enter,
copies the selection into a separate owner model, and applies that model before
the following submission. Its callback never mutates the live entry. The focused
fixture compiles and executes the example with assertions enabled in Release.

Use `cui_color_entry_init` with `{.present = false}` for an empty selection;
transparent black is instead `{.present = true, .color = {0,0,0,0}}`. The pure
`cui_color_entry_parse` accepts an optional hash and 3/4/6/8 ASCII hex digits.
Shorthand duplicates each nibble; missing alpha is opaque. COMPLETE writes the
output; INCOMPLETE and INVALID preserve it. It accepts no CSS names, functions,
spaces or signed numbers. This does not expand legacy `cui_color_parse_hex`.

The real field admits empty text or a hash followed by up to eight hex digits.
It rejects other edits atomically using Input's existing acceptance mechanism.
A complete draft updates preview only. An incomplete draft retains the previous
valid preview and committed value. Empty text cannot commit and does not clear a
selection. Enter commits only complete drafts, including a same-value commit.
The field inherits Input's focused, nonrepeated Enter routing; consumed, unfocused,
IME-owned, disabled, readonly or passive events do not create commits. It does not
close a popup. Standard Input pointer hit testing, caret, selection and textbox
semantics are used by the styled facade. Base supplies semantics and reduction;
the caller supplies its layout, as with Base Input.

`cui_color_entry_result` reports draft changes, status, previous/final selection,
preview, completed commit count and rejected incomplete-submit count. Internal
Input notifications preserve queue order, including text after a commit in the
same frame. `input.rejected` concerns draft admission; `rejected_commit_count`
concerns submitting an admitted but incomplete draft. Counts do not depend on
installing an observer. Multiple completed commits produce one external observer
call after final semantics (Base) or paint (styled), carrying the final committed
value. This summary is UNKNOWN source with actual frame metadata, without a
fabricated event index or input multiplicity. Copy the pointer-free payload to
retain it. Do not mutate/destroy entry, context or borrowed submission resources,
or recursively submit UI, inside its handler.

`cui_color_entry_sync` silently replaces committed/preview and canonicalizes the
draft (uppercase, alpha only when nonopaque). A dirty draft yields CONFLICT unless
`replace_draft` is true. Success recreates the editor with empty undo history;
allocation failure restores the old draft and leaves the old editor/model intact.
Call only outside submissions, after prior draw consumption. Owner sync to absent
clears selection and draft. `cui_color_entry_restore` discards draft/preview and
restores the committed value by the same silent replacement path. The pure
set-draft and commit operations do not notify UI observers.

Theme changes affect field paint, not selection or draft. Incomplete text is not
automatically presented as a source-defined validation error: the frozen source
has no separate error/status taxonomy. Headless tests verify textbox names,
readonly/disabled facts and actual theme backgrounds, not native AT announcements.

This first slice stores byte RGBA. Frozen upstream keeps floating HSLA and uses
truncating automatic hex formatting; half float alpha can format as `7F`, while
byte alpha 128 formats as `80`. Exact float-HSLA precision, popup focus/actions,
palette/HSLA tabs, featured theme palettes and optional trigger presentation
remain next-review work. Screen sampling remains a host async capability; this
API adds no native eye-dropper. Python, Node/TypeScript, Go and Rust builders do
not yet expose this entry model. No full ColorPicker parity is claimed.

For focused local verification, configure `tests/fixtures/color_entry` with
`-DCUI_LIBRARY=/absolute/path/to/matching/libcui.a` and a private Release build
directory, then use `python scripts/test_local.py --build-dir <directory>`.
The fixture enables assertions and strict warnings and also runs existing picker,
picker-event and Input regressions. Set TMPDIR to a private home-cache directory
on hosts with limited `/tmp` quota. It does not install dependencies or use the
maintainer's desktop.

Integrator registration: add `src/base/cui_color_entry.c` and
`src/components/cui_color_entry.c` to the CUI library, then the three new test
executables from `tests/unit/test_color_entry*.{c,cpp}`, linked to `cui` (C++17
for the header check). No extra platform library is needed. Existing picker and
Input regressions in the private fixture link the supplied matching static CUI
library. For standalone amalgamation, embed the guarded private declaration block
from `src/base/cui_color_entry_internal.h` before both new sources and remove their
relative internal-header includes; the private block's public-header include is
handled like other CUI includes. Keep private declarations out of the public
header. All added private functions/types use the ColorEntry prefix.
