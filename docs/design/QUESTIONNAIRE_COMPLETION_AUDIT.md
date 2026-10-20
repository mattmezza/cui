# Questionnaire completion audit and proposed slices

This is a design checkpoint, not an implemented API or a parity promotion.
It audits CUI at `becbe8e0` against frozen upstream commit
`87d10ae5e1299d1be18670a37570c5402f952e34` (the local reference checkout
`/tmp/cui-gpui-reference`). Current upstream main is not evidence. The existing
manifest's `implemented` label describes a substantial implementation; it does
not establish completion of the frozen family. All 78 entries remain unchanged.

## Source authority

Paths below are relative to the frozen checkout. Line numbers identify the
behavior under review; the commit and file contents remain authoritative.

| Source | Evidence |
| --- | --- |
| `crates/base/src/questionnaire/types.rs` | Definitions, answers and query records; validation context at 502; copied answer-change payload at 535; submission at 601; events at 623. |
| `crates/base/src/questionnaire/state.rs` | Retained state; shortcuts default absent at 121 and lookup at 343–348; validation/navigation; submit at 746–767; adjacent-answer focus at 894–975; answer emission at 1052; current-change emission at 1187. |
| `crates/base/src/questionnaire/keyboard.rs` | Held/default-prevented/IME guards, platform secondary modifier, answer navigation and confirmation. |
| `crates/base/src/questionnaire/control.rs` | Checkbox/radio selection, disabled policy, focus, labels and answer-position semantics. |
| `crates/component/src/questionnaire/components.rs` | Compound parts, card paint and size/theme tokens, custom children and style refinements, root capture, progress and action composition. |
| `website/component/questionnaire.md` | Consumer contract and examples; not a substitute for reducer evidence. |
| `crates/component-shell/src/shell/questionnaire/mod.rs` | Typed child composition, retained schema construction and fingerprint-driven rebuilding. |
| `crates/component-shell/tests/questionnaire_host.rs` | Host integration examples/tests. |

Content fingerprints (SHA-256):

| File | SHA-256 |
| --- | --- |
| Base `types.rs` | `c0c41591fc90752031ca7268e3543e1404f8639825ebe6ae1d23e4b26d45c392` |
| Base `state.rs` | `a8c231c29622172662a4191e25aef82173f3bf18e028cc46703464a04779e99b` |
| Base `keyboard.rs` | `394e08cb2edd8fe657a42fe46f77148c93e8af5667c2cbf97de9690de277dacd` |
| Base `control.rs` | `f16b4988bb9743449e8d301ca9db96ebb79a76a33a950c6b6d8d626c0911ea80` |
| Component `components.rs` | `9498f77cbe508fe823e1bed956c8fc9004f3a68c44e5c64b81a1d176c3382cb3` |
| Website guide | `4dd0da1253b532ce55600868bb1a10fbe74a3c91c70b435176957224401e3d28` |
| Shell `mod.rs` | `f0c07b188175ded1e3bb2a4923449a781b6b0a48770f428e9d08712a586ffab6` |

CUI evidence is `include/cui/cui_questionnaire.h`, Base and Styled
`src/{base,components}/cui_questionnaire.c`, `docs/QUESTIONNAIRE.md`,
`tests/unit/test_questionnaire.c`, `tests/unit/test_questionnaire_styled.c`,
`tests/web/test_browser.py` (Questionnaire section beginning around 2806), and
`include/cui/cui_bridge.h` / `src/bridge/cui_bridge.c`. No fresh runtime test was
executed for this documentation-only checkpoint; checked-in assertions below
are evidence of intended coverage, not a new passing run.

## Feature-level coverage

“Present” means implemented with identified tests, not unrestricted parity.
“Gap” identifies an observable missing capability or source difference.
“Unverified” requires targeted evidence before changing behavior or status.

