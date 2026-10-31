# Semantic bridge part shadows: review checkpoint

Status: proposal only, audited at b9c2f3d1. This checkpoint changes no public API,
production source, generated resources, coverage status or beta completion claim.
Implementation requires review of the target table, group opt-in boundary and
storage choice below. The existing six-root V1 implementation remains unchanged.

Read with [element styling](ELEMENT_SHADOW_STYLING.md), the complete 78-family
inventory there, [component ownership](SHADOW_COMPONENT_OWNERSHIP.md),
[approved bridge V1](BRIDGE_ELEMENT_SHADOW_AUTHORING.md),
[named indicator checkpoint](NAMED_SELECTION_INDICATORS.md) and the consumer
[named indicator contract](../NAMED_SELECTION_INDICATORS.md). Historical inventory
rows describe their audit base; implemented C getters/named entry points and the
six-root bridge contract at this checkpoint supersede their older pending text.
This is a small next mapping, not a replacement 78-family completion matrix.

## Proposed scope and vocabulary

A target belongs to one generation-checked bridge node. ROOT means that node's
actual outer named layout/paint box; it never means its visually prominent child.
MAIN means a documented per-kind alias for one physical target. INDICATOR, TRACK
and THUMB name specific physical surfaces. MAIN does not inherit from ROOT, and
an unsupported MAIN never silently falls back to the label row. A shadow on a
rounded rectangular caster is not a shadow of a glyph or arbitrary alpha mask.

| Node and actual route | ROOT | MAIN | Other supported roles |
| --- | --- | --- | --- |
| Existing Row, Column, Label, Button, Badge, Input direct submissions | Existing V1 root | ROOT alias | None |
| Checkbox direct `cui_checkbox` branch | Whole label/control row | INDICATOR alias | INDICATOR square, named only on indicator override |
| Individual Radio direct `cui_radio` branch | Whole label/control row | INDICATOR alias | INDICATOR circle, named only on indicator override |
| Switch direct `cui_switch_ex` branch | Whole label/control row | TRACK alias | TRACK and THUMB |
| Slider direct `cui_slider` branch | Framed label/control root | TRACK alias | TRACK only |
| RadioGroup direct `bridge_render_group` branch | Group layout container | Unsupported: no single group indicator | None |
| ITEM whose immediate owner is the above RadioGroup | Item label/control row | INDICATOR alias | INDICATOR circle, through the group owner |
| Every other node/owner route | Unsupported by this proposal | Unsupported | Unsupported |

MAIN and its explicit alias address exactly the same stored override. Last
successful set through either name replaces that one value; reset through either
name removes it. They cannot be styled independently. ROOT versus INDICATOR, and
Switch ROOT/TRACK/THUMB, are independent slots. A group ROOT affects its own
container, not its items; no wildcard or selected-item propagation is introduced.

Slider's fill is anonymous and exists only at positive ratio. Its current painter
has **no independent thumb box**. THUMB and INDICATOR on Slider must be rejected
at authoring time, even if a frontend convention calls it a knob. No new Slider
appearance, geometry or input behavior is proposed.

ROOT support for the four new control kinds and RadioGroup/item rows is available
only through the proposed explicit-target API. Do not expand the existing
`cui_bridge_box_shadows_supported_v1` six-kind query or old root setter allowlist.
On the original six kinds, new ROOT/MAIN shares the old inline root record, so
there are no two competing root stores or conflicting reset semantics.

## Painter evidence and attachment points

References below identify functions in the audited source, not promises based on
semantic IDs or previous-frame bounds.

- `src/bridge/cui_bridge.c`, `bridge_render_node_other`: Checkbox uses the bool
  convenience `cui_checkbox`, individual Radio uses `cui_radio` with option1, and
  both update the same existing retained value/change notification. The named
  convenience variants can replace exactly those calls when an indicator has an
  authored override. Do not switch Checkbox to the source/events facade or alter
  reducer/focus policy. Preserve bool changed semantics and reduce once.
- `cui_switch_ex` already names the real boxes using `cui_switch_track_id` and
  `cui_switch_thumb_id`. Track owns clipping; a thumb DROP shadow remains clipped
  by that track. Apply distinct overrides after the whole helper returns; do not
  target the separate conditional focus-ring portal.
- `cui_slider` opens its caller root, names its track through
  `cui_slider_track_id`, then conditionally paints an anonymous fill. Apply track
  and root overrides after the helper. Zero value does not omit the track.
