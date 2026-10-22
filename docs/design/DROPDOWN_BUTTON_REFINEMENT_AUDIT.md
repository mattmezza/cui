# DropdownButton inherited Button refinement audit

Initial design checkpoint, 2026-10-22. CUI base `ef086a41`; frozen v0.7.1 source
`87d10ae5e1299d1be18670a37570c5402f952e34`, verified in the local reference checkout.
That checkpoint made no production edits, status promotion, generated changes or
new dispatch design. All 78 entries remain. Its inspected tests and proposal are
preserved below; the separate implementation evidence follows at the end.

## Exact source and coverage

| Frozen source | CUI evidence / precise remaining scope |
| --- | --- |
| `crates/component/src/button/dropdown_button.rs:25-108,148-220` | Optional action/menu, outer variant/size precedence, selected union, action-only disabled/loading, corner anchor and joined borders exist in `include/cui/cui_choice.h:458-485`, `src/components/cui_choice.c:2360-2510`. Tests: `test_dropdown_inheritance_and_ghost_pair`, `test_dropdown_optional_halves`, `test_dropdown_anchor_corners`, `test_dropdown_inner_button_presentation` in `tests/unit/test_choice.c`. |
| `dropdown_button.rs:117-120,171-177` | Outer `Styled` refinement belongs to the **container**, not automatically both Buttons. Current container is a fixed row/gap0 with no patch. This is a real bounded box-style gap; arbitrary Styled remains larger than the proposed paint subset. |
| `dropdown_button.rs:179-220`; `button/button.rs:599-620,705-733,782-803` | Source retains the action Button's own instance/content refinements while overriding shared size/variant/corners/edges. Current split paint is independent (`cui_dropdown_button_box`, lines2246-2339), and cannot accept `cui_button_styles`. The existing Button facade (`include/cui/cui_button_styles.h`, `cui_components.c:426-474`) already defines a reusable masked paint vocabulary and caller-instance replay. |
| `button.rs:505-517,1168-1185,1276-1307` | Source selected/open suppresses pointer layers; trigger menu-open uses selected paint, while ghost action uses held group surface. Current split does not pass `overlay.open` as trigger active/selected paint; selected Ghost can retain hover foreground, and pressed Ghost can retain hover foreground. Dark Ghost hover is full accent rather than half alpha; held/group background should be half of the **resolved hover** background (dark quarter accent), not always half accent. These are concrete source differences, not missing tests alone. |
| `button.rs:1316-1359` | Disabled outlined handling has precedence across variants. Current split's special Ghost/Link/Text branch runs before outlined disabled handling. Colored disabled border uses `colors.border` instead of source semantic variant base color. Custom outline disabled background currently uses `.1*custom.color`; source normal outlined background then `.5` must be resolved from the source normal formula. Correction after tracing `theme/color.rs:228-280,1013-1038`: premultiplied mixing against transparent black/white preserves RGB and scales alpha. Current custom .2/.4/disabled .1 formulas are compatible apart from byte rounding; no color conversion is needed. Custom variant shadow (`button.rs:655-656,1089-1096`) is absent. These need source-differential expectations, including transparent-border themes, rather than copying existing assertions. |
| `button.rs:371-480,805-818`; `dropdown_button.rs:70-79,179-195` | Action semantic name is already an explicit C call argument (custom content can differ); default visible label and that name cannot independently differ without a custom child. Signed tab index, tab-stop eligibility and toggled semantic metadata exist on current `cui_button_options`, but are not forwarded by DropdownButton options. Selection remains paint-only; current Base explicitly resets trigger checked to false. Source string accessibility identity and arbitrary role override are not equivalent to CUI's integer owner ID or existing Button subset. |
| `button.rs:397-435,441-464,742-780,859-870` | Existing split has text leading/trailing/custom body, real caret SVG, stock spinner and delayed text tooltip. `test_dropdown_real_artwork` proves the spinner/caret, **not** general action typed artwork. Source compact/numeric sizing, rounding, font/icon overrides, icon-only action, custom loading icon, typed artwork, content-style, tooltip custom builder/keybinding hint and action hover callback are not fully forwarded. These remain distinct source-functional/content gaps; no catchall “metadata done” claim. |

