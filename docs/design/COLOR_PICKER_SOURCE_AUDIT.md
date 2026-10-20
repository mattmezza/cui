# ColorPicker source audit and additive proposal

Design checkpoint only. CUI baseline: `19e390c4`. Frozen reference:
`87d10ae5e1299d1be18670a37570c5402f952e34`, inspected in
`/tmp/cui-gpui-reference`. No production API or implementation is changed here.

## Authority and actual gap

The frozen implementation is `crates/base/src/color_picker.rs` and
`crates/component/src/color_picker.rs`, including their embedded tests; the
website examples describe composition but do not override that code. CUI
authority is `include/cui/cui_picker.h`, `include/cui/cui_picker_events.h`,
`src/base/cui_picker.c`, `src/components/cui_picker.c`, and their event facades.

The manifest's combined `keyboard/validation/theme` row is too broad to describe
the remaining work. CUI already implements keyboard channel editing and themed
painting. The genuine frozen-source gaps include editable validated hex drafts,
separate preview and committed values, optional empty selection, popup trigger
keyboard behavior, and the HSLA/palette tab presentation. These are not all one
missing parser. An eye-dropper is not implemented in either frozen ColorPicker
module; a repository search of frozen crates and website found no eye-dropper
implementation. Native screen sampling is therefore a separate host capability,
not evidence required to reproduce an upstream ColorPicker method.

## Frozen behavior

### Representation, entry, and notification timing

The state owns `Option<Hsla>` committed value and preview, open state, active tab,
an InputState entity, four SliderState entities, and retained subscriptions.
HSLA channels are floating point; sliders use normalized ranges and step `.01`.
An empty selection is distinct from transparent black.

The pure internal hex parser accepts an optional leading `#`, lengths 3, 4, 6,
or 8, and only ASCII hex digits. Short forms duplicate nibbles. Missing alpha
means opaque. The actual text input has pattern `^#[0-9a-fA-F]{0,8}$`: it admits
incomplete drafts but requires the hash, unlike the pure parser. No RGB function,
CSS color name, decimal channel grammar, whitespace trimming, or arbitrary CSS
parser is supplied. Formatting uses uppercase hex and omits alpha when opaque;
float-to-byte conversion truncates (half alpha formats as `7F`).

Input Change previews a valid draft and synchronizes sliders without committing.
An invalid/incomplete draft leaves the previous preview and committed value
unchanged. Enter attempts commit; invalid text does not close or commit. Selecting
a palette color closes first, then updates value/preview, synchronizes input and
sliders, emits Change, and notifies. Slider events update committed value and
preview and emit Change while leaving the popup open. Slider feedback does not
write back into the active sliders. Programmatic input writes have suppression
to prevent feedback subscriptions from behaving like user edits.

`set_value` and `clear_value` synchronize state without emitting Change.
`preview_color` changes preview and input only; `clear_preview` restores preview
from committed state. `update_color` emits without closing. Selection and update
do not suppress same-value notifications. Thus upstream Change is not equivalent
to CUI's aggregate net RGBA change callback. Initial default synchronization is
deferred until render. Applications must retain the GPUI entities/subscriptions.

### Keyboard and semantics

The Base root is a Button role with expanded state and optional accessible name;
an enabled supplied focus handle is a tab stop. In its ColorPicker key context,
Enter toggles the captured open value through `on_open_change`. Disabled Confirm
propagates. Escape while closed propagates; while open it stops propagation and
requests close. Cancel has no separate disabled guard. This is scoped action
routing, not a global scan of every Escape event.

Swatches have stable independent focus handles, default RadioButton role,
selected/toggled semantics, default hex names, and configurable tab participation.
Disabled swatches omit interaction/focus listeners. Duplicate featured colors use
slot IDs and remain separate focus stops. There is no explicit palette arrow-key
grid-navigation implementation in these modules; do not invent one from the
manifest. Slider and text editing also inherit their respective primitives.

### Theme and presentation

The styled component provides Palette and HSLA tabs, preview and hex entry,
optional visible/accessibility labels, field/compact triggers, placeholder, size,
icon, and featured palette overrides. Default featured colors come from semantic
theme colors. Other palette rows come from DEFAULT_COLORS. Theme tokens supply
field borders, focus ring, foreground, muted placeholder, and radii; color tracks
represent color values. Hovering a swatch previews on entry; its handler does not
explicitly restore the preview on exit. No dedicated invalid/error status API or
validation error message is present in this ColorPicker state/component. A future
status UI must be identified as a CUI addition, not frozen parity.

## Current CUI behavior and proof already present

CUI retains caller-owned RGBA bytes and `cui_color_state` (HSV, last color,
initialization and drag bits). HSV brightness is not HSL lightness. Hue is degrees;
conversion rounds channels. Replacing this state with HSLA or silently forcing
eight-bit round trips would break existing behavior and lose upstream precision.

