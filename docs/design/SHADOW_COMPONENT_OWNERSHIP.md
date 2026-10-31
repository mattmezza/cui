# Component shadow ownership: first rollout proposal

Audit: fb3be922 plus named-element shadow slice c872cb4a. The first approved
root/existing-part slice is implemented; anonymous-part and binding rollout
remain proposals. Evidence below describes the actual tested entry points.
The Core setter accepts current-frame paint IDs, not arbitrary semantic handles.
All rows below require component-owned acceptance tests before coverage is claimed.

## Concrete root and part inventory

| Family / entry | Current painted target | Limits / source evidence |
| --- | --- | --- |
| Button / button_ex / styled | caller `id` | `cui_components.c:609` opens resolved root with this ID; wrappers reuse painter. Custom content is inside it. Radius follows resolved style, not necessarily theme radius. |
| Input / input_ex / input_paint | caller `id` | `cui_components.c:2579` opens actual framed/clipped root. `cui_input_viewport_id(id)` is already public, but is inner text viewport, not surface. No-appearance remains a transparent caster unless explicitly styled. |
| Checkbox / checkbox_ex / source | caller `id` is whole label/content row | `cui_components.c:1323–1329`: column, optional spacer and actual square indicator are anonymous. Shadow on caller ID covers label too. Indicator callback runs while indicator is open but replaces default glyph composition; not a general post-submission part API. |
| Radio / radio_ex | caller `id` is whole row | `cui_components.c:1875–1883`: anonymous column/spacer/circular indicator. `cui_radio_group.c:54–60` independently paints item rows and anonymous indicators; do not infer group parity from radio_ex. |
| Switch / switch_ex | caller `id` whole row; public `cui_switch_track_id(id)` and `cui_switch_thumb_id(id)` are real named boxes | `cui_components.c:1436–1439,1544–1574`; legacy wrapper delegates ex at1625. Track owns clip; thumb shadow is clipped by track. Root shadow includes label. No new part identity needed. |
| Slider / cui_slider | caller `id` framed label/control; track already named privately | `cui_components.c:2079,2085`; track uses existing owner-derived ID; fill is anonymous, conditional on positive ratio. This entry has no independent thumb box: do not invent thumb support. |
| Avatar / avatar_ex / events | caller `id` circular outer surface/ring | `cui_content.c:109,265–289`; image child has separately derived ID via private helper, image may be absent; outer root handles ring/focus. Stack children are another API, not covered by single-avatar evidence. |
| Badge / cui_badge | caller `id` is colored pill | `cui_components.c:1914`. |
| Badge / cui_badge_ex | caller `id` wraps content; visible indicator already named privately | `cui_components.c:1939–1982`: separate absolute overlay and indicator. Root shadow includes wrapped application content. Count0 omits indicator; dot/icon show independently. Indicator has existing STATUS semantics, not a new shadow input target. |
| Popover / cui_popover_begin | `state.overlay.id` is panel portal | `cui_surface.c:160,189,218–233`; begin opens nested body, so open-box setter immediately after begin styles body instead. Apply named setter after surface submission (or end). Refined panel already may have copied shadows: explicit assignment replaces list. |
| Dialog / cui_surface_begin with DIALOG state | `state.overlay.id` is panel portal | Same surface implementation. Separate backdrop, resize, close/title/body parts; `cui_surface_part_id` documents1..8 (`cui_surface.h:162–164`). None is an alternate alias for the panel root. |
| Tooltip / cui_tooltip | caller tooltip `id` is popup portal, only when function returns true | `cui_content.c:1100,1128–1134`; `options.trigger` is a different owner. Inactive/delayed/omitted tooltip has no current-frame panel. Tooltip-events entry follows same panel path; HoverCard is separate contract. |

Paths above are relative to `src/components/` except named headers. Existing part
helpers return identity, not evidence that a part was emitted this frame. The
Core setter supplies current-frame presence validation. Do not call it on absent
conditional parts and treat INVALID_ACTION as ordinary application success.

## Independently reviewable first ownership slice

Start with explicit root contracts/tests for Button, Input, Avatar, simple Badge,
Switch track/thumb and surface/tooltip panel targets. They need no new identity,
Core registry, pending-style mechanism or widget struct layout. Document the
actual root, radius choice and lifecycle alongside its authoritative header.
Root Button/Input/Avatar and legacy Badge IDs already need no identity getter.
Popover/Dialog use public state.overlay.id; Tooltip uses its caller ID. Avoid
redundant aliases that imply a semantic ID is universally a painted surface.

