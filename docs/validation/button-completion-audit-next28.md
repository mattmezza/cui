# Button completion audit, next28

This read-only audit examines CUI `6e36b5a` against pinned v0.7.1 source `87d10ae5e1299d1be18670a37570c5402f952e34` in `/tmp/cui-gpui-reference`. It covers Base Button, Components Button, ButtonGroup, ButtonIcon and Icon. It certifies no complete family, platform or device and changes no manifest status. The JSON companion inventories every public `pub fn` in those source files plus their trait contracts; the mapping below includes render-time behavior and private compound hooks where they are necessary to explain a public collection.

`button/mod.rs` also exports Toggle and DropdownButton. Those remain distinct manifest families and are not silently certified by this Button audit. GPUI entity/view/handle types are not requirements to expose platform types in portable C11: the behavior and ownership must be mapped explicitly. Stories are illustrative, not additional API requirements.

The current suite is strong on a subset: twelve C variants, four legacy sizes, fourteen user-requested design sizes, real named SVGs/slots, loading, callback/focus opt-outs and bounded labels. It does not establish completeness. The ledger's error-state wording does not correspond to a pinned standalone Button error API.

## Contract mapping

### 1. Base: Stable identity, caller children, neutral centered geometry

Status: partial tested. Current mapping: `cui_base_button + cui_begin_box + cui_button_paint`.

Evidence: `crates/base/src/button.rs:20–35,207–255`; `tests/unit/test_components.c:test_button_paint_composition`.

Base is a semantic reducer, not an automatic styled box. Caller layout is explicit; composed paint preserves one semantic and expanded/controls metadata.

### 2. Base: Selected/disabled state refinements and instance-style priority

Status: partial. Current mapping: `cui_interaction + caller layout; styled selected/custom colors`.

Evidence: `crates/base/src/button.rs:141–185`; `src/components/cui_components.c:156–225`.

No general StyleRefinement chain or independent disabled palette override. Existing flat palette tests do not prove upstream style replay priority.

### 3. Base: Role override and explicit accessible name

Status: partial tested. Current mapping: `cui_set_semantic_role; name argument; cui_button_options.accessibility_label`.

Evidence: `crates/base/src/button.rs:82–95`; `tests/unit/test_core.c:1798–1814`; `tests/unit/test_button_behavior.c`.

Core setter supports role override after registration; no Button option duplicates it. Native AT interpretation is not established by semantic unit tests.

### 4. Base: Caller focus handle, tab stop, focusability

Status: portable composition; partial tested. Current mapping: `stable cui_id/cui_focus; cui_set_tab_stop; no_focus/no_pointer_focus details`.

Evidence: `crates/base/src/button.rs:94–128`; `tests/unit/test_pointer_focus_policy.c`.

No GPUI handle/entity types in portable API. Styled skip_tab and pointer-focus opt-out exist; Base focusable(false) semantics need composition-specific proof.

### 5. Base: Ordered signed tab_index

Status: missing. Current mapping: `Core only sets tab stop 0/-1`.

Evidence: `crates/base/src/button.rs:108–111`; `src/core/cui.c:cui_core_tab,cui_set_tab_stop`.

Traversal uses previous semantic order, not ordered indexes. A positive public semantic field is not a supported setter or ordering implementation.

### 6. Base: Mouse and Enter/Space activation, disabled blocking of parent

Status: runtime failure. Current mapping: `cui_base_button/Core`.

Evidence: `crates/base/src/button.rs:337–381`; `crates/component/src/button/button.rs:1396–1437,1496–1580`; `button-disabled-parent-probe.log`.

Inert child does not activate itself, but parent activates for disabled/loading mouse/touch. Requires opt-in activation blocker; see proposal.

### 7. Button: Variants Default/Primary/Secondary/Danger/Warning/Success/Info/Ghost/Link/Text/Custom; outline orthogonal

Status: partial tested. Current mapping: `12 enum variants including legacy OUTLINE + outline flag`.

Evidence: `crates/component/src/button/button.rs:49–183`; `tests/unit/test_components.c:test_button_options,test_button_intent_outlines`; `tests/unit/test_button_behavior.c`.