The inline picker exposes hue, saturation/value plane, alpha, RGBA channels, and
caller-supplied palettes. Arrow keys edit focused channels; Home/End reach limits.
The plane uses Left/Right for saturation and Up/Down for brightness. Pointer
capture retains dragging outside bounds; cancellation and disabled/readonly clear
drag state. Palette selection preserves owner alpha when alpha editing is off.
The legacy parser accepts only `#RRGGBB` and `#RRGGBBAA`; invalid input leaves the
output unchanged. Its formatter's explicit alpha flag and required-length/bounded
copy contract differ from upstream automatic formatting. Preserve both APIs.

The styled picker paints after the Base reducer: themed surface, borders, focus,
disabled text, discrete color tracks, palette and a read-only hex display. It has
no editable hex field, popup root, optional empty value, preview/commit split, or
HSLA tab. Theme changes must not be treated as selected-color edits.

`tests/unit/test_picker.c` covers conversion/hex round trips, geometry, keyboard,
palette, alpha preservation, captured plane dragging, disabled/readonly, and
light/dark/high-contrast/terminal themes. `tests/unit/test_picker_events.c` compares
Base/styled legacy and facade draw/state semantics, including net-zero edits,
latent hue on black, owner synchronization, null handlers, passive frames and
copied callback payloads. These are meaningful partial proofs, not popup/input
or full platform accessibility evidence.

Existing `*_color_picker_events` calls the legacy implementation once. Base
observation follows reduction/semantics; styled observation follows painting.
Only a net byte-color difference produces an observer call, with UNKNOWN source
and actual frame metadata; same-color selection and within-frame net-zero edits
are silent. Existing change_count counts delivered callbacks, not accepted input
actions. Preserve this contract. Borrowed event payloads must be copied to retain
them. A new entry/commit observer needs its own named result and must not retrofit
these aggregate events into per-input evidence.

## Host eye-dropper and binding boundaries

`web/gallery.js` already gates EyeDropper on secure context and browser support,
opens it directly inside a trusted click, and owns AbortController/request identity.
Completion validates exactly six hex digits, checks current component and disabled
state, then calls `gallery_screen_color`. Route/disabled cleanup cancels outstanding
requests. Stale completions cannot apply. `web/gallery.c` accepts bounded sRGB
bytes and preserves alpha; the next picker submission synchronizes HSV. This is
an external owner update, not a picker-input callback.

`tests/web/test_browser.py` exercises this integration using mocked browser
EyeDropper success, cancellation, malformed results, pending cleanup and unsupported
availability. It does not prove real screen sampling, OS permission dialogs, or
native backend support. No portable Core API should expose browser promises,
platform handles, or pretend a host result contains pointer/key activation data.

The bridge currently calls the legacy styled picker with alpha enabled, disabled
state and width; it reports packed color changes through its existing callback.
Python, Node/TypeScript, Go and Rust expose packed RGBA setters/getters and picker
hit bounds. They do not expose this proposed draft/preview/commit model, source
HSLA entities, popup entry, palette configuration, or portable eye-dropper service.
Generated C headers in SDK bundles are not equivalent to handwritten builder
support. Any later binding slice needs typed methods and owned UTF-8 strings,
transactional validation, explicit callback lifetime rules and tests per language.

## Proposed cohesive additive slices (approval required)

1. **Validated hex entry and selection model.** Add a dedicated header and
   caller-owned state with explicit committed/preview presence and bounded draft
   storage, separate from `cui_color_state`. Add pure source-format hex operations
   under new names; retain strict legacy parser/formatter unchanged. Draft updates
   distinguish syntactically admitted incomplete text from a complete parsed color;
   no invalid draft overwrites the last valid preview or selection. Explicit commit,
   preview-clear and owner-sync operations have distinct results. Compose an actual
   existing CUI editable input, with valid Enter commit, not just a parser demo.
   Document the first slice's byte-RGBA precision limit; exact floating HSLA slider
   parity requires a separately reviewed representation rather than a hidden HSV
   substitution. Optional selection must remain explicit.
2. **Popup and source keyboard composition.** Build an additive styled facade using
   existing trigger, popover, focus and input routing, with explicit open ownership
   and palette selection/slider commit policy. Preserve the inline picker unchanged.
   Inspect accepted action routing before specifying Enter/Escape evidence; do not
   rescan consumed trigger events or introduce global Escape ownership. Match the
   source root's closed/open propagation and disabled Confirm distinction. Preview,
   commit, open-change, and aggregate byte-change events remain separate concepts.
   HSLA tab and source swatch semantics belong here only with genuine implementation
   and tests; otherwise document their omission.
3. **Theme/status and host integration.** Give the new facade explicit semantic
   theme presentation, optional featured palette, labels and empty placeholder.
   Draft validity feedback may be added as a documented CUI extension; no built-in
   arbitrary validator, veto engine, or source error taxonomy is implied. Keep async
   eye-dropper requests in hosts. A host can submit a validated owner value through
   a typed sync operation, with UNKNOWN origin unless actual evidence is supplied;
   absence of host support remains explicit. Do not bundle native screen capture
   or all five handwritten bindings into a C11 parser milestone.