Add only two pure public part getters initially:

```c
/* Proposed, not implemented. Return0 for owner0. Identity only, no presence check. */
cui_id cui_badge_indicator_id(cui_id owner);
cui_id cui_slider_track_id(cui_id owner);
```

Internally centralize the existing derivations and replace the current literal
calculations with these getters. Do not alter assigned IDs, element count, clip,
paint, semantics or event ownership. Badge's existing overlay ID need not become
public until a real styling use and tests establish its empty-layout behavior.
A slider fill/knob getter is not part of this slice. Existing switch getters remain
unchanged. Public IDs remain stable contract; users never need private XOR salts.

Acceptance tests must submit each entry in a real frame, apply a distinctive
copied layer after composition, and verify caster matches the intended surface
rather than content row/viewport/trigger. Include actual software pixels, rounded
geometry, both positive/negative spread, inset and DROP, ancestor clip, opacity,
current-frame resize, supplied image/ring Avatar, custom Button style, Input
no-appearance, Switch thumb clip and Badge0/1/dot/icon visibility. Portal tests
assert panel styling changes neither backdrop/arrow nor trigger paint and does
not change modal dismissal/activation. Verify same IDs, element/semantic counts,
legacy command lists/pixels and queued interactions with no shadow opt-in.

Getters need deterministic0 handling, owner separation, actual emitted-ID match,
no duplicate registration and explicit adversarial collision/capacity tests. Test
existing IDs before/after refactoring, not merely getter arithmetic copied into
expectations. Test conditional Badge absence before calling the element setter;
separately assert missing target rejects atomically. Report entry coverage, not
family completion for every variant.

## Anonymous indicators: separate approval boundary

Checkbox and Radio require an additive opt-in composition route exposing stable
indicator identities. Do not silently name anonymous boxes on every legacy call:
that changes registered ID capacity/collision behavior and descendant auto IDs.
First audit both individual Radio and RadioGroup painters, plus Checkbox custom
indicator/state paths. Proposed next decision: an additive named-parts entry point
or explicit part descriptor passed to a new variant; original public options
layouts and all old call paths remain unchanged. Only the opted-in route assigns
indicator IDs; glyph decoration remains passive and uses the existing one reducer.
No global registry, pending-next-box style or new input dispatch is required.

Before approval, specify owner0/part collision rejection, custom indicator presence,
stable identity across checked/mixed/error/disabled and item reorder, and failure
atomics. Tests must compare old path commands/pixels/layout/input/semantics exactly,
then prove opt-in identity adds no nodes or focus targets, retains clipping/radii,
and explicitly accounts for extra named-ID slots and new collision opportunities.
Do not claim anonymous parts covered because the whole row can cast a shadow.

## Foreign-language authoring route

Current bridge enum has the requested widget kinds (`include/cui/cui_bridge.h`),
but its internal retained node (`src/bridge/cui_bridge_internal.h`) has no shadow
property. Submission is centralized in `src/bridge/cui_bridge.c`: Tooltip at4398,
Surface at4423, Input at4580, Switch at4652 and Button at4695. This is a concrete
shared integration point for Python, Node, TypeScript, Go and Rust builders.

Propose a new additive bridge shadow setter with generation-checked node handle,
explicit target enum ROOT/known typed PART and bounded record array. Validate and
copy all <=8 typed records into bridge-owned node storage immediately. A dedicated
boolean distinguishes no override from explicit empty override; an empty override
clears component defaults, while removing the override restores defaults. Keep
legacy bridge structs/ABI version1 layouts stable by adding functions/new records,
not growing existing wire structs. Records carry finite float offsets, sigma,
spread, RGBA bytes and fixed-width kind; bindings must not cast host-language
arrays to arbitrary C layout. Existing Core kinds/presets and CSS factory provide
validation/conversion; do not parse CSS strings or reinterpret sigma.

Resolve each typed target in that component's actual frame submission, then apply
a copied value list after its painter closes. A retained builder property is not
a call to Core before a frame exists. A hidden popup keeps its owned requested
style but emits no setter until its panel is submitted. Present-but-unsupported
parts reject at bridge authoring; known conditional parts can be dormant and
activate on later visible submission. Define that distinction explicitly, instead
of converting every Core INVALID_ACTION into silent success. Deletion/reuse drops
old style with the generation; update failure leaves prior node style intact.

