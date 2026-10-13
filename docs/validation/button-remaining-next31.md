# Remaining Button contracts after automatic label layout

Audit target: merged CUI `f9e4c1f`, with automatic label commit `7183636` integrated. Reference: local GPUI component v0.7.1, commit `87d10ae5e1299d1be18670a37570c5402f952e34`, verified with `git rev-parse HEAD`. This is a read-only delta from `button-completion-audit-next28.md/json` and `button-group-proposal-next29.md/json`, not a new family certification. Source paths below are relative to that pinned repository.

## Closed bounded slice: automatic fixed/grow label layout

The formerly failing fixed-80 and grow-parent-80 probes are historical evidence,
not current defects. The merged implementation uses current-frame label bounds,
owned prepaint header/string copies, a clipped content container and automatic
no-wrap ellipsis. The full original semantic name is copied separately. Explicit
`label_width` and optional caller state remain compatible; `allow_content_overflow`
is a named legacy opt-out. FIT sizing is retained. Arbitrary child callbacks own
their internal layout; a content clip is not proof that all arbitrary nested
content inherits a no-wrap text policy.

`test_button_label_layout.c` covers all 18 size choices and both densities, first
frame grow, immediate width changes, artwork/caret/custom slots, full names,
grapheme prefixes, copied temporary label bytes, invalid shapes and arena
failure/recovery. `test_button_label_font.c` repeats it with actual native font
measurement. The independent WASM fixture verifies visible ellipsis glyph pixels,
clip containment and actual mouse/CDP touch activation. The optional 256-byte
state buffer caps only its diagnostic visible prefix, not the full semantic name
or state-free label. Full semantic bytes and the deferred label tail each use
bounded frame storage, so available frame capacity, alignment and other submitted
strings determine the actual maximum. Capacity errors produce a failed frame;
accepted callbacks already processed are not rolled back. No new heap allocation,
VLA or arbitrary full-label limit was introduced.

The label prepaint callback avoids state-buffer writes under inert ancestry and
layout-only paint is omitted. This is narrower than whole Button lifecycle
immutability: the independent passive-state probe below exposes a remaining
metadata/tooltip issue. Core clip-only leakage was separately corrected by
`eaa5e03`; passive Button lifecycle is not claimed complete from that paint fix.

## Exact remaining contracts

