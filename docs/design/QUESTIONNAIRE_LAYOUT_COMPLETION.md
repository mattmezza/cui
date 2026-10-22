# Questionnaire typography, layout and compound completion

Design checkpoint only, 2026-10-22. No proposed declarations below are implemented.
Baseline CUI: `939c5c2f`; frozen reference:
`87d10ae5e1299d1be18670a37570c5402f952e34`, verified in
`/tmp/cui-gpui-reference`. This does not promote the Questionnaire row or change
any of the 78 entries. It supersedes the *remaining presentation scope* in the
older completion audit, not its independent behavioral/host limitations.

## Source contract and present coverage

Paths below are relative to the frozen checkout unless prefixed `CUI:`.
`crates/component/src/questionnaire/components.rs` is the owning skin source;
`website/component/questionnaire.md` documents the same compound composition.
General GPUI `Styled` is an open framework surface, not a finite Questionnaire
property list. The table distinguishes that from concrete skin behavior.

| Frozen evidence | CUI evidence at baseline | Finding |
| --- | --- | --- |
| `components.rs:27-60,85-177`: root publishes size by state; 128-entry bounded table; per-part override; separate root/item/choices/card/content metrics and custom `Size(value)` | `src/components/cui_questionnaire.c:126-238,418-574`: bounded state size cache, four legacy sizes and shared XS–9XL/density, independent choices gap | Substantial coverage. Arbitrary source custom size and independent geometry tokens are absent; shared scale is a CUI extension, not exact custom-size equivalence. |
| `components.rs:180-253`: answer/secondary/title/progress typography and first-line adornment centering | CUI metrics, choice paint at634-830, heading832-850, error851-880 | Answer size/line pairs match default tokens for four sizes. Description line boxes, progress/title tiers and explicit font refinement differ; no family-wide cascade. |
| `components.rs:297-364,527-678,1238-1290`: root/item/choices/actions accept children and instance styles | CUI begin/end containers and existing composable-container tests | Child composition already exists. Root/card/container box refinement is missing; actions lack source automatic trailing anchors. Root is Group, source Form: separate Core role decision, excluded here. |
| `components.rs:435-522`: independent Title/Description, schema fallback or replacement children; title adds item-gap bottom margin when schema description is absent | Combined `cui_questionnaire_heading` only | Real missing parts. Source text parts check schema existence, not current/disabled status themselves; active Item normally gates them. Preserve this distinction in new parts. |
| `components.rs:685-1020`: stateful card with indicator/content/shortcut, custom content and two renderer seams, three slot styles; instance style last after state paint | Existing choice renderers, masked box styles, state view, semantic identity and first-line wrappers | Already covers bounded slots. Missing card-instance refinement and default answer/description/shortcut typography. Source default shortcut uses theme mono family; CUI current default is medium weight without that theme-family resolution. |
| `components.rs:1024-1077`: standalone ChoiceDescription has children, secondary typography, optional size, no state | No named equivalent; caller can render ordinary text inside existing content callback | Missing convenience/explicit metrics part, not missing custom choice content. |
| `components.rs:1080-1146`: Input delegates to Input with size, padding, radius, invalid border then instance style | CUI input control945-1056 delegates editor reduction/paint; custom left padding and invalid state | Existing real editor path. Generic input skin refinement remains absent; cannot safely replace the editor with a child callback. |
| `components.rs:1150-1233`: Error appears only for invalid item with error, Alert wrapper, secondary text, top margin, custom children | CUI Alert/error labels/custom external messages, current-item gating, fixed default text | Missing custom error children/style and source spacing/typography opt-in. Do not silently change legacy gating. Source Error itself does not require current item. |
| `components.rs:1293-1408`: four action parts use real Button, visibility from navigation, custom children replace translated text, Skip or Next/Submit gets automatic leading margin | CUI action control882-928 uses real Button and legacy reducer; explicit/localized label override | Missing compound child and automatic trailing layout; not missing activation. Custom action content needs a real Button content seam, not overlay controls. |
| `components.rs:367-432`: localized numeric progress, optional replacement children, typography | `cui_questionnaire_progress.h`, production progress_ex, native/WASI/pixel/browser tests | Implemented bounded label/visible/custom child/text refinement. Do not repeat this work or call all localization absent. |

Frozen typography defaults live in `crates/base/src/theme_tokens.rs:157-211`:
xs12/16, sm14/20, md16/24, lg18/28, xl20/28, normal token weight.
Answer uses xs/sm/md/lg; secondary uses xs/xs/sm/md; title uses
sm/md/lg/xl with medium weight; progress uses xs/xs/xs/sm with medium weight.
In CUI Medium, for example, heading currently18/28 matches default title,
but choice description14/24 differs from source14/20; legacy progress12 with
natural line differs from explicit12/16. These are concrete opt-in corrections,
not reasons to change zero-option rendering. Source custom-size formulas are
answer=value, secondary=.875value, title=1.125value, progress=.75value,
answer-line=1.5value; source custom-size secondary/title/progress only set size,
so do not invent guaranteed line-height formulas for those elements.