| Feature | CUI implementation and existing proof | Remaining contract |
| --- | --- | --- |
| Schema and identity | Caller-owned item/choice definitions, duplicate-name/default/capacity validation; `schema_and_shortcuts`, `input_draft_reset_and_capacity`. | Present fixed-schema model. Host must not reorder/change borrowed identities during retained use. No source requirement for an arbitrary branching/schema-edit engine. |
| Field types | Single choice, multiple choice, optional retained text editor, mixed choices/text; disabled/default handling. Base and Styled tests exercise these. | Present source field categories. Date/file/upload/async validation are not frozen Questionnaire requirements. |
| Effective answers | Disabled choices/items excluded without destroying retained choices; single freeform replaces choices, multiple combines; Unicode blank text omitted. | Present, with retained-disabled/pending and draft/reset tests. Add independent source-vector differential fixtures. |
| Editor synchronization | Explicit `set_input`/`sync_input`; Styled input owns ordered editing and synchronization. | Source subscribes to its retained editor entity. Direct host edits to CUI editor require explicit synchronization; document and test this portable integration boundary. |
| Validation | Required/unfilled/custom/external errors, attempted-validation behavior, skipped/disabled suppression; first-invalid navigation. | Present core behavior. Callback-driven conditional policies and dynamic disabled anchors need broader tests; no async validator contract. |
| Navigation | Previous, next, skip, confirm, submit, enabled positions; skip clears effective answer while retaining draft. | Present, including completion order tests. Source query records are richer than CUI's individual accessors. |
| Delayed single-choice confirmation | New choice emits answer then schedules 150 ms confirmation, reduced motion immediate; choosing same answer confirms. | Present. Test live callback-driven policy changes before deferred tick and expired pending confirmation under disabled root. Root currently ticks even when capture is disabled; this is an explicit policy question, not a proven source regression. |
| Completion and events | Synchronous legacy callback; current/answer/completed/submit kinds and item indices. Completed once; successful resubmit emits submit again. | Gap: source emits copied names/answer/status/submission payloads; CUI exposes live state plus `{kind,item,previous}` only. Querying mutable state later is not snapshot equivalence. |
| Reset | Restores initial answers/editor drafts silently; retains runtime disable/external error policy. | Present bounded portable adaptation; assert callback silence and post-reset validation explicitly. |
| Keyboard guards | Root and input reducers handle ordered events, IME, repeat/modifier guards and focus ownership. `automatic_root_keyboard`, `batched_input_confirmation`. | Substantial coverage; not a blanket missing keyboard feature. Two concrete policy differences below need compatible opt-ins. |
| Shortcut configuration | Letters by default; numeric boolean; enabled-choice positions and badge rendering. | Gap: source defaults to no shortcuts and supports optional letters/numbers. CUI has no off mode. |
| Answer-entry focus | Arrow navigation among answers, including optional input; selected-answer restoration. | Difference: CUI restores selected answer from unknown/action focus; source restores it only from item focus. Existing `radio_keyboard` action-focus assertion encodes CUI behavior. Preserve it by default. |
| Submit modifier | Root recognizes Control or Meta plus Enter; explicit shortcut helper accepts host modifier. | Source uses platform secondary modifier. Root needs an explicit portable host policy if exact source behavior is requested. |
| Root and containers | Group root, current-item group, radio/multiple groups, actions row, focus restoration; `composable_containers`. | Gap: source root is Form; CUI core has no Form role. Cross-layer semantic role/mapping work is required. Nested-root capture isolation remains unverified in both implementations. |
| Choice accessibility | Checked/disabled/name/description, enabled positions, radio/checkbox roles, custom-slot semantics. | Present substantial semantics and browser assertions. Physical assistive-technology acceptance is not established by semantic snapshots. |
| Input and errors | Textbox required/invalid, Alert error, active-item submission gating. | Source standalone Error visibility differs from current-only CUI error; separately review explicit composition policy rather than silently altering legacy helpers. |
| Compound parts | Root/progress/item/choices/choice/input/error/actions and four actions; combined heading. | Gap: separate Title, Description and ChoiceDescription parts; source ParentElement custom children on parts/actions and independent styling. CUI heading is not an equivalent substitute for all compositions. |
| Choice content slots | Borrowed indicator/content/shortcut renderers; stable namespaces and masked box refinements. `custom_choice_slots`, `slot_refinements`. | Present bounded subset, not arbitrary Styled. Text/font inheritance and additional root/card/part style properties are missing. |
| Sizes and paint | Four legacy sizes, shared XS–9XL/density, inherited-size cache; theme/card/hover/focus/disabled tests. | Source token-driven typography/spacing/radius/custom size and general refinements need a property-by-property metric/style audit. Palette tests do not prove arbitrary themes or all source geometry. |
| Localization | Six masked action/required/unfilled labels, explicit empty override, custom validation text; `localized_labels`. | Gap: fixed English progress sentence. Separate title/description composition also limits localized custom content. No requirement to invent a translation service. |
| Retained ownership | Caller workspaces and disjoint answer/initial/draft buffers; bounded count and text capacity; borrowed definitions/editors/strings. | Deliberate C11 adaptation of source entities/owned strings. Needs explicit snapshot and callback lifetimes, not hidden allocations or borrowed-event retention claims. |
| Language bridges | Bridge has Questionnaire builder, state query, choice selection and navigation/submit; bounded 16-item/256-choice storage. | Existing bridge is not absent. It does not expose the full mixed-editor/validator/custom-part/snapshot surface. Last generic event fields are not source event payloads. Each binding needs explicit capability accounting. |

