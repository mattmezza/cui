# Single-line masked Input

This implements the pinned `87d10ae5e1299d1be18670a37570c5402f952e34`
Input password slice, together with the checked engine Stage 1. It does not
complete Input parity: regex, mask formatting, and other content types remain
separate. Multiline masking/password purpose and inline token content are
explicitly unsupported; invalid options return INVALID_ARGUMENT before reduction.
Ordinary multiline Input/Editor/Textarea host input remains available.

```c
cui_input_options options = {
    .masked = true,
    .content_type = CUI_TEXT_INPUT_PURPOSE_PASSWORD,
    .normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF,
};
cui_input_result result = cui_input_ex(ui, id, theme, label, edit, state, options);
```

Zero options preserve ordinary Input behavior. Masking and purpose are independent:
masked alone paints bullets and denies Copy/Cut, but keeps the Textbox accessibility
role. PASSWORD/NEW_PASSWORD purpose marks protected accessibility state even when
revealed. Reveal (`masked=false`) deliberately paints the actual value and allows
the established clipboard callback. Masked Copy/Cut are denied before calling that
callback; Paste still follows editability, normalization and acceptance. Word
motion/selection/delete uses whole-value endpoints while masked; double/triple
click selects all. Scalar/grapheme movement retains the existing engine policy.

The mask is one U+2022 per Unicode scalar, including combining marks, matching the
source. Selection, caret, pointer hit testing, horizontal scroll and IME underlines
map original UTF8 byte endpoints to/from three-byte bullet endpoints. The geometry
factory and text draw commands receive only bullet text in masked mode. Entering
masked mode releases previous plaintext geometry sessions and their text copies.
Painting failures never fall back to plaintext. Public placeholder/label strings
remain the application's responsibility; never put a secret in either.

## Hard resource staging

The styled control retains three display/geometry slots: safe committed value,
current visible preedit, and unused candidate stage. Its initial/current seed is
prepared with CUI_EDIT_REQUEST_PRESENT before reducing events. For replacement,
normalized IME UPDATE/COMMIT, Cut/delete and Undo/Redo, the engine prepares the
prospective whole value before document/history/selection writes and before the
acceptance predicate. Preparation failure invokes no predicate and leaves logical
model/session state unchanged. Acceptance rejection discards the prepared stage;
it does not publish it or replay the predicate. Empty values still require a safe
empty presentation. Undo/Redo prepares its historical target without predicate or
normalization replay.

Default slots lazily grow to actual mask bytes plus NUL, bounded by the model's
caller-chosen document capacity. Three display slots, geometry copies and host
sessions remain owned by the UI resource. Idle frames reuse them. After changing a font registry or measurement policy,
call `cui_text_metrics_changed(ui)` outside the frame to invalidate retained
geometry; ordinary control size changes also trigger preparation. An optional
`cui_input_mask_allocator` controls display-buffer allocation only; geometry text
copies and factory sessions keep their existing ownership. Both callbacks must be
present or absent; successful allocation must return fresh disjoint storage. Its
user/callbacks must remain alive until context destruction/resource collection.

For allocation-free display storage, supply `cui_input_mask_storage` with three
disjoint buffers and their capacities including NUL. The smallest capacity is the
hard per-candidate budget. Buffers and their descriptor remain caller-owned and
must stay unchanged/alive through resource release. Accurate descriptor/workspace
extents are declared to the checked engine; known model/session/payload/workspace
aliases reject before preparation. Rebinding these resources drops their previous
owned sessions. Cancel composition and reset the Input state before destroying/replacing
its model; the reset state forces initial resource preparation even if an allocator
reuses the same model address. Reset the host proxy when replacing its bound model; a retained session's numeric model binding must never be reused across
model lifetimes. IME retains the existing one-primary-selection constraint; no
multi-cursor IME support is claimed. Retained resource callbacks never dereference a destroyed model.

If the optional visual factory exists, it must successfully prepare immutable
bullet geometry; failure returns PRESENTATION before committing the edit. Without
one, measured bullet geometry is the legacy fallback. Factory limits remain host
limits. There is no secret-safe rollback promise for direct application writes to
model buffers or raw engine edits outside this checked control. Completion is
caller-owned and outside engine rollback. Base exposes presentation/prepared/applied
completion so a custom frontend can implement the same three-slot publication;
Base alone does not paint a mask for it.

Engine PRESENT is nonmutating and ignores editability. NULL/inactive composition
prepares committed text; an inactive previously-bound session must belong to this
model. Active composition must have a valid materialized composed view, matching
model revision/selection/binding and disjoint buffers; stale views reject before
callbacks. No predicate, normalization, session start/cancel, history or selection
mutation occurs. Successful preparation reports prepared/applied=true even though
the logical value stays unchanged. Known same-model mutation during preparation
fails the outer request. Caller buffer writes and unrelated callback side effects
remain outside that guard.