Names and solid states covered; gradients/Oklab/shadow remain separate gaps. Zero CUI option remains legacy Primary, unlike upstream Default; intentional compatibility difference.

### 8. Button: Theme background tokens, semantic outline depth, gradients

Status: missing gradient parity. Current mapping: `cui_theme_button_colors contains flat cui_color`.

Evidence: `crates/component/src/button/button.rs:900–974,1812–1960`; `include/cui/cui_theme.h:70–77`.

No automatic gradient backgrounds/semantic gradient opacity states. A generic mesh renderer is not a Button token implementation.

### 9. Button: Custom normal/foreground/hover/active plus shadow

Status: partial tested. Current mapping: `custom_colors/colors; no shadow field`.

Evidence: `crates/component/src/button/button.rs:40–140,1093–1100`; `tests/unit/test_button_behavior.c`; `tests/unit/test_components.c:test_button_palette_overrides`.

Resolved palettes work; source custom shadow and Oklab transparent mixing absent. Built-in variants do not request shadow: do not invent mandatory shadows for all buttons.

### 10. Button: XS/S/M/L, custom pixel sizing, compact, centered icon/text

Status: partial tested. Current mapping: `size; height/font_size/icon_size; compact/icon_only/width`.

Evidence: `crates/component/src/button/button.rs:630–690`; `tests/unit/test_button_behavior.c`; `tests/unit/test_components.c:test_icon_button`.

Legacy dimensions and centering covered. Custom size is split into explicit C fields, and callback content needs caller constraints. Explicit width intentionally clips.

### 11. User extension: All 14 design sizes and density

Status: headless tested; responsive marker proof exists. Current mapping: `design_size plus theme font-derived density`.

Evidence: `tests/unit/test_button_behavior.c:432 combinations`; `tests/web/test_button_behavior.py`; `tests/web/test_design_sizes.py:FAMILIES`; `tests/web/test_responsive_gallery.py:38–70`.

432 size/variant/density containment cases use plain label and two named SVGs. The Button-specific browser suite has 11 baseline cases. Responsive gallery tests cover all14 sizes/both densities at390/1280 but assert only that Loading text exists and its bottom fits canvas height. They do not prove every card-local label/icon/slot clip containment or activation at those scales; generic design-size family list omits standalone Button.

### 12. Button: None/Small/Medium/Large/custom rounding

Status: implemented; partial proof. Current mapping: `rounding/radius`.

Evidence: `crates/component/src/button/button.rs:24–36,338–340,604–616`; `tests/unit/test_button_behavior.c`.

Custom radius is exercised; independent draw-radius/state matrix for every rounding mode is not established by that test.

### 13. Button: Normal/hover/pressed/selected colors

Status: partial tested. Current mapping: `shared interaction query and palettes`.

Evidence: `crates/component/src/button/button.rs:703–729`; `tests/unit/test_button_behavior.c`; `tests/web/test_button_behavior.py:paint_states`.

Actual browser hover/pressed changes and headless custom colors are covered; source gradient/state styling and group hover are not.

### 14. Button: selected and open are separate presentation states; toggled explicit semantics

Status: partial tested. Current mapping: `selected; toggle_semantics/toggled; compound expanded metadata`.

Evidence: `crates/component/src/button/button.rs:196–205,497–558,1666–1694`; `tests/unit/test_button_behavior.c`; `tests/unit/test_components.c:test_button_paint_composition`.

Standalone options have no separate open field. Host can derive selected paint from open and preserve expanded metadata, but dedicated selected/open transitions are not fully proven. Selected does not imply checked.

### 15. Button: Disabled/loading inertness, normal 0.8 loading opacity, busy semantics

Status: partial tested plus parent failure. Current mapping: `disabled/loading; description/busy`.

Evidence: `crates/component/src/button/button.rs:510–519,819–856`; `tests/unit/test_button_behavior.c`; `tests/unit/test_components.c:test_button_loading_artwork`; `tests/web/test_button_behavior.py:disabled`.

Self activation suppressed and actual loading SVG/reduced motion covered; parent-blocking fails. Arbitrary caller child semantics remain caller-owned.

