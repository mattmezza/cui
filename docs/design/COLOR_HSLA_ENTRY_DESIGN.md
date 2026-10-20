# High-precision ColorPicker state: design checkpoint

Design only, based on CUI `bffef3c6`. No declarations, production changes, source
metadata, generated files or completed-parity claims accompany this checkpoint.
This extends the decisions in `COLOR_PICKER_SOURCE_AUDIT.md`, without changing
the shipped byte-RGBA ColorEntry contract.

## Exact source evidence

Frozen GPUI Kit revision: `87d10ae5e1299d1be18670a37570c5402f952e34`.
Inspected `crates/base/src/color_picker.rs`, its embedded tests, the styled
`crates/component/src/color_picker.rs`, and Base Slider. The frozen workspace
pins `gpui-pre = 0.3.8`; Cargo.lock checksum is
`d7f0bc3ebba070f8981b3f8e905a12f47d27794a30d029e59cdb9edf7a4b3360`.
The existing cache `/home/matteo/.cache/cui-validation/gpui-pre-0.3.8.crate`
matches that checksum. Its `src/color.rs` supplies the actual Hsla/Rgba
conversions below. No dependency was installed or fetched.

`Hsla` has four `f32` members, normalized hue/saturation/lightness/alpha. The
`hsla` constructor clamps each member to 0..1; the public struct also permits
direct construction. Its equality compares components with `total_cmp`, so
source equality is not rendered-color equivalence. Hue 0 and 1 can represent
the same red while remaining different stored values; hue is meaningful state
even on an achromatic color. Do not erase it by a render-time RGB conversion.

HSL-to-RGB uses chroma `(1 - abs(2*l - 1))*s`, intermediate
`x = c*(1 - abs((h*6)%2 - 1))`, offset `m = l-c/2`, and six hue sectors
(sector 6 uses sector 0). RGB channels are clamped; alpha passes through.
RGB-to-HSL uses min/max/delta, lightness `(max+min)/2`, a lightness-dependent
saturation denominator, and hue sectors with positive remainder for red.
Achromatic RGB converts to hue zero. These are encoded RGB channel operations;
ColorPicker does not perform an sRGB transfer-function conversion here.

The picker hex parser accepts optional hash, exactly 3/4/6/8 ASCII hex digits,
nibble duplication, and default alpha 1. It first divides bytes by 255 as f32,
then converts float RGB into HSL. The actual input pattern requires hash but
admits incomplete drafts. No decimal HSL/RGB text grammar or arbitrary CSS parser
is implemented by this picker. The general GPUI Rgba parser is not its authority.

The picker formatter converts HSL to float RGB and truncates each `channel*255`
to an integer. It emits uppercase six digits when alpha >=1, otherwise eight.
The source test requires `.5` alpha to format as `7F`, not `80`. Near a byte
boundary, float arithmetic can affect truncation; exhaustive byte-exact round
trip claims would be inappropriate without measured proof.

## What precision preservation actually means

Source `update_value` writes the supplied Hsla directly to committed value,
preview and sliders. Its programmatic hex write suppresses the resulting Input
Change callback: that display must not become a new source color. Slider events
read the four slider values, assign the float color, format hex, emit Change,
and deliberately do not rewrite the active sliders. Thus a color such as
`{.h=.1234567, .s=.2345678, .l=.3456789, .a=.5}` survives synchronization and
ordinary rendering without an implicit byte round trip.

Explicit hex entry is different. `commit_hex` always parses the supplied string,
even if it is the unchanged programmatically formatted display. Pressing Enter
can therefore intentionally quantize that float color. For example displayed
alpha `7F` commits as `127/255`, rather than retaining `.5`. Enter on displayed
black can reset latent hue through RGB-to-HSL. A new API must not quietly make
unchanged-display Enter bypass parsing while calling that source parity.

Valid text Change previews and updates the sliders without committing. Invalid
or incomplete text leaves the last valid preview/committed values untouched.
Palette selection closes first, then synchronizes and emits Change; slider
updates emit without closing. Same-value explicit selection/update can emit.
Programmatic set/clear and initial default synchronization emit no Change.
`preview_color` changes preview/display; `clear_preview` restores committed
display. Source preview restoration is separate from rewriting the committed
selection. No general validation/veto engine is present.

## Existing CUI boundaries

`cui_color` is byte RGBA. `cui_hsv` uses hue degrees and brightness, not normalized
HSL lightness; its byte conversions round. `cui_color_entry_state` owns a bounded
draft/editor with optional byte committed/preview values. Its Enter commits bytes,
and its silent sync uses the existing byte formatter. All of these remain ABI
and behavior compatible. Do not reinterpret their members or add hidden precision
storage to the existing structs.

