# Typed Styled Slider presenter checkpoint

Design only, signed against Base `cf3c39312ecab7b1f2daaddecda0c8e712f1ada2`.
No proposed symbol below is implemented. No production, generated files, builds
or dependency changes accompany this checkpoint. All78 entries remain intact;
Styled/host/foreign completion is still open.

## Authority and complete scope

Read the retained pinned primary files at
`/home/matteo/.cache/cui-current-validation/slider-reference`, not a fresh checkout.
The [Component Slider](https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/slider.rs)
is SHA256 `94afa288c201ae24c67bb176d9825eef32d2b32d849810275b5051a6bcdb984d`;
[Base Slider](https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/slider.rs)
is `b677bee1185b65a2fe4b3ecfcd247395be57d8bf7543c3cc0f9e0eac6ff9e0b6`.
Schema/theme hashes and exact spelling/fallback evidence remain in
[SLIDER_THEME_COMPLETION.md](SLIDER_THEME_COMPLETION.md). Current C authority is
[`cui_base_slider_model.h`](../../include/cui/cui_base_slider_model.h), its Base
source/test, [`cui_slider_palette.h`](../../include/cui/cui_slider_palette.h),
[`cui_background.h`](../../include/cui/cui_background.h) and V2 transport.
Root integration `b1847b4d` was also inspected read-only for the latest private
background preparation/validator contracts; it still has no public V2 adapter.

Full scope includes Single and two-endpoint Range, both axes, reverse Single
fill, linear/log values, source palettes/overrides, disabled/readonly behavior,
actual-thumb hover/press, keyboard focus visibility, interruptible rings, source
sizes and responsive parent sizing. Fixed Base dimensions alone cannot close it.
Source has no arbitrary-N handles or named XS/SM/LG Slider variants: test rem,
length and authored dimensions instead of inventing a source size enumeration.

| Source surface/state | Exact evidence and proposed CUI policy |
| --- | --- |
| Palette | `render`: bar uses style.background.color() or slider_bar; thumb uses style.text.color (solid override) or slider_thumb background; ring uses theme.ring. Keep explicit owned palette; never infer preset from byte-theme equality. Four builtins +38 named presets already have independent resolver proof. `slider.background` is authoritative; `slider.bar.background` is not an alias. |
| Track | Normal bar alpha multiplied by.2; active multiplied by.4. Fill keeps bar alpha. Custom track corner entries override individually; remaining corners use theme full radius. Fill always uses full radius, not track overrides. `.active` is an upstream interaction selector, not evidence of a keyboard listener. Proposed new-route activity is admitted logical own drag, not stale global mouse state; exact selector propagation needs a source oracle or an explicit reviewed CUI exception. |
| Thumb | Outer16 at rem16, bar alpha multiplied by.5,1px padding; inner14 holds actual brush. Outer/inner use full radius, zero for square themes. Thumb/padding/ring are separate meaningful paint surfaces. |
| Pointer ring | `ThumbInteraction::is_active`: actual thumb hover OR pressed; width3*progress logical px; ring color ALPHA REPLACED by .5*progress, not original alpha multiplied. Ring border extends outward so inner edge is flush with outer thumb. Ring is not a shadow/blur. |
| Disabled | Component `.when(!disabled)` omits thumb visual subtree/listeners; track/fill keep palette, not muted substitutions. Preserve Base disabled thumb semantic geometry with transparent outer box (documented CUI accessibility exception); omit all visible surface/ring children. |
| Readonly/handlerless/zero span | CUI controlled rules: no proposals/capture start; visible thumb remains, disabled-only omission does not apply. No source readonly policy is invented. |
| Focus | Source Component has no explicit keyboard-focus ring. CUI addition: separate2px border at gap2 around current focused thumb only when Core focus_visible and not disabled; uses palette.ring with its authored alpha, hide-focus option explicit. It does not activate hover spring, add semantics or fabricate focus. |
| Range/reverse/log | Preserve Base controlled math, neighbor limits and request order. Source reverse affects Single fill only. Source range-a11y scalarization/zero-origin overshoot remain rejected CUI exceptions already documented in model design. |

## Current-size source geometry, including mixed units

