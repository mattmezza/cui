# Explicit named Checkbox and Radio indicators: review checkpoint

Status: proposal only, not implemented. Audit follows first ownership slice
94654394. Existing Checkbox/Radio indicators remain anonymous. Their row IDs and
the generic element-shadow API do not establish indicator coverage. The beta
all78-parts and language-authoring milestones remain pending.

## Current entry points and timing

`src/components/cui_components.c` owns Checkbox_ex, source, boolean convenience
and Checkbox_events. Checkbox_ex validates ordinary options, invokes exactly one
Base check reducer (legacy or extended), updates semantics/tab stop, computes
motion/defaults, invokes ordered style refinements, then opens the row/anonymous
column/anonymous indicator. Custom indicator callback replaces glyph painting
inside that indicator box; label content follows it, and optional tooltip follows
the row. The source facade sets source_behavior and delegates ex. The boolean
facade projects bool through a local check-state value. The event facade captures
its value/handler in stack forwarding storage and delegates source once; its
valid/submitted fields do not prove completed paint.

Checkbox indicator identity therefore must be selected before the existing
indicator begin, without moving Base reduction or style/content callbacks.
`docs/CHECKBOX_BEHAVIOR.md:86–103` already confines style callbacks to refinement
and permits balanced custom layout in indicator/content callbacks. The Base
`cui_base_check_indicator` already accepts a caller ID for applications composing
their own noninteractive primitive, but replacing Styled's glyph/spring painter
with it would risk changing pixels and is not this proposal.

Radio_ex independently invokes Base radio once and paints a row plus anonymous
indicator; Radio convenience delegates ex. Radio options alias Checkbox options
but ignore Checkbox-specific extensions. There is no current single-Radio event
facade to imply support for. `src/components/cui_radio_group.c` has a separate
renderer: it invokes Base group or Base group-events once, then directly paints
item rows/anonymous indicators for up to CUI_RADIO_GROUP_MAX64 items. Group-events
preserves legacy owner request before its observer. It must not be implemented by
calling the individual interactive Radio facade for each item.

## Proposed additive public surface

Use one small value descriptor in cui_components.h; no existing options/state
structure grows and no descriptor contains platform types or borrowed pointers:

```c
typedef struct { cui_id indicator; } cui_selection_indicator_parts;

bool cui_checkbox_ex_with_parts(cui_context *, cui_id, const cui_theme *, cui_str,
    cui_check_state *, cui_checkbox_options, cui_selection_indicator_parts);
bool cui_checkbox_source_with_parts(cui_context *, cui_id, const cui_theme *, cui_str,
    cui_check_state *, cui_checkbox_options, cui_selection_indicator_parts);
bool cui_checkbox_with_parts(cui_context *, cui_id, const cui_theme *, cui_str,
    bool *, bool disabled, cui_selection_indicator_parts);
cui_component_action_result cui_checkbox_events_with_parts(cui_context *, cui_id,
    const cui_theme *, cui_str, cui_check_state, cui_checkbox_options,
    cui_selection_indicator_parts, cui_component_check_handler);

bool cui_radio_ex_with_parts(cui_context *, cui_id, const cui_theme *, cui_str,
    int *, int option, cui_radio_options, cui_selection_indicator_parts);
bool cui_radio_with_parts(cui_context *, cui_id, const cui_theme *, cui_str,
    int *, int option, bool disabled, cui_selection_indicator_parts);
```

Both IDs must be nonzero and indicator must differ from row ID. New entry points
require explicit indicator identity; descriptor0 is invalid, not an implicit
request to change anonymous defaults. Old functions keep their anonymous route.
No pure getter advertises a node that old functions never emit. Applications can
retain a stable indicator ID alongside their row ID and apply copied shadows to
it after submission. Bool return keeps existing changed semantics, not validity;
invalid local parts report INVALID_ACTION and return false. Event result.valid
is false on local descriptor rejection; later capacity/layout failure remains a
frame diagnostic rather than an invented changed/result interpretation.

Group additions belong in cui_radio_group.h, preserving existing result types:

```c
cui_radio_group_result cui_radio_group_with_parts(cui_context *, cui_id,
    const cui_theme *, const cui_radio_group_item *, size_t count, cui_id *selected,
    cui_radio_group_state *, cui_radio_group_options,
    const cui_selection_indicator_parts *, size_t part_count);
cui_radio_group_event_result cui_radio_group_events_with_parts(cui_context *, cui_id,
    const cui_theme *, const cui_radio_group_item *, size_t count, cui_id *selected,
    cui_radio_group_state *, cui_radio_group_options,
    const cui_selection_indicator_parts *, size_t part_count, cui_selection_handler);
```

part_count must equal count<=64; a zero-count group allows NULL parts/zero count
under its existing empty-group rules. Item and part identities are stable by item
identity, not array index when reordering. Preflight rejects full-ID duplicates
across group root, all item rows and all indicator parts before reduction. Copy
the <=64 part descriptors into bounded stack storage before invoking any owner
callback. Caller storage may then change after return; no pointer enters paint.
The item array retains its existing borrowed/no-mutation callback contract.

## Collision and failure boundary requiring explicit approval

Local structural validation (nonzero, root/part distinct, group cardinality and
pairwise full-ID uniqueness) can occur before Base and submit no semantics,
callbacks, layout or value writes on rejection. Numeric/style/theme validation
retains each existing facade's rules; do not expand legacy behavior accidentally.

