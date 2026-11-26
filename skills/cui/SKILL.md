---
name: cui
description: Build and debug CUI user interfaces in C11, Python, JavaScript, TypeScript, Go or Rust, for native, browser/WASM, headless or embedded hosts. Use for CUI setup, component/API discovery, layout, fonts, themes, responsive composition and backend integration.
---

# Build with CUI

Use the Python 3.9+ standard-library helper beside this file. Resolve its absolute
path from this skill's location; do not assume the skill is in the app checkout.
In commands below, `HELPER` means that absolute `scripts/cui.py` path. No pip
packages are required. The CLI writes UTF-8 to stdout/stderr, including redirected
Windows pipes; consumers should decode its output as UTF-8. The skill includes offline documentation, headers and
examples; it does not include the CUI implementation or compiler toolchain.

For owned background colors, two-stop values and pure sampling, read
`docs/BACKGROUND_VALUES.md` and `include/cui/cui_background.h`. CSS and SOURCE_038
have different alpha/geometry semantics. Pure values alone do not apply paint.
For opt-in Core V2 mode/setters/emission read `docs/CORE_DRAW_V2.md` and
`include/cui/cui_draw_v2.h`. The C software adapter's actual query/render route is
in `docs/SOFTWARE_V2.md` and `include/cui/cui_software.h`: query the initialized
renderer before composition, use its support/defaults, then render V2 explicitly.
The SDL_Renderer C route is documented in `docs/SDL_V2.md` and
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

Use `cui_stepper_present` for fill connectors. Choose direction from the
container width with `cui_responsive_query`. The request callback validates
the next selection. Keep it in owner state and resubmit on the next frame.
Retained builders provide fixed connectors and queued changes. Probe
`supports_stepper_fill` (`SupportsStepperFill` in Go) before using Fill on an
earlier bridge. Pass an explicit axis, width 0 to grow, and height 0 for
intrinsic height. See the Stepper layouts in `docs/BINDING_BUILDERS.md`.
See `docs/STEPPER_NAVIGATION.md`.

Use `cui_accordion_present` or `cui_collapsible_present` for immediate C.
Pass `cui_disclosure_appearance` for inherited title tokens, state boxes and
joined divider styles. The `_present_with_title_states` variants add hover,
keyboard-focus and press typography and color for built-in enabled titles.
Retained C can copy those states with `cui_bridge_disclosure_set_title_state_v1`.
For language builders, probe `supports_disclosure_title_state`
(`SupportsDisclosureTitleState` in Go) independently. Use
`DisclosureTitleStyle` for copied interaction typography and color.
See `docs/DISCLOSURE.md` and the runnable workspace preferences examples.
For retained Accordion and Collapsible sizes, SVG indicators and trigger
styles, probe `supports_disclosure_presentation` (`SupportsDisclosurePresentation`
in Go), then copy a `DisclosureConfig`. Read queued `checked` events after
a frame. See `docs/BRIDGE_COMPONENTS.md` for typed settings, independent
resets and the runnable workspace preferences examples in all five languages.
For retained heading layers and joined dividers, probe
`supports_disclosure_heading` and `supports_disclosure_divider` independently.
Go uses `SupportsDisclosureHeading` and `SupportsDisclosureDivider`. Each
language's workspace preferences example shows the typed setters.

Use `cui_rating_present` for shared Rating sizes, custom SVG masks, colors and
outline-only preview. The app owns the numeric value. Set
`CUI_RATING_STEP_DOWN_FILLED` for opt-in step-down and choose the bool or the
typed callback for side effects. Retained C uses `cui_bridge_rating_configure_v1`
and queued scalar changes. Python, JS/TS, Go and Rust expose copied
`RatingConfig`. Probe Rating options on earlier providers. See the Rating
feedback forms in `docs/BRIDGE_COMPONENTS.md`.

For retained C VirtualList, use `cui_bridge_virtual_list_set_rows_v1` for copied
row order and extents and opt into anchor preservation. Send scroll input
through the existing host path. See live updates in `docs/VIRTUAL_LIST.md`.