Existing proofs include `tests/unit/test_questionnaire_styled.c` functions
`source_card_styles`, `custom_choice_slots`, `composable_containers`,
`choice_accessibility`, `inherited_sizes`, `slot_refinements`,
`localized_labels`, `invalid_options_preflight`, and ordered keyboard/input
regressions. `test_questionnaire_progress.c` proves independent legacy-zero
values/pixels, custom child/visible/label precedence and preflight; browser
progress and keyboard fixtures exercise actual WASM desktop/touch. Snapshots,
keyboard policy and persistent observer each have their own headers/tests.
None of these proves arbitrary root styles, missing named text parts, or custom
action children. Observer is real synchronous observation, not a style mechanism.

## Proposed cohesive portable surface

Add `include/cui/cui_questionnaire_presentation.h`; do not append legacy options
or state. Reuse `cui_questionnaire_text_refinement` from progress and the existing
slot mask vocabulary rather than duplicate text/style types. Keep progress_ex
independent; adding an outer-box variant can be a later additive overload.
The following are proposed contract shapes, not compilable declarations yet:

- `cui_questionnaire_presentation`: independent masked geometry/style record,
  with root/item/choices/actions/card/input box refinements and answer,
  secondary, title, description, shortcut and error text refinements. A separate
  explicit metrics-policy enum selects LEGACY or TOKEN_DEFAULTS. Zero means
  legacy, including known differences; TOKEN_DEFAULTS opts into the frozen
  default typography/spacing/radius and title-without-description separation.
  Do not use a single boolean claiming complete source styling.
- Geometry is per-part and does not publish a style cascade. Root continues
  publishing only existing size/density. The same presentation record may be
  passed explicitly to parts; omitted record does not inherit it. Independent
  text masks allow font family/weight, color, size and line box; existing slots
  remain independent. Resolve size first, optional token defaults second,
  existing slot refinements third, new explicit part refinement last. New card
  instance patches apply after built-in selected/invalid/hover/focus/disabled
  paint, matching source instance precedence; ancestor opacity still multiplies.
- Add `_with_presentation` counterparts for root/containers/choice/input/error/
  action, with an explicit keyboard pointer on root/choice where required.
  Legacy and current `_with_keyboard` paths share private implementations and
  pass NULL presentation. Avoid combinatorial public wrappers for every pair:
  the new entry accepts both independent pointers and documents each default.
- Add `cui_questionnaire_title_ex`, `...description_ex` with schema item index,
  independent presentation and optional borrowed text-content record; add
  `...choice_description_ex` with explicit borrowed content and size, no state.
  Existing heading remains untouched. Named item parts follow source schema
  existence checks; callers use Item begin to gate current/disabled items.
- Text-content record: explicit VISIBLE mask (including empty), optional child
  callback and user pointer. Child replaces visible fallback, while item/choice
  semantic names/descriptions stay schema-derived. Callback receives immutable
  resolved text/geometry/state view and stable child namespace. It runs once,
  synchronously, must balance children, must not mutate state, route input or
  end its owner. No retained function/user pointer. Strings live through frame
  draw/semantic consumption. Reuse the progress lifetime/ID-collision rules.
- Error replacement stays inside one existing Alert; label remains actual
  localized validation text even with decorative child. New explicit visibility
  policy LEGACY_CURRENT versus SCHEMA_ERROR permits source-like standalone error
  rendering; zero must keep current-item filtering. No validation or model events
  occur merely from submitting presentation parts.
- Actions need bounded custom **decorative** Button content, preserving the one
  Button semantic node and exact existing activation loop before/after content.
  Accessible name remains explicit/localized action label. A trailing-policy
  enum LEGACY versus AUTO places one grow spacer before visible Skip, otherwise
  Next/Finish. Per-action opt-out lets explicit layout win. Apply only inside the
  corresponding new Actions composition; no invisible spacers from omitted
  actions. This requires a predeclared composition plan or explicit caller
  spacer, not guessing which future children will be submitted.

Recommended bounded geometry mask: existing slot width/height/padding/gap/radius/
background/border/align/opacity plus explicit min-height and separate horizontal/
vertical padding where supported by current primitives. Values must affect the
same layout/hit rectangle that is painted. The source card has **minimum** height,
not fixed height: long localized labels must grow. Unsupported box properties
must be rejected, not accepted and ignored. Current `include/cui/cui.h:421-452` exposes physical padding edges with
`custom_padding`, so per-axis padding needs no Core extension. It exposes maximum
but not minimum dimensions or margins. Before production, prove minimum-height
and automatic-margin behavior with private balanced wrappers and account for
their bounds, or leave those properties as an explicit next slice. No silent
Core ABI work.