`cui_plot_hsla` already has normalized float components, but it is a Plot shader
seam, with `cui_plot_linear_rgba` and linear premultiplied raster output contracts.
It must not become a dependency of Base ColorPicker or imply an encoded/linear
color-space identity. `cui_theme_color_parse` supplies theme-oriented syntax and
byte output; it is not a precision-preserving entry model or source picker parser.

## Proposed additive API and acceptance decisions

Provisional dedicated header `cui_color_hsla_entry.h`, with prefixed new types:
`cui_color_hsla { float h,s,l,a; }`, explicit optional
`cui_color_hsla_value { bool present; cui_color_hsla color; }`, and an owned
`cui_color_hsla_entry_state`. These are portable C11 with no platform/Clay/Plot
types. Use C float to retain source-scale precision; do not promise identical
f32 bits on every C platform or compiler contraction setting. Stored accepted
components are copied directly, never reconstructed from their display.

All present input components must be finite and in inclusive 0..1. Reject invalid
values transactionally; no implicit wrapping, clamping, step snapping, hue
normalization, negative-zero canonicalization, or RGB conversion on acceptance.
This checked C boundary is explicitly stricter than calling GPUI's clamping
constructor or constructing its public struct directly. If clamping convenience
is later useful, it needs a separately named operation and tests. In particular,
keep hue 1 and latent hue on zero saturation/lightness.