For rem R, hit cross extent=1.5R, track=.375R, outer thumb=R, inner=max(0,R-2).
Literal padding1, axis margin-8, cross offset-5, ring maximum3 are logical pixels,
not rem. Let track origin be T and current resolved length L; logical endpoint
fraction p comes from the model paint pair. Horizontal outer is
`{T.x+p*L-8, T.y-5, R, R}`. Vertical outer is
`{T.x-5, T.y+(1-p)*L+8-R, R, R}`. These equal centered Base geometry only at R=16.
Do not replace them with thumb/2 centering. Source vertical default length120 is
literal px. Horizontal fills its parent; style-refined root/flex sizing is also
required, not silently replaced by240px. Zero-area inner surfaces need an explicit
no-paint path tested against V2; no negative inner extent or invalid gradient draw.

Root/Track/Fill and semantic outer thumb IDs reuse the existing Base getter.
Proposed Styled getter adds per-endpoint SURFACE, POINTER_RING and FOCUS_RING;
no copied salts and no MAIN alias for ROOT. A pure getter returns identity only.
Single START, disabled decorations, cold/zero-width rings and zero-area paint may
be absent. Decorative children never add semantic/focus/activation nodes or input
consumption. Actual outer thumb bounds remain the nearest-selection/hit authority.

## Responsive composition: existing seams and proof gate

Public Base returns after closing its root, so appending siblings afterward cannot
correctly attach the source surface/rings or obtain current parent resolution.
Propose a private Base reducer/composition seam, not a new Core dispatcher/API:
public Base calls it with NULL/default geometry policy; Styled supplies a private
paint policy and in-box decoration callback. Snapshot options first, reduce ONCE,
then compose actual boxes from preview only during drag, otherwise confirmed entry.
Invoke decoration inside actual thumb/root before closing it. No callback/model
replay, prepaint input, delayed correction frame or second semantic registration.
NULL policy must preserve public Base command IDs/order/bounds/callback timing.

Ordinary Clay flow appears sufficient for parent-responsive geometry, using only
public CUI boxes. This is a proposal until a test-only fixture proves it:

- Horizontal Root GROW-width/fixed-cross; indicator GROW-width/fixed-track-height.
  A fill row uses percent-width leading spacer and percent-width fill. Two
  following zero-height GROW-width rows use percent-width endpoint spacer and a
  zero-size anchor. Fixed absolute thumb child offset(-8,-track_extent-5) from
  that anchor yields the formula above: rows sit at indicator bottom. Floating
  thumbs do not contribute width or displace the other thumb/fill.
- Vertical transpose: Root fixed-cross, fixed/default120 or bounded parent-GROW
  height; indicator fixed-track-width/GROW-height. Fill column uses percent-height
  leading(1-end) spacer/fill. Two following zero-width GROW-height columns use
  percent-height(1-p) spacer/zero anchor. Fixed thumb offset
  (-track_extent-5,8-R) gives the source vertical formula.
- Zero-size anchors/flow spacers are private, passive and unclipped; descendants
  inherit real ancestor clipping. Do not turn zero-size anchors into hit regions.
  Name only meaningful paint surfaces/semantic thumbs; inspect actual Core
  semantic-node rebind and final clip/hit results, not guessed coordinates.

Actual Core mapping: `cui_core_begin_box` maps width/height_percent to Clay percent,
negative non-fixed extents to GROW, and fixed absolute children attach to parent.
Prepaint receives current bounds but forbids layout/focus/input mutation: it cannot
repair semantic thumb boxes and is not an alternative reducer/presenter pipeline.
Underlay paints one owner's rectangle, not multiple correctly hit-tested thumbs.

Actual restriction: `cui_scroll_layout_internal.h` rejects percent HEIGHT children
except its canonical fixed-zero case; arbitrary GROW floating children also fail.
The horizontal plan avoids GROW floating nodes and uses only percent widths; its
admission still needs actual proof. Vertical percentage anchors can work in
ordinary frames yet fail inside registered scroll composition. A fixed positive
ancestor may permit a separately proven Y-independent admission island, but NO
Core admission change is authorized here. Record the actual rejection and no
semantic/readiness promise on that failed frame. Do not label responsive sizing
complete across all contexts, or substitute fixed sizing as full completion.

Required test-only prototype before private hook production: first frame, parent
narrow→wide→narrow, both axes, fixed and bounded-GROW main sizes, R12/16/24,
p0/.25/.75/1 and Range, exact ring/surface bounds, source literal offsets, clipping,
actual next-frame pointer hits and prior-input/current-paint resizing. Run V1/V2
and actual registered-scroll admission/failure with clear-error recovery. Use
independent formula expectations and painted colors/pixels, not command counts.
No new production/public API should precede that proof and root review.

## Proposed separate records and callback/lifetime contract