Existing coverage is substantial: `test_dropdown_button`, `test_dropdown_colored_outline`,
`test_dropdown_presentation_variants`, `test_dropdown_configurable_palettes`,
`test_dropdown_loading_subtree_opacity`, `test_dropdown_loading_keeps_content`,
`test_dropdown_reference_metrics`, `test_dropdown_action_tooltip` and shared-size
checks in `tests/unit/test_choice.c`. `tests/unit/test_dropdown_events.c` exercises
pointer/touch/key/AT action metadata, multiplicity, missing handlers and loading
suppression. It establishes an existing typed facade, which a style extension must
reuse. It does not establish the missing Button style/metadata cases above.

## Smallest compatible next slice (proposed, not implemented)

Use an independent additive record and `_refined`/`_events_refined` entries beside
DropdownButton declarations; do not resize `cui_dropdown_button_options`. Include
the existing Button styles header, rather than invent a second paint mask or
callback signature. Proposed shape:

```c
/* Proposed declarations only. */
typedef enum {
    CUI_DROPDOWN_PAINT_LEGACY,
    CUI_DROPDOWN_PAINT_BUTTON
} cui_dropdown_paint_policy;
typedef struct {
    cui_dropdown_paint_policy paint_policy;
    cui_button_styles action, trigger;
    cui_button_paint_patch container;
} cui_dropdown_button_refinement;
```

`container` uses the existing paint-only masks: background/border color,
foreground, radius/corners and opacity. Because the container has no text, its
foreground bit should be **rejected**, not accepted ineffectively. Border width,
geometry/gap/padding/clip remain inspection-only and unchanged; this first slice
must not pretend to implement the source's arbitrary outer Styled geometry.
The outer container still has no semantic/control role. Container paint is not
an inherited text cascade into either half.

Recommended compatibility policy: zero/NULL takes the exact existing split paint
path. Explicit BUTTON selects source-correct built-in layers above even with no
callbacks. LEGACY plus nonzero callbacks layers refinements over the legacy
palette; it cannot be called complete source state-style parity. This explicit
policy avoids silently changing old pixels when a caller adopts only metadata.
An alternative is to correct old defaults directly as a separately reviewed bug
fix. Root should choose that migration before production; both cannot preserve
old pixels and simultaneously change the mismatching source formulas.

Action refinements cover retained inner-Button customization. Independent trigger
callbacks are an explicit CUI extension: the frozen builder creates that Button
internally and does not expose a separate trigger-style setter.

For each visible half, reuse `cui_button_styles`' phase vocabulary, pure borrowed
callback, patch validity and instance-once captured replay contract. Normal,
instance, eligible pointer layer, selected/open layer+instance replay, disabled
layer+instance replay, loading fade, then eligible focus follow the documented
Component ordering. Press suppresses hover; selected/open suppress both;
loading suppresses pointer phases without pretending to be disabled paint.
Action effective selected is outer OR inner selected; trigger additionally uses
real `overlay.open`. Action menu-open is a ghost group-held surface, not fake
selected/checked state. Snapshot ID is each real half ID. Hidden halves invoke no
callbacks; old content callback still runs once in its original place.

Implementation can remain entirely in DropdownButton's common renderer: retain
the existing Base reduction, calculate the local half paint, apply the existing
masked value vocabulary through a small Dropdown-specific adapter, then submit
the existing layout/content tree. No second control, event loop or generic style
dispatcher. Button's resolver is currently private; copying only the bounded
patch application/phase adapter is preferable to changing Button internals within
this ownership. If later deduplicated, extracting a value-only private helper
needs explicit Button ownership and its own equivalence proof.

Do **not** call `cui_button_styled_ex`: it would reduce events twice. Simply
replacing each half with `cui_button_styled_paint` is also not a proven shortcut:
its padding/label/caret/clip tree and ghost group-held behavior differ from the
existing split tree. That path would need an independently reviewed layout and
content migration, not an assertion that shared callback types imply equivalence.

Unknown option policy/container mask rejects before `state->drag_scroll`, input,
handlers or paint. Invalid callback output discards that whole paint patch, as
Button already specifies; it does not undo an accepted activation. Functions,
user and current/info/out records are synchronous borrowed values; callbacks
must not call CUI, mutate owner/options, consume input or reenter submission.
Descriptors are copied at entry; instance runs once per submitted visible half,
its validated value is replayed without rerunning it. No retained attachment.
Focus styling must explicitly use existing focus-visible/hide-focus-ring gates
under BUTTON; current split colors every focused half, another legacy difference.

## Independently reviewable metadata followup

