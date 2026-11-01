---
name: cui
description: Build and debug CUI user interfaces in C11, Python, JavaScript, TypeScript, Go or Rust, for native, browser/WASM, headless or embedded hosts. Use for CUI setup, component/API discovery, layout, fonts, themes, responsive composition and backend integration.
---

# Build with CUI

Use the Python 3.9+ standard-library helper beside this file. Resolve its absolute
path from this skill's location; do not assume the skill is in the app checkout.
In commands below, `HELPER` means that absolute `scripts/cui.py` path. No pip
packages are required. The skill includes offline documentation, headers and
examples; it does not include the CUI implementation or compiler toolchain.

For unreleased owned background colors/two-stop values and pure sampling, read
`docs/BACKGROUND_VALUES.md` and `include/cui/cui_background.h`. CSS and SOURCE_038
have different alpha/geometry semantics. Pure values alone do not apply paint.
For opt-in Core V2 mode/setters/emission read `docs/CORE_DRAW_V2.md` and
`include/cui/cui_draw_v2.h`. The C software adapter's actual query/render route is
in `docs/SOFTWARE_V2.md` and `include/cui/cui_software.h`: query the initialized
renderer before composition, use its support/defaults, then render V2 explicitly.
The unreleased SDL_Renderer C route is documented in `docs/SDL_V2.md` and
`include/cui/cui_sdl3.h`; query its actual capability before composition.
Never cast to V1 or fabricate consumer support. SDL_GPU, Sokol, Canvas and foreign
bindings remain pending; these C routes do not complete all78 parts. V1 renderer tables/default routes remain unchanged.
`docs/SOFTWARE_V2_PAINTING.md` details the private bounded AREA shadows, per-axis
owned text and publication contract. When copying backend sources, retain
`backends/common/` alongside legacy software private forwarding headers; supplied
SDK/native source bundles carry both. `docs/BACKGROUND_PREPARATION.md` describes
allocation-free preparation, not another public adapter. `docs/TEXT_PREPARATION.md`
describes counted recording, owned source density up to64 and GRAY256 coverage;
public scaled font APIs remain limited to16. No-Bidi rejects contextual/spanned
text, and no-font builds reject TEXT before borrowed bytes. Opaque font/dependency
temporaries are outside the transaction CPU envelope; preserve borrowed graph
lifetime and avoid recursive/concurrent same-font rendering. Existing Plot/effects/
text gradient contracts remain distinct.

## Start with the project's actual integration

Inspect the existing app before choosing a language, host or renderer. Preserve
its stack and dependency strategy. For a new app, infer the intended target from
the request; ask only when the choice materially affects the result and is unclear.
Read `docs/START_HERE.md` through the helper, then only relevant guides below.

```sh
python "$HELPER" read docs/START_HERE.md
python "$HELPER" doctor --profile portable
python "$HELPER" search "searchable dropdown" --kind component
python "$HELPER" component Select --show usage
python "$HELPER" component Select --show limitations
python "$HELPER" component Select --show api
python "$HELPER" component Select --show examples
```

`search` supports component, api and guide filters, weighted literal words, and
labelled partial matches when no entry matches every word. Human output defaults
to five results, JSON to ten; use `--limit N` to request more. Ranking is unchanged.
`component NAME` summarizes usage and retains ALL canonical status, platform,
limitation and feature information, plus canonical public-header paths and handler
types with source-aware full-read commands. Handler-type indexes are not exhaustive;
an empty/missing index means inspect the API and owning headers, not no handlers. `--show examples` indexes guides, sample and
test references rather than printing every shared guide. Read chosen resources.
`component --show all` or `--full` restores the complete previous human output.

`api SYMBOL` and `component --show api` print declaration/resource indexes,
**incomplete contracts**: declarations do not retain header comments, ownership,
errors, types or lifecycle rules. Read the exact authoritative headers and relevant
guides using the printed commands before building. Printed read commands use
POSIX shell quoting; on Windows pass the same arguments with your shell’s quoting. Types/macros without indexed
function declarations produce matching paths only. `api SYMBOL --full` restores
complete matching headers. `--declaration` explicitly selects the concise API view.
No output is clipped or filtered by safety keywords.

