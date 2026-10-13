# Password Input proposal, next28

Production audit only, CUI base `ae8d4a1d766f9b3d9e98720a870d419b81bea62c`.
Reference checkout reports GPUI Kit `87d10ae5e1299d1be18670a37570c5402f952e34`. No production files or tests
changed. This proposal does not claim implemented password support or full Input
parity. Source `masked` is an optional presentation capability; its privacy
behaviors are mandatory when enabled. Password/NewPassword content type is an
independent optional semantic/native hint.

## Confirmed source requirements and CUI gaps

### scalar mask and geometry

U+2022 once per Unicode scalar, not grapheme or byte; map original byte endpoints to scalar ordinal ×3, inverse hit mapping back to original bytes. Caret/selection/IME underline/scroll all use the masked display geometry.

CUI: Input paint and cached hit acquire actual committed/composed text. Existing optional text_geometry cache deep-copies plaintext and caps acquisition at65535bytes; fallback also measures plaintext. No mask/display mapping exists.

Evidence: `crates/base/src/input/mod.rs:11; base/src/input/base/element.rs:343–362,603–606,2548–2553; base/src/input/base/state.rs:3165–3173`.

### word and click privacy

Masked previous-word target0, next-word targetvalue.length; modified movement/selection/deletion use those edges. Double-click selects entire value; ordinary grapheme editing remains.

CUI: Input reducer ignores existing event.click_count and invokes ordinary Unicode-word engine operations. Core click_count already exists, so no event expansion is needed.

Evidence: `crates/base/src/input/base/state.rs:1472–1501,6938–6975; base/src/input/base/selection.rs:24–35`.

### clipboard

Copy and Cut disabled when masked, including readonly and context-menu capability. Paste remains editable and validated; reveal restores ordinary copying.

CUI: Input reducer invokes clipboard_set for any nonempty selection. No masked capability or control context menu API. Clipboard denial must occur before invoking host callback, not merely reject deletion afterward.

Evidence: `crates/base/src/input/base/mod.rs:105–111; base/src/input/base/state.rs:632–634,2657–2677,6900–6935`.

### secret semantics distinct from rendering

Masked alone suppresses accessibility value but does not imply source PasswordInput role. Password/NewPassword content type selects password role and suppresses value even while unmasked. Content type is a hint, not an automatic rendering/validation change. Native hint source is macOS-only.

CUI: CUI semantic details lack secret/password purpose; Base exposes numeric UTF8 byte length, not string value. AccessKit phase1 has no text/value implementation and uses generic TEXT_INPUT. Browser semantic mirror is generic textbox. No reusable Input purpose metadata exists.

Evidence: `crates/component/src/input/input.rs:27–96,326–334,683–689; component/src/input/content_type.rs:7–98,157–174`.

### IME remains supported

Preedit and composition underline are masked and remapped; model validation sees real values; commit and Undo retain real values. Suppress plaintext presentation/decorations, not composition dispatch.

CUI: Composed Input currently paints actual preedit and computes caret from actual value. Native candidate rectangle is already available but follows that actual geometry. Browser invisible textarea temporarily holds host preedit as plaintext and is not a secure password control.

Evidence: `crates/base/src/input/base/element.rs:355–364,4420–4454; base/src/input/base/state.rs:4243–4280`.

### compatibility and lifetime

Independent reveal flag changes presentation/capabilities only, not document/history. Predicate sees normalized real candidate; checked rejection and host-end cancel keep their contracts.

CUI: Mask state/cache/session ownership and failure policy are absent. Large values must not silently fall back to cleartext on mask capacity/allocation/factory errors.

Evidence: `crates/base/src/input/base/state.rs:10526–10536; CUI edit acceptance/normalization guides`.

Primary pinned source: [state](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/input/base/state.rs), [element](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/input/base/element.rs), [styled semantics](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/input/input.rs).

## Smallest complete slice

