# Slider disabled/theme completion checkpoint

Design only; audited CUI `a7dee36090cbe9c1ca1db1c7bed0cd804c69985f`.
No API in the proposal section exists. No production, generator, dependency,
compiler or release-artifact changes accompany this checkpoint. The existing
38 theme presets and all 78 component entries remain intact.

## Exact reference and current evidence

Reference: GPUI Kit 0.7.1 commit
`87d10ae5e1299d1be18670a37570c5402f952e34`. Only individual pinned primary
files were fetched; no source checkout or dependency was installed. Local audit
copies are outside the repository under
`/home/matteo/.cache/cui-current-validation/slider-reference`.

| Pinned path (relative to repository root) | SHA-256 |
| --- | --- |
| [crates/component/src/slider.rs][component] | `94afa288c201ae24c67bb176d9825eef32d2b32d849810275b5051a6bcdb984d` |
| [crates/base/src/slider.rs][base] | `b677bee1185b65a2fe4b3ecfcd247395be57d8bf7543c3cc0f9e0eac6ff9e0b6` |
| [crates/component/src/theme/schema.rs][schema] | `d700cb811d96ed075cdec2df616e0b20321ae6488d2dec36e3fbf85b8892ba23` |
| [crates/component/src/theme/theme_color.rs][colors] | `2d42060c00a0cb0fc91d698a4111c6df2cd80d074a1ed87eb03771e027da38f1` |
| [crates/component/src/theme/default-theme.json][defaults] | `2d9f725adddf769463db20bc807b8d50a059a27987923422e3b0064243b17580` |
| [crates/component/src/theme/registry.rs][registry] | `6522c5b34d52cd8896edf0a1e8ea61542ae59cece896ce119d74fd7f2c293b44` |
| [crates/component/src/theme/mod.rs][theme] | `e3e7871d06d925ed818bc2b4290690f595f7cc383c69f37ed2c9afd0dc81292c` |
| [crates/component/src/theme/default-colors.json][named] | `287a67753d98f68f7b36611e478adc26762ed3c5678d0e4bda132c5ea832046f` |
| [crates/base/src/theme.rs][base-theme] | `4376d3effde9e0f12f32bcf87a126e82771ce2cca17816eece40823f37ea65f4` |
| [crates/base/src/theme_tokens.rs][base-tokens] | `3e8fcec011d18b8d4f0b6beb738851e36134faa0ffccffa72069fdf04b3bc64f` |

Pinned source facts: Component uses a themed bar, thumb brush and animated
hover/press ring. Disabled removes styled thumb visuals and pointer listeners,
without a muted-color transformation. Indicator normal/active opacity is .2/.4;
thumb surround .5; ring replaces alpha with .5 times spring progress. Track
corners accept individual overrides; fill/thumb use theme full radius. Reverse
changes scalar fill only. Horizontal/vertical and single/range are supported.
See Component lines 11–75, 107–137, 144–311 and Base lines 438–876.

Theme resolution must follow code: schema recognizes `slider.background`,
falling back to resolved primary; thumb falls back to primary foreground
(schema lines 537–541, 996–997). Default JSON's `slider.bar.background` is a
*different spelling*, not a documented alias. Registry loads defaults through
that schema. Theme full radius becomes zero for a square theme (mod lines
543–557). Tokens distinguish a representative solid color from a background
brush (theme_color lines 9–47); the component's bar uses the solid, thumb the
background. Aurora Light has a gradient thumb in CUI's pinned palette data.