New header/source `cui_styled_slider_model.h` / components implementation; no
fields added to legacy Slider, theme, Base state/options/result or old bridge ABI.
Proposed sized/versioned style owns palette, flags, rem/length/layout mode,
selected corner overrides and explicit render mode. Inactive override fields are
ignored; unknown tags/masks and invalid ACTIVE finite values reject atomically.
Theme is borrowed only to snapshot full/square radius and spring_control BEFORE
Base callbacks. Caller picks builtin/preset/custom palette explicitly; custom
cui_theme maps through existing from_theme, never inferred source identity.
Base options and handler remain the exact existing types/signature. Styled never
converts REQUEST to CHANGE, writes owner, or rereads external owner after callback.
External owner/userdata mutation and work enqueue remain allowed; active UI/model/
style/theme storage mutation/reentrancy remains prohibited through return.

Proposed presentation state (owned, separate from160-byte reducer state): exact
version/size, owner,last-admitted-frame, last-clock, next-wake and two numeric
spring records(position,velocity,target,updated_at), flags/reserved. No heap,
borrowed bounds pointer, renderer texture or gesture/value ownership in it.
Presentation result wraps passive Base result plus explicit render status and
finite absolute next-wake deadline (zero when idle). Borrowed callback requests
still own copyable records; no presentation-state pointer escapes via request.

Hot is actual prior clipped/occluded own thumb hover, not track hover. Pressed is
that endpoint's admitted logical gesture (including Root-selected capture), not
an independent decorative capture. After original-order reduction, derive each
ring target; same-batch up/cancel cannot leave pressed stuck. Sample old target
through elapsed time, then retarget without retroactive integration. Use checked
theme.spring_control; share/extract the existing private Switch spring math only
with frozen Switch equivalence tests, not copied divergent formulas or public
Disclosure state type-punning. Initial/omitted/reused owner resets cold; reduced
motion, travel=false or response0 snaps target/velocity0. Clamp visible progress
[0,1] independently of underdamped internal travel; epsilon settles exactly.
Backwards time does not integrate negatively; nonfinite clock/policy rejects
before Base input. Schedule at most60Hz while unsettled, then deadline0. Never
advance presentation state during inert/layout-only/blocked/recovery-only calls;
paint cold/confirmed and produce no wake until admitted again. No rollback of
already delivered Base requests after a later Core/renderer failure.

## Brushes and actual adapter authority

Private fieldwise conversion copies source-specific palette straight-linear RGB,
alpha, stop order/positions, SOURCE_038 projection, angle and space into checked
owned `cui_background` constructors. Never round-trip through representative byte
color. Preserve hidden RGB at alpha0, straight source interpolation/independent
alpha and Oklab choice; do not replace with CSS projection/premultiplied stops.

SOLID_V1 mode explicitly encodes solid linear colors to existing sRGBA8 paint;
normal byte quantization is documented. A visible LINEAR thumb in V1 returns
UNSUPPORTED before Base submission/state/input/callbacks, not a flattened color.
Disabled omitted surfaces do not require gradient support. BACKGROUND_V2 mode
requires the HOST to query a real initialized consumer before begin and to open
the matching feature-admitted Core V2 frame. The facade takes copied declared
support/mode, checks known ABI/output/features/limits before reduction, then sets
owned background on actual inner surface while it is open. Core setters remain
authority for current-frame lifecycle/capacity; a lying/mismatched host contract
can fail later and does not roll back requests. There is no public current-frame
V2-support getter to pretend otherwise. Do not add one without separate review.

Current software/SDL renderer/SDL GPU/Sokol/Canvas adapters do NOT advertise or
consume V2. Existing private preparation/validator and fabricated Core capability
fixtures prove transport only. Aurora Light must stay explicitly unsupported on
those live consumers until their actual V2 paths exist. No component-owned bitmap
cache/upload subsystem or gradient-to-solid fallback is proposed. Whole38-theme
rendering cannot be checked off from successful C brush copies or Core commands.

## Reviewable slices and closure oracles

1. Test-only responsive geometry prototype above; publish exact admitted/rejected
   layouts. Separately review any vertical scroll-admission extension/exception.
2. Private Base seam + source metrics/decoration plan, owned pure style resolver
   and color/brush mapping; frozen NULL-Base command/event oracle and all4+38
   independent source-token/alpha/corner/brush cases. No renderer claim.
3. Solid Styled facade + named parts, hover/press/focus policy, motion and complete
   callback/compose example; actual software pixels and touch/key/owner acceptance
   traces for all source states. Explicit gradient-unsupported admission tests.
4. Actual renderer V2 consumer slices remain independently owned/reviewed; after
   support exists, Aurora endpoints/midpoint/Oklab/rounded edge, parent alpha/clip,
   resize/lifetime/capacity failures and direct rendered pixels on each adapter.