### 16. Button: Default loading Spinner/custom loading Icon

Status: partial tested. Current mapping: `loading_icon; cui_icon_spinner; leading_content`.

Evidence: `crates/component/src/button/button_icon.rs:116–129`; `tests/unit/test_components.c:test_button_loading_artwork`; `tests/unit/test_button_behavior.c`.

Loading named glyph rotation/reduced motion tested. Full custom loading asset/state/size/geometry browser matrix absent.

### 17. ButtonIcon: Icon/Spinner/ProgressCircle typed slot; loading preserves explicit Spinner/Progress

Status: composable; runtime proof missing. Current mapping: `leading_content with cui_icon/cui_spinner_ex/cui_progress APIs`.

Evidence: `crates/component/src/button/button_icon.rs:55–129`; `tests/unit/test_button_behavior.c:slot`.

Slot precedence supports composition, but the current test uses colored boxes, not actual persistent ProgressCircle/Spinner across loading states. No typed ButtonIcon discriminator API is required to match paint behavior.

### 18. Button: Label, icon-only, appended children, content clipping and centering

Status: partial tested. Current mapping: `name/icon_only/content/children/leading_content/trailing_content`.

Evidence: `crates/component/src/button/button.rs:371–401,730–775`; `tests/unit/test_button_behavior.c`; `tests/web/test_button_behavior.py:slots`.

Source appends arbitrary children; legacy replacement content remains compatible. Slot dimensions/colors/semantics are caller-owned. Every all14-size rich slot is not browser-tested.

### 19. Button: Ellipsis preserves full accessible label

Status: partial tested. Current mapping: `label_width plus state.label[256]; accessibility_label`.

Evidence: `crates/component/src/button/button.rs:742–756`; `tests/unit/test_button_behavior.c`; `tests/web/test_button_behavior.py:slots`.

Plain UTF-8 grapheme truncation and buffer exhaustion tested. Buffer-limited clipping differs from unconstrained upstream storage; rich child truncation/shaping is not promised.

### 20. Button: Dropdown caret and leading artwork

Status: partial tested. Current mapping: `dropdown_caret/icon/trailing_icon`.

Evidence: `crates/component/src/button/button.rs:486–490,754–774`; `tests/web/test_button_behavior.py:slots`.

Three actual SVG commands fit baseline icon button; no exhaustive native pixel/size/caret matrix.

### 21. Button: on_click event callback and exact per-event multiplicity

Status: runtime divergence. Current mapping: `void(user) callback + boolean frame result`.

Evidence: `crates/base/src/button.rs:100–105`; `crates/component/src/button/button.rs:447–452`; `button-burst-probe.log`.

Two queued click sequences retain two Core event activations but produce one callback/bool result. Existing tests establish one callback per activated frame across frames. No ClickEvent argument; event metadata is available through Core frame events.

### 22. Button: Interactive on_hover edges

Status: partial tested. Current mapping: `on_hover plus caller-owned state`.

Evidence: `crates/component/src/button/button.rs:456–461,855–858`; `tests/unit/test_button_behavior.c`.

State ownership/reset documented; basic edges tested. New presence/leave handling is Core-tested, but Button-specific stationary leave/reenter and disable-during-hover callback matrix lacks direct browser proof.

### 23. Button: Focus ring opt-out, pointer-focus suppression, keyboard focus

Status: partial tested; default differs. Current mapping: `hide_focus_ring/prevent_pointer_focus/skip_tab`.

Evidence: `crates/component/src/button/button.rs:531–539,829–841`; `tests/unit/test_pointer_focus_policy.c`; `tests/web/test_button_behavior.py:activation`.

Source styled pointer-down prevents default; CUI preserves legacy pointer focus unless opt-in. Ring currently uses 2px theme focus border rather than source Inside/Edge/Outside focus-line policy.

### 24. Button: Tooltip plain text, preferred placement, lifecycle

Status: partial tested. Current mapping: `tooltip/state/tooltip_options.side + shared hover`.

Evidence: `crates/component/src/button/button.rs:403–414,858–875`; `tests/unit/test_button_behavior.c`.

Focus/open/Escape/cancel/disabled and description are tested. All side collision placements and focus/mouse/touch browser lifecycle on Button are not fully proven.