| CUI path | Actual behavior/evidence | Gap for the ledger feature |
| --- | --- | --- |
| [`cui_slider`](../../src/components/cui_components.c#L2133) | Float facade, 260-unit labeled framed root; track240×8; anonymous fill, no thumb. Base normalizes against outer root bounds. Disabled changes label/border; primary bar/fill remain. Focus helper can still apply focused border. | No source thumb/ring/active track, independent palette or source geometry. Changing the root to the track would change input semantics. |
| [`cui_slider_events`](../../src/components/cui_styled_slider_events.c#L7) | Double controlled facade; optional label, configurable track/hit/font size, custom track/fill colors; disabled overrides both with muted and track alpha .2. Focus band uses focus alpha .15. Theme radius is clamped numerically. | Custom colors are overridden when disabled; no thumb, hover ring, active track, per-corner source rule or thumb brush. Not the pinned disabled/theme policy. |
| [`cui_base_slider`](../../src/base/cui_base.c#L245) and controlled reducer | Disabled prevents mutation. Controlled reducer retains capture, preview, cancellation, exact double requests and entry-snapshot semantics. | Behavior is already independently meaningful; presentation work must not rewrite it. |
| [`test_slider_semantics`](../../tests/unit/test_components.c#L1040) | One light-theme legacy layout/fill and keyboard/semantic fixture. Other components' four-theme loops do not establish Slider coverage. | No full Slider color/state/theme cross-product. |
| [`test_styled_slider_events.c`](../../tests/unit/test_styled_slider_events.c) | Actual queued touch/keys, acceptance/rejection, precise values, pixels for custom bar colors, invalid admission, Base phase comparison, disabled cancellation. | Good interaction baseline, not disabled visual or all-theme proof; font-free pixels do not establish glyph rasterization. |
| [`test_slider_controlled.c`](../../tests/unit/test_slider_controlled.c), [`test_slider_events.c`](../../tests/unit/test_slider_events.c) | Base capture/cancellation and event evidence. | Preserve/reuse these; not a replacement for source presentation tests. |
| [`bridge Slider`](../../src/bridge/cui_bridge.c#L4847) | Five language builders ultimately use the legacy float facade. | A C-only new facade cannot establish builder theme/disabled completion or double event parity. |
| [`cui_theme`](../../include/cui/cui_theme.h) / [`theme_presets.py`](../../scripts/theme_presets.py) | Four built-in skins; 38 adapted named presets. No independent Slider bar/thumb fields. | Need exact named-token resolution; inferring a preset from theme colors or luminance loses caller edits. |

Current ledger `disabled/theme variants: not_started` is conservative for the
whole feature. Existing disabled behavior is partial evidence, not absence of
functionality. Do not mark this row tested merely because one new C example or
the default light/dark pair looks correct.

## Smallest compatible coherent implementation proposal

Separate presentation from reduction. Introduce an additive source-presentation
header/entry point, rather than growing existing public by-value structs or
silently changing legacy geometry, colors, callbacks or IDs. Suggested working
names are `cui_slider_presentation.h` and `cui_slider_present_events`; exact names
need integrator approval. A distinct options/state pair composes the existing
`cui_slider_options` and `cui_slider_state`, with an independent palette/brush
snapshot and caller-owned ring animation state. Keep the original APIs available
and byte-for-byte unstyled fixtures as compatibility evidence.

The new presentation resolves the complete supported scalar-horizontal policy:
bar, fill, enabled thumb/surround, thumb hover/press ring, disabled omission,
active-state track, square/round and per-corner overrides, custom palette,
transparent colors, named palette selection and theme changes during capture.
It must expose explicit reverse-fill behavior without changing requests.
Do not silently substitute muted colors for pinned disabled behavior. Readonly
and missing-handler rules remain CUI controlled-reducer rules, not new disabled
aliases; their appearance and absence of interaction must be documented/tested.

Avoid `cui_theme` growth: add companion palette constructors for a supplied theme
(fallback primary/on_primary), and an explicit preset-index resolver using the
same index/order as `cui_theme_preset_get`. Resolve `slider.background` and
`slider.thumb.background` from pinned data with schema fallbacks; do not promote
`slider.bar.background` to an alias without a separately documented adaptation.
A mutated custom theme has no inferred preset identity: caller passes its palette
snapshot explicitly. Validate indices and all used dimensions/policies before
submission; failed resolution leaves output unchanged. Respect color alpha:
opacity multiplication and ring alpha replacement are different operations.

Aurora Light is a **closure blocker**, not a default-only exception to hide. A
source-faithful preset path must retain/render its thumb brush, using existing
bounded [`cui_effect_linear_gradient`](../../include/cui/cui_effects.h) and caller-
owned raster storage or another separately reviewed existing paint facility.
Specify stop limits, angle projection, rounded clipping, buffer capacity,
resize/theme invalidation and lifetime through renderer consumption. Do not
silently flatten it to the representative solid and then claim all named themes.
If this support is deferred, label the row partial and name the exact exception.
No new Core command, platform type, dependency or hidden allocation is required
by this proposal; if existing facilities prove insufficient, return for review.

Reduce exactly once using the same hit ID and completed-layout bounds. Preserve
same-call confirmed values, phase order/counts/source indices, capture ownership,
readonly/disabled/zero-span cancellation, owner acceptance/rejection, precision,
keyboard/touch and omission behavior. Rings and thumb paint are decorative:
no extra interactive semantic, focus stop, event consumption or independent drag
state. Hover must mean the actual thumb, not the entire track. Animation state
must not mutate reducer state; use theme spring_control, reduced motion and a
bounded wake policy, with no perpetual idle invalidation.

Source dimensions need explicit rem projection (track6, hit24, thumb16 at rem16;
source offsets/padding mix fixed pixels and rem). Do not normalize literal source
offsets into a different geometric rule at rem sizes other than16. Existing
label and focus behavior are CUI extensions; retain their semantic name and
separate decoration from value-mapping endpoints. Per-corner track overrides do
not automatically override fill/thumb corners. Stable decorative IDs may be
additive, but no legacy anonymous ID changes are part of this checkpoint.

## Completion boundary, exceptions and shipping order

1. Implement/test the independent palette resolver and full presentation for the
   existing horizontal scalar reducer. This is an independently useful C slice,
   **not sufficient to close the whole ledger feature**.
2. Add explicit opt-in presentation authoring through bridge and all five builders,
   preserving old structs/default loads and legacy Slider behavior. An independent
   setter/options API is preferable to appending fields to by-value ABI structs.
   Reuse the same presenter, palette and supported-state policy; do not duplicate
   partial drawing in each language. Each runtime must prove disabled/theme
   selection on actual emitted output, not only setter storage.
3. Update human/LLM docs, exact public-header metadata, all78-preserving manifest
   evidence/limitations, binding coverage and generated SDK/dist/skill resources.
   Closure requires the matrix below across every newly advertised path; root
   owns combined validation/publication.

Full GPUI Slider-family parity still requires range/multiple thumbs, vertical
mapping, logarithmic scale, broader style refinement and host accessibility
behavior. Those do not exist in the current CUI reducer contract. A presentation
slice must not pretend otherwise or retrofit them by changing Core input routing.
The pinned Base also attaches accessibility increment/decrement separately from
its pointer disabled guards; do not copy that as permission for a disabled CUI
control to change value. Existing CUI disabled semantics take precedence.
There is no source tick renderer in the inspected Component; do not invent tick
styling as evidence or use the old manifest's tick limitation as proof of one.

If the ledger row is intended to mean *every pinned orientation/range variant*,
it cannot honestly be closed by the compatibility-preserving scalar slice. Keep
it partial until separate reducer/API designs cover those variants. State the
scope in the row; do not silently reinterpret completion to mean defaults.

## Required proof before changing status

| Axis | Required meaningful proof |
| --- | --- |
| Palette/state cross-product | All four built-ins and38 presets; enabled/disabled, normal/hover/press/drag/focus; readonly/handlerless and zero-span; min/mid/max and reverse. Check resolved colors/alpha and software pixels, not command counts alone. Theme changes while captured; custom alpha0/partial/opaque overrides; high-contrast and terminal square rules. |
| Geometry | Multiple widths/hit/track/rem sizes; square and per-corner track overrides; endpoint thumb overflow and parent clip/opacity; scrolling/resize and previous-layout input bounds. Verify source constants and exceptions, not only aesthetically centered defaults. |
| Thumb/ring | Hover actual thumb versus track, drag out/release, keyboard focus without pointer hover, disable/reenable while hot/dragging, cancellation and reduced-motion transitions. Independent spring samples, finite deadlines and settled idle; no stale ring after omission/recreation or state reuse. |
| Brush/lifetime | Aurora Light gradient endpoints/midpoint and rounded edge; custom supported brushes, explicit unsupported admission, caller storage capacity/resize/failure atomicity, copied/borrowed ownership through rendering and no stack-use-after-scope. No hidden source downloads or unchecked brush flattening. |
| Events/semantics | Replay queued pointer/touch/keys against existing reducer evidence; exact doubles above float precision, accept/reject, multiple same-frame inputs, preview/commit/cancel and phase counts. Compare immutable entry semantics and routing; readonly/disabled never mutate owner; no decorative focus/semantic duplication. |
| Compatibility | Frozen old facade IDs/layout/draw/semantic/callback fixtures, including disabled/focused and invalid values. New API invalid input atomic; old callback timing and capacity unchanged. Strict C11 selective/umbrella declarations and implementations, Base-only dependency direction. |
| Bindings/distribution | Actual Py/JS/TS/Go/Rust opt-in rendering and interaction; TS compile+runtime; rebuilt matching source SDKs/distribution headers and skill generation/tests. Mark old default and unsupported family variants explicitly. |
| Hosts | Software/headless matrix plus headless browser actual-WASM fixture with meaningful pixel/state assertions. Linux native only via scripts/test_local.py private Xvfb. GPU/other OS accessibility remains explicitly unverified until tested. No desktop windows. |

No test or build ran for this design checkpoint. Static checks establish pinned
hashes, 38 preset inventory and38/78 preservation, source-reference existence and
clean documentation scope; they do not establish implementation or rendering.

Signed: `/root/hidden_windows`, 2026-10-31. Await integrator review before source
changes. No merge, push or change to frozen alpha3 artifacts.

[component]: https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/slider.rs
[base]: https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/slider.rs
[schema]: https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/theme/schema.rs
[colors]: https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/theme/theme_color.rs
[defaults]: https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/theme/default-theme.json
[registry]: https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/theme/registry.rs
[theme]: https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/theme/mod.rs
[named]: https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/theme/default-colors.json
[base-theme]: https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/theme.rs
[base-tokens]: https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/theme_tokens.rs