`read PATH` still prints the exact full document; `info` lists paths and hashes.
For broad Markdown guides use `read PATH --headings`, then
`read PATH --section 'Exact heading title'`. Sections include child headings up to
the next heading of equal or higher rank. These are explicitly partial views,
with line ranges, omitted-line counts, source digest and a full-read command;
they do not establish a complete contract or standalone program. The scanner
recognizes ATX (`#`) headings outside fenced/indented code, not every CommonMark
extension. Duplicate titles, missing headings and unterminated fences fail; use the full read rather
than guessing. Select the relevant contract sections, example and shared ownership
headers; use full reads whenever omitted context is required. This saves output,
not the obligation to read the chosen API's complete applicable contracts.

For an existing app, reuse already-understood Core/host setup contracts instead of
rereading them solely for a component lookup. For broad guides inspect `--headings`
first, then retrieve the relevant exact sections. Read complete applicable contracts
and owning headers; use full reads whenever section context or coverage is unclear.

Existing `--json` search/component/api/read shapes and full content are unchanged.
The new summary/declaration/headings/section selectors are human-only and reject
`--json`; omitted component selectors with JSON still return the legacy full data.

Run `doctor --profile TARGET` for the chosen path. For a native window, also run
`doctor --profile native --font /absolute/font.ttf`. These are read-only probes,
not proof that headers, ABI versions or GPU drivers work: configure and build to
verify those. Explain missing dependencies and obtain any necessary authorization
before installing system packages. Do not ask users to manually confirm things
the probe can check. Native Windows prerequisites may need manual/CMake discovery
instead of pkg-config; do not infer unsupported platform parity.

## Pin setup and keep references aligned

For a new source workspace, resolve the desired upstream branch/tag once, then
use its explicit 40-character commit SHA:

```sh
python "$HELPER" resolve --ref main
python "$HELPER" vendor --revision COMMIT_SHA --dest vendor/cui
python "$HELPER" init --template web --revision COMMIT_SHA --dest my-app
```

`init` supports native, portable, web, python, node, typescript, go, rust,
embedding, sdl3, sdl-gpu, sokol and software. `native` creates the existing
standalone SDL3 source bundle, with fonts and Makefile. Other templates create a
complete pinned source workspace and identify an existing example/adapter to
adapt, retaining its build context. They do not synthesize a new production host.
Read the printed entry point and its guide before changing it.

For offline setup, pass `--source /path/to/cui` to vendor/init. This archives
committed Git files only, defaulting to local HEAD; uncommitted edits are excluded.
Alternatively supply `--archive FILE --revision COMMIT_SHA --sha256 HASH` for an
upstream source tarball. Downloads use an immutable GitHub commit URL; the archive
hash is recorded and an optional `--sha256` enforces a previously known checksum.
Existing destinations are refused. No dependency installs or app launches occur.

The helper records `.cui-lock.json` (revision, archive hash and original file
hashes) and `.cui-resources.json` (matching discovery data). Run
`verify --source PROJECT` to detect changes to the original snapshot; application
edits are reported too. New files are not tracked by this verification.

**After setup, pass `--source /path/to/project-or-vendored-cui` on discovery
commands.** For full trees this reads current local headers/docs; standalone
bundles use their matching packaged references. Without `--source`, lookup uses
the installed skill snapshot, which may differ from the app's CUI revision.
Never silently upgrade an existing project to match a newer skill.

## Choose only the references needed

Read these with `read PATH --source CUI_SOURCE`:

| Work | References |
| --- | --- |
| Native C11 app and distribution | `docs/NATIVE_APP_STARTER.md`, `examples/native_bundle/README.md`, `examples/native_starter/README.md` |
| Browser/WASM and Canvas host | `docs/WEB_GALLERY.md`, `docs/WASI.md`; inspect `web/gallery.c` and `web/gallery.js` |
| Python, Node and TypeScript authoring | `docs/BINDING_BUILDERS.md`, `docs/BRIDGE_COMPONENTS.md`, `docs/binding-coverage.json` |
| Go or Rust authoring | `docs/GO_BUILDERS.md` or `docs/RUST_BUILDERS.md`, then bridge coverage |
| Hosting C-authored UI from another language | `docs/LANGUAGE_BINDINGS.md` (different from language component builders) |
| Embedding/custom host or renderer | `docs/INTERFACES.md`, `docs/PLATFORM_MATRIX.md`, relevant `backends/*/README.md` |
| Single-date time editing | `docs/DATE_TIME_PICKER.md`, `include/cui/cui_date_time_picker.h`: separate retained date/time value/state, explicit assignment, Base composition helpers or one-reduction Styled facade, bounded presets/clear/Delete. Same-day confirms; changed-day stays open. Gallery DatePicker has an opt-in Date + time mode with headless desktop/touch proof and copyable C11 usage. No timezone/range/chrono parsing, binding builder or callback stream; oversized Styled popup rejects before mutation. |
| Calendar presentation and localized dates | `docs/PICKERS.md`, `docs/CALENDAR_PRESENTATION.md`, `docs/CALENDAR_BEHAVIOR.md`; read `include/cui/cui_calendar_presentation.h` and `include/cui/cui_calendar_behavior.h` for opt-in month/today/neighbor and independent range/year/recenter policies, matching-header/library rebuild and binding limits. DatePicker forwarding uses `docs/DATE_PICKER_CALENDAR.md` and `include/cui/cui_date_picker_calendar.h`; legacy typed CHANGE does not observe extensions or lifecycle. |
| Layout and responsive composition | `docs/LAYOUT_GUIDE.md`, `docs/APP_RECIPES.md`, `docs/FORMS.md`, `docs/COMPOSED_APPS.md` |
| Fonts, metrics, fallback and shaping | `docs/TYPOGRAPHY.md`, `docs/TEXT.md`, `docs/BIDI_SHAPING.md`, `docs/IME.md` |
| Theme, density, colors and icons | `docs/DESIGN_SYSTEM.md`, `docs/THEME_GUIDE.md`, `docs/THEME_REGISTRY_GUIDE.md`, `docs/ICON.md` |
| State, identity, ownership and failures | `docs/IMMEDIATE_AND_STATE.md`, `docs/IDS_AND_LIFETIMES.md`, `docs/DEBUGGING.md` |
| Accessibility and touch | `docs/ACCESSIBILITY.md`, `docs/NATIVE_ACCESSIBILITY.md`, `docs/TOUCH_COMPONENT_COVERAGE.md` |
| Optional regex provider | `backends/regex/README.md`, `docs/INPUT_PATTERN_FORMATTING.md` |
| Package/release provenance | `docs/PACKAGING.md`, `docs/RELEASING.md` |
| Platform prerequisites and evidence | `docs/DEPENDENCIES.md`, `docs/PLATFORM_MATRIX.md`, `docs/V1_READINESS.md` |

For matching Rust source builds, Cargo's positive `NUM_JOBS` controls the child
CMake `-j` count; use `CARGO_BUILD_JOBS=1` or Cargo `-j1` for serial builds. A
missing/invalid standalone value falls back to1. `CUI_BRIDGE_LIB_DIR` uses an
existing library and skips that CMake build. Read `docs/RUST_BUILDERS.md` and
`bindings/rust/build.rs`; do not assume a separate CMake parallelism environment
variable overrides an explicit Cargo-selected count.

Configure a real font and assets for the selected host. Keep measuring and
painting font configuration consistent; arrange fallback/shaping for the scripts
actually displayed. Read the exact text APIs before implementing font families.
For responsive UI, derive available space from the host and use the documented
layout constraints. Test narrow and wide viewports, long content and overflow;
do not transplant CSS/browser assumptions into the C layout model.

