# Remaining parity: implementable work at the next39 checkpoint

Audit baseline: `9c61c158`, `integration/next39-components`. Frozen comparison:
GPUI Kit 0.7.1, commit `87d10ae5e1299d1be18670a37570c5402f952e34`, recorded in
[the upstream baseline](../upstream-baseline.md). This is a read-only scheduling
audit, not production work, a fresh runtime verification report or a promotion of
any family. All 78 manifest entries must remain.

## How to read the evidence

[The manifest](../gpui-parity.json) contains 78 families: 77 `tested`, one
`implemented` (Questionnaire). Its 360 feature rows contain 315 `tested`, four
`implemented` and 41 `not_started`. These are recorded classifications, not a
completion percentage: `tested_partial` platforms, inherited styling limitations,
provider differences and unverified host behavior still matter. Conversely,
several old feature rows describe work that now has implementation and tests.

This refresh inspected the public declarations, implementation seams and focused
test sources cited below at the audit baseline. Earlier audit conclusions and
recorded passing gates remain historical evidence; **no native, browser, binding
or packaging tests were rerun for this documentation-only refresh**. Inspection
is not a fresh runtime verification or a new frozen-source certification of every
family. Proposed acceptance cases are future work unless explicitly identified as
existing tests. Existing guides can contain chronological paragraphs superseded
by later additions; follow the actual current API and implementation.

## Do not schedule these requests from stale rows

| Stale shorthand | What is already present | What remains distinct |
| --- | --- | --- |
| Questionnaire lacks automatic root capture/cross-item focus | [Styled implementation](../../src/components/cui_questionnaire.c) routes through prior-frame bindings and ordered input reduction; [styled tests](../../tests/unit/test_questionnaire_styled.c) cover automatic routing, omission and focus. [Guide](../QUESTIONNAIRE.md) ends with the newer contracts. | Bounded box-slot refinements and six localized labels landed in the follow-up below; nested roots, comprehensive callback choreography, broader styling and localization still need completion. Earlier paragraphs and manifest prose are not the final source truth. |
| Icon padding/nonzero style is unsupported; no accessible API | [Icon options](../../include/cui/cui_icon.h), [resolver](../../src/base/cui_icon_layout.h), [source-style tests](../../tests/unit/test_icon_style_source.c), [accessibility tests](../../tests/unit/test_icon_accessibility_source.c) implement numeric bounds/padding/border floors, full-box paint, explicit image labels and decorative suppression. | Ambient inheritance, generic Styled layout and SVG renderer fidelity remain limited. Do not redo numeric constraints or add guessed asset-name labels. |
| Slider has no controlled value/semantics | [Base API](../../include/cui/cui_base.h) and [controlled tests](../../tests/unit/test_slider_controlled.c) already define typed BEGIN/UPDATE/COMMIT/CANCEL requests and read-only behavior without a handler. | Typed Base/Styled facades and aggregate bridge scalar notifications now exist; foreign controlled phases remain distinct. Do not replace phased semantics with one bool change callback. |
| Text gradients/tokens/tracking are wholly absent | [Tokens](../TYPOGRAPHY_TOKENS.md), [gradient implementation](../TEXT_GRADIENT.md), [native tracking](../LETTER_SPACING.md), [real gallery gradient proof](../../tests/web/test_gradient_gallery.py). | Canvas tracking, foreign authoring and GPU support are unfinished. Reimplementing solid/gradient paint or native tracking would duplicate completed work. |
| Native SDK text is unshaped; App.run only polls a fixed-frame loop | [SDK parity guide](../SDK_TEXT_PARITY.md), [installed text fixture](../../tests/packaging/test_installed_text.py), [bridge runner](../../src/bridge/cui_bridge.c), language run tests in bindings. | Provider coverage and style/event API gaps below remain. Launcher-only examples are not evidence about the actual bindings. |
| Every native accessibility adapter is absent | [AccessKit adapter](../../backends/accesskit/cui_accessibility.c), [native guide](../NATIVE_ACCESSIBILITY.md), [integration test](../../tests/integration/test_accessibility_native.c) exist. | Real assistive-technology acceptance and complete family/platform mappings must be evaluated within that adapter's documented support, not inferred from semantic snapshots. |

## Ranked production slices

Priority reflects the explicit typography/event/frozen-family requests and a
small reviewable implementation boundary, not a claim that lower entries are
less important forever. Each slice needs a separate approved owner and generated
consumer refresh by the integrator where public ABI/source changes require it.