For implementation review, freeze selected value/options/handler slots at entry.
Owned draft capacity, initialization/destruction, UTF-8/ASCII admission, overflow,
and owner-sync conflict policy must be specified before code. Borrowed palette,
labels and option strings remain stable until return, including callbacks. A
callback may update the application model for its next submission, but may not
mutate descriptors still borrowed for painting. Callback payloads contain copied
values and bounded facts; no invented key, timestamp, input index or activation
multiplicity. Define notification position relative to paint explicitly and test
callback-driven owner changes. No new dispatch architecture is needed.

## Required implementation proof

Use an isolated strict-C11 fixture through `scripts/test_local.py` with a private
Xvfb and home-cache TMPDIR; compile public headers in C and C++. Test all 3/4/6/8
formats, optional hash versus UI admission, mixed case, incomplete drafts, invalid
ASCII/non-ASCII/sign/space, bounded output and unchanged output on failure. Preserve
legacy strict-parser and formatting tests. Include source truncation versus CUI
rounding examples so precision is never accidentally claimed equivalent.

Exercise the real editable field and frame lifecycle: typing previews without
commit; invalid Enter stays open/unchanged; valid Enter commits/closes; palette
selection and slider editing use their distinct policies; programmatic sync is
silent; optional empty versus transparent selection; same-value and multiple
accepted actions; disabled/readonly/passive frames; callback reentrancy and copied
payload lifetime. Verify focus, named selected swatches, expanded state, popup
Escape/Enter scope and propagation with preceding consumed actions. Compare legacy
draw/semantics with all new opt-ins absent.

Theme proofs should compare actual draw colors/geometry and semantic labels under
existing themes while asserting value/draft stability. Headless semantic snapshots
do not prove native AT announcements. Browser host tests should retain async stale,
cancel, malformed and unsupported cases; real OS picking remains a separately
reported manual/hardware boundary. Run existing picker and picker-event regressions
alongside focused tests. Root registers tests and updates metadata, consumer skill
and generated distributions after approved implementation; this checkpoint claims
no additional completed parity.

## Approved first-slice API decisions

The first implementation uses new `cui_color_entry.h` only; existing picker
headers, HSV state, parser, formatter and events remain untouched. A public
caller-owned `cui_color_entry_state` owns a ten-byte NUL-terminated draft buffer
(`#` plus eight digits), an allocated `cui_edit`, and retained input view state.
Zero-initialize before init; init returns failure transactionally; destroy is
idempotent after successful initialization. The initialized state is address-stable
and noncopyable because its editor borrows its buffer. Destroy only after frame
render consumption. No state storage may be rebound during submission.

Committed and preview values use an explicit `{present, color}` optional byte-RGBA
value. Pure source-format parsing reports COMPLETE, INCOMPLETE (hex-only prefixes
of unsupported short lengths), or INVALID; it accepts optional hash and writes
the output only on COMPLETE. The real field additionally requires a leading hash
(or empty draft), ASCII hex only, and at most eight digits. Disallowed edits are
rejected transactionally by existing Input acceptance. Incomplete drafts remain
editable and retain the last valid preview; they cannot commit. Empty draft is
incomplete, not an implicit clear command. Explicit owner sync can clear selection.

Owner sync takes an explicit replace-draft flag. A dirty draft conflicts unless
replacement is requested; conflict or invalid arguments leave all model/editor
state unchanged. Successful sync replaces draft with canonical uppercase bytes,
updates committed/preview presence, and clears editor history through existing
set-content behavior; it emits no UI callback. Canonical alpha is omitted only
when opaque. Draft updates preview but never commit. A valid explicit commit
copies preview into committed and clears dirty, including same-value commits;
it does not rewrite editor text while Input is reducing events.

Base and styled field facades use existing Input event reduction and its accepted,
focused nonrepeated Enter submit notifications, not a second raw key scan. Internal
change/submit observers update entry state in queue order, so text after Enter
cannot retroactively change that commit. Multiple valid Enters produce a commit
count independent of handler presence; incomplete submit attempts have a separate
rejected-commit count. This count represents observed field submits, not every
possible upstream activation. Disabled/readonly/passive submissions cannot commit.
Pure model operations have typed outcomes; they do not fabricate UI events.

The external handler is snapshotted by value and runs once only if at least one
commit completed, after final semantics (Base) or paint (styled). Its pointer-free
summary copies previous/final committed values and accepted/rejected commit counts;
metadata is UNKNOWN with actual active frame only. It does not attribute the
coalesced summary to one input index. The result reports counts even without a
handler. Callback payloads may be copied; handler/user remain borrowed. Callbacks
may update a separate owner model for next submission but may not mutate entry,
input, context or borrowed label/options until return. Draft-change/preview status
is available in the result, without inventing a separate source Change contract.

The facade is an inline hex field with standard Input focus, selection, pointer,
keyboard and semantic textbox behavior. It does not close anything and has no
popup/open state, palette tab, HSLA sliders, or screen sampler. Before the next
slice, review a float-HSLA optional selection representation (including conversion
precision and source truncating formatter), popup action ownership/focus restoration,
and palette/HSLA tab presentation. Byte RGBA is an explicit first-slice boundary,
not the permanent replacement for those remaining frozen-source requirements.