For current-frame intrinsic scroll-body allocation in C Core, read
`docs/SCROLL_LAYOUT.md` and `include/cui/cui.h`. The opt-in APIs require widths
independent of allocated Y, support ordinary GROW descendants and hidden/overlay
bars, and borrow distinct offset floats through end_frame. Optional 48-byte
track records stage actual fixed track/thumb boxes with independent visibility
and interactivity; INTERACTIVE owns bar enabled policy even when its submitted
semantic is a disabled placeholder, so clear it to author a disabled bar. Compose
positive fixed-height placeholders and register against
a current Y clamp. The lazy LP64 layout payload is 12856 bytes (16 KiB bound),
including for V1 body-only users, with one resource slot unchanged. Additive V2
layout-record version is independent of draw ABI2/background negotiation: it
works in ordinary V1 draw frames and requires no new renderer support. V2 uses
a144-byte copied record/24-byte region policies for Content/Body peers and
a separate intrinsic Footer; it adds only a2560-byte sidecar/one slot (15416
combined). Compose conditional spacers with Column gap0, distinguish OMIT from
present-empty, and retain source inner clipping/possible oversized Footer overflow.
No implicit whole-Outer fallback. Read exact topology and input-policy limits;
this does not install a dispatcher or Base reducers. Failed opt-in frames
have no valid geometry/prepaint snapshot; clear the error and rebuild next frame.
Existing Scrollable/Base/Dialog behavior and five foreign builder APIs do not
acquire this feature automatically. For the additive C six-slot route, read
`docs/DIALOG_COMPOSITION.md`, `include/cui/cui_dialog_composition.h` and its
`_styled.h` companion. Retain surface plus layout through end_frame, open through
`cui_dialog_open_v1`, and resolve sizing against the current viewport each frame.
Header/Title/Description remain independent; Content has intrinsic minimum,
Body scrolls, Footer remains outside Inner. Present-empty differs from OMIT.
Old helpers stay unchanged; previous-input caches are not current geometry.
Held child input across reopen follows existing Core behavior. Root/host/five
foreign facade slots and full lifecycle/parity remain unfinished. Read the guide's
current validation limits before treating any example or backend as verified.

For C Button phase-dependent shadows, read `docs/BUTTON_STYLES.md` and
`include/cui/cui_button_styles.h`: the additive surface facade copies at most
eight mixed layers, preserves instance replay, and reports rejected phase
patches independently of activation. Existing styled structs keep their ABI.
This is not gradient/focus-ring/full Styled completion and has no typed foreign
phase authoring or event/group overload.

Typed builder root shadows cover only Row, Column, Label, Button, Badge and
Input. Read `docs/BRIDGE_SHADOWS.md` and the owning `include/cui/cui_bridge.h`
record contract. The exact SDK revision must also provide the typed modules
`bindings/python/cui_bridge/shadows.py`, `bindings/node/shadows.cjs` and
`bindings/rust/src/shadows.rs`; these are readable through this helper. Explicit
empty lists suppress defaults; reset restores defaults. The separate
`docs/BRIDGE_PART_SHADOWS.md` target API supports only the documented ordinary-tree
Checkbox/Radio indicators, Switch track/thumb, Slider track and explicit ROOT
roles. ROOT and MAIN are distinct concepts; this builder Slider target has no
thumb (the separate C-only typed Base route is not mapped). A RadioGroup
with any authored ITEM indicator names all peers until last reset/removal. Query
reports capability, never current presence; unsupported owner paths reject.
Read matching typed modules `bindings/go/cui/part_shadows.go` and
`bindings/rust/src/part_shadows.rs` as well. Other compound/portal parts remain
outside these bounded mappings. Python deferred CSS/preset requests
normalize through the owning App's selected library at mount/update.

## Compose and verify

CUI's C API composes visible UI every frame; application data and explicit models
persist. Language builders have their own retained authoring facade over this
engine. Follow that binding's event and ownership contract.