| Contract and pinned source | Current CUI mapping and remaining limit |
| --- | --- |
| Automatic content clip/nowrap/ellipsis, Button `737-766` | Implemented and tested for fixed/grow labels in `f9e4c1f`; full names and explicit opt-outs retained. Arbitrary custom-child text layout remains caller-owned, with shared content clipping; do not certify arbitrary inherited CSS-like whitespace refinement. |
| Public `Styled` refinement and Base semantic state styles, Base Button `135-181`, Components Button `580-584,611-613,722-723,792-804` | Public Core composition and Button custom colors/geometry/slots cover parts; there is no arbitrary Button root style refinement replayed after each selected/disabled state. Do not equate a palette override with the full style contract. |
| Gradient-backed theme surfaces and Oklab transparent mixing, Button `899-988,1100-1273`, tests `1812-1868` | Button palettes hold RGBA colors, not source Background gradients. Alpha multiplication and supplied outline colors are not Oklab interpolation. This needs a separate paint/theme contract, not another variant enum. |
| `ButtonCustomVariant.shadow(bool)`, Button `138-141,650-656,1093-1099` | Core now supports bounded value-owned drop shadows, but Button has no shadow option and theme has no equivalent global shadow enable flag. Only Custom requests shadow in the pinned Button: built-in variants do **not** require invented shadows. Loading opacity applies to the entire shadow/button; disabled suppresses shadow. GPUI `shadow_xs` is one black alpha .05 layer, offset `(0,1)`, `blur_radius=2`, from pinned dependency macro `gpui-pre-macros-0.3.8/src/styles.rs:416-424`; use the already audited blur conversion rather than guessing its sigma semantics. |
| Focus painting, Button `625-631,879-880`, Components `styled.rs:235-333` | CUI paints a two-pixel border in theme focus color whenever focus-visible and not hidden. Source paints for its focused handle (not specifically keyboard focus-visible) and distinguishes outside three-pixel alpha-.5 ring, border tint without layout changes, and borderless Edge/Inside/Outside fallback lines when theme ring is off. Source filled fallback uses foreground alpha .6; outside/inside gap is .125 rem. Theme `hide_focus_ring` and Core current-frame outline/prepaint support exist, but the Button implementation does not use them for this exact policy. Also source borders are limited to Default or outline (`Button:700-703`), whereas CUI non-borderless variants generally use a one-pixel border. |
| Ordered signed `tab_index`, Base `109-115`, styled `472-483` | Core public tab-stop API sets only 0/-1 and traversal follows semantic order. `skip_tab` is not signed indexed ordering. Full ordering would require an explicitly scoped Core change; it is not Button-only work. |
| Public role override and explicit focusability, Base `88-128`, styled `326-329` | CUI derives Button/Link and permits Core semantic refinement; no Button role option or Base `focusable(false)` equivalent option. Late role refinement is not proof that keyboard/accessibility activation preserves the source behavior under every overridden role. Stable Core IDs replace GPUI focus handles without importing platform types. |
| Native ClickEvent payload and event-local propagation, Base `99-105,248-252`, styled `447-453` | Standalone callbacks now run once per accepted event; Group requests expose event index and stable target. Standalone callback remains `void(user)` and does not receive pointer/key kind, modifiers or event index. Core frame events and consumption are available for composition, but are not a callback payload/propagation contract. No new bubbling claim follows from accepted target records. |
| `Selectable::open`, styled `500-507,553-559` | Caller can OR selected with popup-open presentation; there is no independent `open` field. Pinned open is presentation separate from selected/toggled, not an intrinsic menu implementation or automatic expanded metadata requirement. Treat this as a missing direct API convenience/state distinction, not as an absent menu subsystem. |
| `tooltip_with_action` and action-context keybinding, styled `418-431,863-873` | String tooltip, placement, delay/cancel/focus and borrowed state are implemented and tested. Button does not resolve application actions/context into keybinding presentation. A portable application resolver/content contract is needed, not GPUI Action types. |
| ButtonIcon typed Icon/Spinner/Progress slot, `button/button_icon.rs:55-110,117-130` | Named/data-backed icons, default animated spinner and named loading-glyph rotation are present; `leading_content` can explicitly compose Spinner/Progress. No typed ButtonIcon adapter preserves a supplied Spinner/Progress through loading automatically. Source creates a spinner only when an icon exists; CUI's loading-only button spinner is an intentional extension. Reduced-motion rotation has headless evidence; full typed-slot preservation is not certified. |
| Caret presentation, styled `780-783` | CUI supplies a real named chevron and distribution spacer, but uses current foreground unchanged. Source caret uses normal foreground opacity .75, even when label state changes. Exact caret tint and compound-held hover are not covered merely by SVG command count. |
| Arbitrary numeric `Size::Size`, styled `616-619,663-670` | Explicit height/font/icon/width overrides and four legacy plus fourteen design sizes are tested. They do not automatically apply the source single-pixel-size rule (icon .75 times size, normal horizontal padding .2 times size). This is an API mapping limit, not a missing fourteen-size implementation. |

## Source distinctions that must not become invented requirements

* `tooltip_builder` is a private stored field (`Button:223,270,859`) with no public setter in this pinned file. Its unreachable public-builder branch is not a public Button requirement; older broad documentation listing arbitrary builders as missing should not be interpreted as such.
* `hover_group` and `hover_group_held` are `pub(crate)` compound-control hooks (`Button:358-367,719-726`). Standalone Button and the pinned ButtonGroup do not publicly expose them. Split/Dropdown behavior needs a separate compound-control audit; do not add an unrelated public group-hover engine to finish ButtonGroup.
* Source standalone normal buttons use default cursor, with pointer cursor only for interactive Link/Text (`Button:643-653`); CUI role-default pointer cursor is an intentional existing host convention, with cursor-only override available. Source mouse-down avoids pointer focus (`827-841`); CUI keeps legacy zero behavior and provides tested `prevent_pointer_focus`. These differences are configurable compatibility choices, not newly missing capture fixes.
* Source defaults to Default variant; CUI preserves legacy Primary for zero options. Source Link fits natural content; CUI preserves legacy Link height while Text fits. Source has no touch implementation to copy; CUI's touch pan/cancel/ghost suppression is a user/platform extension and needs its own evidence.
* Named loading icons and default spinner already animate; no independent Button hover/press spring is required by the pinned Button render chain. Button animation requirements come from its spinner/progress content, not an invented transition engine.
* Caller-owned custom RGBA/hotspot pointers remain a separate user/platform request; named cursor hooks alone do not certify that extension.
* Loading/disabled barriers, accepted-event ordering, callback multiplicity and joined group geometry are implemented work. Do not reopen them merely because earlier audits preceded their commits.