### 1. Close Spinner proof and consumer gaps after the shipped corrections

The previously scheduled finite-turn easing, reduced ease(0), and named-rem versus
custom-pixel descriptor changes are implemented in
[Spinner submission](../../src/components/cui_content.c) and
[public options](../../include/cui/cui_content.h). Spinner accepts finite overshoot,
normalizes in double before radians, and evaluates custom easing at zero under
reduction without scheduling a wake. Skeleton and the legacy shared sampler keep
their bounded policy. [Contract tests](../../tests/unit/test_spinner_contract.c)
and [motion tests](../../tests/unit/test_status_motion.c) contain native regression
coverage; this refresh inspected rather than executed them.

Next bounded slice: extend the [real WASM fixture](../../tests/web/test_spinner_gallery.py)
to negative/>1 turns, shifted reduced ease(0), nonfinite failure and named rem at
multiple root scales versus fixed custom64. Separately combine caller-owned raw
SVG, numeric Icon style, explicit color, rotation and source replacement while
consuming retained draws before destruction. The [dedicated audit](../audits/SPINNER_PARITY.md)
retains the full proof boundary, including independent mounts/source curves.
The bridge still uses legacy eight-dot Spinner; source-style bridge options need
their own additive contract and tests. Neither these shipped native corrections
nor old default-curve browser checks establish full Spinner parity.

### 2. Deliver browser tracking with the same declared shaping policy

[Native text](../../src/text/cui_text.c) implements cluster tracking and advertises
`CUI_TEXT_CAP_LETTER_SPACING`; [Core](../../src/core/cui.c) gates unsupported
providers. [Gallery Canvas host](../../web/gallery.js) still measures/draws browser
text without this capability. Nonzero browser tracking is unavailable, not a
completed browser feature. Native tests already cover no trailing gap, ligatures,
negative progression, wraps and density; preserve them.

First decision: evaluate whole-context Canvas `letterSpacing` with actual cluster,
ligature and bidi geometry. It cannot simply be advertised if browser spacing
counts different boundaries or adds a terminal gap. If it fails the portable
contract, use an optional shared WASM-shaped provider plus positioned glyph masks,
with explicit font ownership/capabilities; do not draw UTF-8 characters separately
or silently approximate. This is a provider decision before adding a gallery knob.

Acceptance: a new real-WASM/headless browser tracking fixture, using explicit
fonts and actual pixels/geometry, covering ligature/combining/Arabic/RTL/mixed
runs, narrow/wide reflow, negative rejection, no final gap, selection/carets,
DPR1/2 and mobile width. Compare the policy, not exact cross-platform raster bytes.
Extend [native tests](../../tests/integration/test_letter_spacing.c) only where
new shared behavior needs regression; preserve zero-spacing exact compatibility.

### 3. Extend font selection without pretending four weights are CSS

[Text style declarations](../../include/cui/cui.h) expose family IDs, four named
weights and italic/monospace flags. [Native registry](../../src/text/cui_text.c)
selects registered faces; arbitrary numeric weight, independent same-family
stretch and variable axes are not authorable. Registering a condensed static face
as another family already works and should remain a documented interim solution.
[Typography roadmap](../TYPOGRAPHY_ROADMAP.md) contains illustrative future types,
not shipped declarations; semantic tokens themselves are already implemented.

Bounded first slice: additive versioned face-selection descriptor and explicit
static-face registrations for numeric weight/stretch, with exact/fallback policy
and consistent measurement/raster/cache keys. Variable axes/optical sizing should
follow only with identity, range/clamping, ownership and provider support defined.
Preserve old enum meanings/defaults and reject unsupported combinations observably.
Acceptance needs actual differing registered faces, geometry/pixels, cache
invalidation, missing-face policy and installed binding consumers; font-stack CSS
strings alone cannot prove equivalent native selection.

### 4. Audit uncovered C lifecycle seams instead of rebuilding shipped facades

The activation proposal is no longer the implementation frontier. The current
[78-family inventory and additive checkpoint](COMPONENT_EVENTS_COVERAGE.md)
records shipped Base/Styled facades for activation, boolean requests, Slider,
Input, multiline observers, selections, surfaces, menus, numeric/picker controls,
OTP, disclosures, List/ToggleGroup, Tree, DataTable, Attachment, TimeField,
Sidebar, Command, Toolbar, InputGroup and Settings. Separate
[Scroll](../SCROLL_EVENTS.md) and [Hover](../HOVER_EVENTS.md) guides cover their
newer lifecycle observers. [Public metadata](../../include/cui/cui_component_events.h)
is shared infrastructure, not the only shipped event API. Focused tests such as
[controls](../../tests/unit/test_control_events.c),
[table](../../tests/unit/test_table_events.c),
[scroll](../../tests/unit/test_scroll_events.c) and
[hover](../../tests/unit/test_hover_styled_events.c) are existing source evidence.