For RadioGroup descriptions, decorative content, tooltips, signed tab order
and C-only paint callbacks, read `docs/RADIO_GROUP.md`. For a controlled
standalone Radio, use `cui_radio_events` or `cui_base_radio_events` in C.
An unchecked option can synchronously request selection. The caller owns
acceptance and invalidates to display the change. Retained bindings use queued
events. See `docs/BOOLEAN_EVENTS.md#standalone-radio`. For retained order,
probe the optional bridge symbol `cui_bridge_radio_set_tab_index_v1` or the
matching capability on Python, JS/TS, Go or Rust. For Rust, probe before
`build_child`. See `docs/BRIDGE_COMPONENTS.md#set-signed-radio-tab-order`.

Keep one zero-initialized `cui_radio_tooltip_state` per option. Retained
applications can use the Item detail setter
in `docs/BRIDGE_COMPONENTS.md` for copied accessible descriptions.
Retained C can add an optional copied hint through
`cui_bridge_radio_set_tooltip_v1` on Radio or a direct RadioGroup Item.
Check for the symbol with dynamically loaded providers. In Python and
JS/TS, check `app.supports_radio_tooltip()` before calling
`option.radio_tooltip(text)`. Rust uses the same probe and
`Builder::radio_tooltip(text, delay_ms)`, with `Some(text)` to set a hint
and `None` to clear. In Go, use `app.SupportsRadioTooltip()` and
`option.RadioTooltip(text)`, with empty text to clear.
The same guide covers passive children inside one RadioGroup Item through
`cui_bridge_radio_item_set_content_v1`, `item.decorate_content()` in Python and
JS/TS, `item.DecorateContent(enabled)` in Go, or
`Builder::decorate_content(enabled)` in Rust. For older providers, check
`app.supports_radio_item_content()` or Go's `app.SupportsRadioItemContent()`.
The retained C bridge and all five language builders support copied Radio paint.
Check `supports_radio_paint()` or Go's `app.SupportsRadioPaint()` before using
layers with an older provider. See
`docs/BRIDGE_COMPONENTS.md#style-radio-states-in-language-builders`.
For retained Radio selection animation, read the Radio section of
`docs/BRIDGE_COMPONENTS.md`. Python, JS/TS and Rust use
`app.supports_radio_motion()`. Go uses `app.SupportsRadioMotion()`.