### 25. Button: Tooltip with action/context keybinding

Status: missing. Current mapping: `no action registry or keybinding tooltip option`.

Evidence: `crates/component/src/button/button.rs:418–431`.

Plain text containing a shortcut is not automatic action/context lookup.

### 26. Scope boundary: Arbitrary tooltip builder

Status: not a public pinned Button requirement. Current mapping: `No dedicated builder in CUI`.

Evidence: `crates/component/src/button/button.rs:223,270,858–862`.

Pinned field and render branch exist, but no public setter is present in this Button source. Treat a future arbitrary builder as an extension, not invented completeness criterion.

### 27. Button: Accessibility developer string id, role/name/toggled, platform actions

Status: partial. Current mapping: `numeric stable IDs; semantic role/name/checked; optional native bridge`.

Evidence: `crates/component/src/button/button.rs:377–391,773–818`; `tests/unit/test_accessibility.c`; `tests/unit/test_core.c`.

Numeric identity is portable; no separate developer string identifier. Metadata/unit dispatch is not physical Windows/macOS/Linux AT certification or every browser aria mapping.

### 28. Button: Base supplied by compounds and complete child paint

Status: partial tested. Current mapping: `cui_button_paint separate from Base reducer`.

Evidence: `crates/component/src/button/button.rs:287–290,1727–1794`; `tests/unit/test_components.c:test_button_paint_composition`.

Painting preserves caller semantics and is non-activating. Upstream private with_base is not a public C API requirement. No previous-frame bitmap approximation is used for ordinary button children.

### 29. Compound internals: Connected edge/corner masks, hover-group held/open

Status: missing automatic general Button composition. Current mapping: `Dropdown has local corner helpers; no shared Button masks/hover group`.

Evidence: `crates/component/src/button/button.rs:344–366`; `crates/component/src/button/button_group.rs:184–225`; `src/components/cui_choice.c:cui_dropdown_button_box`.

Private upstream hooks matter to ButtonGroup/split compounds, not standalone public builder completeness. Local Dropdown shape does not supply a reusable ButtonGroup implementation.

### 30. ButtonGroup: Collection identity/children, horizontal/vertical connected layout

Status: missing distinct API. Current mapping: `ToggleGroup APIs are separate`.

Evidence: `crates/component/src/button/button_group.rs:18–95,153–240`; `include/cui/cui_components.h:198–213`.

No cui_button_group. ToggleGroup does not preserve per-child full Button options, callback behavior or connected Button border topology.

### 31. ButtonGroup: Optional inherited size/variant/compact/outline

Status: missing. Current mapping: `no full Button collection composition`.

Evidence: `crates/component/src/button/button_group.rs:132–150,229–233`.

Requires explicit inheritance/override contract without mutating caller data; all14 scales are additional user work.

### 32. ButtonGroup: Controlled rendered selection, single/multiple requested index collection

Status: missing source contract. Current mapping: `ToggleGroup emits bool arrays with different semantics`.

Evidence: `crates/component/src/button/button_group.rs:95–128,154–164,246–264`.

Source derives selected indices from rendered child selected flags, emits request, does not persist selection itself; children expose toggled state.

### 33. ButtonGroup: Group callback replaces child callback; absent group preserves child

Status: missing. Current mapping: `No ButtonGroup callback API`.

Evidence: `crates/component/src/button/button_group.rs:234–239,346–360`.

Do not equate callback replacement with simultaneously firing both callbacks. Requires exactly defined ownership and meaningful reject/accept tests.

### 34. ButtonGroup: Legacy keyboard callback and disabled builder-order edges

Status: source legacy divergence needs decision. Current mapping: `No matching collection API`.

Evidence: `crates/component/src/button/button_group.rs:373–423`.

Pinned tests deliberately show keyboard click never reaches group callback and disabled-after-child leaves child callback active. A new C API should document intentional corrections rather than claim byte-for-byte parity or invent roving arrow requirements.

### 35. Icon: Complete named catalog/default assets/native and WASM loading

Status: partial tested. Current mapping: `1830 catalog;106 default embedded; all-assets/provider`.