Do not add arbitrary GPUI callbacks, CSS values, a retained element tree or a
new theme/style cascade. Existing typed fonts and host font provisioning remain
authoritative. Full arbitrary Styled (absolute positioning, transforms, shadows,
pseudo-state layout, arbitrary inherited styles) remains a separately enumerated
framework gap; this bounded API must not claim it is implemented. An arbitrary
custom numeric scale is best expressed as a checked public metrics record in a
later metrics slice, not a second competing size enum alongside XS–9XL.

## Review boundaries and architectural decisions

Implement in reviewable order within the same presentation vocabulary:

1. Text refinement + independent named text parts and custom error content;
   default-token opt-in with explicit property tests. No Base changes.
2. Containers/card geometry and real input paint geometry; preserve input event
   reduction, IME, scroll/hit-test measurements and callbacks. No fake styled
   wrapper that changes paint only while leaving hit geometry unchanged.
3. Action custom content/trailing layout after a separate Button seam review.
   Existing `cui_button_styled_ex` is **paint-only refinement**, not custom child
   submission. Implementing action children by recreating button internals here
   would duplicate reducers and is rejected. A narrowly additive Button content
   API/private shared seam requires explicit ownership approval; no edits to
   `src/components/cui_components.c` are authorized by this checkpoint.

A single huge options record is an alternative, but independent records/pointers
avoid resizing legacy ABI and make omissions clear. A generic callback replacing
whole interactive parts is rejected: it would transfer accessibility/input duties
and fail to preserve source behavior. Caller-authored ordinary layout is already
valid and remains the fallback for properties awaiting implementation.

No new Core, state or binding ABI is required for slices1–2 **if** the geometry
feasibility check succeeds. Form role is separately unresolved. No new dispatch,
observer events or model ownership is proposed. Bindings do not automatically gain
presentation callbacks, records or font capability; canonical integration must
record omissions, matching headers/library, module/amalgamation ordering, compiled
usage and generated consumer resources. Root owns manifest/integration changes.

## Required evidence before implementation claims

- Independent old body versus new-zero submissions: compare semantic members,
  draw values, bounds and software pixels, never padding bytes. Cover all four
  legacy sizes, shared XS–9XL, root inheritance, explicit per-part override and
  standalone parts. Preserve omission/disabled gating and begin/end balance.
- Differential default token table for each text role, line box, weight, card
  minimum, gap and radius; custom family ID and metrics must reach both layout
  and paint. Multiline long labels, empty/schema fallback/custom content, missing
  description title gap, differing line heights and first-line adornment offsets.
- Active masks: explicit zeros/transparent/normal, ignored invalid unmasked
  values, unknown bits, NaN/infinity/range/UTF-8/NUL errors before callbacks,
  resources or event consumption. Descriptor snapshot before custom callback;
  borrowed bytes and nested independent roots have documented lifetimes.
- Actual pointer/key/AT sequences on styled choices/input/actions: same IDs,
  focus/checked/disabled/invalid/description and legacy callback ordering,
  multiplicity, synchronous callback mutation, delayed confirmation and observer
  payloads. Presentation alone emits nothing. Never replace reducer coverage with
  a paint screenshot or invoke model methods as a substitute for routed input.
- Native private Release wrapper, meaningful software pixels, WASI fixture with
  renderer fingerprint, C11 guide and C++ header compilation, sanitizer where
  practical; actual pinned formatting after final edits. Browser followup after
  canonical integration: desktop/touch, long localized copy, no overflow,
  custom decorative content with one semantic control, copyable compiled sample.

This checkpoint runs no production tests because it changes documentation only.
No production/API/status promotion is included; the compound/button seam and
geometry feasibility questions require review before those implementation slices.

## Approved first-slice contract checkpoint

The proposed declarations in `include/cui/cui_questionnaire_presentation.h` narrow
this implementation to independent named text parts and noninteractive Error
content. No container/card/input/action changes. Legacy zero named text metrics
are extracted from heading; separate wrappers do not reproduce heading's internal
gap automatically. Error zero preserves its existing current-item gate and Alert
name, including localized model error with decorative replacement children.
Source-like standalone error visibility remains deferred. TOKEN policy uses the
frozen four-size text table; shared design sizes retain CUI scaling explicitly.
Existing Alert composition is a single interact+box+text seam with no reducer;
it supports custom noninteractive content without an architectural change.
Production and evidence remain pending at this checkpoint.

Implementation checkpoint: the first slice now supplies the four named APIs in
`cui_questionnaire_presentation.h`, a separate Styled implementation and one
private legacy option/metrics seam. Legacy Error delegates zero options. It does
not implement the earlier proposed schema-error visibility policy, container/card/
input geometry, action children or trailing anchors. No full family promotion.
The worker fixture owns temporary source registration until canonical integration.
