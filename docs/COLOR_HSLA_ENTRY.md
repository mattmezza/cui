# Float-HSLA inline color entry

`cui_color_hsla.h` supplies reusable normalized float HSL/encoded-RGBA conversions
and source-format hex operations. It depends only on Core declarations, and the
primitive implementation can link independently with the math library. This is
HSL lightness, not HSV brightness; encoded RGB is unpremultiplied and receives no
sRGB transfer-function conversion. It is separate from Plot's linear shader seam.

`cui_color_hsla_entry.h` adds a caller-owned float selection/preview/draft model
and a real Input field. Existing byte `cui_color_entry`, HSV `cui_color_picker`,
theme parsing, Plot and their event contracts remain unchanged. Handwritten
Python, Node/TypeScript, Go and Rust builders do not expose this new model.

Use normalized finite components in inclusive 0..1. Checked operations reject
NaN, infinity or out-of-range values atomically; they do not wrap hue, clamp,
snap to steps or discard latent hue on achromatic colors. Hue 1 is retained as
owner state, although it renders like hue 0. Optional absence differs from a
present transparent color. C float preserves source-scale precision, without
promising bit-identical arithmetic on every C platform.

Initialize a zeroed `cui_color_hsla_entry_state` once and retain its address. The
editor borrows its ten-byte owned draft; the state cannot be copied/moved, and its
members are read-only to callers. Destroy after the final draw list has been
consumed. Repeated destroy is safe. Init/sync/preview/restore/direct-value commit
format locally and allocate a fresh editor before publishing new float fields.
Allocation failure restores the old buffer/editor/history/model; success resets
Input state and undo history. Do not call these operations during field submission
or while prior draw data remains borrowed.

`cui_color_hsla_entry_sync` silently copies optional owner committed/preview values
and derives display text. Dirty draft conflicts unless replacement is explicit,
even for an equal owner. Preview replaces only preview/display; restore returns
to committed display. Both respect dirty replacement and leave committed unchanged.
Set-draft admits empty text or hash followed by up to eight ASCII hex digits;
complete text previews, incomplete text retains the last valid preview. Empty
text cannot commit or clear selection; silent owner sync to absent clears it.

The crucial precision distinction is explicit commit versus derived display:

- Sync, direct HSLA preview/commit, ordinary frames and theme changes retain the
  supplied float components. Formatting is never fed back through the hex parser.
- Hex parsing accepts optional hash and 3/4/6/8 digits, expands nibbles, divides
  bytes by 255, then converts float RGB to HSL. No CSS names/functions/decimal
  channel grammar is included. The UI additionally requires hash (or empty draft).
- Source-style formatting truncates channels and omits alpha only when opaque:
  float alpha `.5` displays `7F`. Explicit hex commit reparses even an unchanged
  formatted draft, so Enter on that display commits `127/255`, not `.5`.
  Direct `commit_value` retains `.5`. This deliberate distinction matches the
  frozen source's suppression of programmatic display feedback.

Conversions write output only on success. Hex parse writes only on COMPLETE;
NULL output validates. The formatter reports required payload bytes (7 or 9),
capacity includes NUL, and insufficient capacity leaves destination untouched.
NULL/0 probes length and returns CAPACITY; NULL/nonzero is invalid. Invalid
arguments report required zero. Inputs/outputs must be disjoint. No universal
byte-exact float hex round-trip or cross-platform GPU arithmetic claim is made.

Commit results distinguish SELECT (close requested) and UPDATE (no close request).
Both succeed for same-value commits. The inline field owns no popup, so a close
request is not a completed dismissal. The actual Input field handles focused,
accepted nonrepeated Enter as SELECT in queue order; consumed, IME-owned,
disabled, readonly, inert or layout-only paths do not commit. Invalid admission
is `input.rejected`; submitting admitted incomplete text increments the separate
rejected-commit count. Counts do not depend on installing an observer.

The outer observer receives one copied summary after Base semantics or styled
paint if at least one commit completed. Previous is the entry selection at the
start of submission; final committed may differ from preview if text follows
Enter in the same frame. Metadata is UNKNOWN with actual eligible frame only,
without invented source/index/activation facts. The payload is pointer-free and
can be copied beyond callback/context lifetime. Handler/user/options are entry
snapshots; borrowed label/user must survive return. Do not mutate/free live entry,
context or borrowed resources, or recursively submit UI, inside the callback.
Update a separate owner and sync before the next submission.

The executable owner example is
[`tests/unit/test_color_hsla_entry_owner.c`](../tests/unit/test_color_hsla_entry_owner.c).
It submits the real styled field, enters shorthand text and commits with Enter,
then applies a separately copied owner value on the next submission. The focused
fixture compiles and runs it, with C++ header compatibility and existing byte
ColorEntry/Picker/Picker Events/Input regressions. `test_color_hsla_model.c` uses
linker-wrapped real editor allocation failures to verify rollback at every editor
allocation budget; no public allocator test hook is introduced. The primitive
test links without CUI implementations to prove independent reuse.

Configure `tests/fixtures/color_hsla` with a matching `-DCUI_LIBRARY=/path/libcui.a`
and a private Release build directory. The failure fixture uses GNU/ELF linker
wrapping and is not a claim of Windows/macOS allocator-injection coverage. Build
with strict warnings, then run `python scripts/test_local.py --build-dir <dir>`
with a private home-cache TMPDIR. Assertions remain enabled in Release. The
semantic tests prove textbox naming/readonly/disabled facts and theme backgrounds,
not native AT announcements or hardware rendering.

This slice has no popup/open owner, palette/HSLA tabs, slider UI, native screen
sampler or foreign builders. The next popup coordinator must apply SELECT close
requests before its own outer notification, preserving source close-before-Change
timing; it cannot simply forward an early inner callback. Future sliders must
use direct float updates and avoid rewriting an active drag from formatted hex.
These remain separately reviewed work; full ColorPicker parity is not claimed.

Integrator guidance: register new Base sources `cui_color_hsla.c` and
`cui_color_hsla_entry.c`, and styled `src/components/cui_color_hsla_entry.c`.
Keep primitive tests linked only to the primitive implementation and `m`; field
tests and owner example link CUI. The failure-injection test additionally needs
`--wrap=malloc`, `--wrap=calloc`, `--wrap=realloc`. Embed guarded private
`src/base/cui_color_hsla_entry_internal.h` before both entry sources when generating
standalone headers; no private declarations belong in public headers. Canonical
source metadata, SDK/dist/skill generation and standalone/CUI_ALL verification
remain the integrator's next gate.