- `bridge_render_group` gathers <=64 direct ITEM children into `scratch->radios`
  and `binding.slots`, resolves the selection by live child slot, then calls
  **one** `cui_radio_group`. `src/components/cui_radio_group.c` has a distinct
  painter which emits the group container, item rows, indicators and labels.
  It does not call individual `cui_radio`. The new branch must call one
  `cui_radio_group_with_parts`, retain selected-slot/change handling, then apply
  each item's stored overrides using the same captured slot/ID mapping. Reorder
  uses item identity, not the array index. Do not introduce one reducer per item.
- `bridge_render_node_other` has a deliberate no-op ITEM branch. An epilogue on
  ITEM cannot implement group item shadows. Group scratch allocation can fail
  before painting; generic hooks also cannot infer successful root submission.
- Settings is a concrete counterexample to kind-only support: its retained
  Checkbox/Switch field nodes are converted into `cui_setting_item` data in
  `bridge_render_settings`, then painted by Settings using its ownership scheme.
  They do not reach the direct control dispatcher. Those field routes remain
  unsupported, rather than accepting a style that never reaches its target.

Capability classification must recognize actual direct-dispatch owners and
RadioGroup-owned ITEMs, with a reviewed whitelist for container/panel callbacks
that really call `bridge_render_node`. It must reject owner-rendered Settings
fields and other unmapped model/composite routes. It must not infer support from
node kind alone, the mere existence of `node->id`, a child relationship, semantic
presence or `cui_bounds`. Existing six-root operations retain their current
behavior; this stricter classification governs the new target operation.

Recommended first owner whitelist: the target is the app root, or every ancestor
between a directly dispatched target and the app root is Row/Column. For a
RadioGroup ITEM, its immediate parent is RadioGroup and every ancestor above that
group is Row/Column. RadioGroup ROOT follows the same ordinary-tree rule. This
makes the initial supported topology explicit and excludes Settings/panel/model
ownership without speculative traversal inference. Additional panel callback
contexts require a later reviewed whitelist extension and absence tests;
unreviewed contexts report unsupported, not conditionally hidden. A previously
authored unmounted request placed outside this topology fails at mount rather
than becoming a dormant accepted style. No portal target is added here.

## Additive portable V1 contract candidate

Reuse `cui_bridge_box_shadows_v1` exactly: 224 bytes, schema1, reserved0, <=8
24-byte DROP/INSET layers, numeric0xRRGGBBAA, explicit TL/TR/BL/BR radii. Reuse
existing pure CSS/preset factories; no duplicate math or preset tables. Old ABI
version1, options/state layouts and functions do not change.

Proposed descriptor and discovery record contain only fixed-width integers:

```c
/* Proposed names; not available declarations. Enum values are wire constants. */
/* ROOT=0, MAIN=1, INDICATOR=2, TRACK=3, THUMB=4. */
typedef struct {
    uint32_t struct_size, schema_version, role, reserved;
} cui_bridge_shadow_target_v1; /* size16; offsets0/4/8/12 */
typedef struct {
    uint32_t struct_size, schema_version, canonical_role, flags;
} cui_bridge_shadow_target_info_v1; /* size16; offsets0/4/8/12 */

uint32_t cui_bridge_shadow_target_v1_size(void);
uint32_t cui_bridge_shadow_target_info_v1_size(void);
cui_bridge_status cui_bridge_shadow_target_query_v1(
    cui_bridge_app *, cui_bridge_node, uint32_t target_size,
    const cui_bridge_shadow_target_v1 *, uint32_t info_size,
    cui_bridge_shadow_target_info_v1 *out);
cui_bridge_status cui_bridge_set_target_box_shadows_v1(
    cui_bridge_app *, cui_bridge_node, uint32_t target_size,
    const cui_bridge_shadow_target_v1 *, uint32_t style_size,
    const cui_bridge_box_shadows_v1 *);
cui_bridge_status cui_bridge_reset_target_box_shadows_v1(
    cui_bridge_app *, cui_bridge_node, uint32_t target_size,
    const cui_bridge_shadow_target_v1 *);
```

The descriptor selects only a semantic role. No raw paint ID, index, native enum
layout, pointer-sized field, platform/Clay object or selector string crosses the
wire. Select a RadioGroup item using that ITEM's own bridge handle; the parent is
validated from the retained tree, not supplied as an arbitrary second handle.
Unknown roles/schema/reserved/size are INVALID_ARGUMENT. All offsets/sizes need
C static assertions and facade layout tests before production.