Evidence: `crates/component/src/icon.rs:13–81`; `crates/assets/README.md:35–107`; `tests/unit/test_icon.c:test_assets`.

Counts match source asset description. Upstream WASM on-demand CDN is not an implicit CUI dependency; host loader/full embed required. Default unbundled names explicitly NOT_FOUND; no silent fallback claim.

### 36. Icon: Path/raw copied SVG replacement, empty, source sharing/view

Status: partial tested portable adaptation. Current mapping: `set_path/set_name/set_data/set_empty; caller state`.

Evidence: `crates/component/src/icon.rs:84–170`; `tests/unit/test_icon.c:test_source_and_raster,test_loader`.

Copied source replacement/lifetime tested. No GPUI Entity/View or shared clone API; explicit independent state/resources are portable replacements.

### 37. Icon: Inherited color/text size, XS/S/M/L/custom, explicit width/height precedence

Status: partial tested; automatic inheritance gap. Current mapping: `cui_icon_options/theme/inherited overrides`.

Evidence: `crates/component/src/icon.rs:175–223`; `tests/unit/test_icon.c:test_component_sizes_color_and_lifetime,test_design_sizes`.

Explicit theme/text override supported; automatic global text-style scope inheritance absent. Icon design-size geometry has unit proof; native font/theme scope browser matrix remains partial.

### 38. Icon: Affine transform/rotate paint overflow without changing layout

Status: partial tested. Current mapping: `cui_icon_transform/rotation/prepare`.

Evidence: `crates/component/src/icon.rs:158–200`; `tests/unit/test_icon.c:test_source_and_raster,test_invalid_and_limits`.

Actual alpha/centroid/overflow tests exist. Full upstream transform/SVG renderer equivalence is not established.

### 39. Icon: Paint-only decorative layout and ownership/cache bounds

Status: partial tested. Current mapping: `cui_icon; named context cache; collect/destroy`.

Evidence: `crates/component/src/icon.rs:204–238`; `tests/unit/test_icon.c:test_context_convenience,test_component_sizes_color_and_lifetime`.

No focus/role added. Borrowed immutable masks retained until collect; bounded limits intentional. Caller state is not thread-safe or safely memcpy-clonable.

### 40. Icon: SVG renderer feature coverage and actual native pixels

Status: explicit limitation. Current mapping: `Pinned NanoSVG monochrome mask; portable image command`.

Evidence: `docs/ICON.md:109–126`; `tests/unit/test_icon.c`; `tests/web/test_browser.py:Icon story`.

Not full resvg/SVG CSS/text/filter/external-image parity. All embedded assets rasterize in configured unit builds; inspected tests do not certify every physical renderer/AT platform.

### 41. User pointer extension: Custom per-control named cursor and RGBA pointer images

Status: partial tested; RGBA absent. Current mapping: `custom_cursor/cui_set_cursor; SDL system cursors; browser CSS`.

Evidence: `tests/unit/test_button_behavior.c`; `tests/web/test_button_behavior.py:paint_states`; `tests/unit/test_sdl3.c`; `docs/INTERACTION.md:102–108`.

Hand/crosshair cases tested; SDL grab/zoom/help use documented fallbacks, no caller RGBA cursor+hotspot adapter. Generic queued SDL normalization is not actual Button end-to-end physical touch proof.

### 42. Scope boundary: Error state, URL navigation, spring Button animation, roving group arrows

Status: not pinned standalone requirements. Current mapping: `No dedicated fields`.

Evidence: `crates/component/src/button/button.rs`; `crates/component/src/button/mod.rs`.

Error wording exists in the ledger but no standalone error builder/state is present. Link is a role/visual variant, not automatic URL navigation. Do not import Checkbox motion or RadioGroup arrow requirements.

## Smallest remaining correctness slice