- Use stable IDs for interactive elements, including reordered rows. Retain values,
  edit buffers and explicit models for their documented lifetimes.
- Balance successful begin/end calls. Paint borrowed draw data before the next
  frame; keep borrowed inputs alive for the documented interval.
- Use real declared APIs, types and callbacks. Read each chosen component's
  scope, feature status and limitations; 78 catalog families do not imply full
  behavior, binding-option or platform parity.
- Keep portable UI separate from host and renderer code. Build the unchanged
  chosen example first, then implement the requested UI and its interactions.
- Compile with warnings. Test triggering interactions and expected state changes,
  keyboard/focus behavior and responsive bounds where relevant. A screenshot or
  successful compilation alone does not establish interaction correctness.

For repository builds:

```sh
cmake -S . -B build -G Ninja
cmake --build build
python scripts/test_local.py --build-dir build
```

For portable builds set `-DCUI_BUILD_NATIVE=OFF`; binding and standalone-bundle
builds use their own guide commands. On Linux, native automated tests must use
invisible Xvfb through the wrapper, never DISPLAY=:0 or a user's Wayland session.
Browser tests run headless. GPU observations unsupported by the virtual display
remain explicitly deferred. Never auto-launch a native example on the desktop
as an automated check. Report what actually ran and which host/backend it used.

For validated color text, read `docs/COLOR_ENTRY.md` and
`include/cui/cui_color_entry.h`. ColorEntry is an additive inline byte-RGBA field
with an address-stable owned draft/editor and explicit preview/commit state.
Its coalesced commit observer runs after submission; update a separate owner
model and synchronize on the next submission. It does not add popup focus/actions, tabs, native screen sampling or handwritten
foreign builders. For a separate precision-preserving float-HSLA owner/entry, read
`docs/COLOR_HSLA_ENTRY.md`, `include/cui/cui_color_hsla.h` and
`include/cui/cui_color_hsla_entry.h`. Derived hex display never updates float state;
explicit hex commit reparses/truncates and can quantize an unchanged display.
SELECT reports a close request and owns no popup.

Questionnaire typed observation is an optional persistent C attachment, separate from pure snapshot copy-out. Consult `docs/QUESTIONNAIRE_OBSERVER.md` for matching-header/library rebuild, caller-owned observer/handler/frame arenas, BUSY detach pinning, frozen Completed/Submit payloads and capacity/depth failures. Typed handlers are observational; input metadata is UNKNOWN. Language builders do not expose this observer facade.

For localized/custom Questionnaire progress, read `docs/QUESTIONNAIRE_PROGRESS.md`
and `include/cui/cui_questionnaire_progress.h`. Masks preserve explicit empty/zero
values; strings survive frame consumption and child callbacks are borrowed/pure.
Numeric progress stays model-derived. This is C-only bounded presentation, not a
family typography cascade or language-builder facade.

For independent Questionnaire Title/Description/ChoiceDescription and decorative
Error content, read `docs/QUESTIONNAIRE_PRESENTATION.md` and the matching
`include/cui/cui_questionnaire_presentation.h`. Zero preserves extracted legacy
text/Error paint; token typography is explicit, shared scale tiers are CUI
extensions. Schema text needs Item gating; Error retains one localized Alert.
No root style cascade, binding facade or custom action content is implied.

For bounded Questionnaire geometry, read `docs/QUESTIONNAIRE_GEOMETRY.md` and
`include/cui/cui_questionnaire_geometry.h`. Pass numeric/box/text options explicitly
to each part; root never publishes these opt-ins. Shared size beats custom scalar,
then masks override. NULL/zero keeps the legacy tree. Extended child snapshots are
pure borrowed submission values; new callbacks win over old. Minimum blank area
belongs to the same interactive card. Input asymmetric padding/editor styles,
Button/trailing actions and language-builder geometry remain separate gaps.

