# Button paint refinement: source audit and proposed next slice

Historical design checkpoint, 2026-10-31; audited CUI baseline `b9c2f3d1`.
The approved first shadow-aware facade has since been implemented; use
[BUTTON_STYLES.md](../BUTTON_STYLES.md), `include/cui/cui_button_styles.h` and
`src/components/cui_components.c` for its actual bounded contract. The remaining
background/focus/text/border proposals below are not production APIs. The Button
ledger's “complete source state-style refinement” remains pending. Editor stays
deprioritized. Original review findings and proposed follow-on seams follow.

## Pinned evidence

This audit used the ORIGINAL GPUI Kit v0.7.1 commit
`87d10ae5e1299d1be18670a37570c5402f952e34`, as required by
[upstream-baseline.md](../upstream-baseline.md), rather than current upstream.
Only individual reference files were fetched; no checkout or dependency install.

| Source | Relevant evidence |
| --- | --- |
| [Component Button](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/button/button.rs) | `Styled` delegates to Base; render captures instance refinement, installs variant pointer defaults, replays instance within selected and disabled; gradient variant tests; custom variant shadow flag |
| [Base Button](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/button.rs), [state resolver](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/state_style.rs) | Base resolves instance, selected, disabled; this is different from Component's caller-instance replay |
| [Component styled helpers](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/styled.rs) | `focus_style`, `FocusLine`, external ring and inside/edge/outside borderless focus lines |
| [Workspace manifest](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/Cargo.toml) | Pins GPUI dependency package `gpui-pre` exactly `0.3.8` |
| [Dependency style](https://docs.rs/crate/gpui-pre/0.3.8/source/src/style.rs), [color](https://docs.rs/crate/gpui-pre/0.3.8/source/src/color.rs), [Styled](https://docs.rs/crate/gpui-pre/0.3.8/source/src/styled.rs) | Concrete paint fields, shadow/background ordering, fixed two-stop gradient, sRGB/Oklab interpolation choice, text decoration |

Reproducible SHA-256 of raw pinned Component Button:
`e4423f371ec5e954f0ecb4cd459ee24ef5a158affa6d44dc63e62c5df463d61e`;
Base Button: `ff3026d04f12afe3e6aeb3cc3d95b25c5ea6eb65884258fea1b973ad6d08255e`;
state resolver: `f7e1f85c0215e3cd84df4b6fbce34938a33d275c9db392bdc9025670454760b1`.
The dependency pages above are the pinned published version, not a claim that
GPUI implementation code is present in CUI. Findings below are independent design
analysis; no upstream implementation or documentation is copied wholesale.

## Actual coverage and gaps

| Paint concern | Pinned source | CUI baseline / remaining work |
| --- | --- | --- |
| Background, border color, foreground | Instance and semantic/pointer refinements; normal/hover/active variant palettes | Existing six-mask C facade supports solid RGBA colors; preserves Component replay |
| Radius, per-corner radius, opacity | Arbitrary `StyleRefinement`; absolute/rem lengths | Existing facade permits finite nonnegative radii and opacity 0..1; logical-pixel bounded subset, no rem refinement |
| Shadows | Owned vector, explicit empty-list reset; DROP before background and INSET after background; border after content | Core already copies up to eight mixed layers. Button facade has no shadow field; post-submission setter does not participate in state refinement |
| Default shadow | Custom variant's shadow boolean, gated by theme shadow and not disabled; most variants have no builtin shadow | Do not add a shadow to all Buttons. A shadow-aware facade alone does not complete custom-variant/theme default-shadow policy |
| Gradients | `Background` has exactly two stops, angle, sRGB/Oklab choice; real normal/hover/active/outline/disabled gradient tests | Theme/Button palettes and facade are scalar colors. `cui_effect_linear_gradient` is a raster helper, not an element background contract |
| Other background paint | Source Background also exposes slash pattern and checkerboard | No matching Button/root refinement API; retain explicit remaining scope |
| Border paint | Color, per-edge absolute/rem widths, solid/dashed style | Existing options support per-edge widths; callback widths are inspection-only. No arbitrary border-style refinement |
| Focus | Theme-dependent border tint or separate outer ring; borderless inside/edge/outside line | CUI currently changes border color and width to 2, then focused callback. This is a real remaining paint gap, not fixed by adding shadows |
| Text paint | Inherited foreground, background, underline/strikethrough thickness/color/wavy; Link builtin underline | CUI facade exposes foreground only. Arbitrary text decoration/background remains pending |
| Font/geometry | Source also permits font family/features/fallbacks, size/weight/style, line height, whitespace, clipping, size, margin/padding, flex/grid, visibility, cursor | These affect measurement/layout or framework behavior. They are not covered by a paint-only completion claim |

Custom variant color setters in source take HSLA colors; gradient support comes
through Background-valued theme tokens or Styled background, not an invented
gradient custom-color setter. Source background opacity transforms both gradient
stops. Byte RGBA and premultiplied-sRGB raster interpolation are not evidence of
full HSLA/Oklab or renderer parity.

## Recommended first production slice: owned shadow refinement

Add new records/functions alongside the existing ABI. Reuse existing phase/info,
options, scalar presence-bit values and root ownership; do not enlarge any existing
public struct. Suggested signatures (names require review):

```c
enum { CUI_BUTTON_SURFACE_SHADOWS = 1u << 6 };
typedef struct {
    unsigned rejected_phases; /* one bit per existing phase, zero on entry */
} cui_button_surface_status;
typedef struct {
    cui_button_style_paint paint;
    size_t shadow_count;
    cui_box_shadow shadows[CUI_BOX_SHADOW_MAX];
} cui_button_surface_paint;
typedef struct {
    unsigned fields; /* old six scalar bits plus SHADOWS */
    cui_button_surface_paint value;
} cui_button_surface_patch;
typedef void (*cui_button_surface_fn)(void *user,
    const cui_button_style_info *info,
    const cui_button_surface_paint *current,
    cui_button_surface_patch *out);
typedef struct {
    cui_button_surface_fn instance, hovered, active, selected, disabled, focused;
    void *user;
} cui_button_surface_styles;
bool cui_button_surface_ex(cui_context *, cui_id, const cui_theme *, cui_str,
    cui_button_options, cui_button_surface_styles, cui_button_surface_status *);
void cui_button_surface_paint_only(cui_context *, cui_id, const cui_theme *,
    cui_str, cui_button_options, cui_button_surface_styles, cui_button_surface_status *);
```

`CUI_BOX_SHADOW_MAX` is the existing public eight-layer capacity. The optional
status output reports rejected phase patches independently of activation; its
pointer is borrowed only through the call and never retained.
No gradient placeholders, union with unowned pointers, callback registry, event
routing or retained next-widget style. New functions share the existing private
submit/painter path; Base runs exactly once for the active function. Existing
legacy, styled, event and group routes keep their existing signature and timing.
Do not pretend that new facade overloads exist for event/group APIs.

A surface calculation starts with the ordinary default paint and an empty shadow
list. First slice adds no builtin shadows. Disabled explicitly clears shadows
before its callback, then replays the instance patch: an explicit instance list
can restore them, matching the source's disabled clear/replay rule. Enabling the
source custom-variant/theme shadow default is a separate additive option review.

### Exact proposed phase and reset rules

1. Resolve normal builtin paint; invoke instance once and capture its validated
   value patch by copy.
2. Only when not disabled/loading/selected/open: choose pressed over hover,
   apply that builtin color layer, then its callback. No hover callback while
   pressed. Shadow lists carry through a builtin color-only layer.
3. If selected OR open: apply selected builtin colors, selected callback once,
   then replay the captured instance patch, without recalling instance.
4. If disabled: apply disabled colors, clear shadows, disabled callback, instance
   replay. Otherwise loading multiplies resolved root opacity by 0.8.
5. Existing focus-visible and hide-focus-ring gates apply existing focus paint,
   then focused callback. No new focus event or eligibility rule.

This preserves the documented CUI bounded precedence, including loading
multiplication; it does not claim all GPUI engine hover/active/focus-visible
combinations or arbitrary source opacity conflicts have been reproduced.
Source Component focus helper uses its own focused-handle/theme gates, whereas
CUI's facade uses its existing focus-visible gate; keep this distinction explicit.

An absent scalar bit inherits the preceding value; a present zero/transparent
value replaces it. CORNERS false restores uniform-radius selection. An absent
SHADOWS bit inherits the complete preceding list; present count zero clears DROP
and INSET; present nonempty replaces the complete list, never appends or merges
by index. Instance replay uses those exact presence bits, including explicit
clear. There is no implicit “reset scalar to theme” sentinel: omit its bit or
supply the desired value. Each submission recomputes defaults; supplying zero
styles next frame removes all facade refinements without retained state.

Callbacks are borrowed synchronous pure calculations, as in the existing API.
They cannot call CUI, dispatch/consume input, mutate owner/options/styles, submit
children or reenter. `current`, `info`, `out`, callback/user references expire at
return. Fixed arrays are copied before later callbacks and before content runs;
unused array entries are ignored. No borrowed layer, font, image or gradient
resource pointer survives a callback. The private painter's copied frame-owned
shadow payload follows existing Core draw-list/resource lifetime, never a stack
pointer. No allocation/resource is added for absent/empty decoration.

Validate a candidate patch into a local copy before publishing any field. Unknown
bits, count over eight, invalid kind, nonfinite values, sigma outside [0,256],
offsets/spread outside [-4096,4096], opacity outside [0,1] or radius/corner outside
[0,16384] discard the whole new-facade patch; preceding paint survives and later
eligible callbacks continue. Present radius rules are deliberately bounded for
the NEW facade; the old facade's finite-nonnegative contract stays unchanged.
Validate the effective shadow corner shape too, including radii supplied through
options, before accepting a nonempty list. An unsupported options-derived shape rejects
the whole phase patch and sets its `rejected_phases` bit without dropping ordinary
Button paint. Every invalid phase patch sets its bit; absent callbacks do not.
No Core error is introduced for these authoring errors.
Pure invalid callback patches follow the existing non-poisoning policy; genuine
Core frame/capacity failures remain real frame errors. CSS factory validation
remains explicit: radius [0,512] becomes sigma radius/2, never reinterpret sigma.

Apply a valid nonempty list to the actual caller-id root while open, using the
existing copied shadow mechanism with final resolved corners. No extra layout
node, ID, input target, rectangle cache, previous-frame bound or default-path
custom paint. Radius affects decoration only; hit tests stay rectangular.
Ancestor masks, inherited opacity, DROP/background/INSET/border ordering and
resource ceilings remain [ELEMENT_SHADOWS.md](../ELEMENT_SHADOWS.md)'s contract.
Do not alter existing renderer ordering in this slice to claim GPUI's border-
after-content equivalence: overlapping/overflowing child paint needs its own
oracle and renderer checkpoint. A later explicit root shadow setter replaces
the facade's list; the retained bridge override, applied after owner paint, also
wins as documented in [BRIDGE_SHADOWS.md](../BRIDGE_SHADOWS.md).

### Passive and custom content boundary

The passive function inspects the independently registered owner's interaction
and paints once: no event reduction, semantics registration or activation/hover
callback manufacture. Active callbacks retain current accepted-event order;
style calculations still occur in the paint stage after action callbacks.
Capture the facade descriptor locally before calling user action/content code.
Styles cannot revive disabled/loading activation or change aggregate results.

Leading/trailing content, replacement content, children, artwork/loading slots,
label ellipsis and caret keep existing precedence and exactly-once submission.
The root shadow encloses the final root geometry, including custom children;
it does not target anonymous slots or paint them twice. Foreground remains the
existing label/artwork ink, not a forced recolor of arbitrary child submissions;
caret keeps its builtin normal foreground at 0.75 alpha. Content callbacks run
with their current signatures/context and may submit children under existing
rules; they are not pure style callbacks. Clipping/content-overflow, intrinsic
measurement, layout-only loading work and suppressed lifecycle behavior stay
unchanged. No private source `content_style` API is claimed.

## Follow-on renderer checkpoints, before any COMPLETE label

1. **Background paint:** separately review an owned tagged solid/two-stop-linear
   descriptor, fixed two stops by value, explicit sRGB/Oklab enum, finite angle
   [0,360], finite stop positions [0,1]. Source permits two stop values; pin
   reversed/equal-stop behavior against source renderer before choosing reject
   or normalization. Atomic replacement includes both stops, angle and color
   space; absent background inherits, transparent solid explicitly clears the
   gradient. No stale gradient survives a solid replacement or disabled reset.
   Numeric RGBA representation versus source HSLA conversion needs an explicit
   precision contract. Reject unknown kinds/spaces transactionally.
2. **Gradient seam:** existing effects angle 0 is left-to-right, 90 top-to-bottom;
   source 0 points toward top and rotates clockwise. Existing helper spans
   projected pixel-center bounds and interpolates premultiplied sRGB. Do not
   relabel it as source parity. Review source projection, degenerate bounds,
   hard stops, alpha and Oklab before adding a Core root background primitive.
   Raster fallback must declare cache ownership, invalidation on resize/DPI,
   per-surface/aggregate pixel and byte ceilings, frame lifetime and allocation
   failure policy. Suggested ceiling no larger than existing shadow limits
   (16384 per side, 16M pixels), but total frame budget needs separate approval.
   No new Core payload/draw command or silent image allocation is approved here.
3. **Remaining paint:** slash/checkerboard parameters, decoration/underline and
   strikethrough, border style/width effects, theme/custom default shadows and
   source focus rings need explicit owned descriptors and independent tests.
   Border-width/font changes require measurement-aware design; never mutate
   paint widths while hit/semantic/layout geometry still describes old widths.
4. **Framework gaps:** arbitrary Styled layout, font resources/fallbacks, rem
   resolution, child inheritance/private compound styling, hover-group engine,
   fluent conditional builders, user GPUI handlers and framework focus/event
   integration remain separate. No capture/bubble or dispatch architecture is
   introduced. Even paint completion alone cannot establish complete Button
   source parity, much less all 78 components.

## Required implementation evidence

Before changing the shared painter, capture independent pre-change legacy
software fixtures with library hash/source provenance. Cover legacy simple,
options, existing styled, passive, event and grouped Button routes, including
real queued pointer/key/AT requests and multiple accepted events in one frame.
Compare normalized owned command semantics, geometry/hit bounds, event
consumption, callback order/counts and deterministic stub-font software pixels;
never expected snapshots generated from the new implementation itself.

New tests must exercise every phase and overlapping selected/open/disabled/
loading/focus flags; instance once with replay twice; pressed suppressing hover;
clear versus absent list; mixed DROP/INSET ordering; all eight layers and nine
rejection; presets/CSS factory; transparent/zero scalars; transactional mixed
valid/invalid patches and ignored unused storage; callback buffer mutation and
content overwrites; custom asymmetric corners, move/resize, clipped ancestors,
root opacity, custom content overflow, layout-only/suppression and resource
failure. Check empty/no-op facade draw lists/pixels exactly against legacy.
For opted-in shadows compare ordinary-only independent software output and
actual shadow samples/order; legitimate extra clip commands are not permission
to relax ordinary pixels or ancestor-mask correctness. Test post-owner/bridge
replacement explicitly. Keep diagnostics active under NDEBUG.

Browser fixture runs headless with independent software reference samples,
DPI, resize and clips. Native portable renderer checks use private Xvfb through
`python scripts/test_local.py --build-dir EXISTING_DIR`; no desktop window and no
GPU parity claim from Xvfb. Future builds require the allocated single focused
build directory, nice 19, one available CPU and -j1. This checkpoint runs no build.

C bindings first; Python/JS/TS/Go/Rust currently author static copied root shadows
for the documented bridge subset, not these proposed phase callbacks or arbitrary
Button state refinement. Do not expose foreign callbacks until lifetime, exception,
reentrancy and thread rules are reviewed; a later typed six-phase owned descriptor
is preferable as the first foreign authoring slice. Existing static bridge style
still applies last and can mask a C phase shadow. Add explicit capability/version
checks, deep-copy/reset/error tests and all five language fixtures before claiming
binding parity. On production approval update authoritative headers, SDK/
amalgamation, metadata/ledger limitations and skill guidance through generators,
then run required freshness and installed-skill tests; preserve all 78 entries.

## Decision requested

Approve only the shadow-aware additive facade, optional rejection status and
shared-painter test scope. Keep background/focus/
text/border renderer seams as separately reviewable checkpoints. This signed
design is evidence of an audit, not a production API or a completed parity row.