Add an independent masked action/trigger metadata record, not an unrestricted
`cui_button_options` pass-through. First useful fields: accessible-name override
(valid UTF-8, explicit empty allowed), signed tab index, independent tab-stop
eligibility and optional action toggled value. Reuse Core setters/old Button
conventions without creating a new role, changing selected to checked, or clearing
menu expanded/haspopup/busy/descriptions. Name override is passed to Base for
semantics while the original label still paints. Metadata must be preflighted
before Base; callbacks/accepted activation evidence remain unchanged. Dynamic
focus policy must inherit existing Core timing rather than inventing rerouting.
String accessibility-ID aliases and arbitrary source roles remain explicit gaps;
CUI integer IDs are not a fabricated string identity facade. Broader input/hover
or tooltip-builder lifecycle forwarding needs separate storage/lifetime review.

## Required proof before any coverage promotion

- Independent pre-change legacy/zero draw, software-pixel and memberwise semantic
  oracle across legacy/shared sizes, variants, outlined/selected/disabled/loading,
  optional halves and custom body; preserve all accepted typed event counts/order.
- Source formula matrix for light/dark Ghost hover/group-held/open, selected+
  pressed+hover foreground suppression, open trigger versus held action, disabled
  outline/custom/transparent-border themes, loading fade and focus-visible gates.
- Record callback trace and snapshot values: instance exactly once per visible
  half, press/hover suppression, selected/disabled replay, focus ordering, invalid
  patch discard, no geometry writes; actual software colors/radii/opacity and
  existing joined edges/caret/content bounds must match those resolved values.
- Real pointer/touch/Enter/Space/AT and menu command queues through legacy and
  typed `_events_refined` entries: no duplicate dispatch, callback replacement
  and live disabled/loading mutation preserve current reducer behavior. Distinct
  handler.user versus style.user; no fabricated origin/count metadata.
- Metadata followup: visible versus accessible labels, explicit empty names,
  signed order/skip eligibility, toggled versus selected/expanded/busy and popup
  focus restoration. Invalid descriptors do not consume queued events or mutate
  menu/output. Test borrowed descriptor snapshot and no retained callbacks.
- Strict portable C11, C++ declarations/compiled guide, private Release wrapper,
  WASI and software pixels; browser demonstration follows canonical review.
  No build or native window was necessary for this read-only checkpoint.

## Final declaration checkpoint

The additive contract is now in `include/cui/cui_dropdown_button_styles.h`. It avoids the Choice/Components include cycle. Explicit BUTTON selects source state formulas; two gates select Custom XS shadow; zero stays legacy. Metadata is separate. Source `Colorize::opacity` (`theme/color.rs:156-161`) clamps its multiplier to[0,1], so Secondary disabled .opacity(1.5) preserves alpha rather than increasing it. Tests must cover translucent colors and exact byte rounding. No Theme/Core ABI or new conversion dependency is required.

## Implemented style slice and evidence

The declarations are implemented through the existing Dropdown common submission
path. Base/events reduce once, descriptors copy before reduction, and legacy
callbacks retain their mutation/timing behavior. The metadata proposal above is
still unimplemented. Canonical registration, generated distributions, consumer
skill export and browser demonstration remain separate integration work; no
manifest status is promoted here.

`tests/fixtures/dropdown_button_styles/legacy_box.h` freezes the pre-change
`ef086a41` paint helper and complete submission/paint tree. The new test compares
that oracle with NULL/zero descriptors across every variant, all four existing
sizes and normal/selected/disabled/loading/outlined/omitted-action cases, using
draw values, actual software pixels and memberwise semantic values. Additional
tests verify independently calculated source colors (including translucent
Secondary and distinct border palettes), instance replay/invalid patch discard,
container rejection, real pointer/touch/Enter/Space/accessibility multiplicity,
descriptor-copy behavior under legacy event-handler mutation, body-once and
trigger-open distinction. Actual software pixels exercise hover/selection and
the two shadow gates. Text shaping and source theme-token equality are not
claimed by these software tests.

Frozen `gpui-pre = 0.3.8` runtime evidence (`src/elements/div.rs:3438-3473,3520-3523`)
applies group-hover before own hover/active. Thus an action's own pointer state
overrides its group-held background; the trigger does not inherit that group.

Verified in the private fixture: strict Release native wrapper 3/3 (new styles,
existing Choice and Dropdown events), ASan+UBSan wrapper 1/1, pinned Zig 0.16.0 /
wasmtime 49.0.0 WASI 1/1, strict compiled C11 guide and C++ header, and actual
`python scripts/format.py --check` with clang-format 23.1.2. All builds live under
`/home/matteo/.cache/cui-validation/dropdown-button-styles{,-asan,-wasi}`;
no desktop window, dependency install or full-suite rebuild was used.