Implement masked **single-line** Input plus Password/NewPassword purpose metadata
and secure host bridge, as one coordinated slice. Explicitly reject a masked
multiline model or inline-token content before drawing/reducing, rather than
pretending the ordinary multiline fallback is safe. Source shares masking across
its modes; that broader multiline behavior remains unsupported in this slice.
Plaintext mode, including existing Textarea/Editor behavior, stays unchanged.
A reveal action is application-owned initially; a built-in eye button is optional
story/component decoration, not required for the mask contract. Content-type
metadata alone must not turn masking on.

Proposed exact portable declarations (names pending owner review):

```c
typedef enum {
    CUI_INPUT_CONTENT_UNSPECIFIED = 0,
    CUI_INPUT_CONTENT_PASSWORD,
    CUI_INPUT_CONTENT_NEW_PASSWORD
} cui_input_content_type;
typedef struct { char *buffers[3]; size_t capacities[3]; } cui_input_mask_storage;
/* Append to Base/styled Input options: */
bool masked;
cui_input_content_type content_type;
/* Styled only; NULL uses lazy UI-owned display cache. */
cui_input_mask_storage *mask_storage;
/* Append to Core semantic details: */
bool secret; /* no value exposure; independent of password role */
bool password; /* secure single-line role hint */
```

No general 45-variant content-type promise: the first purpose enum covers only
these two source password variants. Secret is `masked || purpose != UNSPECIFIED`;
password role is purpose-dependent. Base semantic numeric value must be zero for
secret inputs so UTF8 byte length is not accidentally exported as a value. A
future string-value provider must honor the same flag. Do not put real value in
semantic name/description; application labels remain caller-owned and must not be
constructed from secrets. Existing AccessKit snapshots deep-copy strings; use
secret/password metadata to map protected role and omit future values. Core frame
metadata must append fields with zero defaults, preserving old role/event ordinals.

Proposed pure helpers in the editing module:

```c
cui_edit_result cui_edit_mask_text(cui_str source, char *destination,
    size_t capacity, cui_str *display);
cui_edit_result cui_edit_mask_offset(cui_str source, size_t source_byte,
    size_t *display_byte);
cui_edit_result cui_edit_unmask_offset(cui_str source, size_t display_byte,
    size_t *source_byte);
```

Validate original UTF8, half-open scalar endpoints, display bullet boundaries,
scalar-count multiplication overflow and all known source/destination/output
aliases before writes. All output failure is atomic; display has NUL and borrows
destination. No hidden helper allocation. Caller-owned mask storage is disjoint
from model/session/output storage, never freed by UI/model. Opaque model-aware
alias extent checks must precede writes; the generic helper alone cannot detect a
caller buffer overlapping unseen model/history memory. Return CAPACITY on fixed
storage shortage and keep previous valid presentation; never paint/measure/hit-test
real text as a fallback. Model document, selection, history and session state stay
unchanged on mask preparation failure.

Default styled cache is a separate UI resource, not model ownership. Lazily size
from actual composed scalar count ×3 +NUL with checked arithmetic; retain storage
on idle frames and refresh on model identity/revision, composition generation/text,
mode and host font/factory changes. Existing native geometry cache receives only
bullet display bytes in masked mode, never actual text. Clear its old plaintext
session/copy when toggling to masked; no stale reference across model rebinding or
cache release. Caller buffer rebinding releases only owned resources. The current
65535 native geometry cap must either gain a bounded larger-session path or use
bullet-based fallback: it must never fall back to real text. Rendering requires
frame-owned mask bytes or draw-list deep copy through the normal text ownership
contract, not stack storage. Existing geometry sessions may own/display-cache
borrowed pointers; destruction ordering must release sessions before their bytes.

Map original committed or composed selection and IME ranges into display offsets
before native factory calls; map host hit results back before engine grapheme/token
snapping. Original grapheme navigation remains authoritative: combining marks get
multiple bullets but arrow/backspace still act on the source grapheme. RTL actual
text must not influence masked horizontal ordering, bidi levels or scroll width;
shape only bullets. Alignment, scroll limit and candidate caret rectangle all
come from painted bullet geometry. Preedit rejection preserves prior masked view.
Word modifiers route masked targets to document edges; word deletion still uses
checked acceptance, copy-first cut is bypassed entirely because cut is unavailable.
Double-click via existing click_count selects all; Shift/drag policy should remain
explicit and tested. No change to callback seeing normalized actual text or replay
bypassing acceptance/normalization.