Next bounded audit: identify a genuinely uncovered lifecycle/result seam from the
current inventory, specify multiplicity, snapshot point and retention, then add
only that observer with meaningful headless regressions. For example DataTable
viewport/load-edge completion remains result-only; do not reinterpret it as an
existing notification. Render-only primitives do not need invented value events.
Preserve controlled request versus applied CHANGE, aggregate versus per-event
counts, callback order, secure redaction and explicit unsupported accessibility
channels. No generic capture/bubble, context destruction during synchronous
callbacks or universal callback mutation support is established.

### 5. Extend the bridge's remaining typed allowlist and typography authoring

Versioned [V1 notifications](../BRIDGE_NOTIFICATIONS.md) and opt-in
[value notifications](../BRIDGE_VALUE_NOTIFICATIONS.md) are shipped in the
[C bridge API](../../include/cui/cui_bridge.h) and implementation. Python,
JS/TypeScript, Go and Rust have owning manual views. Initial CLICK, boolean,
DataTable and close payloads are followed by explicit scalar
(Slider/NumberInput/Rating/Pagination), plain-text
(Input/Textarea/Editor/InputGroup), and selected-handle
(RadioGroup/Tabs/Select/List/Stepper) allowlists. Copied text, raw table columns,
historical generations and exact uint64 identities must not be rescheduled as
missing blanket features. Existing C tests cover
[initial](../../tests/unit/test_bridge_notifications.c),
[scalar](../../tests/unit/test_bridge_value_notifications.c),
[text](../../tests/unit/test_bridge_text_notifications.c) and
[selection](../../tests/unit/test_bridge_selection_notifications.c) queues.

Next event slice: choose one remaining LEGACY producer, such as Questionnaire,
and define an additive typed notification captured at its real enqueue seam.
Questionnaire still encodes kind in `value` and completion in `checked`;
Combobox/menu/picker and other unlisted producers must stay LEGACY until audited.
Preserve shared-FIFO single consumption, old-poll downgrade, capacity retry,
optional-symbol negotiation and owning wrapper copies. Existing manual polls do
not bind direct-C facades, add per-input multiplicity, register callbacks or offer
synchronous rejection; old callback conversions remain their documented legacy
behavior. [Binding audit](COMPONENT_EVENTS_BINDINGS.md) and the per-language value
guides define the exact boundary, including paired static Rust SDK requirements.

Separate consumer slice: expose native tracking/font/paint authoring and an
explicit source-style Spinner with reduced motion. Current flat styles omit
native `letter_spacing`, and bridge Spinner remains legacy. New fields need
observable unsupported-provider failure and actual installed consumer tests;
manual notification success cannot prove typography support.

### 6. Complete Questionnaire beyond bounded box slots and labels

Follow-up `5744beb8` implements three independent masked box-slot descriptors
and six localized action/error labels. [The guide](../QUESTIONNAIRE.md) defines
default/custom-renderer precedence, submission snapshots, borrowed text lifetime,
invalid preflight and the by-value options ABI rebuild requirement. Focused native
and sanitizer evidence covers all four legacy sizes, zero/transparent overrides,
translated text and default compatibility. This is not browser or full-family proof.

Next slices: expose and verify the remaining typography/general style refinements,
audit frozen part methods, then test nested roots, subscriber-driven enabling/
navigation and callback mutation ordering against frozen Base tests. No arbitrary
callback reentrancy or context destruction contract is implied. Six caller-supplied
labels are not a general locale/formatting engine or automatic root inheritance.

Extend [model](../../tests/unit/test_questionnaire.c) and
[Styled](../../tests/unit/test_questionnaire_styled.c) coverage plus actual gallery
WASM interactions for the new styles/translations, callback sequences,
disabled/omitted/reintroduced parts and nested focus ownership. Do not schedule
the already delivered box masks or label bundle again, or promote parity from
palette screenshots or reducer tests alone.

### 7. Close the remaining Icon/Spinner inherited-style boundary deliberately