For HoverCard, read the fit section of `docs/CONTENT.md`. Zero height fits
children and may request one follow-up for edge placement. A child height
change remeasures automatically. Tap dismissal and its host limits are in
`docs/HOVER_CARD_EXTENDED.md`.
For Popover focus and trigger semantics, read `docs/POPOVER_ANCHORS.md`.
Use `cui_focus` only on the accepted opening action inside a successful begin,
then call `cui_popover_link_trigger` with the current begin result after the panel.
Use `cui_popover_begin_with_outside_focus` and
`CUI_POPOVER_OUTSIDE_FOCUS_KEEP_POINTER` to keep a nonmodal outside click's focus.
For right-click, call `cui_popover_take_secondary_press` after submitting the trigger.

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
| Video surface and player | `docs/VIDEO_PLAYER.md`, `include/cui/cui_video_controls.h`. C audio controls require confirmed mute/gain and host capabilities. The browser demo and optional native FFV1/PCM example provide distinct host paths. No foreign player facade. |
| Python, Node and TypeScript authoring | `docs/BINDING_BUILDERS.md`, `docs/BRIDGE_COMPONENTS.md`, `docs/binding-coverage.json` |
| Go or Rust authoring | `docs/GO_BUILDERS.md` or `docs/RUST_BUILDERS.md`, then bridge coverage |
| Hosting C-authored UI from another language | `docs/LANGUAGE_BINDINGS.md` (different from language component builders) |
| Embedding/custom host or renderer | `docs/INTERFACES.md`, `docs/PLATFORM_MATRIX.md`, relevant `backends/*/README.md` |
| Single-date time editing | `docs/DATE_TIME_PICKER.md`, `include/cui/cui_date_time_picker.h`: separate retained date/time value/state, explicit assignment, Base composition helpers or one-reduction Styled facade, bounded presets/clear/Delete. Same-day confirms; changed-day stays open. Gallery DatePicker has an opt-in Date + time mode with headless desktop/touch proof and copyable C11 usage. No timezone/range/chrono parsing, binding builder or callback stream; oversized Styled popup rejects before mutation. |
| Localized TimeField and bridge delivery | Use `docs/TIME_FIELD_LOCALIZED.md` for caller-owned C fields, sizes, ordered semantic values, accessibility Focus and optional IME. Try **One stop per field** in the gallery or `examples/time_field_ordered.c` for the C roster. Ordered IME uses a generation-qualified segment in the gallery, SDL3 or a custom C host. [Python](docs/PYTHON_TIME_FIELD_LOCALIZED.md), [Go and Rust](docs/GO_RUST_TIME_FIELD_LOCALIZED.md), and [JS/TS](docs/NODE_TIME_FIELD_LOCALIZED.md) include headless alpha.5 examples. The builders omit the ordered frame, direct C sizes and IME. |
| Responsive Pagination | `docs/NUMERIC.md#pagination` covers wrapping at the current inner `available_width`, localized chevrons, jump menus and themed states for the extended pager. Set `custom_size` (1–512 logical px, zero restores named sizing) in immediate C or use `cui_bridge_pagination_set_custom_size` in retained C. Python, JS/TS, Go and Rust require a provider with both height exports. Check support in the examples in `docs/BINDING_BUILDERS.md`, `docs/GO_BUILDERS.md` and `docs/RUST_BUILDERS.md`. |
| Chart display density | Use bitmap pixels for `cui_chart_options.line_width`. For a uniformly scaled Line or Area bitmap, multiply the desired logical width by the bitmap-to-display scale. In C, use `CUI_CHART_LAYOUT_SOURCE_POINT` and set `show_dots` for eight-logical-pixel Line markers. See `docs/CHART.md`. |
| Plot gestures | Retain `cui_plot_viewport`, reduce before projecting, and bind after submission. Use `cui_plot_viewport_reduce_with_touch_policy` with `CUI_PLOT_TOUCH_YIELD_VERTICAL` to offer unconsumed vertical touch pans to an enclosing host. See `docs/PLOT_INTERACTIONS.md`. |
| Calendar presentation and localized dates | `docs/PICKERS.md`, `docs/CALENDAR_PRESENTATION.md`, `docs/CALENDAR_BEHAVIOR.md`; read `include/cui/cui_calendar_presentation.h` and `include/cui/cui_calendar_behavior.h` for opt-in month/today/neighbor and independent range/year/recenter policies, matching-header/library rebuild and binding limits. DatePicker forwarding uses `docs/DATE_PICKER_CALENDAR.md` and `include/cui/cui_date_picker_calendar.h`; legacy typed CHANGE does not observe extensions or lifecycle. |
| Layout and responsive composition | `docs/LAYOUT_GUIDE.md`, `docs/APP_RECIPES.md`, `docs/FORMS.md`, `docs/COMPOSED_APPS.md` |
| Fonts, metrics, fallback and shaping | `docs/TYPOGRAPHY.md`, `docs/TEXT.md`, `docs/BIDI_SHAPING.md`, `docs/IME.md` |
| Theme, density, colors and icons | `docs/DESIGN_SYSTEM.md`, `docs/THEME_GUIDE.md`, `docs/THEME_REGISTRY_GUIDE.md`, `docs/ICON.md` |
| Accordion and Collapsible sizes, indicators, joined groups and trigger styles | `docs/DISCLOSURE.md`, `docs/DISCLOSURE_EVENTS.md`, `docs/BRIDGE_COMPONENTS.md#style-retained-accordion-and-collapsible-from-c` |
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
For native typed Slider accessibility on Linux, read the Numeric Slider actions
section of `docs/NATIVE_ACCESSIBILITY.md` and `include/cui/cui_accessibility.h`.
Use the opt-in adapter and keep numeric requests separate from confirmed values.
For responsive UI, derive available space from the host and use the documented
layout constraints. See `docs/RESPONSIVE_LAYOUT.md` and the native starter in
`docs/NATIVE_APP_STARTER.md` for a sidebar that becomes a compact drawer while
preserving page, account and edit state. Use distinct IDs for controls in
alternate navigation layouts and transfer focus to a visible counterpart when
closing an overlay at a breakpoint. The starter also uses auto-measured
`cui_scrollable_begin` / `cui_scrollable_end_ex` below its pinned Menu and
reveals focused controls in short windows. Its Projects page keeps separate
Resizable state per axis, sizes a bounded Tabs strip from the detail pane,
and restores tab focus when the overflow button leaves the layout.
For multiple visible Carousel slides, use `cui_carousel_with_basis` and pass
one fraction per slide. In the retained C bridge, set the fraction on each
Carousel Item with `cui_bridge_set_carousel_slide_basis_v1`. In the language
builders, check `supports_carousel_basis` (`SupportsCarouselBasis` in Go) and
use `carousel_basis` (`CarouselBasis` in Go) on a direct Item. For vertical
layout, probe `supports_carousel_axis` (`SupportsCarouselAxis` in Go) before
configuring the Carousel. Vertical fractions use the viewport height. Probe
`supports_carousel_looping` (`SupportsCarouselLooping` in Go) before enabling
retained edge wrapping with `carousel_looping` (`CarouselLooping` in Go).
Check `supports_carousel_controls` (`SupportsCarouselControls` in Go) for
opt-in arrows and slide markers. The controls share the ordinary Carousel
selection event. Probe `supports_carousel_slots` (`SupportsCarouselSlots` in
Go) to compose passive arrow and marker artwork. Attach arrow slots to the
Carousel and marker slots to the owning Item. The outer button still owns
interaction. Probe `supports_carousel_control_sizes` (`SupportsCarouselControlSizes`
in Go) to set independent arrow and marker sizes with the shared design
scale. Immediate C also offers `cui_carousel_with_layout` for content-sized
slides. Retain one zero-initialized `cui_carousel_fit` per fitted slide and keep
it with the slide ID when reordering. Drive follow-up frames with
`cui_needs_redraw`. In retained C and language builders, the bridge owns
fit measurements. Use `cui_bridge_set_carousel_slide_fit_v1` or
`carousel_fit` (`CarouselFit` in Go) on a direct Item. Probe
`supports_carousel_fit` (`SupportsCarouselFit` in Go) with an older
provider. A valid fraction replaces fit, and enabling fit replaces a
fraction. For freely placed retained navigation buttons, use
`cui_bridge_carousel_adjacent_v1` to check availability and
`cui_bridge_carousel_step_v1` to select and enqueue one copied change.
Probe `supports_carousel_navigation` (`SupportsCarouselNavigation` in Go)
before using the wrapper methods. Query after layout. Read
`docs/CAROUSEL.md` for layout and examples.
For a Command palette inside a dialog, set `cui_command_options.width = -1` to fit
the body. Custom rows measure at the current width in a fixed-width, unclipped
column, or follow up when layout still needs to allocate width. Reflow keeps
a fully visible selection in view. See `docs/COMMAND.md`.
For shortcut hints, `cui_kbd_format_stroke` uppercases the first Unicode scalar,
including expansions. Use `CUI_KBD_TEXT_CAPACITY` for a complete result. Hosts
own shortcut bindings. Retained C and the five language builders support
typed strokes and accessible names when the provider exports
`cui_bridge_set_kbd_stroke_v1`. Use a literal with older providers.
Use `cui_kbd_styled` with shared box, typography or text paint for keycaps.
Plain hints skip those overrides. See `docs/KBD.md`.
For loading placeholders, see `docs/SKELETON_RESPONSIVE.md`. Keep motion per
shape, schedule the earliest wake and attach busy state to the application region.

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
The composed route discards previous scoped input and captures before submitting
the first frame after reopening. Use its open helper for every lifetime.
Root/host ownership and foreign slot builders remain open. Read the guide's
validation scope for each example and backend.

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