## Host seam needed for complete delivery

Append a portable focused text-input configuration query/set seam to Core,
independent of Theme and platform types:

```c
typedef enum { CUI_TEXT_INPUT_TEXT = 0,
    CUI_TEXT_INPUT_PASSWORD_HIDDEN, CUI_TEXT_INPUT_PASSWORD_VISIBLE
} cui_text_input_type;
typedef struct {
    cui_text_input_type type;
    cui_text_input_purpose purpose; /* enum defined in Core; no reverse dependency */
} cui_text_input_config;
bool cui_set_text_input_config(cui_context *, cui_id owner, cui_text_input_config);
/* Final exact state/query and purpose declarations are specified below. */
```

Only the currently validated focused editable owner contributes config; reset
when absent/disabled/readonly, with frame lifetime and stable-ID/source guards.
Purpose and rendering remain independent. SDL adapter should use its optional
`SDL_StartTextInputWithProperties` password type and disable autocorrect; installed
primary SDL headers define hidden/visible password types and say not every type
is supported on every platform. Current adapter calls plain `SDL_StartTextInput`.
This observation is availability evidence, not a tested mobile/password guarantee.
Pin/check actual supported SDL build versions before implementation; handle errors
and older hosts explicitly rather than claiming secure OS keyboard support.

Browser must replace the generic plaintext textarea bridge for secret inputs with
a real secure password input, retain real composition events and exact UTF16→UTF8
conversion, clear transient value on end/cancel/blur/rebinding, and suppress
accidental duplicate semantic nodes/value exposure. Current/password-new purpose
can map autocomplete tokens; masked-only has no invented autofill purpose. An
unmasked canvas reveal with password purpose still must withhold accessibility
value; a text-type transient bridge can violate that contract, so secure bridge
presentation and canvas reveal need independent handling. Native/browser IME may
receive the real typed/preedit value from the OS: this slice cannot promise memory
erasure, concealment from the IME/process/developer tools, or physical candidate
window behavior. It does promise CUI draw/geometry/semantic/clipboard surfaces do
not expose unintended plaintext while masked.

## Acceptance evidence required before a completion claim

Actual composed headless/Core scenes must inspect every draw command and factory
argument for only bullets, including non-ASCII, combining marks, flags, ZWJ emoji,
RTL, empty/placeholder, selected ranges, long horizontally scrolled values and
active preedit. Exact scalar counts and byte maps must be asserted independently
from the implementation. Pixel/native-font test must show actual bullet glyphs,
caret/selection/underline alignment, inverse hit and painted candidate rectangle.
Test hostile fixed capacities, overflow, model aliases, cache rebind/destroy/reveal,
no idle reallocation and fail-closed behavior. Test word movement/select/delete,
double-click all, zero clipboard callback calls for Copy/Cut (also readonly),
Paste/acceptance/rejection/healing, rejected host commit→typing, one Undo and reveal
restoring copy. Required/invalid flags remain distinct from secret semantics.

Real pinned Chromium CDP must exercise native composition on secure bridge,
UTF16 emoji ranges, exactly-once commit/cancel, canvas bullets, DOM accessibility
snapshot with protected role and no secret string/value, no transient plaintext
textarea, focus transfer and reveal. Native SDL injected events and real-font
rendering can validate API/property/caret wiring; AT-SPI/AccessKit snapshot checks
must preserve its phase1 no-text/value limitation. Physical OS candidate window,
screen-reader speech, platform password managers/mobile keyboards require separate
runtime evidence and remain unclaimed. WASI and sanitizer suites cover portable
storage/reentry compatibility. No tests executed for this documentation-only batch;
prior normalization evidence is not presented as password validation.