## Implemented work retained, with bounded evidence

Standalone: twelve variant names, solid normal/hover/active palettes, colored outlines, selected versus explicit toggle semantics, four legacy/fourteen design metrics, compact/radius/corner/edge overrides, icon/caret/leading/trailing/custom content, full accessible name under explicit ellipsis, controlled hover/tooltip state, loading busy/inert/whole-opacity/animated glyph, focus opt-out/custom cursor, accepted-event callbacks and cancellation. Relevant tests are `test_button_behavior.c`, `test_button_callback_events.c`, pointer activation blocker/scope tests, and `test_button_behavior.py`. This inventory describes implemented subsets; it does not resolve the table's exact style/layout/platform gaps.

ButtonGroup: portable staged Core-only API, at most 256 snapshotted direct children, controlled accept/reject, reread between events, exact input order/repeats, no duplicate callbacks, immediate final checked paint, both-axis edge/corner joins, size/variant inheritance, per-child slots/state/cursor/loading, passive explicit/ancestor inert/layout-only layout and permitted paint. Source default group callback is pointer-only; ALL_ACTIVATIONS is an explicit extension. Disabled builder-order quirks and sticky background child identity are intentional corrections. Arbitrary group `Styled` refinement remains subject to the generic style gap above; source direct children do not imply roving arrow keys or mandatory selection.

Evidence includes `test_accepted_activation.c`, `test_button_group_base.c`, `test_button_group.c` (all 864 geometry cases retained, now bounded fixture allocations), `test_button_group_sdl.c`, actual WASM software-pixel probe and `test_button_collection_gallery.py` at 1280/390 pixels. These are not physical native touch, screen-reader acceptance or every platform's font shaping proofs.

## New concrete CUI extension gap: passive Button state lifecycle

CUI inert/layout-only ancestry is an architectural extension; the pinned GPUI
Button does not expose these scopes. `cui_button_ex` at current
`src/components/cui_components.c:531-553` calls metadata and tooltip helpers even
when Core rejects interaction registration under suppressed ancestry.
`cui_button_metadata:475-507` and `cui_button_tooltip:510-529` derive their local
inert condition only from `options.disabled || options.loading`.

A strict external probe initializes caller state with ID 10, initialized/hovered
true, an open tooltip and wake deadline 42, then submits the same ordinary enabled
Button under each ancestry. Exact observed output:

```text
inert draws=11 semantics=0 hover_callbacks=1 hovered=0 tooltip_open=0 wake=0
layout-only draws=0 semantics=0 hover_callbacks=1 hovered=0 tooltip_open=0 wake=0
```