For Input and Textarea keyboard behavior, see `docs/INPUT.md` and
`docs/TEXTAREA.md`. Both support Ctrl/Alt+Left/Right word movement and Shift
selection. Single-line Input keeps its caret in place on Up/Down. Textarea
navigates visual rows and keeps a full-width wrap caret and its IME anchor inside
the viewport. Growing single-line Input uses the current layout width. Set
`width = -1` in a sized parent and query the actual offset with
`cui_input_scroll_x` after a completed frame. The query is C-only. Growing
Textarea still has documented same-frame rewrap limits.

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

For bounded Tabs, retain scroll state and set `bar_extent`. The gallery and
SDL3 support Shift-wheel for horizontal scrolling. See
`docs/TABS_SCROLL_CHAINING.md` for input ownership and host limitations.
Set `reveal_selected` to scroll newly selected tabs into view, including
keyboard and touch selection from the optional overflow menu.
Use the SDL window interface for TitleBar. Decorated Cocoa windows report
native traffic lights, and borderless windows use client controls. Read
`docs/CHROME.md` for available actions.
DataTable wheel steps match Scrollable at 48 logical pixels. Custom hosts
that send pixel deltas must set `scroll_precise`. See `docs/TABLE.md`.
Reuse the bound model between frames so only virtual cells are composed in
long inboxes. See the measured SDL workload in `docs/PERFORMANCE.md`.
For live DataTable updates in C, capture with `cui_table_capture_row_anchor`
before changing the borrowed rows. After a successful bind, assign the result
of `cui_table_restore_row_anchor` to `state.scroll_y`. Pass the corresponding
table options, including the effective scrollbar style. Selection stays keyed.
See the **Live updates** gallery example and `docs/TABLE.md`. The retained C
bridge offers `cui_bridge_data_table_set_keyed` for copied matrices up to 32 rows.
For larger C matrices, query `cui_bridge_data_table_max_row_capacity_v1` and
use `cui_bridge_data_table_set_keyed_capacity_v1` with an explicit row capacity.
The current provider supports up to 10,000 rows within an app-wide 32 MiB
retained budget. Python exposes the same opt-in through
`table_data(headers, rows, row_keys=keys, row_capacity=capacity)`; query
`app.data_table_max_row_capacity()` first. See `docs/BRIDGE_COMPONENTS.md`
and `docs/BINDING_BUILDERS.md`. For ordinary Python tables, use `table_data(headers, rows,
row_keys=keys)` after checking `app.supports_keyed_data_table()`. The runnable
inbox in `bindings/bridge/examples/data_table_live.py` defaults to headless.
JavaScript and TypeScript use `table_data(headers, rows, rowKeys)` with the same
keyed capability query. For larger snapshots, query
`app.data_table_max_row_capacity()` and pass `{rowCapacity}` as the fourth
argument. See `docs/BINDING_BUILDERS.md`. Go uses `TableDataKeyed` and
`SetTableKeyed`, with `SupportsKeyedDataTable` for provider checks.
For larger snapshots query `app.DataTableMaxRowCapacity()` and use
`TableDataKeyedCapacity` and `SetTableKeyedCapacity`. A zero limit means the
provider lacks the opt-in API. See `docs/GO_BUILDERS.md` and
`bindings/go/examples/data_table_live/main.go`. In Rust, use
`Builder::data_table_keyed` or `App::data_table_keyed` after checking
`app.supports_keyed_data_table()`. For larger snapshots, query
`app.data_table_max_row_capacity()` and use `data_table_keyed_capacity` on
the app or builder. The query returns `None` when the provider lacks it.
See `docs/RUST_BUILDERS.md` and
`bindings/rust/examples/data_table_live.rs`.
For paging, use `result.load_more` with an app-owned in-flight flag and keep
the current rows interactive. See **Paged inbox** and the paging guide in
`docs/TABLE.md`. Retained C owners configure the threshold with
`cui_bridge_data_table_configure_load_more_v1` and poll a copied record via
`cui_bridge_data_table_poll_load_more_v1`. See the paging inbox in
`docs/BRIDGE_COMPONENTS.md`. Python and JS/TS use `configure_load_more` and
`poll_load_more` after checking `app.supports_data_table_paging()`. JS/TS
demand has a `bigint` frame sequence. See offline inboxes and loop options in
`docs/BINDING_BUILDERS.md`. Go uses `LoadMore`, `ConfigureLoadMore` and
`PollLoadMore` after checking `SupportsDataTablePaging`. See the inbox and
owner-thread `RunWith` hook in `docs/GO_BUILDERS.md`. Rust uses
`data_table_load_more` and `poll_data_table_load_more` with the same capability
query. See the headless and native inbox in `docs/RUST_BUILDERS.md`.
For variable-height VirtualList updates in C, capture an anchor before
rebuilding keys or changing heights and restore it before the next draw.
Retain the within-row offset for surviving keys. Clear it if a removed item's
replacement should begin at the viewport edge. The retained C, Python, JS/TS,
Go and Rust APIs copy a complete projection of up to 64 vertical Items. Probe
live-row support before enabling anchoring. Explicit scroll wins. Earlier
providers keep uniform rows. See the runnable inboxes in `docs/VIRTUAL_LIST.md`.
For Resizable composition, read `docs/RESIZABLE.md`. Keep caller state/slots alive
through frame completion and ensure the container fits the panels' combined
min/max range. A pane at its constraint cannot move farther in that direction.
Group padding reduces the available extent; reserve each pane's inner room in
its minimum size when supplying panel padding. The divider stays on the seam.
Repeated drag and keyboard resizing have focused WASM and private X11 SDL
software coverage, including a padded workspace with focused editing across
host resize. Physical devices and hardware rendering remain open.
The gallery nested editor/console mode uses distinct
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
namespaces may have no box. For a responsive toast and keyboard/touch Close
before hover, use direct C `cui_notifications_responsive`; honor its wake deadline.
See `docs/STATUS_COMPONENTS.md#notification`. Scroller row IDs follow retained keys, not stable
indices. Language-builder MAIN/ROOT/subpart targets remain pending.
For grapheme-safe one-line Attachment labels, use `cui_attachment_with_metadata`
or `cui_attachment_events_with_metadata` in immediate C. Zero typography options
give medium titles and regular descriptions. The semantic tree keeps the full
labels. Retained C can use `cui_bridge_attachment_configure_v1` and
`cui_bridge_attachment_set_preview_rgba_v1` for fitted labels, axis, status
refinements and copied decoded pixels. Probe the capabilities separately.
Python, JS/TS, Go and Rust provide typed options and independent probes for
presentation and previews. Retained progress uses a fraction. See
`docs/MEDIA.md#fit-file-names-and-descriptions` and
`docs/BRIDGE_COMPONENTS.md#show-an-attachment-preview-in-a-language-builder`
for calls and runnable examples. Retained C can opt into built-in Open, Remove
and Retry with `cui_bridge_attachment_set_actions_v1`. Request
`CUI_BRIDGE_VALUE_ATTACHMENT_ACTION_V1` when polling. The actions request app
work and never change transfer status. Python, JS/TS, Go and Rust support the
independent actions through optional builders and manual component notification
polls. Manual polling and ordinary dispatch share one queue. For immediate C
slot composition, read `docs/MEDIA.md#compose-a-file-card-with-custom-slots` and
`web/attachment_slots_example.h`. In retained C use
`cui_bridge_attachment_slot_create_v1` for preview fallback, overlay, metadata
or an action row. Children keep their own events and are removed with the slot.
In Python and JS/TS compose `attachment_slot(key, role, preferred_width)` as
a direct child of an Attachment and probe `supports_attachment_slots()` for
older providers. Go uses `AttachmentSlot` and `SupportsAttachmentSlots`.
Rust uses `App::create_attachment_slot` with an `AttachmentSlot` variant and
checks `App::supports_attachment_slots`. See
`docs/BRIDGE_COMPONENTS.md#compose-a-retained-attachment-from-c`,
`docs/BRIDGE_COMPONENTS.md#compose-a-retained-attachment-in-a-language-builder` and
`docs/ATTACHMENT_EVENTS.md#retained-actions-from-language-builders`.