Numeric sizing and explicit semantics are complete scoped work. Remaining
[Icon limitations](../ICON.md) are automatic ambient text-style inheritance,
arbitrary source flex/percentage refinements and NanoSVG-versus-resvg coverage.
The supported parser may accept constructs without reproducing full source SVG
rendering; imported source support must not be inferred from successful parsing.

Choose explicit, composable ambient typography inputs before a global style stack;
add an SVG feature/capability corpus and reject/document unsupported constructs,
or approve a new optional renderer dependency with licensing/resource limits.
Do not fake percentages as pixels or change padding to inset SVG paint: source
Icon paints full resolved bounds. Extend [Icon style browser fixture](../../tests/web/test_icon_style.py)
and native source/lifetime tests for any newly supported subset; combine custom
raw-data Icon styling with animated Spinner before claiming the composition.

### 8. Finish editor navigation and performance beyond shipped visual geometry

The old blanket claim that editor hit/caret geometry is absent is stale.
[Input visual geometry](../INPUT_VISUAL_GEOMETRY.md) supplies an optional host
session, and [Textarea source](../../src/base/cui_textarea.c) now uses visual
geometry for row x/hit, caret affinity and selection spans, caching bounded line
sessions. [Editor](../../src/base/cui_editor.c) reuses these row helpers.
The native [geometry fixture](../../tests/integration/test_native_text_geometry.c)
contains real shaped Input checks and a separate `--audit` mode; its default run
must not be mistaken for proof that every multiline audit case passed.
Older INPUT/TEXTAREA/EDITOR paragraphs retain superseded descriptions alongside
later work. Native shaping alone still does not establish complete edit behavior.

Next bounded slice: audit and test mixed-direction visual Left/Right, Home/End,
preferred-x movement and multiline selection against provider geometry, including
run-boundary affinities, wrapping, folds and syntax-painted segments. Existing
logical grapheme movement is not visual bidi navigation. Preserve byte offsets,
undo, token boundaries, optional-provider fallback and resource failure behavior;
inspect actual geometry use before replacing already working hit/caret paths.

Separately instrument layout/allocation work across edits and scrolls in a
200K-line document. Existing large-document correctness does not prove viewport-only
layout or incremental performance. Set measurable budgets before implementation.
Multicursor IME, asynchronous browser clipboard and physical OS/mobile candidate
acceptance remain separate work.

### 9. Renderer and host completion follow feature delivery

[Text gradients](../TEXT_GRADIENT.md) and tracking explicitly preflight-reject
unsupported Sokol/SDL_GPU commands. Implementing those GPU paths is a real
feature slice: shared shaping, continuous glyph-mask paint, premultiplied color,
physical-density/clip handling, negative tracking and whole-frame rejection
must remain consistent. Software/SDL/Canvas success is not GPU hardware proof.

[Speech](../SPEECH.md) still has injected recognizers rather than complete real
capture/conversion/recognition integration. Choose one optional host, bounded
PCM ownership/cancellation/wake semantics and deterministic provider tests before
live-device acceptance. Existing speech controls do not prove a microphone works.
[Surface](../SURFACES.md) follow-up should target body scrollbar/focus reveal and
explicit Popover anchor/offset/arrow policy; Root layers and Sheet motion now exist
in scoped APIs, so do not globally label all overlay management/animation absent.

These are distinct owners and gates, not one giant “finish platforms” patch.
Native tests must use the private-display wrapper; browser tests stay headless.
Unsupported GPU/device/screen-reader checks remain explicit host acceptance work.

## Ledger reconciliation after implementation, not instead of it

The 41 `not_started` features are not 41 independent missing implementations.
Examples include controlled Slider, DataTable sort/virtualization, Table semantics,
Sheet transitions, Theme preferences and Empty slots, all with later implementation
or scoped tests. Conversely Spinner's family text needs current easing/rem evidence and the
remaining browser/consumer boundary. The integrator should reconcile those rows against current source,
record the exact supported subset/tested hosts and retain full-family limitations.

Do not delete families or invent completion to make counts uniform. Each delivered
slice needs source tests, actual relevant consumer tests, human/LLM guide examples,
source metadata and regenerated distributions/SDKs through existing tooling.
The optional [async-fetch example](../../examples/async_fetch/README.md) is already
present and offline-tested; it is not a missing new event API. Its bounded wake
retries do not guarantee recovery from persistent SDL event rejection. Preserve
that exception rather than scheduling duplicate networking/Core thread-pool work.