## Callback, ordering and lifetime boundaries

Frozen submit constructs one submission before emitting Completed with a clone
and Submit with that same content (`state.rs:746–767`). CUI's legacy callback can
query live state; it does not freeze an equivalent submission for both callbacks.
The callback receives a const state pointer, but a host may retain another mutable
pointer. Existing tests establish basic event order and copy input text into a
trace; they do not establish unrestricted reentrant mutation, teardown safety,
or immutable event retention. Do not change legacy timing to manufacture parity.

Definitions, editor pointers and workspace buffers outlive the state. Custom
renderers and style callbacks execute synchronously; borrowed strings must remain
valid for their documented submission/validation lifetime. Validators are pure
and must not mutate the model. A future observer must state whether callback
mutation is forbidden, observed live for later events, or supported through an
explicit queue; it must not silently introduce an event dispatcher.

Source constraints matter: one active question, synchronous validation, fixed
schema, host-owned persistence/transport and conditional disabled policy. The
shell can rebuild retained state when its typed schema fingerprint changes;
that is not arbitrary in-place schema mutation. Source framework ownership is
not a reason to expose platform/entity types in portable public APIs.

## Proposed independently reviewable production slices

All names below are proposed, not declarations available to consumers. Each
slice needs its own exact header contract and approval. Additive entry points
and zero-valued options preserve current CUI behavior and callback timing.

1. **Explicit keyboard policies.** New Questionnaire options header and `_ex`
   root/choice seams with typed shortcut mode `LEGACY`, `OFF`, `LETTERS`,
   `NUMBERS`; typed answer-entry policy `LEGACY` / `ITEM_FOCUS_ONLY`; and a
   portable submit-modifier policy supplied by the host. Invalid enum values
   fall back independently. Off suppresses both badges and activation. Inherited
   options and standalone part/helper behavior must agree. Keep the existing
   numeric boolean API unchanged. First add source-differential action-focus,
   item-focus, no-shortcut and modifier tests; investigate nested roots before
   promising a nearest-root isolation rule.
2. **Bounded answer/query snapshots.** Add checked copy-out records for answer,
   schema-ordered enabled submissions and navigation state. Caller supplies
   record/string workspace; report required counts/bytes with checked overflow,
   fail without partial valid output on insufficient capacity, and define NUL
   and empty-string rules. Snapshots must survive subsequent model edits while
   their caller buffers remain alive. A borrowed view alternative is smaller but
   cannot satisfy copied event retention; keep that distinction explicit.