For a pinned navigation header/footer with body-only scrolling, use
`cui_sidebar` with a persistent measurement cache. Tab and accessibility focus
reveal mounted controls using the previous completed geometry. Pass a
`scrollbar_style` to configure the lane. See `docs/SIDEBAR.md` for scroll,
focus and virtualization behavior.

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

For the C-only typed Slider value, domain and math helpers, read
`docs/SLIDER_MODEL_VALUES.md` and `include/cui/cui_slider_model.h`. Single/Range
records and initialized state own their data; proposals require explicit owner
acceptance. For the explicit-size C Base route include
`cui/cui_base_slider_model.h` and read `docs/BASE_SLIDER_MODEL.md`. It paints
Single/Range horizontal/vertical geometry and emits typed synchronous proposals
with frozen current/evolving requested intent; callback may mutate external
owner/userdata, never active UI/state/options. Numeric input is current-thumb
opt-in and busy rejection is independent of owner acceptance. See the Styled and
retained builder guides below for their additional contracts. Preserve legacy
scalar routes and whole-family partial status.

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

Slider's C-only `cui_slider_style_resolve_v1` owns source metrics and
an exact background brush value; read `docs/SLIDER_STYLE_VALUES.md` and its header.
It is a pure resolver, not a Styled widget or renderer capability. Preserve
Aurora's gradient and distinguish transparent masked overrides; never infer a
preset from custom theme bytes or flatten a brush for unsupported adapters.