Disabled/loading parent activation is a reproduced failure, not just an absent API. A temporary public CUI scene registers an activatable parent and places an inert styled Button inside its actual bounds. One mouse or touch down/up yields parent=1, child=0 for both disabled and loading. Pinned Base `disabled_button_is_inert_and_blocks_parent_activation` and styled `disabled_legacy_button_keeps_existing_pointer_blocking`, `loading_button_keeps_existing_pointer_blocking`, and `loading_button_without_callback_still_blocks_parent_activation` require parent=0. Log: `/home/matteo/.cache/cui-validation/button-disabled-parent-probe.log`; temporary fixture: `tmp-storage/button-audit/disabled_parent_probe.c` in that validation tree. No repository test or production source was edited.

Proposed exact scope: append zero-default `cui_semantic_details.blocks_pointer_activation` in `include/cui/cui.h`; narrow previous-frame event hit routing in `src/core/cui.c`; only `cui_base_button` in `src/base/cui_base.c`; only Button registration in `src/components/cui_components.c`; new dedicated headless Core/Button blocker tests, actual WASM/browser probe and a focused guide. Base sets it when disabled; styled Button preserves it when disabled or loading. This opts Button into blocking rather than changing all disabled semantics globally.

The topmost eligible blocker must respect clipped hit bounds, modal scope, layer and paint order; deny new focus/capture/activation beneath it; preserve unrelated keyboard/programmatic focus and captured motion until release/cancel; deny release activation through an inert top blocker. Do not suppress ancestor wheel/touch panning, infer physical leave from cancellation, or alter pointer presence. Activation blocking is not full hover/scroll occlusion: hover/prepaint behavior must be inspected and documented separately.

Existing `cui_set_hover_hitbox` applies only to current-frame prepaint nodes. It does not register a previous-frame event blocker; adding it to Button paint cannot undo a parent activation dispatched before building this frame. Faking an enabled pointer-activatable child would corrupt disabled semantics and focus. The explicit seam is necessary unless a different existing previous-event mechanism can satisfy the same tests.

Required regressions: enabled child activation; disabled/loading mouse and touch; no callback/no capture; keyboard and programmatic focus preserved; cancellation and next tap; release over blocker; overlap, clipped-away blocker, later-layer enabled control, modal exclusion; parent pan/wheel still works; zero flag preserves legacy other controls. Real browser input must reproduce the parent failure before the fix and pass afterward without reducer/model-call substitutions. Integrator approval is required before implementation.

## Other work and completion verification

A second temporary probe queues two click sequences before a frame. Core retains two event activations, but Button coalesces them into one callback and boolean result (`button-burst-probe.log`). Existing exact-once tests are across frames. Preserve the legacy boolean result; decide and document event callback multiplicity before an independent patch. Do not bundle this decision into the blocker fix.

Remaining collection work needs a distinct bounded ButtonGroup API preserving per-child Button options, controlled requested selection, connected edges/corners, inheritance and callback ownership. Pinned legacy keyboard non-bubbling and disabled builder-order bugs must be recorded as intentional deviations if corrected; neither creates a roving-arrow requirement. Gradient/semantic outline backgrounds, custom shadow, ordered tab indexes and action-keybinding tooltip support are independent missing source contracts. A private unused tooltip-builder branch is not a public requirement.

Completion evidence must include all14 Button sizes in comfortable/compact density at mobile/desktop widths with real labels, slots, loading and icon/caret containment; source state/style/focus-line and gradient/shadow cases; actual Spinner/Progress loading slots; collection accept/reject, callback ownership and immediate coherent rendering; burst events; Tooltip placement/lifecycle; and native adapter/renderer evidence with real queued SDL scenes. Screen-reader or physical OS/device claims need their own evidence. RGBA cursor/hotspot support is a remaining user extension rather than a pinned Button requirement.

## Validation and limits

The isolated warning-enabled native build completed without warnings. Full CTest passed 122/122 in 15.70 seconds. Logs: `/home/matteo/.cache/cui-validation/button-audit-{configure,build,ctest}.log`. Both temporary public-API probes compiled with strict C11, `-Wall -Wextra -Wpedantic -Werror` against this build. Existing browser/headless tests were inspected; no new browser run or sanitizer run was performed for this docs-only audit. Earlier green Chromium/CDP and queued SDL tests are bounded historical evidence, not physical iOS/Android/Windows/macOS or complete family certification. Root's broader frozen test configuration may register more tests than this checkout's default build; counts must not be interchanged.