Probe: `/home/matteo/.cache/cui-validation/button-passive-state-probe.c` and `.log`.
It compiled with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude` against the
validated merged-equivalent Button library. The probe establishes one unwanted
hover-exit callback and mutation of caller state, not a new activation leak:
accepted activation queries already reject suppressed scopes. There is no claim
of physical device or upstream source behavior from this synthetic Core scene.

## Smallest next complete production slice

Correct the **passive standalone Button lifecycle**, without adding a public API.
Use existing `cui_frame_is_submitting` and `cui_frame_sequence` phase queries to
separate ordinary interactive submission from ordinary suppressed paint/layout.
Outside ordinary submission, do not start new Button interaction/state work.
Within suppressed ancestry, preserve root/content dimensions, slots, artwork,
selected/disabled/loading presentation and current label paint, but skip semantic
metadata, action/hover callbacks, tooltip reducer submission, wake scheduling and
all caller-state mutation. Keep the original caller-owned state pointer available
to the deferred label paint: its existing `info.inert` guard forbids writes and
retains the configured diagnostic-prefix cap. A temporary stack state pointer
must never be retained by deferred paint. This requires no Core allocation,
frame-state redesign or public phase extension.

Explicit disabled/loading options during an ordinary frame remain ordinary
interactive registration with disabled/busy blockers and existing cancellation;
they must not be mistaken for suppressed ancestry. Styled Group already has a
separate passive paint path and must retain its semantics/callback policy.
Arbitrary content callbacks execute to compose passive visuals; action callbacks
and hover notifications do not. That distinction is part of the CUI composition
contract, not an assertion that all caller code is pure.

Proposed ownership: Button-only `src/components/cui_components.c` helpers and
entry point, new focused portable lifecycle test and independent browser fixture,
plus `docs/BUTTON_BEHAVIOR.md`. No public declarations, Core, Base, theme, adapters,
existing Input code or global registrations. Root handles registration/generated
output. This audit authorizes no production edits.

Meaningful tests must preload a byte-comparable complete caller state, use both
same and changed Button IDs, both explicit and inherited inert/layout-only scopes,
hover-entered and tooltip pending/open/suppressed states, and zero/nonzero dt.
Require no callback and no state-byte/deadline mutation; inert retains actual
clipped paint and bounds, layout-only retains bounds with no paint/semantics.
Exercise ordinary next-frame reuse, pointer presence/cancel, keyboard activation,
disabled/loading blockers, group children and state-free/custom-slot composition.
A measurement or prepaint callback must not register new interactive state.
Browser evidence should use genuine move/keyboard/touch, then host-controlled
scope changes, to prove no extra hover/action callbacks and correct subsequent
normal reuse; no direct reducer calls as input evidence. Capacity failures and
caller lifetimes retain the existing current-label contract.

## Next source-only candidate after the correctness slice

The smallest independent source-style slice is `ButtonCustomVariant.shadow(bool)`
(`button.rs:138-141,650-656,1093-1099`), not shadows on every variant. A reviewed
proposal can append `bool custom_shadow` to Button options and a theme-level
`bool button_shadows` policy, with zero retaining legacy no-shadow behavior and
built-in themes explicitly enabling the source-compatible policy. Both gates
must be true, variant must be Custom and disabled must be false; loading retains
normal shadow with whole-control opacity. Use existing `cui_set_drop_shadows`
immediately after opening the root, one value-owned audited `shadow_xs` layer,
resolved corner radii/edge joins and ordinary ancestor clipping/opacity. Do not
borrow a shadow array past registration or introduce resources. Root must review
compatibility policy and audited GPUI blur conversion before implementation.

That proposal owns Button-only declarations/helpers, the narrow theme policy,
new headless/software/native/WASM pixel tests and guide. Tests require disabled
suppression, loading/ancestor opacity, outline/selected/hover/pressed states,
rounded/group joins, clip containment, every design size/density, policy off and
capacity failure; screenshots alone do not prove blur equivalence. This closes
one public property, leaving gradient/Oklab, arbitrary Styled refinement, exact
focus painting, signed tab order, role/focusability, rich click payload,
action-keybinding tooltips, typed icon preservation, caret tint, open presentation
and numeric Size rules explicitly outstanding.

## Validation provenance

This reconciliation changes documentation only. Source pin was independently
verified with `git rev-parse HEAD`. The passive-state probe compiled warning-clean
and ran as above. The preceding automatic-label implementation ran warning-clean
native CTest 144/145 (only stale generated distribution), portable and actual
native font matrices, scoped ASan/UBSan and pinned Chrome1208 WASM mouse/touch and
pixel assertions; logs are `button-label-{native-build,ctest,web,asan-build}.log`
under `/home/matteo/.cache/cui-validation`. Root subsequently reports combined
Next31 native 160/160 and portable 138/138. These counts are provenance, not an
independent rerun on this documentation worktree or whole-family certification.
Eight existing merged-equivalent native checks were independently rerun and passed (Components, Button behavior, accepted activation, callback events, Base/styled Group, prepaint parts and layout-only clips); log `button-reconcile-ctest.log`. No fresh full build is needed for this read-only amendment; no physical OS or
screen-reader acceptance is claimed.