For a valid known role and live handle, query returns OK and writes the full info
record. Flags are SUPPORTED=1, NEEDS_NAMED_INDICATOR_ROUTE=2 and
CONDITIONAL_SUBMISSION=4; all other bits are zero. Unsupported known mappings
return flags0 and canonical_role=UINT32_MAX, without promising a future mapping.
Query never allocates, invokes component code or resolves current-frame bounds.
It reports declared capability, not whether paint exists this frame. The initial
ordinary control/group/item mappings have no internal conditional indicator,
track or thumb, so do not set CONDITIONAL_SUBMISSION merely because unchecked,
disabled or outside a painted traversal. A reviewed future callback/panel route
may use this flag with its explicit normal-absence condition.

Query and mutation validate app/lifecycle/thread/handle first using existing
setter rules; then descriptor shape, owner route and role. Mutation on a known
unsupported mapping returns INVALID_ARGUMENT, matching the existing style setter
precedent. For set, validate the complete style next; then acquire needed storage
and commit once. Query output and old authored styles remain unchanged on any
error. No additional callback phase restriction is introduced. Missing newer
symbols yield an explicit unsupported-feature/matching-revision error in facades,
without breaking default loads of older Python/Go libraries.

## Owned values, replacement and identity

Each canonical target has three states: absent override, explicit empty, or a
nonempty copied list. Empty and preset NONE actively suppress both shadow kinds
on every painted frame. Reset removes the override and restores the component's
normal defaults. Replace/reset affects only the specified canonical target; it
cannot clear another part's style. Whole-style validation and allocation happen
before changing retained state. Multi-call edits are separate transactions; no
cross-call batch atomicity is promised. A future multi-target transaction needs
its own bounded descriptor and review.

The bridge retains copied scalar values only. Unmounted builder sequences are
copied immediately; mounted methods commit local state only after C succeeds.
Preset/CSS requests in Python normalize through the owning App's chosen library
at mount/update, preserving the existing deferred-provider contract. App lifetime,
stale generation, failed mount rollback and mutation errors follow current
facade rules. Node removal/subtree destruction frees part state; recycled slots
start absent. Item reorder preserves styles on item handles; removing/replacing
an item must not transfer styles by index or equal text/key alone.

Use stable bridge-private part identities associated with the owner's existing
stable `node->id`, never expose numeric salts to callers. The implementation must
choose and test one deterministic private domain-separated derivation, including
nonzero/full-ID distinction checks against group/item descriptors. Do not copy
Core's folded-ID hash to pretend to reserve identities globally. Private derived
IDs can still collide with other submitted/full/folded identities under the
existing Core collision boundary. Naming allocates explicit identity registration
capacity even though no extra layout box is added. This proposal adds no global
identity registry, reservation phase or input architecture.

Standalone Checkbox/Radio uses its new C `_with_parts` route **only while its
INDICATOR canonical slot is authored, including explicit empty**. Root-only
styles retain the old anonymous route. Reset of INDICATOR/MAIN restores the old
anonymous route on the next submission, even if a row ROOT override remains.
Do not submit old and named facades together, and do not replace custom glyph
painting with a second component or wrapper.

### RadioGroup all-descriptor boundary

The existing C named group API requires one nonzero descriptor per item, with
count equal to item count; partial zero descriptors are invalid. Therefore if
**any** child ITEM indicator override is authored, call the named group route
with captured stable IDs for **all** current items, while applying shadows only
to authored targets. Unstyled peers acquire explicit indicator identities in
that opted-in group submission; their layer defaults and pixels remain normal.
This identity/capacity change must be documented and tested, not called a
per-item anonymous-preservation guarantee. Group ROOT or item ROOT alone does not
activate named indicators. Resetting the last authored item indicator returns
the whole group to the legacy anonymous route next frame. Removing the last such
item has the same effect; adding/reordering items regenerates descriptors from
stable item identity before the single reducer call.

This is the recommended first compatible route with the existing C API. If review
requires unstyled peers to remain anonymous while another item is styled, stop
before production: that requires a separately approved additive partial-naming C
contract and tests. Do not pass zero descriptors to today's named API, mutate its
validation, silently reject mixed authored groups or repaint individual Radios.

## Submission, absence and error boundaries

Capture routing/descriptor choices before owner callbacks. Component defaults and
all complete owner paint run first; apply each authored override afterward using
the actual public getter or captured named descriptor. Use
`cui_frame_paint_enabled`, not the input-observation `cui_frame_sequence` guard.
Inert ancestry still paints decorations. Measurement/layout-only passes neither
apply styles nor report missing paint; retained values survive for real paint.

Distinguish four cases explicitly:

| Case | Required behavior |
| --- | --- |
| Declared unsupported role/owner path | Reject mutation before copying; no row fallback and no dormant-success fiction. |
| Supported owner not visited in this frame | Retain styles, do no Core lookup, no frame error merely for omission. No previous-frame presence required. |
| Reviewed conditional owner not admitted by its actual rendering branch | Retain styles; branch reports normal omission internally and does not attempt part styling. No entire-UI failure. |
| Owner has entered ordinary painting and its expected mandatory part is missing, collided, or rejected | Existing bridge CORE_ERROR/frame diagnostic; do not swallow Core INVALID_ACTION as ordinary absence. |

The first mapped control parts are mandatory once their owner successfully enters
ordinary paint. Unchecked Radio still has its circular indicator; empty label
still has indicator/track; Slider ratio0 omits fill, never track. An empty
RadioGroup has no ITEM targets to style, and may still have its group ROOT.
Detached/unvisited items are not absent items of a different group; handle/owner
validation prevents that ambiguity. An implementation should carry a small
stack-local submission outcome (not-attempted/omitted/submitted/failed) at the
owner branch, not add a retained style graph or infer admission from changed/valid
component return values. Group `result.valid` is input validation, not completed
layout; bool changed is activation, not paint admission.

Authoring allocation failure returns OUT_OF_MEMORY with previous styles and
local facade state unchanged. Descriptor/capability/style errors occur before
allocation. Exhausted allowed identity counters, if an implementation chooses
allocation rather than derivation, must return CAPACITY before committing any
identity/style state; that alternative requires its own exact rollback design.
Do not introduce that allocator implicitly while implementing this proposal.

Frame node/command/ID capacity or global/folded collision remains an ordinary
composition failure. Reduction/callbacks/accepted owner values may already have
occurred, exactly as documented by the C named-indicator API; no rollback of
input, callbacks or owner state is promised. Retained requested styles remain
available for recovery. Rejected authoring and late frame failure are different
atomicity boundaries. Preserve the first meaningful frame error and avoid
converting an earlier OUT_OF_MEMORY into a misleading later missing-target error.

## Storage budget and choice requiring review

The existing inline root style costs224 bytes on every node; its measured complete
Linux x86-64 node was73,128 bytes before this extension. Reuse that field for ROOT
on every admitted kind. Do not move or resize any public wire/options struct.
Nonroot roles are bounded: one indicator for Checkbox/Radio/RadioGroup ITEM, one
track for Slider, two independent track/thumb values for Switch. MAIN is an alias
and consumes no additional slot.

Recommended: one nullable private extension pointer per node, allocated only on
first nonroot set (including empty), freed after the last nonroot reset/removal.
A pointer contributes8 bytes/node on the audited64-bit ABI, including idle nodes;
32-bit cost/packing must be measured separately. A concrete payload can contain
uint32 presence/reserved (8 bytes) plus one or two224-byte V1 values:232 bytes
for indicator/Slider,456 for Switch, plus allocator overhead. ROOT stays inline.
No per-frame allocation or borrowed layer pointer is needed; descriptor arrays
for <=64 group items fit512 bytes of stack without retaining new item-ID buffers.
If a new persistent field is added for identity, revise these numbers before code.

At the current app maximum1024 nodes, new idle pointer overhead is8,192 bytes on
that ABI; all-Switch nonroot payloads are at most466,944 bytes excluding allocator
metadata. Existing root storage is229,376 bytes and is not a new part cost.
These are bounded estimates, not measured final `sizeof` claims. Styles on
unsupported roles cannot allocate more slots. Check size arithmetic before malloc
and test injected allocation failure on initial set and existing-target replace.

Alternatives to review:

| Choice | Idle overhead | Active cost/tradeoff |
| --- | --- | --- |
| Two extra inline styles + mask per node | At least452 bytes/node even with no part authoring | No allocation failure, but >=462,848 extra bytes at1024 nodes; wasteful for most kinds. |
| Recommended nullable extension | One pointer/node |232/456 payload per styled node, allocator overhead and explicit failure/free paths. |
| App-owned sparse array keyed by slot+generation+canonical role | Potentially zero new node bytes, plus app bookkeeping | Bounded search/growth/removal complexity and explicit capacity policy. Could share a small hard cap, but cannot silently evict another style. |

Do not introduce an unbounded map, inherited style graph or generalized resource
registry for this slice. The sparse-app alternative needs separately approved
capacity/lookup behavior before replacing the simpler pointer proposal. No new
option is appended to the old bridge options struct.