Requested coordinated ownership for implementation: edit helpers/masked checked
word behavior; Input-only Base/Components reducer/geometry/paint/options; Core
secret/password details and focused config; optional AccessKit role mapping;
SDL type properties; web secure bridge plus a focused password fixture and CDP
suite; new unit/native-font tests and guide. Global CMake/dist registration remains
integrator-owned. Splitting host/semantic work from dots without keeping the
feature explicitly unavailable would be an incomplete password implementation.

## Revision 1: authoritative staging and focused-config contracts

This revision supersedes the earlier single-buffer storage/query sketches. It is
still proposal-only; Core production ownership is held by `touch_cursors_sol61`.
No production work is authorized until the integrator approves ownership after
that handoff.

### Hard presentation preparation, separate from acceptance

A post-reduction `malloc`/factory failure cannot preserve an already changed
model. Do not put mask capacity inside `validate`: old-invalid healing would then
allow an oversized candidate, and callback order/count would differ. Instead add
a synchronous **hard preflight** to checked engine requests, used before the
candidate acceptance predicate and before document/history/selection mutation.
The frontend stages only bullets and their geometry; the engine never takes
ownership of frontend sessions or foreign memory.

Proposed exact engine declarations:

```c
typedef struct { void *data; size_t bytes; } cui_edit_workspace;
typedef cui_edit_result (*cui_edit_prepare_fn)(void *user,
    cui_str candidate, bool provisional);
typedef struct {
    cui_edit_prepare_fn prepare;
    void *user;
    cui_edit_workspace caller_workspaces[3];
    size_t caller_workspace_count; /* 0..3; fixed buffers declare all */
} cui_edit_presentation;
/* Append to cui_edit_request: */
cui_edit_presentation presentation;
/* Append to cui_edit_checked_result: */
bool presentation_prepared;
bool presentation_applied;
```

`prepare` borrows the complete normalized real candidate synchronously. It is a
hard resource/representation check, not a value-acceptance predicate. Engine
same-model guards are active during it, including void destroy and cache getters;
attempted mutation makes the checked request fail before commit. Exact declared
caller workspace extents are validated before callbacks against engine/model/
history/selection/columns, current session/preedit/view buffers, the original
payload and each other. Known descriptor/user-state aliases must also be rejected
before writes. Direct writes to undeclared buffers/external callbacks still lie
outside the enforceable C borrowing contract; this seam must not promise global
UI reentry protection. No allocator or text factory is called by portable Core.

There is no engine-owned finish callback. `presentation_prepared` means successful
preflight produced frontend staging; `presentation_applied` means that candidate
became the visible logical value. On rejection/error the caller discards staging.
This avoids a fallible or model-mutating publication callback inside the engine.
A checked caller must consume/discard staging before the next request. Built-in
Input uses a private frontend publication function after each returned checked
result; publication performs pointer/session swaps and releases only, never
allocation, user validation or fallible shaping. Base Input event reduction needs
a narrow completion hook to its styled owner (exact public declaration below),
because several edits in one frame must publish/discard in event order, not once
at the frame end.

```c
typedef void (*cui_input_presentation_complete_fn)(void *user,
    bool prepared, bool applied);
/* Append to Base Input options (styled options expose storage, not hooks): */
cui_edit_presentation presentation;
cui_input_presentation_complete_fn presentation_complete;
void *presentation_user;
```

The completion hook runs *after* the checked engine call and is notification/
frontend ownership plumbing, outside that call's model transaction. User-written
hooks must not mutate/reenter the model or fail; the built-in hook is private,
reviewable, no-fail CUI code. Do not extend the engine's rollback promise over
arbitrary completion side effects. The Base helper must invoke it once for every
checked result with prepared staging, including rejection; raw host CANCEL still
clears session through the existing lifecycle path and invalidates frontend
provisional staging. Callback pointers/user data are borrowed for that request,
not persisted by the engine. No custom predicate is invoked while publishing.

Preparation points are explicit:

