# IME composition

CUI supports a reusable single-selection composition session for Input, Textarea,
Editor and Questionnaire input controls. Native SDL and the web gallery transport
preedit updates, the selected preedit range, commit and cancellation. Preedit is
rendered with an underline and selected range; it does not modify the edit document
or emit document-change callbacks until commit. Commit is one undo step when edit
history is enabled. Cancellation leaves document, original selection and history
intact. Starting composition separates typing groups on either side.

## Public byte offsets and bounded ownership

`CUI_EVENT_COMPOSITION_START`, `UPDATE`, `COMMIT` and `CANCEL` use `text_data` and
`text_length` for arbitrary-length UTF-8 within configured bounds. Legacy inline
`text[64]` remains available. `composition_anchor` and `composition_cursor` are
UTF-8 byte offsets in preedit, not offsets in the document. They must lie on scalar
boundaries. `cui_queue_event` deep copies payload bytes before returning; caller
storage can immediately be changed or released. Dispatched payloads remain valid
until the next `cui_begin_frame`, including while queuing that next frame.
`cui_config.event_text_bytes` bounds aggregate queued bytes (zero defaults to
65536). Queue overflow/allocation failure returns false and reports
`CUI_ERROR_CAPACITY`, without partially enqueueing an event.

`cui_edit_composition_event` uses a caller-owned `cui_edit_composition` with separate
preedit and rendered snapshot buffers. Buffers must remain alive and disjoint from
the model and session. Set `buffer`/`capacity`, `view_buffer`/`view_capacity`, and
zero the remaining fields before first use. Capacities include a trailing NUL.
Attach its pointer to `cui_input_state.behavior.composition` or
`cui_textarea_state.composition` (Editor uses `editor.view`). Leave
`composition_owned` false for caller-owned storage.

Built-in controls lazily allocate a retained default resource when composition
first arrives, bounded to 4095 preedit bytes. Styled Input, Textarea, Editor and
Questionnaire enable `grow_view`: their edit model privately owns rendered storage
sized from the actual document plus preedit and grows geometrically when needed.
There is no 65535-byte document limit for these built-in consumers. Growth is
bounded by the model's configured document capacity plus bounded preedit capacity;
overflow or allocation failure returns `CUI_EDIT_CAPACITY` before changing the
previous visible preedit or document. Snapshots are built only on successful
composition updates; idle view reads reuse the existing snapshot.

The default UI resource remains included in `resource_bytes`; dynamic rendered
storage is owned by the edit model and freed by `cui_edit_destroy`, independently
of UI resource collection. Its borrowed view is valid until the next composition
update/growth or model destruction. The session's original `view_buffer` remains
caller/UI-resource owned and never points at freed model storage. A retained
session rebound to a replacement model discards its old active composition.
Keep session and preedit buffers alive during use, and do not use borrowed views
after model destruction. Caller-owned sessions retain their explicit fixed
rendered-buffer bound unless they opt into `grow_view`; this flag also allows an
initially null rendered buffer. Applications needing larger preedit must attach
appropriately sized caller buffers. Fixed rendered capacity failure likewise
preserves composition and document. No adapter truncates text to 63 bytes.
A reusable headless consumer can call `cui_edit_composition_view` and
`cui_edit_composition_selection` to paint the temporary document and caret.

`composition_target` optionally identifies the originating focused control; the
core drops stale host updates/commits addressed to another focus. SDL captures
this owner and rejects text input between window focus loss and regain; the
browser rejects composition commits after a canceled session. Untagged manually
queued events use the current focus.

Read-only, disabled, unfocused and removed controls cancel their session. External
document revision or selection changes invalidate the old replacement range.
Keys and ordinary text events yield while composition is active. Pointer selection
cancels preedit before moving the document selection. Multiple active selections
reject composition with `CUI_EDIT_INVALID_ARGUMENT`; CUI does not claim multi-cursor
IME parity. Token annotations and folds are temporarily excluded from preedit layout;
committing retains the model's normal atomic token replacement policies.

## Host adapters and validation

