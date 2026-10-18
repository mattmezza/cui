# Remaining parity: implementable work at the next39 checkpoint

Audit baseline: `99a55efb`, `integration/next39-components`. Frozen comparison:
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

I inspected all manifest limitations/feature rows, the event request/contract and
binding audit, current public declarations and the source/test paths cited below.
The deeper frozen-source comparisons here concern Questionnaire, Spinner and
Icon; this is not a new line-by-line source certification of every family. For
other families the existing guide/ledger limitations identify follow-up audit
boundaries. Tests cited were inspected, not rerun for this documentation change.
Proposed tests below are acceptance work, not claims of existing coverage.

## Do not schedule these requests from stale rows

| Stale shorthand | What is already present | What remains distinct |
| --- | --- | --- |
| Questionnaire lacks automatic root capture/cross-item focus | [Styled implementation](../../src/components/cui_questionnaire.c) routes through prior-frame bindings and ordered input reduction; [styled tests](../../tests/unit/test_questionnaire_styled.c) cover automatic routing, omission and focus. [Guide](../QUESTIONNAIRE.md) ends with the newer contracts. | Nested roots, comprehensive callback choreography, slot refinements and localization still need completion. Earlier paragraphs and manifest prose are not the final source truth. |
| Icon padding/nonzero style is unsupported; no accessible API | [Icon options](../../include/cui/cui_icon.h), [resolver](../../src/base/cui_icon_layout.h), [source-style tests](../../tests/unit/test_icon_style_source.c), [accessibility tests](../../tests/unit/test_icon_accessibility_source.c) implement numeric bounds/padding/border floors, full-box paint, explicit image labels and decorative suppression. | Ambient inheritance, generic Styled layout and SVG renderer fidelity remain limited. Do not redo numeric constraints or add guessed asset-name labels. |
| Slider has no controlled value/semantics | [Base API](../../include/cui/cui_base.h) and [controlled tests](../../tests/unit/test_slider_controlled.c) already define typed BEGIN/UPDATE/COMMIT/CANCEL requests and read-only behavior without a handler. | Bridge exposure and shared metadata adaptation are separate; do not replace its phased semantics with one bool change callback. |
| Text gradients/tokens/tracking are wholly absent | [Tokens](../TYPOGRAPHY_TOKENS.md), [gradient implementation](../TEXT_GRADIENT.md), [native tracking](../LETTER_SPACING.md), [real gallery gradient proof](../../tests/web/test_gradient_gallery.py). | Canvas tracking, foreign authoring and GPU support are unfinished. Reimplementing solid/gradient paint or native tracking would duplicate completed work. |
| Native SDK text is unshaped; App.run only polls a fixed-frame loop | [SDK parity guide](../SDK_TEXT_PARITY.md), [installed text fixture](../../tests/packaging/test_installed_text.py), [bridge runner](../../src/bridge/cui_bridge.c), language run tests in bindings. | Provider coverage and style/event API gaps below remain. Launcher-only examples are not evidence about the actual bindings. |
| Every native accessibility adapter is absent | [AccessKit adapter](../../backends/accesskit/cui_accessibility.c), [native guide](../NATIVE_ACCESSIBILITY.md), [integration test](../../tests/integration/test_accessibility_native.c) exist. | Real assistive-technology acceptance and complete family/platform mappings must be evaluated within that adapter's documented support, not inferred from semantic snapshots. |

## Ranked production slices

Priority reflects the explicit typography/event/frozen-family requests and a
small reviewable implementation boundary, not a claim that lower entries are
less important forever. Each slice needs a separate approved owner and generated
consumer refresh by the integrator where public ABI/source changes require it.

### 1. Complete the demonstrated Spinner motion and size differences

Concrete source mismatch, not just absent tests: [motion sampler](../../src/base/cui_status.c)
rejects finite easing outside 0..1 and bypasses easing in reduced motion;
[Spinner submission](../../src/components/cui_content.c) always chooses
`CUI_ICON_CUSTOM`. Frozen `crates/component/src/spinner.rs` delegates easing to
pinned GPUI animation, which permits finite overshoot and evaluates ease(0) when
reduced. Named source sizes resolve in rem, custom pixels do not.
[The dedicated audit](../audits/SPINNER_PARITY.md) establishes the source lock and
separates these from Skeleton's bounded-opacity contract.

Bounded change: Spinner-specific finite rotation policy, reduced ease(0), and an
additive preset/custom/rem descriptor in [content options](../../include/cui/cui_content.h).
Keep legacy eight-dot `cui_spinner` behavior and Skeleton validation intact.
Preserve atomic state/result failure and no wake under reduced motion; reduce
large turns before radians to keep finite paint geometry.

Acceptance: extend [contract tests](../../tests/unit/test_spinner_contract.c),
[motion tests](../../tests/unit/test_status_motion.c) and [WASM test](../../tests/web/test_spinner_gallery.py)
with negative/1.25-turn/shifted ease(0), NaN/infinity, two rem scales versus fixed
64px, omission/remount, custom asymmetric Icon pixels and retained draws after
source replacement. Existing default/linear curves cannot prove those cases.

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

### 4. Implement the already-designed discrete C event facades

Only [shared metadata/copy helpers](../../include/cui/cui_component_events.h) are
shipped; [the first activation slice](COMPONENT_EVENTS_ACTIVATION_SLICE.md) is
explicitly proposed. No production Button/Check/Switch/Toggle facade should be
reported as available from the proposal's illustrative declarations.

