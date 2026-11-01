# Skeleton own-root responsive and Styled presentation

Design only, against worker `65742454` and frozen source reviewed in
[Skeleton parity](SKELETON_PARITY_CHECKPOINT.md). Palette C compilation/runtime
acceptance is still pending; no production authorization is implied here. Keep
all old phase/ex/options functions and layouts unchanged. Full family and all
five foreign authoring surfaces remain open.

## Existing mechanisms and actual limits

Pinned `skeleton.rs:40–60` builds a single div: full width, height4, theme Skeleton
background with optional .5 alpha, then `.refine_style(&self.style)`, then an
animation closure `this.opacity(1 - delta*.5)`. Refinement can replace the base
background, dimensions, corners, border and spacing. The animation sets the root
opacity AFTER refinement: a caller's root opacity does not multiply the pulse.
Ancestor opacity remains independent. This ordering also animates authored border
paint, not just background. There is no child content/slot or input behavior.

Current Core already submits these properties in one `cui_box`:

* `src/core/cui.c:1995–2023`: positive dimension fixed, negative grow, zero FIT;
  percentage flag takes precedence over fixed/max with fraction clamped0..1.
  Explicit fixed zero uses Core's canonical zero handling. These are existing
  public conventions, not a proposed new layout engine.
* `cui.c:2063–2066`: custom root opacity clamps then multiplies ancestor opacity;
  box background/border paint and corner geometry are emitted by the existing
  pipeline. `include/cui/cui.h:431–464` provides root sizing, max caps, physical
  padding/border edges, offsets, clip, corners, inert and layout_only.
* `src/core/cui_scroll_layout_internal.h:397–409`: registered scroll-tree admission
  rejects percentage HEIGHT except requested canonical fixed zero. Width percent
  is not rejected by that rule, but final width fingerprint and ordinary/floating
  admission checks still apply. No blanket registered-scroll support inferred.
* A grow/percent PARENT around existing fixed Skeleton does not change its actual
  child paint width. Therefore the existing palette/ex function is not a substitute
  for the proposed direct root route. No prior-frame bounds read, input replay,
  prepaint geometry hook, portal or global dispatcher is necessary for this
  leaf's ordinary responsive sizing.

Full-width under an unconstrained/FIT parent has Core/Clay's existing dependency
semantics; no guessed viewport width fallback. The meaningful source full-width
proof must use a positive definite parent extent and resize that actual parent.

## Proposed exact public value and entrypoints

Add declarations to `cui_content.h` only after review and palette acceptance:

```c
typedef struct {
    cui_box root;
    bool custom_background;
    bool secondary;
    float duration;
    double now_seconds;
    bool reduced_motion;
    cui_loading_ease_fn ease;
    void *ease_user;
} cui_skeleton_presentation;

cui_skeleton_presentation cui_skeleton_presentation_default(void);
bool cui_skeleton_present(cui_context *ui, cui_id id,
                          const cui_skeleton_palette *palette,
                          cui_loading_motion *state,
                          cui_skeleton_presentation presentation,
                          cui_loading_motion_result *out);
```

This is a new by-value submission record, not an extension of old options/state.
No version/size field, new enum, retained object, public platform/Clay type or
extra animation authority. The only persistent state is existing caller-owned
`cui_loading_motion`. `ease_user` is borrowed synchronously. `cui_box` already is
the portable public own-root style vocabulary; no family-specific duplicate
fraction/flex/corner/edge flags are justified.

Default constructor returns all-zero fields EXCEPT:

* `root.width=1`, `root.width_percent=true`: source full-parent width, NOT fixed160
  and NOT grow(-1). Caller may explicitly select grow or another fraction through
  ordinary root fields.
* `root.height=16`: source default4rem-quarter units at CUI default16px rem. This
  is literal16 logical pixels; source user rem scaling is explicit application
  assignment (`root.height=rem_size`). No invented theme-font inference. No size
  variants exist in source and no new size enum is proposed.