5. New bridge sized typed receipts/options with full pair/domain doubles, source
   generation, event/raw input, owner acceptance and presentation status/wake.
   Preserve old scalar getters/config/event ABI. Python/Go optional new-symbol
   resolution and clear unsupported errors; JS addon/TS declarations+runtime,
   Rust Result/lifetimes own copied records. JS/TS64-bit identities/counters use
   exact BigInt (or existing explicitly lossless record mapping), never Number
   coercion. No Range→scalar or float narrowing; legacy notifications remain
   notifications. Test actual five runtimes, not only generated SDK compilation.

Final cross-product: four builtins+38 presets/custom alpha/overrides; enabled,
disabled, readonly, handlerless and zero span; normal/actual hover/drag-out/release,
keyboard-only focus, source generation/domain/kind changes and omission; rem and
size/axis/reverse/range/log; V1 solid vs actual V2 consumer vs unsupported. Include
transparent authored colors (mask establishes presence), ring alpha replacement,
square theme/corner overrides, reduced motion/reversal/settled idle, source rem
literal offsets, modal/inert/layout-only/invalid snapshot recovery and parent
resize. Font-free pixels do not prove glyphs; source has no icon to invent.

## Complete companion example plan

Add `examples/slider_model_controlled.c` plus a small focused compile/run target
sharing an existing Core fixture; extend the guide with a link and self-contained
callback/compose definitions. The companion must compile independently, own
persistent reducer/application state, initialize once, define on_request and
application, and show acceptance only on UPDATE (never blindly accept a CANCEL
pair that belongs to an old domain/kind). A concrete Base companion can ship
before a Styled symbol exists; do not publish an unimplemented Styled call.

```c
#include <cui/cui_base_slider_model.h>
#include <assert.h>
typedef struct { cui_slider_model_value_v1 confirmed;
                 cui_slider_model_state_v1 slider; } application;
static void on_request(void *user, const cui_slider_model_request_v1 *request) {
    application *app = user;
    if (request->phase == CUI_SLIDER_MODEL_UPDATE)
        app->confirmed = request->requested; /* Explicit external acceptance. */
}
static void compose(cui_context *ui, application *app) {
    cui_slider_model_options_v1 o = {.version=1,.struct_size=sizeof o,
        .domain={0,1,.125,CUI_SLIDER_MODEL_LINEAR,0},.name=cui_string("Selection")};
    cui_base_slider_model_layout_v1 l = {.version=1,.struct_size=sizeof l,
        .length=240,.cross_extent=24,.track_extent=6,.thumb_extent=16,.axis_inset=8,
        .track={.background={80,80,80,255}},.fill={.background={60,120,240,255}},
        .start_thumb={.background={240,240,240,255},.radius=8},
        .end_thumb={.background={240,240,240,255},.radius=8}};
    assert(cui_base_slider_model_events_v1(ui,100,app->confirmed,&app->slider,
        &o,&l,(cui_slider_model_handler_v1){on_request,app}).status==CUI_SLIDER_MODEL_OK);
}
int main(void) {
    application app={.confirmed={CUI_SLIDER_MODEL_RANGE,0,.25,.75}};
    assert(cui_slider_model_state_init_v1(sizeof app.slider,&app.slider));
    cui_context *ui=cui_create((cui_config){.max_elements=32,.max_commands=64});
    assert(ui && cui_begin_frame(ui,300,80,0)); compose(ui,&app); (void)cui_end_frame(ui);
    cui_focus(ui,cui_base_slider_model_part_id_v1(100,CUI_SLIDER_MODEL_END_THUMB));
    assert(cui_queue_event(ui,(cui_event){.type=CUI_EVENT_KEY_DOWN,.key=CUI_KEY_LEFT}));
    assert(cui_begin_frame(ui,300,80,0)); compose(ui,&app); (void)cui_end_frame(ui);
    assert(app.confirmed.start==.25 && app.confirmed.end==.625);
    assert(cui_last_error(ui)==CUI_OK); cui_destroy(ui); return 0;
}
```

This is a planned compile-tested companion, not a claim that this design-only
checkpoint ran it. Native execution must use the private-Xvfb wrapper. A GUI host
uses the same persistent application/compose functions inside its real loop,
consumes draw lists before the next begin and owns renderer support negotiation.

Signed design checkpoint: `/root/hidden_windows`, 2026-10-31. Production requires
root review; full Styled/renderer/native/allfive family closure remains pending.