For bounded DropdownButton paint, read `docs/DROPDOWN_BUTTON_STYLES.md` and
`include/cui/cui_dropdown_button_styles.h`. NULL/zero retains the legacy path;
explicit BUTTON policy enables source state ordering with CUI palette tokens.
Independent action/trigger callbacks are borrowed and pure; instance values replay
after selected/disabled. Container has no foreground cascade. Custom XS shadows
require both explicit intent and the host-enable gate. For independent accessible names and signed tab ordering, read
`docs/DROPDOWN_BUTTON_METADATA.md` and its authoritative sibling header. Names
are UTF-8, explicitly empty when masked, and borrowed immutable through submission.
Metadata is visible before existing synchronous callbacks. The gallery has opt-in
name/order/skip-tab/checked-value selectors and a compilable C snippet. Checked/value follows
the existing Button convention; optional toggled presence/pressed-button adapter
mapping and language-builder facades remain gaps. Use matching rebuilt source,
standalone distribution headers or packaged SDKs; do not mix revisions.

Growing single-line Input now contains long populated values within its resolved
frame, including InputGroup composition. During resize, caret/selection scroll
and IME geometry may lag by one frame; read `docs/INPUT.md`. Do not describe this
containment fix as complete responsive text-input behavior.

For Resizable composition, read `docs/RESIZABLE.md`. Keep caller state/slots alive
through frame completion and ensure the container fits the panels' combined
min/max range. A pane at its constraint cannot move farther in that direction.
Repeated mouse/touch dragging has focused actual-WASM coverage; native window/GPU
interaction remains separate. The gallery nested editor/console mode uses distinct
caller-owned states/IDs and supplies current outer pane width to the inner group;
copy its compilable C example. See `docs/RESIZABLE_BROWSER_REAUDIT.md`.

For Spinner, read `docs/SPINNER.md`: `cui_spinner_ex` is decorative and distinct
from legacy language-builder/eight-dot Spinner. Optional sizing preserves named
rem versus fixed pixels. Finite easing can return negative or extra turns;
reduced motion evaluates ease(0), while nonfinite output rejects without committing
motion/result or painting. Schedule only successful results; do not reuse a stale
deadline after failure. Gallery selectors and copied C examples cover this subset;
inherited Icon style/lifecycle and a source-style binding facade remain gaps.

For hidden native windows, read `docs/BINDING_BUILDERS.md` section `Native window
visibility` and the chosen Go/Rust guide. C SDL3 uses
`cui_sdl3_create_with_options` with `initially_hidden`; Python uses
`initially_hidden=True`, JS/TS `initiallyHidden:true`, Go `Options.InitiallyHidden`,
Rust `sdl_with_window_options`/`WindowOptions`. Defaults stay visible. Hidden is
passed to the constructor; never substitute show-then-hide. Visibility controls
belong on the owning UI thread outside active Core stepping; native after-frame
callbacks are supported. Headless hidden/control requests fail unavailable.
Sokol/SDL_GPU renderer hosts own creation policy. Linux private-Xvfb proof covers
all five runtimes (TS compiled/executed), not other OS/window systems or hardware.

For compound C shadow targeting, read `docs/COMPOUND_SHADOW_PARTS.md` and the
component's authoritative header. Bubble, Message/MessageScroller, Notification
and Attachment have typed part-ID getters. A valid identity is not a presence
check: submit first, then check the current-frame shadow setter before end_frame.
Notification logical keys and stack scopes are not card paint IDs; custom callback
namespaces may have no box. Scroller row IDs follow retained keys, not stable
indices. Language-builder MAIN/ROOT/subpart targets remain pending.

For opt-in C control shadow examples, read `docs/CONTROL_SURFACE_SHADOW_GALLERY.md`.
Button's captured instance patch replays after disabled; an instance shadow must
explicitly clear for disabled if that is the intended result. Named selection
indicators do not add semantics; Switch thumb shadows remain track-clipped, and
legacy Slider exposes only its track. These gallery routes do not establish all78
shadow coverage or foreign-builder callback parity. Check the guide's proof status.
## Slider palette values