Pure checked conversions expose normalized float encoded RGBA separately from
`cui_color` bytes. Named byte export is a lossy presentation boundary, not model
ownership: use source-style truncation for the source display/export helper;
leave legacy rounding conversion unchanged. Every conversion validates first and
writes outputs only on success. New source-style hex parse/format functions use
the picker grammar and float HSL conversion above. Formatter capacity includes
NUL; report required payload length and make insufficient-capacity output atomic
(unlike the legacy formatter's intentionally retained truncated-copy contract).
Inputs and output storage must be disjoint. No locale-sensitive parsing.

The owned state retains optional full-precision committed/preview values, ten-byte
hex draft, editor, input view state, draft validity and dirty state. Zero-init and
checked init; address-stable/noncopyable until idempotent destroy after render
consumption, as with ColorEntry. No foreign-owned string pointers persist. Init
and silent owner-sync copy the float selection directly and format its display
without invoking hex-preview callbacks. Dirty owner-sync conflicts unless explicit
replacement is requested; allocation/format failure preserves selection, draft,
editor identity, history and preview. Successful replacement resets input history.

Typed operations distinguish these paths:

- **Set/admit hex draft:** empty or hash plus 0..8 ASCII hex digits in the actual
  field; pure parser may omit hash. Complete text parses to float preview only.
  Incomplete retains previous preview; invalid admission is atomic rejection.
- **Commit hex draft:** parse current draft at that explicit operation, including
  unchanged display; complete result commits parsed float HSL. Same-value commits
  succeed/count. Incomplete never commits or requests close. Empty is incomplete,
  not an implicit clear selection.
- **Preview HSLA:** validate and retain supplied float preview, format display
  without feeding it back into parsing, and leave committed value unchanged.
  Dirty draft replacement requires an explicit replace-draft decision.
- **Restore preview:** restore committed optional value and display silently,
  without recomputing HSL through bytes. Distinguish preview restoration from
  owner-sync and from commit.
- **Commit HSLA:** accept the supplied float color directly, for a future palette
  or slider owner. Distinguish SELECT (request close) from UPDATE (keep open) in
  the typed outcome. Formatting is derived display only; active slider values
  must not be rewritten by a display-feedback path. Explicit absence/clear is a
  separate owner operation, not transparent black.

First implementation review must finalize names, return structs and the precise
transactional allocation order before declarations. Do not implement two competing
models by nesting byte ColorEntry and treating its bytes as authoritative. Reuse
existing Input primitives and private acceptance logic only where their contracts
fit; leave shipped ColorEntry source, structure and observer semantics unchanged.

## Events, acceptance and lifetime

Pure model operations return typed outcome/selection/close-request facts; they
create no fabricated frame or user-input metadata and call no UI observers. The
new real Input facade uses existing per-event Input CHANGE/SUBMIT notifications
internally, preserving queue order and explicit hex quantization. It does not
rescan raw Enter, accept consumed/IME-owned keys, or add dispatch/capture policy.
Disabled/readonly/passive frames cannot produce commit observations. Same-value
commits and multiple accepted submits count independently of handler presence;
incomplete attempts and admission rejection remain distinct outcomes.

For consistency with ColorEntry, the external hex-field observer is one copyable
summary after final semantics (Base) or paint (styled), with full-precision previous
and final selections and completed/rejected counts. UNKNOWN source and actual
eligible frame only; no invented input index or attribution of the whole summary
to one activation. This is not a claim to emit every upstream entity event.
Direct pure operations give their caller each result; future slider/palette UI
must preserve per-accepted-operation evidence before choosing an outer summary.

Freeze options and handler slots at submission entry. Payloads own scalar values;
handler/user, labels and palettes remain borrowed until outer return. Callbacks
can update a separate application owner for next submission, not mutate/free live
entry, descriptors, active sliders or context, and cannot recursively submit UI.
Teardown waits for draw consumption. Source close-before-Change timing belongs
to a future popup coordinator: an accepted SELECT outcome must apply close before
its outer notification. The model alone requests close and claims no actual popup.

## Staged implementation and required headless proof

1. Review and implement float primitives plus owned model and actual editable
   field. Compare source formulas/known vectors, not the legacy HSV converter.
   Test finite/range rejection (NaN/infinity/out-of-range), hue endpoints,
   achromatic latent hue, optional absence versus transparent, and preservation
   of supplied non-byte-aligned floats across sync, render, preview and restore.
   Formatting must not change those stored components or create commit callbacks.
2. Exercise all source hex widths/cases and malformed/incomplete/non-ASCII input;
   assert failure leaves outputs intact, capacities 0 through required+1 and
   guard bytes, exact opaque/translucent formatting, `.5 -> 7F`, and explicit
   unchanged-display Enter producing `127/255`. Check representative RGB/HSL
   sectors, black/white/grey, bounded conversion error and near-byte-boundary
   truncation. Do not assert universal bit-identical floating arithmetic or
   byte-exact parse/format round trips without evidence.
3. Test the actual field's pointer/caret/key routing and textbox name/readonly
   semantics, invalid admission, preview without commit, repeated/consumed Enter,
   same-value/multiple submits, text after Enter, disabled/readonly/passive frames,
   owner conflict/replacement/history behavior, copied payload lifetime and a real
   callback-driven next-submission owner example. Theme paint must leave floats
   and draft unchanged. Native AT announcements remain outside semantic snapshots.
4. Then review Palette/HSLA tabs: four normalized .01-step slider controls and
   source selected swatch semantics, with full-precision owner sync and no active
   slider feedback. Review popup trigger naming, focus, scoped Enter/Escape,
   disabled Confirm and open Cancel behavior, close ordering, and theme featured
   palettes separately before production. These must consume this float owner,
   not downgrade its selection through `cui_color` for state updates.

Use strict C11/C++ header consumers, assertion-enabled Release fixtures, actual
standalone and CUI_ALL compilation, and the invisible-Xvfb test wrapper with
home-cache TMPDIR. Retain existing ColorEntry, Picker, Picker Events and Input
regressions. No dependency installation or desktop test is required. Subsequent
approved integration must regenerate source metadata, SDK/dist and consumer skill
with explicit precision boundaries. Handwritten Python, Node/TypeScript, Go and
Rust support needs separate typed owned-string/float APIs and language tests;
copied SDK headers alone do not establish builder support. No eye-dropper service,
full popup, tabs, theme parity or native AT parity is completed by this design.

## Exact first implementation contract (review gate)

The following declaration-shaped API supersedes provisional naming above. Shared
primitives live in `cui_color_hsla.h`, depending only on `cui.h`; the owned field
API lives in `cui_color_hsla_entry.h`, including those primitives and existing
Input declarations. Neither header depends on byte ColorEntry or Plot.

```c
typedef struct { float h, s, l, a; } cui_color_hsla;
/* Encoded, unpremultiplied channels; explicitly not Plot linear RGBA. */
typedef struct { float r, g, b, a; } cui_color_encoded_rgba;
typedef enum {
    CUI_COLOR_HSLA_OK,
    CUI_COLOR_HSLA_INVALID_ARGUMENT,
    CUI_COLOR_HSLA_CAPACITY
} cui_color_hsla_status;
typedef enum {
    CUI_COLOR_HSLA_HEX_INVALID,
    CUI_COLOR_HSLA_HEX_INCOMPLETE,
    CUI_COLOR_HSLA_HEX_COMPLETE
} cui_color_hsla_hex_status;
typedef struct {
    cui_color_hsla_status status;
    size_t required; /* Payload bytes, excluding NUL; 7 or 9 for valid color. */
} cui_color_hsla_format_result;
bool cui_color_hsla_valid(cui_color_hsla);
cui_color_hsla_status cui_color_hsla_from_rgba(cui_color_encoded_rgba,
                                             cui_color_hsla *out);
cui_color_hsla_status cui_color_hsla_to_rgba(cui_color_hsla,
                                           cui_color_encoded_rgba *out);
cui_color_hsla_status cui_color_hsla_from_color(cui_color, cui_color_hsla *out);
/* Explicit source-style truncating export, never updates a retained model. */
cui_color_hsla_status cui_color_hsla_to_color(cui_color_hsla, cui_color *out);
cui_color_hsla_hex_status cui_color_hsla_parse_hex(cui_str, cui_color_hsla *out);
cui_color_hsla_format_result cui_color_hsla_format_hex(cui_color_hsla,
                                                      char *out, size_t capacity);

typedef struct {
    bool present;
    cui_color_hsla color;
} cui_color_hsla_value;
typedef enum {
    CUI_COLOR_HSLA_ENTRY_OK,
    CUI_COLOR_HSLA_ENTRY_INVALID_ARGUMENT,
    CUI_COLOR_HSLA_ENTRY_CONFLICT,
    CUI_COLOR_HSLA_ENTRY_INCOMPLETE,
    CUI_COLOR_HSLA_ENTRY_ALLOCATION_FAILED,
    CUI_COLOR_HSLA_ENTRY_EDIT_ERROR
} cui_color_hsla_entry_outcome;
typedef enum {
    CUI_COLOR_HSLA_COMMIT_SELECT, /* Successful commit requests close. */
    CUI_COLOR_HSLA_COMMIT_UPDATE  /* Successful commit leaves open policy alone. */
} cui_color_hsla_commit_kind;
typedef struct {
    cui_color_hsla_entry_outcome outcome;
    bool committed, close_requested;
    cui_color_hsla_value previous, value;
} cui_color_hsla_commit_result;
typedef struct {
    char draft[10];
    cui_edit *edit;
    cui_input_state input;
    cui_color_hsla_value committed, preview;
    cui_color_hsla_hex_status status;
    bool dirty;
} cui_color_hsla_entry_state;
cui_color_hsla_entry_outcome cui_color_hsla_entry_init(
    cui_color_hsla_entry_state *, cui_color_hsla_value);
void cui_color_hsla_entry_destroy(cui_color_hsla_entry_state *);
cui_color_hsla_entry_outcome cui_color_hsla_entry_sync(
    cui_color_hsla_entry_state *, cui_color_hsla_value, bool replace_draft);
cui_color_hsla_entry_outcome cui_color_hsla_entry_set_draft(
    cui_color_hsla_entry_state *, cui_str);
cui_color_hsla_entry_outcome cui_color_hsla_entry_preview(
    cui_color_hsla_entry_state *, cui_color_hsla, bool replace_draft);
cui_color_hsla_entry_outcome cui_color_hsla_entry_restore(
    cui_color_hsla_entry_state *, bool replace_draft);
cui_color_hsla_commit_result cui_color_hsla_entry_commit_hex(
    cui_color_hsla_entry_state *, cui_color_hsla_commit_kind);
cui_color_hsla_commit_result cui_color_hsla_entry_commit_value(
    cui_color_hsla_entry_state *, cui_color_hsla,
    cui_color_hsla_commit_kind, bool replace_draft);

typedef struct {
    bool disabled, readonly;
    float width; /* Styled Input width; Base caller supplies layout. */
} cui_color_hsla_entry_options;
typedef struct {
    cui_input_result input;
    bool submitted; /* Eligible active field submission, not an Enter count. */
    bool draft_changed;
    size_t commit_count, rejected_commit_count, close_request_count;
    cui_color_hsla_hex_status status;
    cui_color_hsla_value previous, committed, preview;
    cui_component_event_metadata event;
} cui_color_hsla_entry_result;
typedef void (*cui_color_hsla_entry_commit_fn)(void *,
                                              const cui_color_hsla_entry_result *);
typedef struct {
    cui_color_hsla_entry_commit_fn on_commit;
    void *user;
} cui_color_hsla_entry_handler;
cui_color_hsla_entry_result cui_base_color_hsla_entry(
    cui_context *, cui_id, cui_str name, cui_color_hsla_entry_state *,
    cui_color_hsla_entry_options, cui_color_hsla_entry_handler);
cui_color_hsla_entry_result cui_color_hsla_entry(
    cui_context *, cui_id, const cui_theme *, cui_str name,
    cui_color_hsla_entry_state *, cui_color_hsla_entry_options,
    cui_color_hsla_entry_handler);
```

All required pointer outputs must be nonnull except parse's output, which may
be NULL for validation. Present float values require finite normalized channels;
absent optional values ignore their color and store zeroed color. Parse grammar
and admission remain as specified above. Parse failure writes nothing. Conversion
failure writes nothing. Format validates color and output arguments before any
write: NULL with capacity zero is a length probe (CAPACITY with required 7/9),
NULL with nonzero capacity is INVALID_ARGUMENT, and insufficient capacity leaves
all destination bytes unchanged. Invalid format arguments return required zero.
All input/output extents must be disjoint. No malloc is needed for primitives.

Initialization requires a zeroed state; double init and use-after-destroy return
INVALID_ARGUMENT. State members remain read-only to callers, address-stable and
noncopyable. Init/sync/preview/restore/direct-value commit use this replacement
transaction: validate state/value/mode and dirty conflict first; format the new
display into a local ten-byte array; save the old owned draft locally; copy the
candidate into the final owned buffer and allocate a fresh editor bound to that
final address. No user callback or UI submission runs during this synchronous
temporary buffer replacement. On allocation failure restore every old buffer
byte and keep the old editor, selection/history, input state and float values.
Only after allocation succeeds destroy the old editor, install the new editor,
zero retained Input view state, then publish float/model fields. Destroy remains
idempotent. These operations are forbidden during submission and before prior
render consumption. There is no allocation after the transaction's publish point.

Successful sync sets committed=preview=the copied optional owner value, sets the
formatted draft/status and clears dirty. Preview replaces only preview/display
and clears dirty, retaining committed. Restore applies committed to preview/display
and clears dirty. Neither operation commits or invokes a handler. A dirty draft
conflicts unless replacement was requested; even an equal owner value cannot
silently erase a dirty draft. Set-draft uses editor's atomic whole-content operation,
marks dirty, and parses complete text into preview only; failure leaves all model
fields unchanged. It may preserve editor undo history, unlike replacement sync.

Commit-hex validates kind, reparses the exact current draft regardless of dirty,
and on COMPLETE publishes parsed committed=preview and clears dirty. It does not
rewrite editor bytes/history while Input borrows them. Failure returns committed
false/close_requested false and copied unchanged previous/value; incomplete has
INCOMPLETE outcome, malformed state/text has INVALID_ARGUMENT. Direct-value commit
uses the replacement transaction and copies the supplied floats, never hex-parsing
their display. Successful SELECT sets close_requested true, successful UPDATE
sets it false; both set committed true even for equal values. Neither changes or
owns popup open state. Optional clearing occurs through silent owner sync only.

The real hex field always uses SELECT for accepted Enter submits. Every successful
submit increments commit_count and close_request_count, including same value;
an incomplete accepted submit increments rejected_commit_count. Counts are bounded
by the frame's input event count and independent of an installed handler. Close
request count is evidence of requested policy, not completed popup dismissals.
The inline field never closes a popup. A later popup coordinator must consume these
requests before its own outer observer to match source close-before-Change timing;
it must not pass an early inner observer through unchanged.

`previous` snapshots committed at field entry; `committed` and `preview` snapshot
the final model after ordered reduction. `draft_changed` tracks accepted text
changes, not cursor motion or owner sync. `submitted` snapshots a valid active
non-inert/non-layout-only submission with disabled/readonly false. Metadata kind
is SUBMIT, target is field ID, source UNKNOWN, and HAS_FRAME is present only for
that eligible submission; no INDEX/SOURCE/ACTIVATION flags are invented. Invalid
arguments return input.error INVALID_ARGUMENT, submitted false and no metadata.
An external observer is invoked once only when commit_count>0, after final Base
semantics or styled paint. It receives the same copied summary returned to the
caller. The handler and user are entry snapshots, borrowed through outer return.
All previously stated mutation/reentrancy/render-lifetime restrictions apply.

Production remains gated on review of this addendum. The first implementation
must prove primitive reuse independently of the owned field, precision retention
versus explicit hex quantization, transaction rollback, queue ordering and close
request facts. Popup open ownership, HSLA sliders/tabs and foreign builders remain
separate approved implementation slices, not declarations hidden in this model.