With a hard presentation callback, START prepares its prospective empty-preedit
view (the current selected range is replaced by empty text) before mutating the
session or coalescing state. It invokes no acceptance/normalization and requires
an empty START payload. Null presentation preserves the old START bypass exactly.
Failure preserves the prior session and safe committed view; success publishes
matching prefix/suffix bullets immediately. CANCEL needs no allocation because the
committed slot remains intact.
UPDATE rejection preserves prior visible preedit. A rejected or resource-failed host COMMIT first
preserves engine logical state, then Base cancels the session because the host IME
has ended. Mode/purpose changes, readonly/disabled and focus loss cancel composition.
Following typing/shortcuts remain available. Ordinary zero configuration is unchanged.

## Finalized host configuration

Core owns portable type/purpose enums, `cui_set_text_input_config` and
`cui_get_text_input_config`. Register against an already-submitted textbox during
the frame. The getter is available only after end_frame and validates the actual
eligible focused owner, current modal scope, readonly/disabled and no-focus state.
Failure leaves output unchanged. It borrows no strings/models/platform pointers.
Missing explicit registration yields ordinary TEXT/UNSPECIFIED for eligible
textboxes, including existing Editor/Textarea/Numeric/OTP and text children.
Finalized `multiline` selects ordinary textarea behavior. Explicit secure types or
purposes reject multiline atomically. Generation changes on owner/config/explicit
versus implicit/multiline/removal transitions; UINT64_MAX exhaustion fails closed
until context recreation. Host reset must discard old proxy/preedit when it changes.
An application replacing its Core context must explicitly reset/dispose the bridge.

SDL poll consumes the finalized state, stops input for absent/ineligible focus and
restarts on generation changes, cancelling the old tracked IME session and dropping
text/editing events already queued for that superseded host configuration. It uses
SDL_StartTextInputWithProperties, TEXT_PASSWORD_HIDDEN/VISIBLE and disabled
AUTOCORRECT for secure purposes, plus MULTILINE for ordinary multiline text. These
are verified against repository SDL pin
[`7f3ae3d57459e59943a4ecfefc8f6277ec6bf540` keyboard header](https://github.com/libsdl-org/SDL/blob/7f3ae3d57459e59943a4ecfefc8f6277ec6bf540/include/SDL3/SDL_keyboard.h).
They exist since SDL3.2; SDL explicitly does not promise every input type on every
platform. NEW_PASSWORD cannot be distinguished from PASSWORD through this SDL
keyboard type. Physical OS keyboard suggestions/IME privacy are not proven by
injected events or Xvfb. SDL's global queue is not a per-window multicast service.

The optional AccessKit bridge maps password purpose to PASSWORD_INPUT and observes
secret/protected changes. It remains phase 1: no editable text/value support or
full screen-reader claim. CUI semantic value is zero for secret Inputs.

## Browser module and proof

`web/secure_input.js` is a reusable ES module. Pass parent, UTF8 `text`,
`composition(kind, bytes, anchorByte, cursorByte, owner)`, `key(event,owner)`,
`focusLost(owner)` and `changed()` callbacks. Call `sync` after finalized Core frames
with `{owner,type,purpose,generation,multiline,label}` and logical caret rectangle.
Use opaque stable owner/generation values without lossy conversions. The module
never seeds the committed value into DOM. Hidden passwords use a password input,
revealed passwords use text input, and ordinary multiline uses textarea. Purpose
selects current-password/new-password autocomplete. It disables spelling/correction,
converts DOM UTF16 scalar endpoints to UTF8, suppresses the composition-end duplicate
input, and clears transient value/preedit on blur, generation rebind/reset/dispose.
Composing shortcuts yield to the browser IME. Native DOM selection represents the
range the browser exposes; no claim of every platform's internal IME clause range.
The module provides no async clipboard implementation or cross-platform virtual
keyboard guarantee. Root gallery integration is separate from this isolated fixture.

The standalone fixture uses actual CUI Core/styled Input compiled to WASM, with
real Chromium CDP imeSetComposition/insertText and actual canvas font measurement
and painting of Core draw commands. It checks identical pixels for distinct
equal-scalar-count passwords, selected Unicode START prefix/suffix, bullet draws,
unchanged preedit document, exactly-once commit/Undo, fixed-capacity host rejection
followed by ordinary typing, cancel, reveal/purpose, readonly, blur, and
generation-switched input/textarea. Run:

```sh
uv run tests/web/test_secure_input.py --output build/secure-proof --chromium /path/to/chrome
```

Headless tests: test_password_present, test_password_start, test_password_input, test_secure_focus,
test_secure_focus_generation and optional test_password_accessibility. Native font
proof: test_password_native (actual NotoSans bullets, plaintext cache teardown,
factory failure before writes); SDL proof: test_password_sdl3 (real SDL property
calls and queued Unicode editing/commit, Xvfb). Registration is integrator-owned.
Stage 2 distribution headers remain stale until integrator regeneration.
