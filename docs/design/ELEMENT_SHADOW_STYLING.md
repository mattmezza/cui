# Unified element shadows: proposed architecture

Status: design for review, no new API implemented. Audit base: fb3be922.
This proposal covers the 78 catalog families as an implementation plan, not a
claim that all roots or nested parts already expose shadow styling.

## Evidence and gap

`include/cui/cui.h:375–416,670–673` already defines copied, bounded eight-layer
DROP/INSET lists, named presets and open-box setters. `src/core/cui.c:2111`
validates a whole list before replacing the current open node. The renderer
pipeline and budgets are documented in DROP_SHADOW.md and INSET_SHADOW.md.
No new renderer is needed for ordinary rectangular element shadows.

The missing seam is addressing component-owned geometry consistently. Most
one-call widgets close their root before returning. Applying the existing setter
after that call styles the caller's open parent. A wrapper adds layout geometry
and cannot reach a detached popup. `cui_bounds` (`src/core/cui.c:3046`) may return
previous-frame bounds, so it cannot implement correct move/resize decoration.
Core records current-frame nodes, anonymous synthetic layout IDs and portal paint
roots (`src/core/cui.c:1825–1858`); these are private implementation details.
`src/components/cui_components.c:1323–1329` illustrates named outer and anonymous
indicator nodes. `src/components/cui_surface.c:149–189` has separate resize,
backdrop and surface portals; only the surface receives its existing style list.
`cui_picker.c:370`, `cui_content.c:1100`, `cui_editor.c:119` and `cui_dock.c:253`
provide further independently owned portal examples.

## Recommended decision

Review an additive current-frame by-ID setter in Core, plus explicit public
component part resolution. Do not add a global pending style that silently applies
to whichever internal box happens to be emitted next. Do not change existing
public structure layouts, defaults, preset tuples or sigma meanings. A by-ID
setter alone does not cover anonymous parts or conceptual components.

Proposed names/signatures (not declarations available to consumers):

```c
bool cui_element_set_box_shadows(cui_context *, cui_id,
    const cui_box_shadow *, size_t count, cui_corner_radii);
bool cui_element_set_shadow_preset(cui_context *, cui_id,
    cui_shadow_preset, cui_corner_radii);
bool cui_box_shadow_from_css(float x, float y, float blur_radius,
    float spread, cui_color, cui_box_shadow_kind, cui_box_shadow *out);
```

The element setter accepts only an already submitted, named current-frame box,
including a closed box or portal, before end_frame. ID zero, missing/previous-frame
ID, layout-only nodes, collision, invalid list or prohibited mutation phase reject
atomically with documented diagnostics. Open-node setters retain their contract.
No deferred target registration: application order is component submission, then
styling. Replace all DROP/INSET layers, last successful call wins; count zero and
NONE clear. Existing component defaults survive absent explicit opt-in; an
explicit setter after submission replaces them. A later component-owned setter
would replace that explicit list, so begin/end consumers apply styling after end.
No retained cross-frame style or automatic state inheritance is introduced.

Reuse the node's copied eight-layer storage and existing paint emission. Decoration
must mark the target's layout custom data even after closure; review that Core
operation explicitly. Store no Clay pointer in a public value. Linear lookup is a
bounded initial implementation; optimize only with measured evidence. No new
layout pass, input target, semantic node, bounds expansion or capture is added.
Opacity, ancestor masks, local overflow clipping, portal ordering and resource
failure boundaries remain the existing renderer contracts. DROP remains before
background, INSET after background before local content; supplied order is retained
within each phase. This is CUI paint order, not a promise of complete CSS stacking.

CSS convenience converts blur_radius to sigma=blur_radius/2 explicitly; existing
sigma-based APIs and named source presets never change. Validate finite radius in
[0,512], offsets/spread in [-4096,4096], kind and output before writing, preserving
existing supported-domain checks. CSS defines blur using half-radius Gaussian
standard deviation: https://www.w3.org/TR/css-backgrounds-3/#shadow-blur . CUI's
bounded kernel, spread/radius handling, source mask culling and RGBA8 quantization
remain different from full browser box-shadow. This factory promises a useful
CSS-inspired parameterization, not browser pixel identity. Custom sigma lists
remain available without conversion.

## Component-owned parts