The C-only typed Styled Slider facade is in `cui_styled_slider_model.h`;
read `docs/STYLED_SLIDER_MODEL.md` and the complete compiled
`examples/styled_slider_model_controlled.c`. It reuses the Base handler and double
Single/Range ownership, with separate owned ring motion and passive absolute Core
wake deadlines. Visible V1 gradient thumbs reject atomically; copied V2 support is
a host obligation, and current adapters do not render Aurora. Inert/layout-only/
blocked/recovery calls never advance presentation. This C facade covers the
documented Slider presentation subset.

For the browser Slider demonstration, use the complete callback/init/compose
copy panel in `web/modules/slider_usage.js` and the gallery section of
`docs/STYLED_SLIDER_MODEL.md`. Its owned pair accepts or rejects proposals
explicitly; it does not use the legacy float autoaccept path. Both axes and
reverse/log are supported in ordinary flow. Keep the old reducer during config
transitions to deliver captured CANCEL; do not accept a cancelled old-domain pair
blindly. Convert passive Core-clock deadlines to the host monotonic clock.
The demonstration does not confer native numeric/all-five builder/V2 adapter
support or vertical registered-scroll admission.

For C bridge controlled Slider kind81, read
`docs/BRIDGE_SLIDER_MODEL.md` and matching `include/cui/cui_bridge.h`. Resolve the
complete optional symbol group before use; enable the separate owned FIFO, poll
after step and explicitly accept full double Single/Range pairs using receipts.
Polling never autoaccepts or emits legacy CHANGE. Expired records remain readable,
not acceptable. Native owner TLS and explicit final-module-verified unshared WASM
are the supported modes; default/shared WASM remains versions0. The Python controlled builder is documented in
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
current paint bounds or perform event propagation. Use `cui_hovered` for the
latest pointer position and ordinary hover styling. Virtualize long lists
when layout permits; see `docs/PERFORMANCE.md`. Enabled movable Table headers
own touch reorder while body/nonmovable header pan remains available. Follow
`docs/TABLE.md`; do not infer Dialog reopen or all-consumer lifecycle completion.
Use `cui_table_begin_row_ex` for a static Table's custom header and row fills.
Set `striped` for DataTable body stripes. Selected cells take precedence.