`cui_set_text_input_rect` / `cui_text_input_rect` expose the last painted focused
caret in logical pixels. SDL polling forwards this rectangle to
`SDL_SetTextInputArea`; browser input positions its native textarea at this caret.
SDL `TEXT_EDITING` selection offsets are Unicode scalar counts: the pinned SDL
source converts Cocoa UTF-16 to UTF-32 counts and Wayland byte positions with
`SDL_utf8strnlen`. CUI converts those counts to UTF-8 bytes. Provenance:
[SDL Cocoa keyboard](https://github.com/libsdl-org/SDL/blob/7f3ae3d57459e59943a4ecfefc8f6277ec6bf540/src/video/cocoa/SDL_cocoakeyboard.m),
[SDL Wayland events](https://github.com/libsdl-org/SDL/blob/7f3ae3d57459e59943a4ecfefc8f6277ec6bf540/src/video/wayland/SDL_waylandevents.c).
The browser bridge converts native textarea UTF-16 selection offsets to UTF-8
bytes, commits `compositionend` once and excludes composing key shortcuts.
The host receives the preceding frame's caret rectangle; candidate presentation
and virtual keyboard behavior remain browser/OS dependent.

Numeric uses an optional `cui_number_state.composition` caller session or a lazy
UI-owned default with 4095 preedit bytes and model-owned rendered storage. It
renders arbitrary valid preedit without changing the numeric draft/value. Commit
uses the existing normalization (ASCII plus the already-supported fullwidth
digits/signs/decimal point and ideographic decimal point), existing prefix grammar
and 127-byte draft bound. Invalid commit leaves the previous preedit and draft
intact. One successful composition commit is one edit history transaction;
Ctrl/Meta-Z, Shift-Z and Y support Undo/Redo. Existing Enter/blur clamping remains
in effect. Read-only/disabled/focus loss and controlled-source or edit-model
replacement cancel composition, and stale commits remain suppressed until a new
start/update. Caller sessions keep the reusable edit-session lifetime rules above.

OTP uses `cui_otp_state.composition`, optionally pointing to a zero-initialized
caller-owned `cui_otp_composition` with `buffer`/`capacity` (including NUL). Keep
both alive and disjoint from the OTP value/state during use. Otherwise it lazily
retains a UI-owned default with 4095 preedit bytes, accounted in `resource_bytes`.
The default pointer is internal to the UI resource lifetime; do not dereference
it after UI destruction or retain it outside the widget state. Each frame renews
the resource, so omitted-widget collection cannot leave a dereferenced old
resource. Caller-owned sessions and buffers remain caller-owned.

OTP deliberately preserves its existing first-scalar filtering: only the first
ASCII or fullwidth digit in a commit is normalized and appended. Trailing scalars
are ignored (`"34"` appends `3`, `"2x"` appends `2`); an invalid first scalar
(`"x3"`) is ignored. Multi-digit composition commits are therefore limited to one
digit, as in the pinned reference `crates/base/src/otp_input.rs`, `edit_value`
(`chars().next()`). Preedit paints only the first eligible proposed scalar in the
next cell; masked preedit reuses the existing `*` rendering and never draws raw
preedit digits. Selected ranges use UTF-8 byte boundaries; the cell caret is
clamped to that proposed scalar and published for the host candidate rectangle.
Full OTP targets ignore further input. Buffer exhaustion reports `CUI_EDIT_CAPACITY`
before any append/callback and preserves preedit. A successful append reports one
change and, when it fills the target, one completion. OTP has no Undo history.
Cancel, disabled/focus loss, and document pointer/length/content fingerprint
changes discard preedit without modifying the value. Normal composing shortcuts
yield; stale commits after source replacement or a successful commit are ignored
until the next start/update. OTP has no read-only option.

Reference behavior was inspected in the frozen GPUI source
`crates/base/src/input/base/state.rs` (`marked_text_range`, `unmark_text`,
`replace_text_in_range`, `replace_and_mark_text_in_range`). GPUI may mutate marked
text transactionally; CUI's explicit virtual snapshot keeps document mutation at
commit. This is substantive single-selection input support, not full SDK parity
with surrounding-text reconversion, platform replacement ranges or candidate UI.

Headless unit tests cover Unicode/emoji ranges, queue deep-copy lifetime, bounded
failure, large caller buffers, cancellation, external drift, single undo and
multiple-selection rejection. Large-document tests cover more than 200000 bytes,
owned allocation growth, idle reuse, scrolling, commit/Undo and model rebinding. `tests/web/test_ime.py` uses Chromium CDP native
`Input.imeSetComposition` and `Input.insertText` for all four text consumers;
it also checks read-only and blur/disabled cancellation.
`tests/web/test_ime_additional.py` covers Numeric/OTP/Combobox through real CDP
composition, Numeric Undo, masked OTP first-scalar commit, cancel and disable.
Native headless Numeric/OTP tests cover filtering, trailing payload, callback
counts, caller capacity, source replacement and stale-focus rejection. SDL injected preedit
checks scalar-to-byte conversion with an emoji and an accented scalar. Linux and
Chromium are exercised here; macOS/Windows native IME UI remains unverified.