3. **Typed observation with real payloads.** Separately design an observer/results
   facade using the snapshot primitives. Preserve legacy callback ordering and
   live eligibility; freeze the completed/submit pair before the first callback.
   Specify mutation/reentrancy and capacity-failure behavior before coding.
   Test conditional follow-up enablement, edits during notification, skipped and
   disabled entries, repeated submit and immutable retained snapshots. Do not
   label a wrapper that rereads live state as source payload parity.
4. **Compound presentation and localization.** Add separate title, description
   and choice-description parts; explicit content callbacks for actions/error/
   progress where source composition needs them; a borrowed pure progress
   formatter with bounded output/error rules. Preserve combined heading and
   English defaults. Reuse ordinary CUI composition and existing masked-style
   conventions; define whether standalone inactive parts render explicitly.
5. **Semantic and style completion.** Review Form role addition across core,
   native/browser adapters and bindings as a separate ownership request. Audit
   source root/card/part box and text properties into a finite presence-masked
   portable style/metrics record, including theme token defaults and explicit
   custom sizes. Reuse text primitives and preserve zero-style pixels. General
   source Styled remains unimplemented until its properties have concrete
   supported equivalents; it is not a permanent blanket exception.
6. **Host/binding acceptance.** Extend bridge schemas/queries/events only after
   ownership and bounded callback/editor lifetimes are settled. Record each
   language's supported subset. Add actual WASM gallery fixtures for the new
   policies, localized parts and copied payloads; generated source availability
   alone is not binding behavior proof.

These slices require Questionnaire-only Base/Styled private seams and new
headers/tests/guides initially. Form requires separately assigned core/adapter
ownership. Canonical CMake, modules/amalgamation, SDK/distribution, source metadata,
human usage mirror and generated consumer skill belong to a later integration
step. No such files change in this checkpoint.

## Proof plan and completion criteria

Existing Base tests cover disabled retained pending answers, validation/skip/
completion order, draft reset/capacity, schema/shortcuts and enabled positions.
Styled tests additionally cover cards, slots, progress, containers, accessibility,
size cache eviction/context isolation, ordered input, automatic root keyboard,
capacity failure, localized labels and invalid options. Preserve these tests.

The existing actual-WASM browser story already checks radiogroup positions and
descriptions, custom content semantics, delayed choice progression, required
alerts, mixed checkbox/freeform submission, event counters, letters/numbers,
focus and Unicode IME. It is not proof of later slot/localization APIs or of all
callback contracts. WASI already registers `questionnaire` and
`questionnaire_styled`; do not describe those suites as missing.

For each new slice, use strict portable C11 Release tests via
`python scripts/test_local.py --build-dir <external-private-build>` with dedicated
home-cache TMPDIR; never a desktop display. Exercise real queued pointer, key
and accessibility activations, multiplicity and ordering, previous-grid/part
identity where applicable, input composition, disabled transitions and reduced
motion. Add software-pixel assertions for actual hover/focus/disabled/style and
text/size containment, alongside semantic assertions. Include zero-option draw,
semantics and callback equivalence against legacy paths; avoid tests that only
mirror a resolver implementation.

Run the focused WASI suites and headless actual-browser fixtures after native
proof. Check bounded capacities, omissions/reintroduction, invalid options,
borrowed lifetimes and independently normalized policies. Run repository pinned
`format.py --check` after final source edits, plus freshness/consumer checks at
canonical integration. Physical AT and unavailable GPU hardware remain explicitly
unverified; do not substitute DOM/software assertions for those claims.

Completion requires feature-by-feature source reconciliation, meaningful tests
for newly supported behavior, real compiling consumer examples and honest
binding/platform limitations. Passing existing tests alone cannot promote the
family to full frozen parity. Production changes await review of this checkpoint.