Implement that bounded slice in Base/Styled headers and
[src/base/cui_base.c](../../src/base/cui_base.c)/[components](../../src/components/cui_components.c),
using Button's frozen activation evidence. Switch/Toggle legacy aggregate changes
cannot supply per-event requests by wrapping their one callback. Preserve old
ordering, mutation behavior, userdata and result reads; new request callbacks use
immutable entry snapshots. This does not authorize global capture/bubble or
turn every family into the same event shape.

Acceptance: per-event multiplicity, metadata provenance, read-only missing
handler, disabled/loading/inert/omitted behavior, callback-driven owner changes,
callback/result coexistence without replay, and conflict preflight. Retain
[Button snapshots](../../tests/unit/test_button_click_snapshot.c),
[Checkbox](../../tests/unit/test_checkbox_source.c),
[controlled siblings](../../tests/unit/test_controlled_siblings.c) and
[metadata tests](../../tests/unit/test_component_events.c). Then schedule continuous
editing, selections and lifecycle adapters separately using the
[78-family matrix](COMPONENT_EVENTS_COVERAGE.md).

### 5. Add typed bridge events and foreign typography authoring

[C bridge](../../include/cui/cui_bridge.h) exports queued CLICK/CHANGE/CLOSE,
`uint32_t checked`, `double value`, copied text and selected handles. Its retained
reducers mutate before CHANGE; foreign callbacks cannot veto those mutations.
[Questionnaire bridge callback](../../src/bridge/cui_bridge.c) packs event kind into
`value` and completion into `checked`; [Spinner bridge rendering](../../src/bridge/cui_bridge.c)
still invokes the legacy eight-dot primitive. Flat style authoring does not expose
native `letter_spacing`. These are concrete missing surfaces even when the
underlying native C facilities work.

Split delivery: (a) additive version/capability-safe typed event payloads and
copied ownership for table indices, Questionnaire events and activation evidence;
(b) separate explicit controlled requests if desired, without relabeling existing
post-mutation queues; (c) additive tracking/font/paint authoring and source-style
Spinner options. Keep old dynamic symbol contracts and predictable unavailable
results. Do not add fields silently ignored by a backend.

Wire actual [Python](../../bindings/python/cui_bridge/__init__.py),
[Node](../../bindings/node/index.cjs)/[declarations](../../bindings/node/index.d.cts),
[Go](../../bindings/go/cui/native.go), [Rust](../../bindings/rust/src/lib.rs) and
TypeScript surfaces. Python/Node/Go currently bool-convert generic checked values;
Rust also retains raw checked evidence as documented in the
[binding audit](COMPONENT_EVENTS_BINDINGS.md). Check JS uint64 precision separately.
Acceptance must use installed/native consumers, nonzero table columns, long frame
identities, stale handles, queue overflow, retained handlers, exception/panic
containment and callback teardown—not only launcher compilation.

### 6. Finish Questionnaire's actual remaining Styled/localization contract

The retained model, root routing, ordered TEXT/Enter batches, four-size/palette
cards, slots, focus band, semantic positions and delayed validation/navigation
already exist. Do not restart the component because its family row is only
`implemented`. [Public options](../../include/cui/cui_questionnaire.h) expose render
callbacks, not source `indicator_style`, `content_style`, `shortcut_style`
refinements. Frozen `crates/component/src/questionnaire/components.rs` exposes
those methods and uses localized action labels; current custom rendering is not
equivalent to independently refining each default slot.

First slice: bounded explicit slot-style descriptors/precedence and action/error
label localization hooks that retain default semantics, layout and lifetime
rules. Audit all remaining frozen part methods and callback choreography before
choosing the final verification boundary. A second slice verifies nested roots,
subscriber-driven item enabling/navigation and callback mutation ordering against
frozen Base state/keyboard/control tests; no generic arbitrary callback reentrancy
or context destruction guarantee is implied.

Acceptance: extend [model](../../tests/unit/test_questionnaire.c) and
[Styled](../../tests/unit/test_questionnaire_styled.c) regressions, plus actual gallery
WASM interactions, covering slot-only overrides/precedence, translated long labels,
all four sizes, callback sequences, disabled/omitted/reintroduced parts and nested
focus ownership. Reconcile the guide's older unfinished paragraphs after proof;
do not promote from palette screenshots or only reducer tests.

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

### 8. Complete editor geometry and large-document performance separately

[Input](../INPUT.md), [Textarea](../TEXTAREA.md) and [Editor](../EDITOR.md) retain
bidi/caret and host editing limitations despite native paragraph shaping. Native
text geometry availability does not automatically make all retained edit reducers
bidi-correct. The 200K-line correctness test is not a viewport-only performance
proof. Browser asynchronous clipboard is distinct from the synchronous native seam.

Schedule one bounded geometry slice in [editor](../../src/base/cui_editor.c) and
[textarea](../../src/base/cui_textarea.c) with real mixed-direction selection/hit
fixtures; preserve grapheme and undo contracts. Separately instrument layout work
and allocations across edits/scrolls in a 200K-line document before implementing
incremental viewport layout. Set measurable budgets rather than promoting from
successful allocation or one screenshot. Multicursor IME and physical mobile/OS
candidate acceptance remain distinct later work.

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
or scoped tests. Conversely Spinner's family text omits demonstrated easing/rem
mismatches. The integrator should reconcile those rows against current source,
record the exact supported subset/tested hosts and retain full-family limitations.

Do not delete families or invent completion to make counts uniform. Each delivered
slice needs source tests, actual relevant consumer tests, human/LLM guide examples,
source metadata and regenerated distributions/SDKs through existing tooling.
The optional [async-fetch example](../../examples/async_fetch/README.md) is already
present and offline-tested; it is not a missing new event API. Its bounded wake
retries do not guarantee recovery from persistent SDL event rejection. Preserve
that exception rather than scheduling duplicate networking/Core thread-pool work.