* `duration=2`: source period. `duration=0` also selects2 as existing options.

Default root has radius0, no border/padding/clip/custom background. A zero-initialized
record is legal Core FIT sizing; it is NOT a second default spelling. Applications
wanting source defaults call the constructor, then change fields explicitly.

## Authoritative precedence, sampling and failure rules

1. Validate mandatory ui/id/palette/state/out, duration and existing sampler state/
   now/ease contract before any box submission. Copy the entire presentation and
   palette color at entry; no borrowed public config/theme is reread after user
   easing. Existing ex remains post-callback theme-color behavior unchanged.
2. Select base background from owned palette. If secondary is true, apply .5 to
   palette alpha using the established encoded-byte adaptation. If
   `custom_background` is true, replace that result with `root.background`
   completely, INCLUDING authored alpha0; secondary no longer attenuates the
   explicit replacement, matching source refinement order. No zero sentinel.
   Inactive root.background is ignored. Other root paint fields remain active.
3. Sample existing motion exactly once on a local state copy. Palette/style values
   cannot influence easing input. Reduced motion combines caller/Core preference,
   skips easing and resolves root opacity1/no wake as existing Skeleton policy.
4. Apply selected background to copied root. Set `root.custom_opacity=true` and
   `root.opacity=result.opacity`, replacing both caller root opacity fields.
   Caller root opacity is deliberately inactive even if nonfinite: source animation
   writes it last. To fade a whole loading group, use ancestor opacity. Border
   color, per-edge border widths and background all receive the existing Core
   inherited opacity pipeline; never multiply only the background by the pulse.
5. Submit exactly one `cui_begin_box(ui,id,root)`/end pair, then commit local motion
   and result. No generated child identity, semantic registration or extra scope.
   Root contains no children; padding/border/FIT sizing use actual Core leaf rules.
   Scheduling remains caller-owned absolute `next_wake_seconds` in supplied clock
   units. Omission preserves caller state; explicit reset is caller policy.

Core style normalization remains Core's existing contract: negative/nonfinite
padding/edges normalize, percent/max/fixed precedence remains unchanged, corner
radii are renderer-clamped. Do not inconsistently reject Core-normalized values
only for Skeleton or duplicate a private layout validator. Pointer-valued members
are only ease_user (and existing cui_box contains no borrowed text/children).
Inputs/state/out must be valid disjoint objects; no generic alias guarantee.

On invalid mandatory arguments or failed motion prevalidation/sampling, return
false without box output/state/result changes. Callback side effects are not
rolled back. On successful component submission commit state/result as existing
ex; later Core capacity/layout/registered-scroll errors remain Core failures,
not a hidden transaction or rollback. Caller must inspect completed-frame error
before claiming geometry/paint. No presence query or hardware delivery promise.
A failed current registered layout must not be described as supported Skeleton
geometry simply because sampling returned true.

### Byte adaptation tradeoff requiring explicit acceptance

Current fixed palette painter multiplies background alpha by secondary and pulse
then truncates once. This new source-ordered route applies secondary to background
before root opacity, and Core may truncate at its own existing paint conversion.
It can differ by one byte from the fixed painter for fractional/translucent
values. This is a new opt-in source-order presentation route; preserve old/ex/
fixed-palette output EXACTLY, and document/test the new rounding stages rather
than weakening a frozen comparator or pretending both routes are identical.
No new renderer/float-color architecture is proposed to erase this difference.

## First proof slice before production

Prepare a test-only ordinary-box prototype using palette plus existing sampler,
not a private production hook. Under a separately granted small cached-Core lease:

| Witness | Required actual observation |
| --- | --- |
| First frame at parents320/91px, width100% | Own ID bounds equal parent inner width immediately, height16; no prior frame/bounds dependency. |
| Parent resize320→91→420, nested padding/border | Actual own root and rectangle track inner extent; clip and rounded pixels agree. No wrapper-only measurement. |
| Width50%, explicit120, grow with fixed sibling/spacers, max caps, explicit zero/FIT | Follow actual existing Core conventions in rows and columns; stable IDs; no viewport fallback. Test percentage flag precedence explicitly. |
| Square/circle/rectangular own custom corners and borders | Actual rectangle/border commands and software pixels, not only count; per-corner and per-edge clear/paint bounds. |
| Secondary palette vs explicit authored background(alpha0 included) | Source precedence proven independently. Explicit background unaffected by secondary, RGB retained for transparent authored input; no fallback. |
| Caller root.opacity0/.2/NaN vs parent opacity.5 | Root authored opacity ignored because animation replaces it; parent remains multiplicative; pulse fades border AND background. Exact expected byte stages recorded. |
| Custom ease mutates source descriptor/palette | Both new snapshots remain call-entry values; one synchronous invocation, original userdata mutations visible externally. Frozen old ex timing remains unchanged. |
| Reduced motion, omission/reset/reinsert, backwards clock | No implicit invalidation; finite absolute wakes, idle quiescence and explicit owner lifecycle. |
| Registered Y body: fixed16 height/full or half width | Actual successful current-frame admission, first/resize width fingerprint, final bounds and retained scroll offsets. No broad claim until this passes. |
| Registered body percent HEIGHT and problematic floating/height-dependent cases | Expected existing Core diagnostic/empty failed snapshot, no rollback claim. Clearing diagnostic alone does not certify geometry; fresh good frame recovers. |
| Layout-only/inert/clip, capacity error | Existing Core suppression/normalization and later-error contract, no semantic/focus/action nodes created; state effects explicitly observed. |

Do not change scroll admission to make these tests green. If default full width or
supported fixed-height cases fail, report exact witness and reconsider only that
portable presentation descriptor before any Core proposal. Percent-height inside
registered scroll remains an explicit unsupported combination unless separately
reviewed Core work establishes it.

After proof/review, implementation can reuse the Base sampler and existing box
submission with one Components function; no engine architecture is required by
this proposal. A complete authored loading profile/list example should demonstrate
actual narrow/wide own-root fractions and loading→real-content replacement plus
application busy/status semantics. Five modern bindings and bridge wake/copy/
owner policies remain separate required delivery work; old constructors stay frozen.
Metadata/SDK/skill changes wait for verified implementation under integrator ownership.

## Prepared public implementation checkpoint

Source-only separate `include/cui/cui_skeleton_presentation.h` and
`src/components/cui_skeleton_presentation.c` implement the reviewed record/default/
function without editing old content code/headers. The public header includes the
existing portable content/palette types; the source depends only on ordinary Core
box submission and the existing Base motion sampler. Palette/style snapshot,
secondary→custom replacement→root opacity order and later-Core-error limits are
explicit in the public comments. No layout/dispatcher changes were required.

Prepared public fixtures: `tests/unit/test_skeleton_responsive.c` repeats the
proven prototype's independent geometry/pixel/registered-body expectations through
`cui_skeleton_present`; `test_skeleton_presentation_contract.c` tests15 atomic
rejections, easing mutation/copy ordering, explicit defaults, grow/fraction/fixed/
zero sizing and layout-only/inert sampling. Header C/C++ include-order fixture is
`tests/fixtures/skeleton/presentation_header.c`. All remain uncompiled/unexecuted.

Integrator-owned wiring needed after review/lease: add the new Components source
and public header to module/source/SDK order (after content header), link both
new C tests against `cui` (responsive also `cui_software`), and register four C/C++
header-order compile/link tests. Preserve the frozen legacy palette/presentation
fixture and rerun it against the combined object set. Strict WASM source/header
and selective/umbrella distribution closures remain later gates. No shared CMake,
manifest, metadata, skill, generator or generated-resource edit in this checkpoint.