There is no public current-frame layout identity query/reservation API. Semantic
queries omit passive boxes; cui_bounds may be previous-frame. A component must
not duplicate Core's private folded-ID hash to pretend it checked collisions.
Thus collision with already submitted application boxes, internal conditional
parts/tooltip IDs, or distinct IDs sharing a folded hash is caught by existing
Core begin_box registration at paint submission. At that point Base and owner
callbacks may already have run, as with existing composition/capacity failures.
This proposal does NOT promise rollback of input, owner callbacks or state for
such late failures. Do not use a known stale bounds/semantic query as preflight.

Recommended first contract: retain that existing failure boundary, document it,
and test it precisely. Local descriptor rejection is early/atomic; a global/folded
collision is a frame diagnostic that suppresses ambiguous element styling through
the existing Core guard. If acceptance requires no reduction/callback for every
global collision, stop implementation and separately review a Core batch identity
reservation design. A read-only query alone cannot prevent later callback/content
composition from taking an ID. No new Core query, reservation or input dispatch
architecture is approved or implemented by this checkpoint.

Callbacks may accept/reject the requested owner value exactly as today. Do not
mutate/free active state, options' borrowed arrays/strings/resources, theme or item
arrays, destroy the context, or recursively submit the same owner. Style callbacks
remain refinement-only. Descriptor values are captured before callbacks, so
changing external caller descriptor storage cannot redirect this submission.
Custom indicator/content callbacks may compose balanced child UI under existing
rules, but must not reuse row/indicator IDs or change owner geometry after the
owner has been captured. No new callback or dispatch sequence is introduced.

## Internal painter change

Extract existing Checkbox_ex body to one private renderer accepting a captured
indicator ID argument (0 for all old calls). Change only the existing indicator
`cui_begin_box(ui, 0, paint.indicator)` to use that argument. Keep column/spacer,
glyphs, spring state, styles, semantic owner, focus, label, content and tooltip in
place. Old ex/source/bool/events preserve their exact chain and reduce once;
new wrappers preflight/capture parts and reach the same renderer once. Events
forwarding must route to the named source branch without reducing a second time.

Apply the same parameterization to Radio_ex's private painter; keep ignored
Checkbox-only extension fields ignored. Parameterize the separate group renderer
with a captured optional part array; old group functions pass no array and retain
all anonymous IDs. New group functions select each copied item indicator ID but
still execute the single group Base/event call. Do not synthesize extra wrapper
boxes, interactions, semantics, focus stops or pointer hitboxes for naming.

New explicit IDs occupy named-ID registration slots and can alter descendant
Clay auto IDs in the opted-in route. Node count need not change, but naming is
not free identity capacity and is not promised to be byte-identical internally.
Legacy routes must retain exact IDs, capacity boundaries, command streams and
pixels. New route budgets, registration failures and new collision opportunities
are explicit tests, not silently inherited parity assumptions.

## Shape, clip and shadow contract

Named indicators remain ordinary paint boxes. Shadow lists are copied by the
existing element setter after composition; no list or descriptor is retained by
the component. Checkbox style refinement can change indicator radius/corners and
size; Radio uses its existing size/2 circle. Callers supply matching radii when
applying shadows. Shadow does not infer circle shape or a custom glyph alpha mask.
DROP/INSET follow current caster/radii, ancestor masks and opacity, with existing
phase ordering and local child-overflow rules. Label-row bounds and indicator
bounds differ; tests must assert the actual caster is the indicator. Disabled,
checked/mixed and invalid states do not create a second semantic control. Custom
indicator content can still be independently clipped by its own descendants.

## Acceptance and future bridge mapping

Prove old ex/source/bool/events Checkbox routes and individual/group/events Radio
routes unchanged with command/pixel, capacity/identity and genuine queued input
regressions. Named tests cover all check states, enabled/disabled/invalid, bounded
labels, design sizes, spring and custom indicator/style callbacks, content order,
source pointer-focus policy and event owner/observer ordering. Explicitly count
Base requests/callbacks once. Local invalid descriptor/cardinality/duplicate tests
must show no reduction or semantic submission; late full/folded collision tests
must assert the documented composition-failure boundary without claiming rollback.

Also cover stable reordering, omitted owners, copied descriptors surviving callback
storage mutation, current-layout move/resize, actual DROP/INSET software pixels,
matching circular/custom radii, clipping/opacity, no extra focus/input/semantic
records, command/node/ID capacity failures and recovery. Browser headless probes
and language examples follow implementation; none run for this proposal.

Current bridge paints Checkbox/Radio through boolean convenience facades in
`src/bridge/cui_bridge.c:4636–4651`; RadioGroup uses its own retained group path.
Future MAIN_PAINT_PART maps to the explicitly named indicator for these families,
while an explicit ROOT target remains the full row. The bridge owns stable
per-node/per-item IDs and copied <=8-layer style values; no frontend derives
private salts. Only nodes opting into indicator styling use named composition;
unset style preserves legacy anonymous paths. Explicit empty style still requires
a named route so it can clear component defaults without meaning unset. Hidden,
deleted/reused and reordered retained nodes obey bridge generation/lifetime rules.
Never map MAIN silently to a label row because its indicator is still anonymous.
All five typed language facades need the same declared target capability and
copy/error/clear semantics. That rollout and the remaining78-family ownership
matrix stay pending, rather than marking Checkbox/Radio or shadow authoring full.