For Skeleton, read `docs/SKELETON_PALETTE.md`: legacy phase APIs remain distinct
from scheduled fixed-size `cui_skeleton_ex` and owned-color `cui_skeleton_with_palette`.
Explicit preset resolution preserves Skeleton source colors and authored alpha0;
`from_theme` is a secondary-color fallback, never inferred preset identity.
Focused worker native/software/WASM proof and root strict/distribution checks
pass (17 headless checks and 18 skill tests). The additive `cui_skeleton_present` route owns its responsive root layout; read
`docs/SKELETON_RESPONSIVE.md` and its header. Use the optional retained C bridge in `docs/BRIDGE_SKELETON.md` for copied
configuration, responsive sizing, and per-node wake hints on the Core clock.
Use `docs/PYTHON_SKELETON.md` for the optional Python responsive builder and
the runnable profile UI. See `docs/NODE_SKELETON.md`, `docs/GO_SKELETON.md`
and `docs/RUST_SKELETON.md` for their responsive builders. The legacy factory
works with alpha.4.

For app-owned SVG data, read `docs/BRIDGE_ICON_RESOURCES.md` and
`include/cui/cui_bridge.h`. Working source adds
`docs/BRIDGE_ICON_PRESENTATION.md` for copied styling, shared resources and
per-frame results in retained C nodes. Probe presentation separately from the
alpha.4 resource API. `docs/BRIDGE_BADGE.md` covers the optional C container and
its child events, artwork and indicator result. Keep resources on the app's owner
thread. Python supports those resources and retained builders. Run the Python
inbox example from `docs/PYTHON_ICON_BADGE.md` or use the JavaScript and TypeScript
builders in `docs/NODE_ICON_BADGE.md`. Go covers resources and retained
authoring in `docs/GO_ICON_RESOURCES.md` and `docs/GO_ICON_BADGE.md`.
Rust covers both the owned SVG resource API in `docs/RUST_ICON_RESOURCES.md`
and retained Icon and Badge authoring in `docs/RUST_ICON_BADGE.md`.
