# Remaining Stepper presentation checkpoint

Read-only audit; CUI base `2929aa04`, frozen GPUI source
`87d10ae5e1299d1be18670a37570c5402f952e34`. No implementation or parity promotion.

## Exact source requirements

| Source evidence | Behavior still missing or different in CUI |
| --- | --- |
| `crates/component/src/stepper/stepper.rs:111-133` | Root is `w_full`, row/column according to axis. Current `cui_stepper_visual.c:36-41` is content-sized with explicit gaps. |
| `stepper/item.rs:116-162` | Non-last item gets `flex_1`; last stays intrinsic unless `text_center`, when **every** item gets `flex_1().justify_center()`. Source also sets items-start. This is not an unconditional equal-width grid: intrinsic/minimum sizes and caller refinements can matter. |
| `stepper/trigger.rs:108-119` | Horizontal trigger is a column with gap4; vertical trigger is a row with gap8. Default cross-axis items-start changes to items-center for `text_center`; it does not set text-align-center. Children are arbitrary elements, not only CUI title/description strings. |
| `stepper/item.rs:226-269` | Separator is absolute/zero-flow, not a fixed spacer. Thickness1.5/2/3 for XS/default/LG, gap4. Passed color remains `index < selected`. No separator after last. Current painter uses fixed36px horizontal or20px vertical spacer lengths. |
| `stepper/trigger.rs:120-151` | Unchecked indicator uses `tokens.secondary` and `secondary_foreground`; enabled unchecked indicator adds `secondary_hover` and `secondary_active`. Checked means `index <= selected`, then primary/primary-foreground wins, even disabled. Current painter uses surface/text for unchecked and never queries interaction state. |
| `stepper/trigger.rs` and `item.rs` (entire files) | No explicit focus tracking, focus-visible condition or focus-ring style invocation. Do not report a source-authored focus-ring feature as missing. GPUI engine implicit behavior is not established by these files. |
| `styled.rs:164-165`, `theme/mod.rs:551-557` | `rounded_full_style` becomes square when source theme radius is zero. CUI currently always uses `side/2`. |
| `sizing.rs:99-107,245-252` | Trigger uses `input_text_size(size.smaller())`, rather than CUI's current title=theme font and ordinal/description=80%. Exact typography remains partial. |

The source helper in `crates/base/src/styled.rs:105-115` makes h-flex center its
cross axis and v-flex leave stretch behavior; subsequent items-start/center calls
above override that locally. Do not infer a vertical full-height container from
`w_full`: the Stepper never requests `h_full` itself.

The separator expressions must be preserved rather than replaced by an idealized
"line to next center" algorithm. For an item with width W, horizontal centered
left/right are `0.5W` and `-0.5W`, with horizontal margins `side/2+4`;
non-centered left/right are0 with left margin `side+4` and right margin4. Both
have height=stroke and top margin `side/2` (not minus half the stroke). Vertical
non-centered uses top/bottom0, top margin `side+4`, bottom margin4, width=stroke,
left margin `side/2`. **Vertical centered literally uses `mx(side/2+4)` and
top/bottom ±50%**. It does not use vertical margins there. This surprising
expression needs an independent frozen-render fixture or equivalent pinned
layout proof before claiming exact vertical-centered pixels; silently correcting
it would not be source parity. Unequal slots likewise require source evidence.

## Compatible implementation direction

Preserve all existing functions and the published `cui_stepper_visual_options`
layout. A new extended visual entry may take copied typed presentation settings;
zero/absent settings forward to the current painter. No new input reducer,
retained application callback or Core dispatch is needed. Existing request
ownership/order, genuine prior target eligibility and captured selected painting
remain unchanged. Descriptor bytes/theme stay stable through submission, bitmap
pixels through rendering. No loading state is invented.

First reviewable slice: an opt-in **interaction + non-centered fill** presentation,
with a typed custom indicator palette, theme-radius behavior, and existing shared
design sizing. Unchecked default normal/hover can use `theme.secondary` and
`theme.secondary_hover`, foreground `theme.secondary_foreground`. CUI has no
standalone `secondary_active` field; `buttons[SECONDARY].active` is a possible
explicitly documented CUI fallback, **not** proof of the source global token
(button-specific overrides can differ). A custom pressed color can express that
source token without resizing the shared Theme ABI. Obtain approval for that
fallback policy before publishing the new declaration. Checked colors override
hover/pressed/disabled exactly as source. An optional keyboard focus-visible ring
uses `cui_interaction_state` and theme.focus as a **CUI extension**, off in the
source-style default; pointer focus alone must not turn it on. An unclipped outer
indicator wrapper can own the ring while an inner box clips bitmap corners.

Indicator hover is narrower than the trigger's clickable label/description area.
Do not just use whole-item `cui_hovered`. Use current-layout passive indicator
prepaint/occlusion, with no new semantic/tab target. Press-origin bookkeeping, if
needed, belongs in bounded UI-owned resource storage (at most100 items), read-only
observation of already routed events and current capture; never a second reducer.
Raw pointer position alone proves neither eligible hover nor accepted press.
The frozen component files establish `.active` authoring, but not the dependency
engine's drag-out/re-entry or keyboard-active details. Those details require a
pinned GPUI engine probe before an exact-equivalence claim; CUI Space feedback can
be documented as its existing keyboard model. Reset private visual press state
on cancel, omission, disabled/inert admission or context destruction.