Each family must publish which ID denotes its rectangular paint root and which
parts need explicit stable IDs or checked part getters. Do not expose private
numeric XOR salts. Preserve semantic ownership; an indicator paint ID must not
become an input target. For currently anonymous nodes, assigning an ID can affect
capacity/collision behavior and needs owner-specific tests. Indexed parts require
stable keys, bounds checks and absence reporting. Popup surface, trigger, backdrop,
arrow and resize handles are separate targets; root styling must never silently
style all five. Nonrectangular/vector/glyph masks remain outside box-shadow.

The matrix below is a complete catalog inventory and proposed acceptance route.
R = rectangular owner/root audit and test; P = additional detached surface audit;
N = nested/repeated/anonymous parts require explicit mapping; H = host/behavior
family, no independent rectangular caster promised. Every R/P/N row is pending
per-API ID verification. Header links are canonical catalog authority, not proof
that each entry point has a suitable root today.

| Family | Route | Header |
| --- | --- | --- |
| Accordion | R/N | `include/cui/cui_disclosure.h` |
| AlertDialog | R/P/N | `include/cui/cui_surface.h` |
| Alert | R | `include/cui/cui_content.h` |
| Attachment | R/N | `include/cui/cui_media.h` |
| Avatar | R | `include/cui/cui_content.h` |
| Badge | R | `include/cui/cui_components.h` |
| Bubble | R | `include/cui/cui_content.h` |
| Button | R | `include/cui/cui_components.h` |
| Checkbox | R/N | `include/cui/cui_components.h` |
| Clipboard | H | `include/cui/cui_sdl3.h` |
| Collapsible | R/N | `include/cui/cui_disclosure.h` |
| DropdownButton | R/N | `include/cui/cui_choice.h` |
| Empty | R | `include/cui/cui_content.h` |
| Focus Trap | H | `include/cui/cui_overlay.h` |
| GroupBox | R | `include/cui/cui_content.h` |
| HoverCard | R/P/N | `include/cui/cui_content.h` |
| Icon | R | `include/cui/cui_icon.h` |
| Image | R/N | `include/cui/cui_media.h` |
| Kbd | R | `include/cui/cui_content.h` |
| Label | R | `include/cui/cui_components.h` |
| Marker | R | `include/cui/cui_status.h` |
| Message | R/N | `include/cui/cui_message.h` |
| MessageScroller | R/N | `include/cui/cui_message.h` |
| Notification | R/P/N | `include/cui/cui_status.h` |
| Pagination | R | `include/cui/cui_numeric.h` |
| Progress | R | `include/cui/cui_components.h` |
| Radio | R/N | `include/cui/cui_components.h` |
| Rating | R/N | `include/cui/cui_numeric.h` |
| Shimmer | R | `include/cui/cui_status.h` |
| Skeleton | R | `include/cui/cui_content.h` |
| Spinner | R | `include/cui/cui_content.h` |
| Stepper | R/N | `include/cui/cui_numeric.h` |
| Switch | R/N | `include/cui/cui_components.h` |
| Tag | R | `include/cui/cui_status.h` |
| Toggle | R/N | `include/cui/cui_components.h` |
| Tooltip | R/P/N | `include/cui/cui_content.h` |
| Calendar | R/N | `include/cui/cui_picker.h` |
| ColorPicker | R/P/N | `include/cui/cui_picker.h` |
| Combobox | R/P/N | `include/cui/cui_choice.h` |
| DatePicker | R/P/N | `include/cui/cui_picker.h` |
| Editor | R/P/N | `include/cui/cui_editor.h` |
| Form | R/N | `include/cui/cui_form.h` |
| Input | R | `include/cui/cui_components.h` |
| Input Group | R/N | `include/cui/cui_form.h` |
| NumberInput | R/N | `include/cui/cui_numeric.h` |
| OtpInput | R/N | `include/cui/cui_otp.h` |
| Questionnaire | R/N | `include/cui/cui_questionnaire.h` |
| Select | R/P/N | `include/cui/cui_choice.h` |
| Slider | R/N | `include/cui/cui_components.h` |
| Speech | H | `include/cui/cui_speech.h` |
| Textarea | R | `include/cui/cui_textarea.h` |
| TimeField | R | `include/cui/cui_picker.h` |
| Carousel | R/N | `include/cui/cui_carousel.h` |
| DescriptionList | R/N | `include/cui/cui_description.h` |
| Dialog | R/P/N | `include/cui/cui_surface.h` |
| Dock | R/P/N | `include/cui/cui_dock.h` |
| List | R/N | `include/cui/cui_choice.h` |
| Menu | R/P/N | `include/cui/cui_choice.h` |
| Popover | R/P/N | `include/cui/cui_surface.h` |
| Resizable | R/N | `include/cui/cui_resizable.h` |
| Root View | R/N | `include/cui/cui_root.h` |
| Scrollable | R/N | `include/cui/cui_scroll.h` |
| Settings | R/N | `include/cui/cui_settings.h` |
| Sheet | R/P/N | `include/cui/cui_surface.h` |
| Sidebar | R/N | `include/cui/cui_sidebar.h` |
| StatusBar | R | `include/cui/cui_chrome.h` |
| Tabs | R/N | `include/cui/cui_choice.h` |
| Theme | H | `include/cui/cui_theme.h` |
| TitleBar | R | `include/cui/cui_chrome.h` |
| Toolbar | R/N | `include/cui/cui_toolbar.h` |
| Tree | R/N | `include/cui/cui_tree.h` |
| VirtualList | R/N | `include/cui/cui_virtual_scroll.h` |
| Chart | R/N | `include/cui/cui_chart.h` |
| Command | R/P/N | `include/cui/cui_command.h` |
| DataTable | R/N | `include/cui/cui_table.h` |
| Plot | R/N | `include/cui/cui_plot_view.h` |
| Table | R/N | `include/cui/cui_table.h` |
| TextView | R/N | `include/cui/cui_text_view.h` |