Read `docs/SLIDER_PALETTE.md` and `include/cui/cui_slider_palette.h` for the
C-only owned value resolver. Four CUI builtin adaptations decode encoded theme
bytes; 38 named source palettes preserve pinned source color semantics and the
Aurora Light two-stop gradient. Custom themes never acquire a named identity.
These values do not change Slider rendering: there is no palette setter, brush
sampler or Python/JS/TS/Go/Rust builder API. Disabled/theme presentation remains
partial; do not claim complete Slider parity from these values.

## Core numeric accessibility input

Read `docs/CORE_NUMERIC_ACCESSIBILITY.md` and exact `include/cui/cui.h` before
using explicit numeric capabilities, typed SET_VALUE queueing or claims. Legacy
Slider roles do not opt in automatically. Snapshot readiness certifies completed
Core semantics only; not rendering, native numeric AT delivery or owner acceptance.
No new host/model/bridge/Python/JS/TS/Go/Rust numeric authoring is implied.

For the unreleased C-only typed Slider value/domain/math helpers, read
`docs/SLIDER_MODEL_VALUES.md` and `include/cui/cui_slider_model.h`. Single/Range
records and initialized state own their data; proposals require explicit owner
acceptance. For the separate C-only explicit-size Base route include
`cui/cui_base_slider_model.h` and read `docs/BASE_SLIDER_MODEL.md`. It paints
Single/Range horizontal/vertical geometry and emits typed synchronous proposals
with frozen current/evolving requested intent; callback may mutate external
owner/userdata, never active UI/state/options. Numeric input is current-thumb
opt-in and busy rejection is independent of owner acceptance. Styled typed
presentation, native numeric host delivery and all five language builders remain
pending. Preserve legacy scalar routes and whole-family partial status.

For in-frame prior Core geometry/semantic validity use the narrowly scoped
`cui_frame_input_snapshot_ready`, separately from outside-frame numeric readiness.
Read `docs/CORE_NUMERIC_ACCESSIBILITY.md`: readiness does not imply current
modal/target eligibility, renderer/host success or numeric capability; inert and
layout-only calls still require their ordinary frame eligibility checks. Failed
V2 admission invalidates new readiness without changing legacy input routing.

For a complete C-only controlled Base Slider program, read
`examples/slider_model_controlled.c` and `docs/BASE_SLIDER_MODEL.md`. It defines
persistent state, callback, composition and a windowless driver; owner acceptance
is explicit on UPDATE. CANCEL may carry an old kind/domain and must not be blindly
accepted. This example does not add Styled rendering or foreign builders.

Slider's unreleased C-only `cui_slider_style_resolve_v1` owns source metrics and
an exact background brush value; read `docs/SLIDER_STYLE_VALUES.md` and its header.
It is a pure resolver, not a Styled widget or renderer capability. Preserve
Aurora's gradient and distinguish transparent masked overrides; never infer a
preset from custom theme bytes or flatten a brush for unsupported adapters.

The unreleased C-only typed Styled Slider facade is in `cui_styled_slider_model.h`;
read `docs/STYLED_SLIDER_MODEL.md` and the complete compiled
`examples/styled_slider_model_controlled.c`. It reuses the Base handler and double
Single/Range ownership, with separate owned ring motion and passive absolute Core
wake deadlines. Visible V1 gradient thumbs reject atomically; copied V2 support is
a host obligation, and current adapters do not render Aurora. Inert/layout-only/
blocked/recovery calls never advance presentation; native numeric hosts and five
typed builders remain pending. Do not describe this C slice as whole Slider parity.

For the browser Slider demonstration, use the complete callback/init/compose
copy panel in `web/modules/slider_usage.js` and the gallery section of
`docs/STYLED_SLIDER_MODEL.md`. Its owned pair accepts or rejects proposals
explicitly; it does not use the legacy float autoaccept path. Both axes and
reverse/log are supported in ordinary flow. Keep the old reducer during config
transitions to deliver captured CANCEL; do not accept a cancelled old-domain pair
blindly. Convert passive Core-clock deadlines to the host monotonic clock.
The demonstration does not confer native numeric/all-five builder/V2 adapter
support or vertical registered-scroll admission.