Non-centered fill can use existing growing item boxes and a current-size underlay
inside each item: its connector stays within that item, grows with its resolved
main-axis extent, and never enters layout flow. Width0 in an extended source-fill
contract would mean available parent width; vertical height0 stays intrinsic,
positive height is an explicit available extent. Validate finite extents before
Base registration. Negative connector length clamps to empty geometry. Preserve
content mode and all existing legacy/visual pixels when the new profile is absent.

Centered fill is a **separate geometry checkpoint**, not an unsupported flag in
that first API. Its separator extends past its item's bounds. Existing underlays
clip to their owner; local `cui_prepaint_emit` also clips to visible_bounds.
Deferred emission clips to viewport and would lose ancestor clipping, so it is
not a valid workaround. Nor can `cui_track_bounds` provide sibling rectangles
inside prepaint: Core writes those retained results after command construction
(`src/core/cui.c:4089`), and `cui_bounds` prefers the previous frame. A group-owned
current-layout emission scheme or a narrowly reviewed private geometry lookup may
be needed for centered/unequal-slot fidelity. Do not add a Core/public seam or
claim same-frame correctness until that design/probe is reviewed independently.

## Meaningful acceptance

Use distinct custom normal/hover/pressed/primary/foreground colors so fallback
coincidences cannot pass tests. Actual pointer down/hold/up/cancel/leave/re-entry,
label-only hover, modal/overlapping passive blockers, disabled transitions,
checked/passed overrides and Tab-vs-pointer focus exercise pixels plus unchanged
per-activation requests. Test Space separately as the declared CUI policy.
No focusable separators/indicators and no extra callbacks may appear.

Geometry oracles should independently calculate default source offsets and zero-
flow connector bounds for both axes, multiple widths/heights, first-frame grow,
shrink/grow, zero/one/100 items, unequal labels, density XS/SM/MD/LG, shared XS..9XL,
square/rounded themes and nested clipping. Use exact software command/pixel checks,
then headless browser DPR1/2 and touch390 DPR2. Retain old painter/legacy request
regressions, preflight/capacity recovery and bitmap lifetimes. Centered negative-
right/bottom expressions, asymmetric vertical margins and intrinsic-size clamps
need their own frozen-reference proof before promotion. Native tests use the
private Xvfb wrapper. This audit runs no builds or browsers and claims no live AT,
complete styling, binding or full Stepper parity.

## Approved first-slice contract (2026-10-22)

This section supersedes the tentative zero-settings/focus-ring direction above.
The existing visual entry and options retain their ABI, pixels and reducer. A new
`cui_stepper_presentation.h` exposes an explicitly opt-in entry:

```c
typedef enum {
    CUI_STEPPER_LAYOUT_CONTENT = 0,
    CUI_STEPPER_LAYOUT_FILL
} cui_stepper_layout;
typedef struct {
    cui_color normal, hover, pressed, foreground;
} cui_stepper_indicator_palette;
typedef struct {
    cui_stepper_layout layout;
    float width, height;
    bool custom_palette;
    cui_stepper_indicator_palette palette;
} cui_stepper_presentation_options;
cui_stepper_navigation_result cui_stepper_present(
    cui_context *ui, cui_id id, const cui_theme *theme,
    const cui_step *steps, size_t count, size_t selected,
    cui_stepper_visual_options visual,
    cui_stepper_presentation_options presentation,
    cui_stepper_navigation_handler handler);
```

Calling this new entry opts into secondary indicator colors, current-layout
indicator hover/press feedback and theme-radius behavior even with zero options.
CONTENT retains the existing fixed connector spacer layout. FILL has zero-flow
non-centered connectors; its root width0 grows to available parent width, positive
width is explicit. Height0 is intrinsic, positive height is explicit (including
vertical available height). CONTENT requires width=height=0. Reject unknown enums,
nonfinite/negative extents, invalid visual/icon/design settings before registering
semantics or reducing input. All existing count/selection/string validation remains
owned by the single Base navigation call. Failure returns invalid; no new callback
or dispatch path exists. Shared design sizes remain available through `visual`.

Normal/hover/foreground defaults are secondary/secondary_hover/secondary_foreground.
Pressed defaults to `buttons[CUI_THEME_BUTTON_SECONDARY].active`: this approved CUI
fallback can differ from frozen global secondary_active. `custom_palette` replaces
all four unchecked colors. Checked primary/on_primary wins in every interaction
state; disabled unchecked indicators never use hover/pressed. Radius is zero when
theme radius is zero, otherwise half the indicator side. No focus ring is added.

Passive indicator prepaint observes current clipped/occlusion-aware geometry. It
adds no semantic or input target. Press is only decorative observation of the
already accepted trigger capture: primary-pointer capture paints pressed while
that pointer is visibly over the indicator; Space capture paints pressed without
requiring pointer hover. Label-origin capture dragged onto the indicator follows
this CUI policy too. This is not a claim about GPUI engine press-origin, drag
re-entry, or keyboard-active equivalence. There is no private gesture state or
second event reduction to become stale after omission, cancel or disable. Painting
uses captured options and selection, so callbacks updating owner state still take
effect on the next submission. Current-layout passive blockers/modal admission
remain authoritative for pointer feedback.

Prepaint payloads copy colors, dimensions, trigger identity and interaction flags;
never caller descriptor-array pointers. Bitmap metadata is copied by existing
paint submission and bytes remain borrowed through renderer completion. No new
application-retained state is introduced. FILL connector rectangles stay within
their item and clamp negative lengths to empty; centered crossing connectors and
exact source typography remain deferred. Tests must cover legacy command/pixel
stability, independent fill geometry, indicator-only hover, accepted capture,
Space, cancel/disabled, occlusion, callbacks, shared sizing and invalid preflight.