## Typed facade shape

Reuse existing `BoxShadow`, `CornerRadii`, `ShadowStyle`, typed presets and Core
factories. Add typed role selection rather than raw paint IDs or CSS strings.
Example spelling to review:

```text
Python/JS: control.part_shadows(ShadowPart.INDICATOR, layers, radii)
           control.part_shadow_preset(ShadowPart.MAIN, preset, radii)
           control.reset_part_shadows(ShadowPart.INDICATOR)
Go:        control.PartShadows(ShadowIndicator, style).ResetPartShadows(ShadowIndicator)
Rust:      app.part_box_shadows(control, ShadowPart::Indicator, &style)?
           app.reset_part_box_shadows(control, ShadowPart::Indicator)?
```

Explicit ROOT uses these new target methods; current `.box_shadows` methods keep
their exact six-kind behavior. `[]` is clear; reset is separate. Mounted runtime
capability always decides the actual owner route. Python/Go preserve optional
new-symbol loading. Rust error and app identity checks precede mutation; JS
retains its current failed-mount disposal behavior. None may commit local copied
state after a native failure. Unmounted ITEM requests may be stored before its
RadioGroup parent is assigned, but mount must validate ownership and roll back
with the facade's existing contract if placed under Tabs/List/other owners.

TypeScript should expose bounded per-constructor part interfaces rather than add
all roles to shared Label/Input/base interfaces. Checkbox/Radio allow ROOT/MAIN/
INDICATOR, Switch ROOT/MAIN/TRACK/THUMB, Slider ROOT/MAIN/TRACK. A RadioGroup item
builder can provide an owner-specific typed result; generic ITEM must still
validate at runtime and cannot promise every ITEM is an indicator. JS and TS are
one runtime proof plus a separate strict declaration test, not two independent
renderers. Keep the typed definitions in their narrow modules and revision-matched
skill resource exports; no bulky binding-wide context dump is required.

## Acceptance gate after review

No production or tests run for this document. Before implementing, approve the
exact descriptor/info signatures, role table, context whitelist, whole-group
naming boundary, pointer budget and failure semantics. Then require:

1. Portable real bridge capture for every table row and alias. Assert actual
   caster is square/circle/track/thumb rather than label row; independent ROOT
   and part lists, both Switch parts, mixed DROP/INSET, eight layers, movement,
   explicit radii, clipping and opacity. Slider THUMB/fill rejects. Original
   no-opt-in streams/pixels and old six-root setters remain unchanged.
2. Standalone Checkbox/Radio named route activates only with authored indicator,
   including empty; ROOT-only stays anonymous and reset restores legacy. Count
   Base requests/callbacks once. All state/disabled/empty-label variants preserve
   existing focus, semantics, glyphs and activation; introduce no new hit target.
3. RadioGroup uses its real owner painter once, styles distinct ITEMs independently,
   and retains selection and notification order. Cover mixed styled/unstyled peers,
   group/item ROOT, reorder, add/remove, last-reset legacy return,64 items, group
   descriptor copies, full/folded collision and ID-capacity costs. Other ITEM
   parents and Settings-owned field controls reject without allocating.
4. Copied caller arrays, replacement, alias last-write-wins, empty/NONE versus reset,
   stale handle/subtree destruction/slot reuse, invalid style and allocation
   failure atomicity. Measure actual node/payload sizes and active allocation
   balance; check no allocation or ID growth on each repeated frame.
5. Inert paint, measurement/layout-only work, omitted traversal and any admitted
   conditional callback route. Assert normal omission succeeds without styling
   stale geometry, while unexpected missing mandatory parts and capacity errors
   preserve the documented frame/error boundary and recover next frame.
6. All five facades: pre-mount and mounted ownership, unsupported owner mount
   rollback, native failure/local-state preservation, callback mutation under
   existing rules, float32 overflow/radii ordering, canonical factory parity and
   missing-symbol compatibility. Strict TS rejects nonexistent Slider thumb and
   generic compound claims. Wire size/offset tests cover every FFI.

Use the existing focused build and coordinated nice19, single-CPU/-j1 slot;
native executions only through test_local.py/private Xvfb and browser probes
headless. Fontless software may omit TEXT only with independent original text
command checks; it cannot establish glyph/GPU/browser parity. Update source
coverage, docs, examples, SDK/amalgamation and skill resources only when production
is approved and actual tests pass. Keep all78 manifest entries and the broader
beta shadow milestone pending. Portals, other compound parts, generic style
inheritance, Core presence registries and input architecture remain separate work.