## Bindings and rollout

The shared bridge is `include/cui/cui_bridge.h`; no box-shadow authoring operation
is declared there today. Source SDK availability is not language-builder support.
Python, Node/TypeScript, Go and Rust must all expose the same copied style value,
preset and CSS factory semantics. Prefer a versioned additive bridge operation
with bounded explicit layer records and count validation, then typed facade
methods in each builder. Do not expose pointers into host arrays or depend on C
structure layout across the wire. Builders must apply style after actual native
submission, not when retained authoring objects are merely constructed. Detached
popup styling must run only when its current-frame surface exists. Go/Rust bundled
C source is regenerated separately from their typed facade implementation.

First review Core addressing, anonymous-part policy and replacement precedence.
Then implement pure CSS factory and named-root seam with focused tests. Migrate
representative Button, Input, anonymous Checkbox indicator, compound Stepper,
clipped Scrollable and portal Popover before expanding family-by-family. Reuse
existing presets; add an optional explicitly chosen visual recipe/gallery page
with light/dark surfaces, subtle/raised/floating examples, mixed inset and custom
layers. No new implicit shadows on older components. Only update completion
metadata for rows with actual root/part and binding evidence.

## Acceptance tests and maintenance

Headless tests must prove submitted/closed/current-frame resolution, same-frame
move/resize, missing/zero/duplicate IDs, omission next frame, copied stack lists,
all eight layers, transactional ninth/NaN rejection, NONE/clear, preset/custom
replacement, CSS conversion endpoints and unchanged output on rejection. Compare
no-opt-in legacy command streams and software pixels. Exercise ancestor clip,
local overflow, opacity once, translucent backgrounds, nested parts, independent
popup/trigger/backdrop targeting, modal layering and unchanged semantics/input.
Test resource exhaustion and recovery using the existing emission boundaries.

The 78-row matrix becomes an acceptance tracker with per-entry-point test evidence,
not one generic test presented as all-component coverage. Binding tests assert the
same layer records, ownership, clear/error behavior and live popup lifecycle for
all five language facades. Reuse renderer regressions; no redundant raster suite.
Browser probes run headless at DPR1/2; native checks use test_local.py/Xvfb. GPU
checks unavailable in virtual display stay deferred. Keep one focused build, -j1,
nice19, and avoid package tests that silently build unrelated targets.

When implementing public contracts, update canonical header/docs, binding coverage,
source manifests, SDK/amalgamation generators and consumer skill resources. Run
generate_agent_skill.py and test_agent_skill.py under the agreed resource budget.
This design-only file changes no consumer contract, generated data or manifest
entry. No compilation or heavyweight test was run for this proposal.