Expose matching typed `ShadowLayer`/preset/CSS-constructor helpers plus node
shadow/clear/reset methods in all five facades. Switch/Badge/Slider part selectors
must match the reviewed public ownership API; Checkbox/Radio indicators stay
unsupported until their opt-in composition route is integrated. Contract tests
exercise copying, stale handles, all kinds/counts, preset replacement, empty vs
unset, invalid transaction, popup hide/show and real target pixels through each
language. Bundled Go/Rust C source alone remains insufficient binding evidence.

## Proposed beta-roadmap status

- Delivered: portable eight-layer DROP/INSET renderer capability, named presets,
  current-frame named-box addressing and explicit CSS blur conversion.
- Next independently testable slice: common painted roots plus existing named
  Switch parts and additive Badge-indicator/Slider-track getters.
- Pending: anonymous Checkbox/Radio indicators and remaining78-family part audit;
  owned typed bridge and Python/Node/TypeScript/Go/Rust authoring; cohesive visual
  examples and relevant headless/browser evidence.

The beta target remains consistent style selection across applicable component
paint parts and bindings. The generic C API is an enabling slice, not completion
of that target. Host/behavior-only families still need an explicit not-applicable
caster decision rather than fabricated rectangular surfaces. No compilation,
renderer work or generated consumer resources are changed by this proposal.

## Approved first ownership contract

The first implementation centralizes Badge_ex's existing visible indicator ID and
cui_slider's existing track ID behind checked pure getters. Each returns0 for
owner0; nonzero owners retain exactly their current derived IDs. The functions
report identity only. No anonymous boxes receive IDs, no component nodes are
added, and old behavior/pixels/semantics remain unchanged absent explicit styling.

Authoritative header comments identify actual surfaces: Button, Input, Avatar and
legacy Badge use caller ID; Checkbox/Radio caller IDs denote whole rows; Switch
has existing track/thumb getters; Slider exposes track but no thumb; Badge_ex's
caller ID wraps content and its indicator can be absent. Popover/Dialog panels
use state.overlay.id and Tooltip's emitted popup uses its caller ID. Tests style
these current-frame paint targets after their owner submits them. Corner radii
remain explicit caller style values, not inferred from a semantic record.

New getter validation does not change old zero-ID submission behavior: composition
uses the existing internal derivation while public getter0 returns0. Collisions
must still be rejected by existing Core identity validation. No all-family or
foreign-language style support is implied by this additive C-only contract.


## First ownership implementation evidence

The Badge indicator and Slider track getters centralize existing assigned IDs;
they add no paint/layout/semantic nodes or new dispatch path. Header comments and
consumer metadata identify actual root/part ownership. Tests freeze preexisting
ID values, reject owner0/derived0 as style targets, check omitted count0 Badge
and existing application/part collision rejection. Slider's legacy owner0 painter
keeps its original track derivation, independent of checked getter0 behavior.

`test_component_shadow_ownership.c` exercises Button_ex with custom rounding,
Input_ex framed and no-appearance, Avatar_ex initials and actual copied photo
cache/ring, legacy Badge, Switch_ex track and thumb, Slider track, and Badge_ex
count/dot/icon indicators. Actual tracked current-frame bounds match emitted
mixed DROP/INSET casters; software output changes under opt-in and returns exactly
to default after clear. Cases cover movement, applicable resizing, ancestor
clip/opacity, unchanged semantic IDs and real queued Switch activation with and
without track styling. Text commands are omitted from font-free software raster
oracles; actual layout uses an explicit measurement callback, not fabricated
glyph rendering or source typography claims.

Popover (anchored with arrow), Dialog and focused Tooltip tests separately prove
panel caster ownership, conditional tooltip omission, unchanged semantic IDs/roles
and preserved trigger/backdrop/arrow paint. An early Tooltip assertion exposed a
legitimate command-placement distinction: explicit CUSTOM shadow paint emits the
background before child-overflow clip, while ordinary rectangle paint can be
inside that clip. The test does not require these active opt-in wrapper positions
to be identical. Instead an independent ordinary-only software render removes
shadow commands and compares every RGBA byte to the unstyled baseline, including
all ancestor masks, trigger, backdrop and arrow. Cleared/unmodified frames retain
full ordinary-command order/bounds/colors and identical software pixels.

Focused strict Release build/tests use the existing build-shadow directory,
nice19, CPU0 and -j1. The new ownership and existing element-shadow tests passed
2/2 through private Xvfb. Source SDK/distribution/skill resources are generated by
canonical tools. Installed-skill tests passed17/17; SDK checksum, amalgamation,
skill freshness and78-entry manifest checks passed. No browser, native GPU,
full-78-family, anonymous Checkbox/Radio
indicator or typed binding completion evidence is added by this slice.