1. Replacement/type/paste/delete: validate original UTF8/range, normalize/count,
   check document capacity and declared workspace aliases, construct candidate,
   run preflight exactly once, then run candidate/old predicates with current
   source ordering. Commit through existing history/coalescing only after both
   gates pass. Empty candidates still require a representable empty mask even
   though acceptance bypasses its predicate.
2. IME UPDATE/COMMIT: original preedit cap and UTF8 endpoints first, normalized
   composed candidate next. Stage display bytes/geometry before candidate
   acceptance; commit has no additional preflight or predicate replay. Rejected
   UPDATE discards staged mask and preserves prior preedit/view/cache. Rejected
   host COMMIT then cancels the session; frontend publication must select the
   already prepared committed-view cache rather than allocate at that boundary.
3. Undo/Redo: preflight the recorded target full value **before** history restore
   or selection/coalescing mutation. Resource failure leaves replay state/model
   untouched. Replay bypasses acceptance/normalization, not the hard display
   resource gate. Caller-fixed display budget may prevent Undo of a larger value;
   return CAPACITY explicitly, never reveal it or silently drop history.
4. START/CANCEL/focus-loss/read-only transition: reserve/retain a committed-view
   mask cache before beginning composition, so ending it can select that cache
   with no allocation. Provisional updates use a separate staging slot; entering
   composition must not discard the committed-view resource. Initial submission,
   explicit reveal→masked transition and model rebinding prepare the current
   composed and committed representations before reducing events.

This requires **three owned cache slots** while composing: committed base,
currently visible preedit, and candidate staging. For fixed caller bytes the exact
storage contract therefore must include three disjoint equal-budget display
buffers, not the earlier two-buffer sketch:

```c
typedef struct {
    char *buffers[3];
    size_t capacities[3];
} cui_input_mask_storage;
```

The public engine declared workspace array/count is likewise three (0..3)
for this final contract. All three caller capacities include NUL; effective hard
budget is their minimum, so every candidate fits any role after a swap. Caller
struct fields remain unchanged; private frontend state tracks slot roles. Only
slots not referenced by live immutable geometry/draw sessions may be overwritten.
Release old staged factory/session references before reuse. On successful UPDATE,
swap candidate/visible while retaining committed; on COMMIT swap candidate into
committed and clear provisional. On CANCEL return to retained committed slot.
No allocation or shape operation occurs after model commit or during cancellation.
If capacity was changed by the caller during a retained session, reject it as an
invalid rebinding rather than use stale pointers. Buffers live until explicit
unbind/UI resource destruction, and remain caller-owned throughout.

Factory sessions may allocate independently even with fixed display bytes. Their
failure is caught during staging. Owned default slots lazily grow before commit,
with checked scalar×3 arithmetic and transactional replacement: retain all prior
slots/session pointers until the new stage is complete. Allocation failure returns
CAPACITY; host factory failure needs an appended `CUI_EDIT_PRESENTATION` result
rather than conflating unsupported/invalid shaping with a valid predicate rejection.
A missing native factory uses the existing *bullet-only* measured geometry path;
an installed factory returning failure is a hard staging error, not plaintext or
partially changed-model fallback. A valid measured fallback must itself be finite
and bounded. Presentation failure returns `rejected=false`: it is not validation.
No per-edit hidden engine allocation; frontend allocations are explicit owned
cache growth/session creation and tested separately.

On plain→masked toggle, release every plaintext geometry session before freeing
its copied text, clear affinities tied to plaintext widths, then prepare the current
mask. Failure renders no secret text, leaves the model unchanged and exposes a
presentation error; the old plaintext cache must not be painted or reused as a
fallback. Blank fail-closed initial/toggle presentation is allowed when no prior
safe mask exists. A previous safe masked display may remain on resource rejection,
but its caret/hit interaction cannot be associated with a changed value. Toggle
and cache failure tests must inspect factory destruction order and every draw call.

### Exact Core config lifecycle, no reverse dependency

Purpose is defined in Core, not the component header:

```c
typedef enum {
    CUI_TEXT_INPUT_PURPOSE_UNSPECIFIED = 0,
    CUI_TEXT_INPUT_PURPOSE_PASSWORD,
    CUI_TEXT_INPUT_PURPOSE_NEW_PASSWORD
} cui_text_input_purpose;
typedef struct {
    cui_text_input_type type;
    cui_text_input_purpose purpose;
} cui_text_input_config;
typedef struct {
    cui_id owner;
    cui_text_input_config config;
    uint64_t generation;
} cui_text_input_state;
bool cui_set_text_input_config(cui_context *, cui_id owner,
    cui_text_input_config);
bool cui_get_text_input_config(const cui_context *, cui_text_input_state *out);
/* Base/styled option uses the Core type directly: */
cui_text_input_purpose content_type;
```

Setter is frame-only and validates ID/enums before recording provisional metadata;
false leaves previous staged metadata unchanged. It cannot borrow a model/string/
platform pointer. Multiple setters for different controls are permitted, but only
the finalized validated focused owner is chosen at end_frame. Owner must be a
live single-line editable textbox in the active focus scope, not disabled, readonly,
no-focus, removed or modal-obscured. Readonly controls still focus/allow selection,
but never produce active host text config. The getter is valid after end_frame;
during a frame, null output, absent owner, changed current focus or invalid owner
returns false with output unchanged. It rechecks current focus against finalized
semantic metadata; no old frame's secure config may be applied to a different ID.
Each begin_frame clears pending candidates, and an omitted widget clears active
config at end_frame. Zero means ordinary TEXT/UNSPECIFIED, not password; a valid
ordinary editable textbox can explicitly publish that zero config.

Generation increments when finalized owner/config becomes different or disappears,
including remove/re-add of the same ID. It is an opaque host session stamp, not a
model pointer. At UINT64_MAX, an attempted configuration transition fails closed
(no active host config) until context recreation; do not allow generation reuse to
validate old async input. Getter false tells adapters to end/clear native/browser
text sessions and reset previously applied purpose/type. SDL should stop text
input for absent/readonly/disabled owner; a new ordinary owner explicitly restarts
ordinary settings. Browser clears transient input, cancels composition, releases
bridge focus and removes secure purpose metadata. App explicit reveal affects
canvas masking independently of purpose; secret semantic exposure stays disabled
when Password/NewPassword purpose remains set.

### Forced failures and integration ownership

Meaningful precommit tests must inject allocator failure at each owned mask slot
growth and factory create failure after allocation, with old fixed mask fitting but
new scalar count exceeding budget. Compare complete logical model/history/redo/
selection/coalescing/session ownership plus previous safe display bytes/terminator
and cached session identity. Assert user predicate is not called on hard failure;
invalid-old healing must not override capacity and predicates keep normal order
once preflight succeeds. Include typed/paste/delete/IME/Undo/Redo, empty value,
multiple edits per frame and cancellation after a larger provisional value. Assert
prepare once, complete once, no publication allocation, no predicate replay and
exact release counts. Include fixed-buffer aliases into model/history/current
view, descriptor overlap, forced fresh binding/toggle failure and stale factory
pointers after destruction. Sanitizers and caller allocator/factory spies must
prove no leak/double free and no overwritten live borrowed mask bytes.

Core tests need config before/after frame, late focus change, omitted target,
readonly/disabled transitions, modal obscuration, ordinary zero config, bad enum,
null output, same-ID removal/reappearance and generation saturation. Native/real
CDP tests must validate secure type/reset state and composition ownership; physical
OS password/candidate behavior remains unclaimed. No tests have been run for this
proposal-only revision.

Revised ownership request: edit.h/edit.c hard preflight/restore seams/helpers;
Input-only Base/Components APIs/reducers/cache/paint; Core header/source config and
secret metadata **only after touch worker handoff**; AccessKit protected mapping;
SDL config application; web secure fixture/bridge and CDP tests; new focused unit/
native-font failure tests/docs. CMake/generated distribution stay integrator-owned.
The Core-purpose definitions must land before frontend/adapter declarations use
them. Implementation approval remains pending.