For unreleased C bridge controlled Slider kind81, read
`docs/BRIDGE_SLIDER_MODEL.md` and matching `include/cui/cui_bridge.h`. Resolve the
complete optional symbol group before use; enable the separate owned FIFO, poll
after step and explicitly accept full double Single/Range pairs using receipts.
Polling never autoaccepts or emits legacy CHANGE. Expired records remain readable,
not acceptable. Native owner TLS and explicit final-module-verified unshared WASM
are the supported modes; default/shared WASM remains versions0. The unreleased Python controlled builder is documented in
`docs/PYTHON_SLIDER_MODEL.md`; read its complete source records in
`bindings/python/cui_bridge/slider_model.py` and runnable UI in
`bindings/bridge/examples/slider_model.py`. Resolve the complete optional group,
keep int identities and frozen copies, dispatch after legacy events, explicitly
accept full pairs, and never retain ctypes views across callbacks. Typed callbacks
may mutate external owner state/enqueue later work; nested step/run/dispatch reject.
For the matching Node/TypeScript subset read `docs/NODE_SLIDER_MODEL.md` and
`bindings/node/slider-model.cjs`; complete UI examples are
`bindings/node/examples/slider-model.cjs` and
`bindings/typescript/examples/slider_model.ts`. Keep uint64 identities as BigInt,
retain original frozen receipts (numeric clones confer no authority), and preserve
Node close versus dispose semantics. Callbacks run after legacy dispatch with
no C views; direct C custom Styled style/motion structs remain unbound.
For the Go controlled subset read `docs/GO_SLIDER_MODEL.md`, owned definitions
in `bindings/go/cui/slider_model.go`, and the complete UI in
`bindings/go/examples/slider_model/main.go`. Keep float64 values/uint64 identities,
explicit receipt acceptance and the existing creating-goroutine OS-thread lock
through Close. RequestClose only requests shutdown; copied records retain no App.
For the Rust subset read `docs/RUST_SLIDER_MODEL.md`, owned definitions
in `bindings/rust/src/slider_model.rs` and the complete UI in
`bindings/rust/examples/slider_model.rs`. Keep f64 values/u64 identities, local
issued receipt tokens and explicit full-pair acceptance. The HRTB callback context
forwards only approved UI/owner operations, never App/step/run/dispatch or owned
destruction. App remains !Send/!Sync; close only requests shutdown, Drop destroys.
Linux1.99.1 dynamic and matching bundled-static checks passed; MSRV1.85.1
remains a required follow-up. Source closure proof is not package publication. Native numeric delivery and V2
gradient adapters remain pending.

For controlled Python Slider provider records, absent input fields and inactive
numeric evidence must be zero, coordinates/numeric evidence finite, repeat 0/1,
and absent source UNKNOWN. Invalid copied evidence raises BridgeError before
publishing a request; do not invent metadata. See docs/PYTHON_SLIDER_MODEL.md and
docs/design/SLIDER_BINDING_WIRE_CONFORMANCE.md for per-language omissions.

Node/TypeScript and Go controlled Slider copied requests also reject nonzero
inactive numeric evidence before publishing receipt authority. Accessibility
INCREMENT/DECREMENT carry zero numeric evidence; explicit typed SET remains
supported. This does not imply complete type-specific inactive-field or native
accessibility coverage; inspect the per-language wire conformance guide.

## Indexed pointer regions and Table touch

Read `docs/EVENT_HOVER.md` and the full `include/cui/cui.h` contract before using
`cui_event_hovered`. It reads an indexed pointer event against prior clipped
semantic geometry, independent of consumption/capture; it does not publish
current paint bounds or perform event propagation. Enabled movable Table headers
own touch reorder while body/nonmovable header pan remains available. Follow
`docs/TABLE.md`; do not infer Dialog reopen or all-consumer lifecycle completion.
